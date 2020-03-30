/******************************************************************************
 * Copyright (c) 2019 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include <mosquitto_broker.h>

#include "libwebsockets.h"
#include <string.h>
#include <net_mosq.h>
#include "logging_mosq.h"
#include "memory_mosq.h"

#include <sys/epoll.h>

#include "uthash.h"

#define LOG_DEBUG(...) if(IS_DEBUG_ENABLED && IS_CATEGORY_ENABLED(MOSQ_LOG_CATEGORY_WEBSOCKETS)) _mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, __VA_ARGS__)
#define LOG_INFO(...) if(IS_INFO_ENABLED && IS_CATEGORY_ENABLED(MOSQ_LOG_CATEGORY_WEBSOCKETS)) _mosquitto_log_printf(NULL, MOSQ_LOG_INFO, __VA_ARGS__)
#define LOG_ERROR(...) if(IS_CATEGORY_ENABLED(MOSQ_LOG_CATEGORY_WEBSOCKETS)) _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, __VA_ARGS__)

int g_ws_pre_buffer_size = LWS_PRE;

struct ws_listener {
    struct lws *wsi;
    int sock;
};

// There will be only one WS listener for both IPv4 and IPv6.
static struct ws_listener ws_listener = {0};

struct wsi_hash {
    struct lws *wsi;
    struct mosquitto *context;    
    int sock;
    UT_hash_handle hh;
};

// Hash map of context, with wsi as key.
// This will be used until the connection has reached established state
//  (after which, the context is stored in the callback user data).
static struct wsi_hash *wsi_context_map = NULL;

// Per connection user data.
struct libws_mqtt_data {
    struct mosquitto *mosq;
};

static int
callback_mqtt(struct lws *wsi, enum lws_callback_reasons reason,
              void *user, void *in, size_t len);

// Only mqtt and mqttv3.1 sub-protocols are supported.
static struct lws_protocols protocols[] = {
    {
        "mqtt",                                 // Name of the sub protocol
        callback_mqtt,                          // Callback function
        sizeof(struct libws_mqtt_data),         // Size of per-client data
        1024 * 1024,                            // RX buffer size.
        0,                                      // ID (unused)
        NULL,                                   // user data (unused)
        1024 * 1024                             // TX packet size.
    },

    {
        "mqttv3.1",                             // Name of the sub protocol
        callback_mqtt,                          // Callback function
        sizeof(struct libws_mqtt_data),         // Size of per-client data
        1024 * 1024,                            // RX buffer size.
        0,                                      // ID (unused)
        NULL,                                   // user data (unused)
        1024 * 1024                             // TX packet size.
    },

    { NULL, NULL, 0, 0 } //terminator
};

static struct lws_context_creation_info lws_ctx_info;
static struct lws_context *lws_ctx = NULL;

static uint32_t epoll_to_lwspoll_events(uint32_t epoll_events)
{
    return epoll_events;
}

static uint32_t lwspoll_to_epoll_events(uint32_t lwspoll_events)
{
    return lwspoll_events;
}

bool mosquitto_epoll_flag_is_websocket(struct epoll_event *event)
{
    return (event->data.u64 & MOSQUITTO_EPOLL_DATA_WEBSOCKETS_FLAG) != 0;
}

static const char* get_callback_reason_str(int reason)
{
    switch (reason) {
        case LWS_CALLBACK_WSI_CREATE:
            return "LWS_CALLBACK_WSI_CREATE";
        case LWS_CALLBACK_WSI_DESTROY:
            return "LWS_CALLBACK_WSI_DESTROY";
        case LWS_CALLBACK_ADD_POLL_FD:
            return "LWS_CALLBACK_ADD_POLL_FD";
        case LWS_CALLBACK_CHANGE_MODE_POLL_FD:
            return "LWS_CALLBACK_CHANGE_MODE_POLL_FD";
        case LWS_CALLBACK_DEL_POLL_FD:
            return "LWS_CALLBACK_DEL_POLL_FD";
        case LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION:
            return "LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION";
        case LWS_CALLBACK_ESTABLISHED:
            return "LWS_CALLBACK_ESTABLISHED";
        case LWS_CALLBACK_SERVER_WRITEABLE:
            return "LWS_CALLBACK_SERVER_WRITEABLE";
        case LWS_CALLBACK_RECEIVE:
            return "LWS_CALLBACK_RECEIVE";
        case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
            return "LWS_CALLBACK_WS_PEER_INITIATED_CLOSE";
        default:
            return "";
    }
}

static int
callback_mqtt(struct lws *wsi, enum lws_callback_reasons reason,
              void *user, void *in, size_t len)
{
    struct libws_mqtt_data *data = (struct libws_mqtt_data *)user;

    LOG_DEBUG("Callback called. wsi[%p] reason[%d-%s] user[%p] in[%p] len[%zu]",
            (void*)wsi, reason, get_callback_reason_str(reason), user, in, len);

    switch (reason) {

    case LWS_CALLBACK_WSI_CREATE:
    {
        struct wsi_hash *s = NULL;
        struct mosquitto* new_context;

        HASH_FIND_PTR(wsi_context_map, &wsi, s);
        if(!s){
            
            new_context = mqtt3_context_init(-1);

            if(!new_context){
                return MOSQ_ERR_NOMEM;
            }
            new_context->wsi = wsi;  

            if(mqtt3_db_add_new_context(_mosquitto_get_db(), new_context)){
                LOG_ERROR("WsiCreate: Failed to add new context to db");
                return -1;
            }

            LOG_DEBUG("Create WSI: wsi[%p] num_id[%d] ", (void*)wsi, new_context->numericId);
            s = (struct wsi_hash *)malloc(sizeof(struct wsi_hash));
            s->wsi = wsi;
            s->context = new_context;
            HASH_ADD_PTR(wsi_context_map, wsi, s);
        }

        break;
    }
    case LWS_CALLBACK_WSI_DESTROY:
    {
        struct wsi_hash *s = NULL;

        if(!data || !data->mosq){
            // Client never established a connection. 
            // So, just cleanup only the context.
            HASH_FIND_PTR(wsi_context_map, &wsi, s);
            if(s){
                struct mosquitto_db *db = _mosquitto_get_db();
                struct mosquitto *mosq = s->context;
                if(mosq){
                    if(mosq->state != mosq_cs_ws_dead){
                        mqtt3_context_disconnect(db, mosq);
                    }
                    LOG_DEBUG("Destroy WSI-1: wsi[%p] num_id[%d]", (void*)wsi, mosq->numericId);
                    mosq->wsi = NULL;
                }
                HASH_DEL(wsi_context_map, s);
                free(s);
            }
        }
        else{
            if(data->mosq){
                struct mosquitto_db *db = _mosquitto_get_db();
                struct mosquitto *mosq = data->mosq;
                if(mosq){
                    if(mosq->state != mosq_cs_ws_dead){
                        mqtt3_context_disconnect(db, mosq);
                    }
                    LOG_DEBUG("Destroy WSI: wsi[%p] num_id[%d]", (void*)wsi, mosq->numericId);
                    mosq->wsi = NULL;
                }
                data->mosq = NULL;
            }
        }
        break;
    }

    case LWS_CALLBACK_ADD_POLL_FD:
    {
        lws_pollargs* pa = (lws_pollargs*) in;
        struct epoll_event event = {0};

        LOG_DEBUG("LWS_CALLBACK_ADD_MODE_POLL_FD: wsi[%p] fd[%d] ev[%d] prev_ev[%d]", (void*)wsi, pa->fd, pa->events, pa->prev_events);

        // The very first ADD_POLL_FD is for the listener socket.
        // Unfortunately, there is no other better mechanism to detect this.
        if(ws_listener.wsi == NULL){
            ws_listener.wsi = wsi;
            ws_listener.sock = pa->fd;
            event.data.u32 |= MOSQUITTO_EPOLL_DATA_LISTENER_FLAG;
        }
        else{

            // When ADD_POLL_FD is called, the client is not yet in ESTABLISHED state.
            // So, track the socket within the wsi-context map.
            struct wsi_hash *s = NULL;

            HASH_FIND_PTR(wsi_context_map, &wsi, s);
            if(!s){
                LOG_ERROR("Add-Poll: Could not find wsi in map");
                return -1;
            }
            s->context->ws_sock = pa->fd;
            event.data.u32 = s->context->numericId;
        }
        
        event.data.u64 |= MOSQUITTO_EPOLL_DATA_WEBSOCKETS_FLAG;
        event.events = lwspoll_to_epoll_events(pa->events);                
        
        mosquitto_ws_add_fd(pa->fd, &event);
        break;
    }

    case LWS_CALLBACK_CHANGE_MODE_POLL_FD:
    {
        lws_pollargs* pa = (lws_pollargs*) in;
        struct epoll_event event = {0};

        LOG_DEBUG("LWS_CALLBACK_CHANGE_MODE_POLL_FD: wsi[%p] fd[%d] ev[%d] prev_ev[%d]", (void*)wsi, pa->fd, pa->events, pa->prev_events);
        if(wsi == ws_listener.wsi){
            event.data.u32 |= MOSQUITTO_EPOLL_DATA_LISTENER_FLAG;
        }
        else{
            if(!data || !data->mosq){
                // In this case, the client connection is not yet in established state.
                // Use the wsi-context map for storing the socket value.
                struct wsi_hash *s = NULL;

                HASH_FIND_PTR(wsi_context_map, &wsi, s);
                if(!s){
                    LOG_ERROR("Change-Mode-Poll: Could not find wsi in map");
                    return -1;
                }
                event.data.u32 = s->context->numericId;
            }
            else{
                event.data.u32 = data->mosq->numericId;
            }
        }

        event.data.u64 |= MOSQUITTO_EPOLL_DATA_WEBSOCKETS_FLAG;
        event.events = lwspoll_to_epoll_events(pa->events);

        mosquitto_ws_mod_fd(pa->fd, &event);
        break;
    }

    case LWS_CALLBACK_DEL_POLL_FD:
    {
        lws_pollargs* pa = (lws_pollargs*) in;
        LOG_DEBUG("LWS_CALLBACK_DEL_POLL_FD: wsi[%p] fd[%d]", (void*)wsi, pa->fd);
        mosquitto_ws_del_fd(pa->fd);
        break;
    }

    case LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION:
    {
        // Here are the parameters for this callback:
        // user => X509_STORE_CTX*
        // in => SSL*
        // len => preverify_ok
        // For this callback, return 0, if verification is successful. Non-zero, otherwise.

        struct wsi_hash *s = NULL;

        HASH_FIND_PTR(wsi_context_map, &wsi, s);
        if(s){
            if(!mosquitto_process_client_certificate((X509_STORE_CTX*)user, s->context)){
                LOG_ERROR("Failed to verify client");
                return -1;
            }
        }
        else{
            LOG_ERROR("Unexpected openssl client cert verify callback. WSI: [%p]", wsi);
            return -1;
        }

        if(!len){
            LOG_DEBUG("OpenSSL pre-verification failed. Rejecting the connection.");
            return -1;
        }

        break;
    }

    case LWS_CALLBACK_ESTABLISHED:
    {
        if(!data){
            LOG_ERROR("Established: Data is NULL!");
            return -1;
        }

        struct wsi_hash *s = NULL;

        HASH_FIND_PTR(wsi_context_map, &wsi, s);
        if(s){            

            // From now on, use the "user" parameter to access the context.
            data->mosq = s->context;
            HASH_DEL(wsi_context_map, s);
            free(s);

            // Fill in the address field.
            char clientIP[127] = {0};
            if(lws_get_peer_simple(wsi, clientIP, sizeof(clientIP))){
                data->mosq->address = _mosquitto_strdup(clientIP);
            }
            else{
                data->mosq->address = _mosquitto_strdup("WS-Unknown");
            }
            LOG_DEBUG("Connection from %s established.", data->mosq->address);

        }
        else{
            LOG_ERROR("Established: Could not find wsi in map!");
            return -1;
        }

        break;
    }

    case LWS_CALLBACK_SERVER_WRITEABLE:
    {
        int ret = 0;

        if(!data || !data->mosq)
            return -1;

        // If the connection is marked dead, nothing to do for us.
        if(data->mosq->state == mosq_cs_ws_dead)
            return -1;

        ret = _mosquitto_packet_write(data->mosq);
        LOG_DEBUG("Exiting LWS_CALLBACK_SERVER_WRITEABLE. wsi[%p]", (void*)wsi);
        return ret;
        break;
    }

    case LWS_CALLBACK_RECEIVE:
    {
        int rc = 0;

        LOG_DEBUG("LWS_CALLBACK_RECEIVE: %4d (rpp %5d, first %d, last %d, bin %d)",
              (int)len, (int)lws_remaining_packet_payload(wsi),
              lws_is_first_fragment(wsi),
              lws_is_final_fragment(wsi),
              lws_frame_is_binary(wsi));
        
        if(!data || !data->mosq || (data->mosq->state == mosq_cs_ws_dead))
            return -1;

        struct mosquitto* mosq = data->mosq;
        struct mosquitto_db* db = _mosquitto_get_db();

        if((rc = _mosquitto_packet_read(db, mosq, (uint8_t*) in, (uint32_t)len))){
            if(db->config->connection_messages == true){
                if(mosq->state != mosq_cs_disconnecting){
                    if(IS_NOTICE_ENABLED)
                        _mosquitto_log_printf(NULL, MOSQ_LOG_NOTICE,
                            "Packet read failed for WS client %s, disconnecting.", mosq->id);
                }else{
                    if(IS_NOTICE_ENABLED)
                        _mosquitto_log_printf(NULL, MOSQ_LOG_NOTICE, "WS Client %s disconnected.", mosq->id);
                }
            }
            /* Read error or other that means we should disconnect */
            mqtt3_context_disconnect(db, mosq);
            return rc;
        }
        break;
    }

    default:
        break;
    }

    return 0;
}

int lws_to_mosq_log_level(int lws_level)
{
    switch(lws_level){
        case LLL_ERR:
            return MOSQ_LOG_ERR;
        case LLL_WARN:
            return MOSQ_LOG_WARNING;
        // LWS INFO and NOTICE are quite noisy. So, it is treated as DEBUG level.
        case LLL_NOTICE: /* Fall through */
        case LLL_INFO: /* Fall through */
        case LLL_USER: /* Fall through */
        case LLL_DEBUG:
            return MOSQ_LOG_DEBUG;
    }
    return MOSQ_LOG_DEBUG;
}

void mqtt3_websockets_logger(int level, const char* line)
{
    if(IS_LOGLEVEL_ENABLED(lws_to_mosq_log_level(level)) && IS_CATEGORY_ENABLED(MOSQ_LOG_CATEGORY_WEBSOCKETS)){
        _mosquitto_log_printf(NULL, lws_to_mosq_log_level(level), "<WS> %s", line);
    }
}

void mosquitto_ws_init(struct mqtt3_config *config)
{

    int log_level = 0;
    if(IS_CATEGORY_ENABLED(MOSQ_LOG_CATEGORY_WEBSOCKETS)){
        log_level = LLL_USER | LLL_ERR | LLL_WARN;

        // LWS INFO and NOTICE are quite noisy. So, add them only if debug level logging is enabled.
        if(IS_DEBUG_ENABLED)
            log_level |= LLL_NOTICE | LLL_INFO | LLL_DEBUG; 
    }

    lws_set_log_level(log_level, mqtt3_websockets_logger);

    memset(&lws_ctx_info, 0, sizeof lws_ctx_info);
    lws_ctx_info.port = config->ws_port;
    lws_ctx_info.protocols = protocols;
    lws_ctx_info.pt_serv_buf_size = 1024 * 1024;
    lws_ctx_info.options = LWS_SERVER_OPTION_VALIDATE_UTF8 |
        LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE | 
        LWS_SERVER_OPTION_IPV6_V6ONLY_MODIFY; // This enables single listener for both IPv4 and IPv6.


    if(config->listener_count > 0){
        lws_ctx_info.ssl_ca_filepath = config->listeners[0].cafile;
        lws_ctx_info.ssl_cert_filepath = config->listeners[0].certfile;
        lws_ctx_info.ssl_private_key_filepath = config->listeners[0].keyfile;
        lws_ctx_info.ssl_cipher_list = config->listeners[0].ciphers;
        lws_ctx_info.ssl_options_set = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION | SSL_OP_CIPHER_SERVER_PREFERENCE;
        lws_ctx_info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT |
                        LWS_SERVER_OPTION_SKIP_SERVER_CANONICAL_NAME |
                        LWS_SERVER_OPTION_DISABLE_OS_CA_CERTS |
                        LWS_SERVER_OPTION_REQUIRE_VALID_OPENSSL_CLIENT_CERT; 
    }

    lws_ctx = lws_create_context(&lws_ctx_info);
    if (!lws_ctx) {
        LOG_ERROR("Failed to create LWS context!");
        return;
    }

    LOG_INFO("Websockets server initialized on port %d", config->ws_port);
}

void mosquitto_ws_destroy()
{
    if(lws_ctx){
        lws_context_destroy(lws_ctx);

        // Reset listener
        memset(&ws_listener, 0, sizeof ws_listener);

        // Reset wsi-context map;
        struct wsi_hash *current, *tmp;

        HASH_ITER(hh, wsi_context_map, current, tmp){
            HASH_DEL(wsi_context_map, current);
            free(current);
        }

        LOG_INFO("Websockets server destroyed.");
        lws_ctx = NULL;
    }
}

void mosquitto_ws_handle_poll(struct epoll_event *event)
{
    lws_pollfd p;

    if(mosquitto_epoll_flag_is_listener(event)){
        p.fd = ws_listener.sock;
    }
    else{        
        if(_mosquitto_get_db()->contexts[event->data.u32] == NULL){
            LOG_ERROR("Handle-Poll: Context is NULL. event u32[%d] events [%d]", event->data.u32, event->events);
            return;
        }
        p.fd = _mosquitto_get_db()->contexts[event->data.u32]->ws_sock;
    }
    p.events = epoll_to_lwspoll_events(event->events);
    p.revents = p.events;

    LOG_DEBUG(">mosquitto_ws_handle_poll: fd[%d] events[%d] revents[%d]", p.fd, p.events, p.revents);
    lws_service_fd(lws_ctx, &p);
    LOG_DEBUG("<mosquitto_ws_handle_poll");
}

void mosquitto_ws_request_writeable_callback(struct mosquitto *mosq)
{
    lws_callback_on_writable((struct lws*)(mosq->wsi));
}

int mosquitto_ws_write(struct mosquitto *mosq, uint8_t* buf, uint32_t len)
{
    return lws_write((struct lws*)(mosq->wsi), buf, len, LWS_WRITE_BINARY);
}

void mosquitto_ws_do_maintenance()
{
    lws_service_fd(lws_ctx, NULL);
}