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

#include <config.h>

#include <mosquitto_broker.h>
#include <memory_mosq.h>
#include <time_mosq.h>

#include "uthash.h"

#include "dxl.h"

struct mosquitto *mqtt3_context_init(int sock)
{
    struct mosquitto *context;
    char address[1024];

    context = (struct mosquitto *)_mosquitto_calloc(1, sizeof(struct mosquitto));
    if(!context) return NULL;
    
    context->state = mosq_cs_new;
    context->sock = sock;    
    context->last_msg_in = mosquitto_time();
    context->last_msg_out = mosquitto_time();
    context->keepalive = 60; /* Default to 60s */
    context->clean_session = true;
    context->disconnect_t = 0;
    context->id = NULL;
    context->canonical_id = NULL; // DXL
    context->cert_chain = NULL; // DXL
    context->last_mid = 0;
    context->will = NULL;
    context->listener = NULL;
    /* is_bridge records whether this client is a bridge or not. This could be
     * done by looking at context->bridge for bridges that we create ourself,
     * but incoming bridges need some other way of being recorded. */
    context->is_bridge = false;

    context->in_packet.payload = NULL;
    _mosquitto_packet_cleanup(&context->in_packet);
    context->out_packet = NULL;
    context->current_out_packet = NULL;

    context->address = NULL;
    if(sock != -1){
        if(!_mosquitto_socket_get_address(sock, address, 1024)){
            context->address = _mosquitto_strdup(address);
        }
        if(!context->address){
            /* getpeername and inet_ntop failed and not a bridge */
            _mosquitto_free(context);
            return NULL;
        }
    }
    context->bridge = NULL;
    context->msgs = NULL;
    context->last_msg = NULL;
    context->msg_count = 0;
    context->msg_count12 = 0;
#ifdef PACKET_COUNT
    context->packet_count = 0;
#endif
    context->ssl = NULL;
    // DXL Begin
    context->tls_certtype = unknown;
    context->numericId = 0;
    context->clean_subs = false;
    context->dxl_client_guid = NULL;
    context->dxl_tenant_guid = NULL;
    context->cert_hashes = NULL;
    context->epoll_events = 0; // EPOLL
    context->dxl_flags = 0;    
    // DXL End
    context->wsi = NULL;
    context->ws_sock = INVALID_SOCKET;

    return context;
}

/*
 * This will result in any outgoing packets going unsent. If we're disconnected
 * forcefully then it is usually an error condition and shouldn't be a problem,
 * but it will mean that CONNACK messages will never get sent for bad protocol
 * versions for example.
 */
void mqtt3_context_cleanup(struct mosquitto_db *db, struct mosquitto *context, bool do_free, bool clean_subs /* DXL */)
{    
    struct _mosquitto_packet *packet;
    struct mosquitto_client_msg *msg, *next;
    struct _clientid_index_hash *find_cih;

    if(!context) return;

    if(context->ssl){
        SSL_free(context->ssl);
        context->ssl = NULL;
    }
    if(context->sock != -1){
        if(context->listener){
            context->listener->client_count--;
            if(context->listener->client_count < 0){
                _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "assert: context->listener->client_count >= 0: %d", 
                    context->listener->client_count) ;
            }
        }
        _mosquitto_socket_close(context);
        context->listener = NULL;
    }

    if(context->wsi){
        context->state = mosq_cs_ws_dead;
        mosquitto_ws_request_writeable_callback(context);
    }

    if(context->clean_session && db){
        // DXL Begin
        if(clean_subs){
            context->clean_subs = true;
            mqtt3_subs_clean_session(db, &db->subs);
            context->clean_subs = false;
        }
        // DXL End
        mqtt3_db_messages_delete(context);
    }
    if(context->address){
        _mosquitto_free(context->address);
        context->address = NULL;
    }
    // DXL begin
    if(context->canonical_id){
        mqtt3_db_remove_canonical_client_id(context->canonical_id);
    }
    // DXL end
    if(context->id){
        assert(db); /* db can only be NULL here if the client hasn't sent a
                       CONNECT and hence wouldn't have an id. */

        // Remove the context's ID from the DB hash
        HASH_FIND_STR(db->clientid_index_hash, context->id, find_cih);
        if(find_cih){
            // FIXME - internal level debug? _mosquitto_log_printf(NULL, MOSQ_LOG_INFO,
            // "Found id for client \"%s\", their index was %d.", context->id, find_cih->db_context_index);
            HASH_DEL(db->clientid_index_hash, find_cih);
            dxl_on_client_disconnected(context); // DXL
            _mosquitto_free(find_cih);
        }else{
            // FIXME - internal level debug?
            // _mosquitto_log_printf(NULL, MOSQ_LOG_WARNING, "Unable to find id for client \"%s\".", context->id);
        }
        _mosquitto_free(context->id);
        context->id = NULL;
        _mosquitto_free(context->canonical_id); // DXL
        context->canonical_id = NULL;
    }
    // DXL begin
    if(context->cert_chain){
        _mosquitto_free(context->cert_chain);
        context->cert_chain = NULL;
    }
    if(context->dxl_client_guid){
        _mosquitto_free(context->dxl_client_guid);
        context->dxl_client_guid = NULL;
    }
    if(context->dxl_tenant_guid){
        _mosquitto_free(context->dxl_tenant_guid);
        context->dxl_tenant_guid = NULL;
    }
    mqtt3_context_cleanup_certs(context);
    // DXL end
    _mosquitto_packet_cleanup(&(context->in_packet));
    _mosquitto_packet_cleanup(context->current_out_packet);
    _mosquitto_free(context->current_out_packet); // DXL
    context->current_out_packet = NULL;
    while(context->out_packet){
        _mosquitto_packet_cleanup(context->out_packet);
        packet = context->out_packet;
        context->out_packet = context->out_packet->next;
        _mosquitto_free(packet);
    }
    if(context->will){
        if(context->will->topic) _mosquitto_free(context->will->topic);
        if(context->will->payload) _mosquitto_free(context->will->payload);
        _mosquitto_free(context->will);
        context->will = NULL;
    }
    if(do_free || context->clean_session){
        msg = context->msgs;
        while(msg){
            next = msg->next;
            msg->store->ref_count--;
            _mosquitto_free(msg);
            msg = next;
        }
        context->msgs = NULL;
        context->last_msg = NULL;
        // DXL start
        context->msg_count = 0;
        context->msg_count12 = 0;
        // DXL end
#ifdef PACKET_COUNT
        context->packet_count = 0;
#endif
    }
    if(do_free){
        context->wsi = NULL;
        context->ws_sock = INVALID_SOCKET;
        _mosquitto_free(context);
    }
}

void mqtt3_context_disconnect(struct mosquitto_db *db, struct mosquitto *ctxt)
{
    // DXL Begin
    // Notify that a bridge has been disconnected
    if(ctxt->is_bridge){
        dxl_on_bridge_disconnected(ctxt);
    }
    // DXL End

    if(ctxt->state != mosq_cs_disconnecting && ctxt->will){
        /* Unexpected disconnect, queue the client will. */
        mqtt3_db_messages_easy_queue(db, ctxt, ctxt->will->topic, ctxt->will->qos,
            ctxt->will->payloadlen, ctxt->will->payload, ctxt->will->retain);
    }
    if(ctxt->will){
        if(ctxt->will->topic) _mosquitto_free(ctxt->will->topic);
        if(ctxt->will->payload) _mosquitto_free(ctxt->will->payload);
        _mosquitto_free(ctxt->will);
        ctxt->will = NULL;
    }
    if(ctxt->listener){
        ctxt->listener->client_count--;
        if(ctxt->listener->client_count < 0){
            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "assert: ctxt->listener->client_count >= 0: %d", 
                ctxt->listener->client_count) ;
        }
        ctxt->listener = NULL;
    }
    ctxt->disconnect_t = mosquitto_time();

    // DXL: Remove the context explicitly
    mosquitto_remove_context(ctxt);

    _mosquitto_socket_close(ctxt);
}

// DXL Begin
void mqtt3_context_disconnect_byid(struct mosquitto_db *db, const char* contextId)
{
    struct _clientid_index_hash *find_cih;
    HASH_FIND_STR(db->clientid_index_hash, contextId, find_cih);
    if(find_cih){
        mqtt3_context_disconnect(db, db->contexts[find_cih->db_context_index]);
    }
}

void mqtt3_context_cleanup_certs(struct mosquitto *context)
{
    struct cert_hashes *current, *tmp;
    HASH_ITER(hh, context->cert_hashes, current, tmp){
        HASH_DEL(context->cert_hashes, current);
        _mosquitto_free((void*)current->cert_sha1);
        _mosquitto_free(current);
    }
    context->cert_hashes = NULL;
}
// DXL End
