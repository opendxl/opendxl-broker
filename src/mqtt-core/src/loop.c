/*
Copyright (c) 2009-2013 Roger Light <roger@atchoo.org>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. Neither the name of mosquitto nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <config.h>

#include <assert.h>
#include <poll.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>

#include <string.h>
#include <limits>
#include <inttypes.h>
#include <stdbool.h>

#include <mosquitto_broker.h>
#include <memory_mosq.h>
#include <time_mosq.h>
#include <util_mosq.h>

#include "uthash.h"

// DXL Begin
#include "dxl.h"
#include <malloc.h>
#include "search_optimization.h"
// DXL End

// EPOLL Begin
#include <sys/epoll.h>
#include <errno.h>

#ifndef EPOLLRDHUP
/* Ignore EPOLLRDHUP flag on systems where it doesn't exist. */
#define EPOLLRDHUP 0
#endif
// EPOLL End

extern int run;

static int epoll_add_listeners();
static void handle_read(struct mosquitto_db *db, struct mosquitto *context, struct epoll_event *event);
static void handle_write(struct mosquitto_db *db, struct mosquitto *context,
    struct epoll_event *event, uint32_t contextId);
static void loop_handle_reads_writes(struct mosquitto_db *db, struct epoll_event *events, int eventcount);

/* The EPOLL file descriptor */
static int efd = 0;

#include "uthash.h"
static bool new_client = false;

struct context_hash {
    struct mosquitto *context;
    UT_hash_handle hh;
};

static struct context_hash *new_msgs_hash = NULL;

void mosquitto_add_new_msgs_set(struct mosquitto* context)
{
    struct context_hash *s = NULL;

    HASH_FIND_PTR(new_msgs_hash, (&context), s);
    if(!s){
        s = (struct context_hash *)malloc(sizeof(struct context_hash));
        s->context = context;
        HASH_ADD_PTR(new_msgs_hash, context, s);
    }
}

static void remove_new_msgs_set(struct mosquitto* context)
{
    struct context_hash *s = NULL;

    HASH_FIND_PTR(new_msgs_hash, (&context), s);
    if(s){
        HASH_DEL(new_msgs_hash, s);
        free(s);
    }
}

static struct context_hash *new_clients_hash = NULL;

void mosquitto_add_new_clients_set(struct mosquitto* context)
{
    struct context_hash *s = NULL;
    new_client = true;

    HASH_FIND_PTR(new_clients_hash, (&context), s);
    if(!s){
        s = (struct context_hash *)malloc(sizeof(struct context_hash));
        s->context = context;
        HASH_ADD_PTR(new_clients_hash, context, s);
    }
}

static void remove_new_clients_set(struct mosquitto* context)
{
    struct context_hash *s = NULL;

    HASH_FIND_PTR(new_clients_hash, (&context), s);
    if(s){
        HASH_DEL(new_clients_hash, s);
        free(s);
    }
}

/*
 * DXL:
 * Cleans sessions that are currently marked for cleaning
 * db - The database
 */
static void clean_sessions(struct mosquitto_db *db);
// DXL: The sessions to clean (via clean_sessions)
static struct mosquitto **sessions_to_clean = NULL;
// DXL: The size of the session to clean array
static int sessions_to_clean_size = 0;

/*
 * DXL: Resizes the clean sessions array to match the current context array size
 * db - The database
 */
static void resize_sessions_to_clean(struct mosquitto_db *db)
{
    if(!sessions_to_clean || sessions_to_clean_size < db->context_count){
        if(sessions_to_clean){
            free(sessions_to_clean);
        }
        sessions_to_clean_size = db->context_count;
        sessions_to_clean = (struct mosquitto **)malloc(sessions_to_clean_size * sizeof(struct mosquitto *));
        memset(sessions_to_clean, 0, sessions_to_clean_size * sizeof(struct mosquitto *));
    }
}

void write_context_messages(struct mosquitto_db *db, struct mosquitto *context, time_t now)
{
    /* Local bridges never time out in this fashion. */
    if(!(context->keepalive) 
            || context->bridge
            || now - context->last_msg_in < (time_t)(context->keepalive)*3/2){

        if(mqtt3_db_message_write(context) != MOSQ_ERR_SUCCESS){
            mqtt3_context_disconnect(db, context);
        }
    }else{
        if(db->config->connection_messages == true){
            if(IS_NOTICE_ENABLED)
                _mosquitto_log_printf(NULL, MOSQ_LOG_NOTICE,
                    "Client %s has exceeded timeout, disconnecting.", context->id);
        }
        /* Client has exceeded keepalive*1.5 */
        mqtt3_context_disconnect(db, context);
    }
}

void restart_bridge_connection(struct mosquitto_db *db, struct mosquitto *context, time_t now, uint32_t ctx_idx)
{
    /* Want to try to restart the bridge connection */
    if(!context->bridge->restart_t){
        context->bridge->restart_t = 1; // DXL
        context->bridge->cur_address++;
        if(context->bridge->cur_address == context->bridge->address_count){
            context->bridge->cur_address = 0;
        }
        // DXL
        if(context->bridge->round_robin == false &&
            context->bridge->cur_address >= context->bridge->primary_address_count){
            if(IS_DEBUG_ENABLED)
                _mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG,
                    "Setting primary retry timeout, current address:%d, primary count:%d",
                    context->bridge->cur_address, context->bridge->primary_address_count);
            context->bridge->primary_retry = now + 5;
        }
    } 
    
    // DXL - We don't want to wait for the next maintenance invocation
    if(context->bridge->start_type == bst_lazy && context->bridge->lazy_reconnect){
        int rc = mqtt3_bridge_connect(db, context);
        if(rc){
            context->bridge->cur_address++;
            if(context->bridge->cur_address == context->bridge->address_count){
                context->bridge->cur_address = 0;
            }
        }
    }
    if(context->bridge->start_type == bst_automatic && now > context->bridge->restart_t){
        mosquitto_remove_context(context); // Remove context from EPOLL

        // This static assumes we only ever have one bridge we are initiating (we only ever have one parent)
        // from this broker. If we wanted to allow for more, we would have to associate
        // the connection ID with each context that is attempting to bridge.
        const static size_t check_connection_id = dxl_check_connection_create_id();
        int status = dxl_check_connection_get_status(check_connection_id);
        if(IS_INFO_ENABLED)
            _mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "dxl_check_connection_get_status: %d, %d, %d",
                context->numericId, (int)check_connection_id, status);

        if(status == dxl_check_connect_nothing){
            dxl_check_connection_push(
                check_connection_id, 
                context->bridge->addresses[context->bridge->cur_address].address,
                context->bridge->addresses[context->bridge->cur_address].port
            );
            if(IS_INFO_ENABLED)
                _mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "dxl_check_connection_push: %d, %d, %s:%d",
                    context->numericId, (int)check_connection_id, 
                    context->bridge->addresses[context->bridge->cur_address].address, 
                    context->bridge->addresses[context->bridge->cur_address].port);
        }else if(status == dxl_check_connect_has_result){
            struct dxl_check_connection_result* result = dxl_check_connection_pop(check_connection_id);
            if(result != NULL){
                context->bridge->restart_t = 0;
                char* host = context->bridge->addresses[context->bridge->cur_address].address;
                uint16_t port = context->bridge->addresses[context->bridge->cur_address].port;
                if(!strcmp(host, result->host) && port == result->port){
                    if(result->result == MOSQ_ERR_SUCCESS){
                        if(IS_INFO_ENABLED)
                            _mosquitto_log_printf(NULL, MOSQ_LOG_INFO,
                                "Connection check successfully found parent %s, trying to establish connection...", 
                                result->host);
                        int rc = mqtt3_bridge_connect(db, context);
                        if(rc == MOSQ_ERR_SUCCESS){
                            mosquitto_update_context(ctx_idx, context); // Add context to EPOLL
                        }
                    }else{
                        if(IS_INFO_ENABLED)
                            _mosquitto_log_printf(NULL, MOSQ_LOG_INFO,
                                "Connection check was unable to connect to broker: %s", 
                                result->host);
                    }
                }else{
                    if(IS_INFO_ENABLED)
                        _mosquitto_log_printf(NULL, MOSQ_LOG_INFO,
                            "Connection check broker %s is not the same as the context broker, %s", 
                            result->host, host);
                }
                dxl_check_connection_free_result(result);
            }
        }else{
            /* Retry later. */
            // DXL: This has been purposely removed. The issue is that a bug occurs
            // if the "primary" address has been ping'd successfully, but Mosquitto
            // is unable to bridge to it (it went down between the ping and connect
            // attempt). If it gets in this state, it will forever attempt to connect
            // to the primary and never round-robin the other addresses. By disabling
            // this, If it is unable to connect, it will move to the next address.
            //context->bridge->restart_t = now+context->bridge->restart_timeout;
        }
    }
}

void clean_session(struct mosquitto_db *db, uint32_t ctx_idx)
{
    // DXL Begin
    resize_sessions_to_clean(db);
    if(sessions_to_clean_size > 0){
        sessions_to_clean[0] = db->contexts[ctx_idx];
        db->contexts[ctx_idx]->clean_subs = true;
        clean_sessions(db);
    }
    // DXL End
}

/*
 * DXL:
 * Cleans sessions that are currently marked for cleaning. This method
 * should only ever be invoked by the "maintenance loop"
 * db - The database
 */
static void clean_sessions(struct mosquitto_db *db)
{
    bool cleaned_subs = false;    
    for(int i = 0; i < sessions_to_clean_size; i++){
        mosquitto* ctx = sessions_to_clean[i];
        if(!ctx){
            break;
        }

        if(!cleaned_subs){
            // Clean the subscriptions
            mqtt3_subs_clean_session(db, &db->subs);
            cleaned_subs = true;
        }

        /*
        * Remove context from EPOLL
        */
        mosquitto_remove_context(ctx);
        remove_new_msgs_set(ctx);
        int contextIndex = ctx->numericId;
        mqtt3_context_cleanup(db, ctx, true, false);
        db->contexts[contextIndex] = NULL;

        // Clear session to clean
        sessions_to_clean[i] = NULL;
    }
}

void write_message_loop(struct mosquitto_db *db)
{
    struct context_hash *hash = NULL, *tmp = NULL;

    if(new_client){
        HASH_ITER(hh, new_clients_hash, hash, tmp){
            struct mosquitto *context = hash->context;
            if(context->sock != INVALID_SOCKET){
                mosquitto_update_context(context->numericId, context); // EPOLL
            }
            remove_new_clients_set(context);
        }
        new_client = false;
    }

    hash = NULL;
    tmp = NULL;

    HASH_ITER(hh, new_msgs_hash, hash, tmp){
        struct mosquitto *context = hash->context;
        if(!IS_CONTEXT_INVALID(context)){ 
            write_context_messages(db, context, mosquitto_time()); // EPOLL
            if(!context->msgs){
                remove_new_msgs_set(context);
            }
        }
    }
}

static void checkPrimaryBridge(time_t now, mosquitto* context)
{    
    struct _mqtt3_bridge* bridge = context->bridge;

    if(bridge->round_robin == false &&
       bridge->cur_address >= bridge->primary_address_count &&
       now > bridge->primary_retry){
        if(IS_INFO_ENABLED)
            _mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "Attempting to ping primary address:%d, primary count:%d",
                bridge->cur_primary_address, bridge->primary_address_count);

        // Attempt to ping using the current primary address
        int cur_primary_address = bridge->cur_primary_address;

        const static size_t check_connection_id = dxl_check_connection_create_id();
        int status = dxl_check_connection_get_status(check_connection_id);
        if(IS_INFO_ENABLED)
            _mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "(primary ping) dxl_check_connection_get_status: %d, %d, %d",
                context->numericId, (int)check_connection_id, status);
        if(status == dxl_check_connect_nothing){
            dxl_check_connection_push(
                check_connection_id, 
                bridge->addresses[cur_primary_address].address,
                bridge->addresses[cur_primary_address].port);

            if(IS_INFO_ENABLED)
                _mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "(primary ping) dxl_check_connection_push: %d, %d, %s:%d",
                    context->numericId, (int)check_connection_id, 
                    bridge->addresses[cur_primary_address].address, bridge->addresses[cur_primary_address].port);
        }else if(status == dxl_check_connect_has_result){
            struct dxl_check_connection_result* result = dxl_check_connection_pop(check_connection_id);
            if(result != NULL){
                char* host = bridge->addresses[cur_primary_address].address;
                uint16_t port = bridge->addresses[cur_primary_address].port;
                if(!strcmp(host, result->host) && port == result->port){
                    if(result->result == MOSQ_ERR_SUCCESS){
                        if(IS_INFO_ENABLED)
                            _mosquitto_log_printf(NULL, MOSQ_LOG_INFO,
                                "(primary ping) Connection check successfully found parent %s, trying to establish connection...",
                                result->host);

                        // Notify that a bridge has been disconnected
                        dxl_on_bridge_disconnected(context);

                        _mosquitto_socket_close(context);

                        // Set the address that we successfully ping'd for the next reconnect attempt
                        bridge->cur_address = 
                            (cur_primary_address == 0 ? bridge->address_count - 1 : cur_primary_address - 1);
                    }else{
                        if(IS_INFO_ENABLED)
                            _mosquitto_log_printf(NULL, MOSQ_LOG_INFO,
                                "(primary ping) Connection check was unable to connect to broker: %s",
                                result->host);

                        // Move to the next primary address
                        cur_primary_address++;
                        bridge->cur_primary_address = 
                            (cur_primary_address >= bridge->primary_address_count ? 0 : cur_primary_address);
                    }
                }else{
                    if(IS_INFO_ENABLED)
                        _mosquitto_log_printf(NULL, MOSQ_LOG_INFO,
                            "(primary ping) Connection check broker %s is not the same as the context broker, %s",
                            result->host, host);
                }
                dxl_check_connection_free_result(result);
            }
        } 
    } 
}

void maintenance_loop(struct mosquitto_db *db)
{

    time_t now = 0;
    int i;


    int clean_session_index = 0; // DXL
    resize_sessions_to_clean(db); // DXL


    for(i=0; i<db->context_count; i++){
        if(db->contexts[i]){
            now = mosquitto_time();            
            db->contexts[i]->pollfd_index = -1;

            if(!IS_CONTEXT_INVALID(db->contexts[i])){
                if(db->contexts[i]->bridge){
                    _mosquitto_check_keepalive(db->contexts[i]);

                    // DXL Begin
                    if(db->contexts[i]->sock == INVALID_SOCKET){ 
                        // Bridge connection timed out, fire a bridge disconnected event
                        dxl_on_bridge_disconnected(db->contexts[i]);
                    }
                    // DXL End

                    checkPrimaryBridge(now, db->contexts[i]);
                } 
                if(!db->contexts[i]->bridge &&
                    db->contexts[i]->keepalive &&
                    ((now - db->contexts[i]->last_msg_in) >= ((time_t)(db->contexts[i]->keepalive)*3/2))){
                    if(db->config->connection_messages == true){
                        if(IS_NOTICE_ENABLED)
                            _mosquitto_log_printf(NULL, MOSQ_LOG_NOTICE,
                            "Client %s has exceeded timeout, disconnecting.", db->contexts[i]->id);
                    }

                    /* Client has exceeded keepalive*1.5 */
                    mqtt3_context_disconnect(db, db->contexts[i]);
                }
            }else{
                if(db->contexts[i]->bridge){
                    restart_bridge_connection(db, db->contexts[i], now, i); // EPOLL
                }else{ 
                    if(db->contexts[i]->clean_session == true){
                        if(IS_DEBUG_ENABLED)
                            _mosquitto_log_printf(NULL, MOSQ_LOG_NOTICE,
                            "Cleaning session %s with index %d and numeric_id %d.", 
                            db->contexts[i]->id, i, db->contexts[i]->numericId);
                        // DXL Begin
                        sessions_to_clean[clean_session_index++] = db->contexts[i];
                        db->contexts[i]->clean_subs = true;
                        //clean_session(db, i);
                        // DXL End
                    }
                }
            }
        }

        if(db->contexts[i] && db->contexts[i]->msgs)
            mqtt3_db_message_timeout_check(db->contexts[i], db->config->retry_interval);
    } 

    // DXL Begin
    if(clean_session_index > 0){
        clean_sessions(db);
    }
    // DXL End

    // DXL: Force trim of memory. For some reason the allocation pattern of Mosquitto
    // causes memory to not be returned to the OS. This forces the memory to be returned.
    malloc_trim(0);

}

static struct epoll_event epoll_events[MAXEVENTS];

// DXL
int loop_exit_code = MOSQ_ERR_SUCCESS;

int mosquitto_epoll_init()
{
    /*  
     * EPOLL related setup
     */
    efd = epoll_create(0x1FFFE);
    if(efd == -1){
        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: %s.", strerror(errno));
        return 1;
    }   
    return 0;
}

void mosquitto_epoll_destroy()
{
}

int mosquitto_main_loop(struct mosquitto_db *db)
{
    time_t start_time = mosquitto_time();
    time_t last_store_clean = mosquitto_time();
    int fdcount;
    int i;

    sigset_t sigblock;
    sigemptyset(&sigblock);
    sigaddset(&sigblock, SIGINT);

    time_t do_maintenance = start_time; // Loop OPT

    // Add listeners to EPOLL
    if(epoll_add_listeners()){
        return 1;
    }

    while(run){
        write_message_loop(db);
        /* The wait time is 100 milliseconds.  Run this loop every 10 seconds. */
        time_t now = mosquitto_time();
        if(now >= do_maintenance){
            maintenance_loop(db);
            dxl_on_maintenance(now);
            if(db->config->ws_enabled){
                mosquitto_ws_do_maintenance();
            }
            do_maintenance = now + 10;
        }

        /* See if there are any events */
        fdcount = epoll_pwait(efd, epoll_events, MAXEVENTS, 100, &sigblock);
        if(fdcount == -1){
            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                "epoll_wait: error %d", errno);
        }else{
            loop_handle_reads_writes(db, epoll_events, fdcount);

            /*
             * Check listeners 
             */
            int *listensock = mosquitto_get_listensocks();
            for(i=0; i<fdcount; i++){
                if(mosquitto_epoll_flag_is_listener(&epoll_events[i])){
                    if(mosquitto_epoll_flag_is_websocket(&epoll_events[i])){
                        mosquitto_ws_handle_poll(&epoll_events[i]);
                        
                    }
                    else{
                        if(epoll_events[i].events & (EPOLLIN | EPOLLPRI)){
                            uint32_t ctx_idx=epoll_events[i].data.u32;
                            while(mqtt3_socket_accept(db, listensock[ctx_idx^MOSQUITTO_EPOLL_DATA_LISTENER_FLAG]) != -1){
                            }
                        }
                    }
                }
            }
        }
        if(!db->config->store_clean_interval || last_store_clean + db->config->store_clean_interval < mosquitto_time()){
            mqtt3_db_store_clean(db);
            last_store_clean = mosquitto_time();
        }

        // Run the work queue (if there are any pending tasks)
        dxl_run_work_queue();
    }

    return loop_exit_code; // DXL
}

static void do_disconnect(struct mosquitto_db *db, int context_index)
{
    if(db->config->connection_messages == true){
        if(db->contexts[context_index]->state != mosq_cs_disconnecting){
            if(IS_NOTICE_ENABLED)
                _mosquitto_log_printf(NULL, MOSQ_LOG_NOTICE,
                    "Socket error on client %s, disconnecting.", db->contexts[context_index]->id);
        }else{
            if(IS_NOTICE_ENABLED)
                _mosquitto_log_printf(NULL, MOSQ_LOG_NOTICE,
                    "Client %s disconnected.", db->contexts[context_index]->id);
        }
    }
    mqtt3_context_disconnect(db, db->contexts[context_index]);
}

static void loop_handle_reads_writes(struct mosquitto_db *db, struct epoll_event* events, int eventcount)
{
    int i;

    for(i=0; i<eventcount; i++){
        struct epoll_event *event = &events[i];
        if(mosquitto_epoll_flag_is_listener(event)){
            continue;/* Skip listeners */
        }
        if(mosquitto_epoll_flag_is_websocket(event)){
            mosquitto_ws_handle_poll(event);
            continue;
        }
        uint32_t contextid = event->data.u32;
        struct mosquitto *context = db->contexts[contextid];

        handle_write(db, context, event, contextid);

        handle_read(db, context, event);

        if(context && context->sock != INVALID_SOCKET){
            if(event->events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)){ /* DXL: 1.3.5 uses POLLERR | POLLNVAL */
                do_disconnect(db, contextid);
            }
        }
    }
}

int mosquitto_epoll_restart_listeners()
{    
    // Remove listeners from EPOLL
    for(int i = 0; i < mosquitto_get_listensock_count(); i++){
        int s = 0;
        int sock = mosquitto_get_listensocks()[i];
        if(sock != INVALID_SOCKET){
            struct epoll_event event;
            s =    epoll_ctl(efd, EPOLL_CTL_DEL, sock, &event);
            if(s == -1){
                _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: %s.", strerror(errno));
            }
        }
    }
    
    // Free global socket information
    mosquitto_set_listensocks(NULL, 0);

    int* listensock = NULL;
    int listensock_count = 0;

    // Close listener sockets
    struct mosquitto_db* db = _mosquitto_get_db();
    struct mqtt3_config* config = db->config;
    for(int i = 0; i < config->listener_count; i++){
        struct _mqtt3_listener* listener = &config->listeners[i];
        for(int j = 0; j < listener->sock_count; j++){
            COMPAT_CLOSE(listener->socks[j]);
            if(listener->ssl_ctx){
                SSL_CTX_free(listener->ssl_ctx);
                listener->ssl_ctx = NULL;
            }
        }

        _mosquitto_free(listener->socks);
        listener->sock_count = 0;
        listener->socks = NULL;
    }

    if(config->ws_enabled){
        mosquitto_ws_destroy();
    }

    // Create listener sockets
    int listensock_index = 0;
    for(int i = 0; i < config->listener_count; i++){
        if(mqtt3_socket_listen(&config->listeners[i])){
            return 1;
        }
        listensock_count += config->listeners[i].sock_count;
        listensock = (int *)_mosquitto_realloc(listensock, sizeof(int)* listensock_count);
        if(!listensock){
            return 1;
        }

        for(int j = 0; j < config->listeners[i].sock_count; j++){
            if(config->listeners[i].socks[j] == INVALID_SOCKET){
                return 1;
            }
            listensock[ listensock_index ] = config->listeners[i].socks[j];
            
            listensock_index++;
        }
    }
    // Set new listener sockets
    mosquitto_set_listensocks(listensock, listensock_count);

    // Add listeners to EPOLL
    epoll_add_listeners();

    if(config->ws_enabled){
        mosquitto_ws_init(config);
    }

    return 0;
}

int mosquitto_ws_add_fd(int sock, struct epoll_event* event)
{       
    if(epoll_ctl(efd, EPOLL_CTL_ADD, sock, event) == -1){
        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "(mosquitto_ws_add_fd) Error: %s.", strerror(errno));
        return 1;
    }
    return 0;
}

int mosquitto_ws_mod_fd(int sock, struct epoll_event* event)
{
    if(epoll_ctl(efd, EPOLL_CTL_MOD, sock, event) == -1){
        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "(mosquitto_ws_mod_fd) Error: %s.", strerror(errno));
        return 1;
    }

    return 0;
}

int mosquitto_ws_del_fd(int sock)
{       
    struct epoll_event event = {0};
    if(epoll_ctl(efd, EPOLL_CTL_DEL, sock, &event) == -1){
        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "(mosquitto_ws_del_fd) Error: %s.", strerror(errno));
        return 1;
    }

    return 0;
}

static int epoll_add_listeners()
{
    /*
     * Add listeners to EPOLL, add a flag to indicate that it is a listener
     */
    for(int i = 0; i < mosquitto_get_listensock_count(); i++){
        int sock = mosquitto_get_listensocks()[i];
        struct epoll_event event = {0};
        event.data.u32 = (MOSQUITTO_EPOLL_DATA_LISTENER_FLAG | i);
        event.events = EPOLLIN;
        int s = epoll_ctl(efd, EPOLL_CTL_ADD, sock, &event);
        if(s == -1){
            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: %s.", strerror(errno));
            return 1;
        }
    }

    return 0;
}

bool mosquitto_epoll_flag_is_listener(struct epoll_event *event)
{
    return (event->data.u32 & MOSQUITTO_EPOLL_DATA_LISTENER_FLAG) != 0;
}

static void handle_write(struct mosquitto_db *db, struct mosquitto *context, struct epoll_event *event, uint32_t contextId)
{
    if(context && context->sock != INVALID_SOCKET){
        //assert(pollfds[db->contexts[i]->pollfd_index].fd == db->contexts[i]->sock);
        if(event->events & EPOLLOUT || context->want_write ||
            (context->ssl && context->state == mosq_cs_new)){
            if(_mosquitto_packet_write(context)){
                if(db->config->connection_messages == true){
                    if(context->state != mosq_cs_disconnecting){
                        if(IS_NOTICE_ENABLED)
                            _mosquitto_log_printf(NULL, MOSQ_LOG_NOTICE,
                                "Socket write error on client %s, disconnecting.", context->id);
                    }else{
                        if(IS_NOTICE_ENABLED)
                            _mosquitto_log_printf(NULL, MOSQ_LOG_NOTICE,
                                "Client %s disconnected.", context->id);
                    }
                }
                /* Write error or other that means we should disconnect */
                mqtt3_context_disconnect(db, context);
            }

            // Update context, if no packets left, EPOLLOUT will be removed
            mosquitto_update_context(contextId, context);
        }
    }
}

static void handle_read(struct mosquitto_db *db, struct mosquitto *context, struct epoll_event *event)
{
    if(context && context->sock != INVALID_SOCKET){
        //assert(pollfds[db->contexts[i]->pollfd_index].fd == db->contexts[i]->sock);
        if(event->events & EPOLLIN || (context->ssl && context->state == mosq_cs_new)){
            if(_mosquitto_packet_read(db, context, NULL, 0)){
                if(db->config->connection_messages == true){
                    if(context->state != mosq_cs_disconnecting){
                        if(IS_NOTICE_ENABLED)
                            _mosquitto_log_printf(NULL, MOSQ_LOG_NOTICE,
                                "Socket read error on client %s, disconnecting.", context->id);
                    }else{
                        if(IS_NOTICE_ENABLED)
                            _mosquitto_log_printf(NULL, MOSQ_LOG_NOTICE, "Client %s disconnected.", context->id);
                    }
                }
                /* Read error or other that means we should disconnect */
                mqtt3_context_disconnect(db, context);
            }
        }
    }
}

void mosquitto_remove_context(struct mosquitto *context)
{
    int s = 0;
    if(context->sock != INVALID_SOCKET){
        struct epoll_event event;
        s = epoll_ctl(efd, EPOLL_CTL_DEL, context->sock, &event);
        if(s == -1){
            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: %s.", strerror(errno));
        }
    }
}

void mosquitto_epoll_update_context_index(struct mosquitto *context, uint32_t new_ctx_idx)
{
    struct epoll_event event = {0};
    event.data.u32 = new_ctx_idx;
    event.events = context->epoll_events;
    epoll_ctl(efd, EPOLL_CTL_MOD, context->sock, &event);
}

void mosquitto_update_context(uint32_t ctx_idx, struct mosquitto *context)
{
    int s = 0;
    struct epoll_event event = {0};
    event.data.u32 = ctx_idx;
    event.events = (EPOLLIN | EPOLLRDHUP);
    if(context->current_out_packet){
        event.events |= EPOLLOUT;
    }
    if(event.events != context->epoll_events){
        if(!context->epoll_events){
            s = epoll_ctl(efd, EPOLL_CTL_ADD, context->sock, &event);
        }else{
            s = epoll_ctl(efd, EPOLL_CTL_MOD, context->sock, &event);
        }
        context->epoll_events = event.events;
        if(s == -1){
            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: %s.", strerror(errno));
        }
    }
}
