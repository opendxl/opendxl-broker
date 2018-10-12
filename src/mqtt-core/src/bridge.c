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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <netdb.h>
#include <sys/socket.h>

#include <config.h>

#include <mosquitto.h>
#include <mosquitto_broker.h>
#include <mosquitto_internal.h>
#include <net_mosq.h>
#include <memory_mosq.h>
#include <send_mosq.h>
#include <time_mosq.h>
#include <tls_mosq.h>
#include <util_mosq.h>
#include <will_mosq.h>

// DXL Begin
// The default bridge keepalive in seconds
static int default_bridge_keepalive = 60;

void mqtt3_set_default_bridge_keepalive(int keepalive)
{
    default_bridge_keepalive = keepalive;
}
// DXL End

int mqtt3_bridge_new(struct mosquitto_db *db, struct _mqtt3_bridge *bridge)
{
    int i;
    struct mosquitto *new_context = NULL;
    int null_index = -1;
    struct mosquitto **tmp_contexts;
    char hostname[256];
    int len;
    char *id;

    assert(db);
    assert(bridge);

    if(bridge->clientid){
        id = _mosquitto_strdup(bridge->clientid);
    }else{
        if(!gethostname(hostname, 256)){
            len = (int)(strlen(hostname) + strlen(bridge->name) + 2);
            id = (char *)_mosquitto_malloc(len);
            if(!id){
                return MOSQ_ERR_NOMEM;
            }
            snprintf(id, len, "%s.%s", hostname, bridge->name);
        }else{
            return 1;
        }
    }
    if(!id){
        return MOSQ_ERR_NOMEM;
    }

/* 
 * DXL: We always force a new context, because the following code does not properly reset
 * the context's current state, which leads to numerous bugs...
 */

    // DXL modified (no longer looks for existing identifier)
    /* Look for a gap in the db->contexts[] array */
    for(i=0; i<db->context_count; i++){
        if(db->contexts[i] == NULL && null_index == -1){
            null_index = i;
            break;
        }
    }

    if(!new_context){
        /* id wasn't found, so generate a new context */
        new_context = mqtt3_context_init(-1);
        if(!new_context){
            return MOSQ_ERR_NOMEM;
        }

        // DXL Begin
        new_context->canonical_id = _mosquitto_strdup(id);
        if(!new_context->canonical_id){
            return MOSQ_ERR_NOMEM;
        }
        // DXL End

        if(null_index == -1){
            /* There were no gaps in the db->contexts[] array, so need to append. */
            tmp_contexts = (struct mosquitto **)_mosquitto_realloc(
                db->contexts, sizeof(struct mosquitto*)*(db->context_count+CONTEXT_REALLOC_SIZE));
            if(tmp_contexts){
                null_index = db->context_count;
                memset(tmp_contexts+db->context_count, 0, (CONTEXT_REALLOC_SIZE*sizeof(struct mosquitto*)));                  
                db->contexts = tmp_contexts;                                
                db->context_count+=CONTEXT_REALLOC_SIZE;
                db->contexts[null_index] = new_context;
                mosquitto_add_new_clients_set(new_context); // Loop OPT
            }else{
                _mosquitto_free(new_context);
                return MOSQ_ERR_NOMEM;
            }
        }else{
            db->contexts[null_index] = new_context;
            mosquitto_add_new_clients_set(new_context); // Loop OPT
        }
        new_context->id = id;
        // DXL Begin
        new_context->numericId = null_index;
        new_context->db_index = null_index;
        // DXL End
    }else{
        /* id was found, so context->id already in memory. */
        _mosquitto_free(id);
    }
    new_context->bridge = bridge;
    new_context->is_bridge = true;

    new_context->tls_cafile = new_context->bridge->tls_cafile;
    new_context->tls_capath = new_context->bridge->tls_capath;
    new_context->tls_certfile = new_context->bridge->tls_certfile;
    new_context->tls_keyfile = new_context->bridge->tls_keyfile;
    new_context->tls_cert_reqs = SSL_VERIFY_PEER;
    new_context->tls_version = new_context->bridge->tls_version;
    new_context->tls_insecure = new_context->bridge->tls_insecure;

    bridge->try_private_accepted = true;

    // DXL: We force it to be asynchronous via "bridge retries"
    // Attempting to bridge here would pause the broker whenever bridging changes were made.
    return MOSQ_ERR_NO_CONN;
}

int mqtt3_bridge_connect(struct mosquitto_db *db, struct mosquitto *context)
{
    int rc;
    int i;

    if(!context || !context->bridge) return MOSQ_ERR_INVAL;

    context->state = mosq_cs_new;
    context->sock = -1;
    context->last_msg_in = mosquitto_time();
    context->last_msg_out = mosquitto_time();
    context->keepalive = default_bridge_keepalive; // DXL
    context->clean_session = context->bridge->clean_session;
    context->in_packet.payload = NULL;
    context->ping_t = 0;
    context->bridge->lazy_reconnect = false;
    context->epoll_events = 0; // EPOLL

    mqtt3_bridge_packet_cleanup(context);
    mqtt3_db_message_reconnect_reset(context);

    if(context->clean_session){
        mqtt3_db_messages_delete(context);
    }

    /* Delete all local subscriptions even for clean_session==false. We don't
     * remove any messages and the next loop carries out the resubscription
     * anyway. This means any unwanted subs will be removed.
     */

    // DXL Begin
    context->clean_subs = true;
    mqtt3_subs_clean_session(db, &db->subs);
    context->clean_subs = false;
    // DXL End

    for(i=0; i<context->bridge->topic_count; i++){
        if(context->bridge->topics[i].direction == bd_out || context->bridge->topics[i].direction == bd_both){
            if(IS_DEBUG_ENABLED)
                _mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG,
                    "Bridge %s doing local SUBSCRIBE on topic %s",
                    context->id, context->bridge->topics[i].local_topic);
            if(mqtt3_sub_add(db, context, context->bridge->topics[i].local_topic,
                context->bridge->topics[i].qos, &db->subs)) return 1;
        }
    }

    if(IS_NOTICE_ENABLED)
        _mosquitto_log_printf(NULL, MOSQ_LOG_NOTICE,
            "Connecting bridge %s (%s:%d)", context->bridge->name,
                context->bridge->addresses[context->bridge->cur_address].address,
                context->bridge->addresses[context->bridge->cur_address].port);
    rc = _mosquitto_socket_connect(context,
            context->bridge->addresses[context->bridge->cur_address].address,
            context->bridge->addresses[context->bridge->cur_address].port, NULL, true);
    if(rc != MOSQ_ERR_SUCCESS){
        if(rc == MOSQ_ERR_TLS){
            return rc; /* Error already printed */
        }else if(rc == MOSQ_ERR_ERRNO){
            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                "Error creating bridge to %s: %s.",
                context->bridge->addresses[context->bridge->cur_address].address, strerror(errno)); // DXL
        }else if(rc == MOSQ_ERR_EAI){
            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                "Error creating bridge to %s: %s.", context->bridge->addresses[context->bridge->cur_address].address,
                gai_strerror(errno)); // DXL
        }

        return rc;
    }

    rc = _mosquitto_send_connect(context, context->keepalive, context->clean_session);
    if(rc == MOSQ_ERR_SUCCESS){
        return MOSQ_ERR_SUCCESS;
    }else if(rc == MOSQ_ERR_ERRNO && errno == ENOTCONN){
        return MOSQ_ERR_SUCCESS;
    }else{
        if(rc == MOSQ_ERR_TLS){
            return rc; /* Error already printed */
        }else if(rc == MOSQ_ERR_ERRNO){
            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error creating bridge: %s.", strerror(errno));
        }else if(rc == MOSQ_ERR_EAI){
            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error creating bridge: %s.", gai_strerror(errno));
        }
        _mosquitto_socket_close(context);
        return rc;
    }
}

void mqtt3_bridge_packet_cleanup(struct mosquitto *context)
{
    struct _mosquitto_packet *packet;
    if(!context) return;

    _mosquitto_packet_cleanup(context->current_out_packet);
    _mosquitto_free(context->current_out_packet); // DXL
    context->current_out_packet = NULL; //DXL
    while(context->out_packet){
        _mosquitto_packet_cleanup(context->out_packet);
        packet = context->out_packet;
        context->out_packet = context->out_packet->next;
        _mosquitto_free(packet);
    }

#ifdef PACKET_COUNT
    context->packet_count = 0;
#endif

    _mosquitto_packet_cleanup(&(context->in_packet));
}
