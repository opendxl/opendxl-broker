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

#include <stdio.h>
#include <string.h>

#include <config.h>

#include <mosquitto_broker.h>
#include <mqtt3_protocol.h>
#include <memory_mosq.h>
#include <send_mosq.h>
#include <time_mosq.h>
#include <tls_mosq.h>
#include <util_mosq.h>

// DXL begin
#include "dxl.h"
#include "DxlFlags.h"
#include <uuid/uuid.h>
// DXL end

// DXL Begin

// Limit the number of connections to the broker (0 = unlimited)
static int s_max_connect_count = 0;

/**
 * Returns the limit on the number of connections to the broker (0 = unlimited)
 *
 * @return  The limit on the number of connections to the broker (0 = unlimited)
 */
int mqtt3_dxl_get_max_connect_count()
{
    return s_max_connect_count;
}

/**
 * Sets the limit on the number of connections to the broker (0 = unlimited)
 *
 * @param   count The limit on the number of connections to the broker (0 = unlimited)
 */
void mqtt3_dxl_set_max_connect_count(int count)
{
    s_max_connect_count = count;
}
// DXL End

int mqtt3_handle_connect(struct mosquitto_db *db, struct mosquitto *context)
{
    char *protocol_name = NULL;
    uint8_t protocol_version;
    uint8_t connect_flags;
    char *client_id = NULL;
    char *canonical_client_id = NULL; // DXL
    char *cert_chain = NULL; // DXL
    char *will_payload = NULL, *will_topic = NULL;
    uint16_t will_payloadlen;
    struct mosquitto_message *will_struct = NULL;
    uint8_t will, will_retain, will_qos, clean_session;
    int i;
    int rc;
    struct mosquitto_client_msg *msg_tail;
    int slen;
    struct _clientid_index_hash *find_cih;
    struct _clientid_index_hash *new_cih;

    /* Don't accept multiple CONNECT commands. */
    if(context->state != mosq_cs_new){
        mqtt3_context_disconnect(db, context);
        return MOSQ_ERR_PROTOCOL;
    }

    if(_mosquitto_read_string(&context->in_packet, &protocol_name)){
        mqtt3_context_disconnect(db, context);
        return 1;
    }
    if(!protocol_name){
        mqtt3_context_disconnect(db, context);
        return 3;
    }
    if(_mosquitto_read_byte(&context->in_packet, &protocol_version)){
        _mosquitto_free(protocol_name);
        mqtt3_context_disconnect(db, context);
        return 1;
    }
    if(!strcmp(protocol_name, PROTOCOL_NAME_v31)){
        if((protocol_version&0x7F) != PROTOCOL_VERSION_v31){
            if(db->config->connection_messages == true){
                if(IS_INFO_ENABLED)
                    _mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "Invalid protocol version %d in CONNECT from %s.",
                        protocol_version, context->address);
            }
            _mosquitto_send_connack(context, CONNACK_REFUSED_PROTOCOL_VERSION);
            mqtt3_context_disconnect(db, context);
            _mosquitto_free(protocol_name);
            return MOSQ_ERR_PROTOCOL;
        }
        context->protocol = mosq_p_mqtt31;
    }else if(!strcmp(protocol_name, PROTOCOL_NAME_v311)){
        if((protocol_version&0x7F) != PROTOCOL_VERSION_v311){
            if(db->config->connection_messages == true){
                if(IS_INFO_ENABLED)
                    _mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "Invalid protocol version %d in CONNECT from %s.",
                        protocol_version, context->address);
            }
            _mosquitto_send_connack(context, CONNACK_REFUSED_PROTOCOL_VERSION);
            mqtt3_context_disconnect(db, context);
            _mosquitto_free(protocol_name);
            return MOSQ_ERR_PROTOCOL;
        }
        if((context->in_packet.command&0x0F) != 0x00){
            /* Reserved flags not set to 0, must disconnect. */ 
            mqtt3_context_disconnect(db, context);
            _mosquitto_free(protocol_name);
            return MOSQ_ERR_PROTOCOL;
        }
        context->protocol = mosq_p_mqtt311;
    }else{
        if(db->config->connection_messages == true){
            if(IS_INFO_ENABLED)
                _mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "Invalid protocol \"%s\" in CONNECT from %s.",
                    protocol_name, context->address);
        }
        _mosquitto_free(protocol_name);
        mqtt3_context_disconnect(db, context);
        return MOSQ_ERR_PROTOCOL;
    }
    _mosquitto_free(protocol_name);

    context->tls_certtype = client;
    if((protocol_version&0x80) == 0x80){
        // Determine if the broker attempting to bridge is a valid
        // broker certificate
        struct cert_hashes *current, *tmp;
        HASH_ITER(hh, context->cert_hashes, current, tmp){
            if(mqtt3_config_is_broker_cert(current->cert_sha1)){
                context->tls_certtype = broker;
                break;
            }
        }

        if(context->tls_certtype != broker){
            _mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "Client \"%s\" not authorized to bridge to broker",
                context->address);
            _mosquitto_send_connack(context, CONNACK_REFUSED_NOT_AUTHORIZED);
            mqtt3_context_disconnect(db, context);
            return MOSQ_ERR_PROTOCOL;
        }

        context->is_bridge = true;
    }

    // DXL Begin
    if(!context->is_bridge){
        int maxCount = mqtt3_dxl_get_max_connect_count();
        int clientCount = mqtt3_dxl_db_client_count(db);
        if(maxCount > 0 && clientCount >= maxCount){
            if(db->config->connection_messages == true){
                if(IS_INFO_ENABLED)
                    _mosquitto_log_printf(NULL, MOSQ_LOG_INFO,
                        "CONNECT rejected, maximum number of clients connected: %d.", clientCount);
            }
            _mosquitto_send_connack(context, CONNACK_REFUSED_SERVER_UNAVAILABLE);
            mqtt3_context_disconnect(db, context);
            return MOSQ_ERR_PROTOCOL;
        }
    }
    // DXL End

    if(_mosquitto_read_byte(&context->in_packet, &connect_flags)){
        mqtt3_context_disconnect(db, context);
        return 1;
    }
    clean_session = connect_flags & 0x02;
    will = connect_flags & 0x04;
    will_qos = (connect_flags & 0x18) >> 3;
    if(will_qos == 3){
        if(IS_INFO_ENABLED)
            _mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "Invalid Will QoS in CONNECT from %s.",
                context->address);
        mqtt3_context_disconnect(db, context);
        return MOSQ_ERR_PROTOCOL;
    }
    will_retain = connect_flags & 0x20;

    // DXL Begin
    if(!clean_session){
        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Persistent sessions are not supported, disconnecting client.");
        mqtt3_context_disconnect(db, context);
        return 1;
    }
    // DXL End

    if(_mosquitto_read_uint16(&context->in_packet, &(context->keepalive))){
        mqtt3_context_disconnect(db, context);
        return 1;
    }

    if(_mosquitto_read_string(&context->in_packet, &client_id)){
        mqtt3_context_disconnect(db, context);
        return 1;
    }

    // DXL Begin
    if(IS_DEBUG_ENABLED){
        _mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "Context connect, id: %s",
            client_id);
    }
    // DXL End

    slen = (int)strlen(client_id);
#ifdef WITH_STRICT_PROTOCOL
    if(slen > 23 || slen == 0){
#else
    if(slen == 0){
#endif
        if(context->protocol == mosq_p_mqtt31){
            _mosquitto_free(client_id);
            _mosquitto_send_connack(context, CONNACK_REFUSED_IDENTIFIER_REJECTED);
            mqtt3_context_disconnect(db, context);
            return MOSQ_ERR_PROTOCOL;
        }else{ /* mqtt311 */
            _mosquitto_free(client_id);

            if(clean_session == 0){
                _mosquitto_send_connack(context, CONNACK_REFUSED_IDENTIFIER_REJECTED);
                mqtt3_context_disconnect(db, context);
                return MOSQ_ERR_PROTOCOL;
            }
            _mosquitto_send_connack(context, CONNACK_REFUSED_IDENTIFIER_REJECTED);
            mqtt3_context_disconnect(db, context);
            return MOSQ_ERR_PROTOCOL;
        }
    }

    // DXL begin
    bool is_multi_tenant = dxl_is_multi_tenant_mode_enabled();

    bool dxl_tenant_guid_check_failed =
        (is_multi_tenant && !context->dxl_tenant_guid);
    if(dxl_tenant_guid_check_failed){
        _mosquitto_free(client_id);
        _mosquitto_send_connack(context, CONNACK_REFUSED_NOT_AUTHORIZED);
        mqtt3_context_disconnect(db, context);
        return MOSQ_ERR_SUCCESS;
    }
    // DXL end

    // DXL begin
    if(true){ // Check currently disabled
        if(!context->is_bridge){
            // Unmanaged connection
            struct cert_hashes *current, *tmp;
            const char* cert = NULL;
            unsigned int num_certs = HASH_COUNT(context->cert_hashes);

            size_t cert_chain_len = 41 * num_certs + 1; // 40: size of thumbprint + 1 for semicolon
            cert_chain = (char*)_mosquitto_malloc(cert_chain_len);
            if(!cert_chain)
                return MOSQ_ERR_NOMEM;
            cert_chain[0] = '\0';
            int cert_index = 0;
            HASH_ITER(hh, context->cert_hashes, current, tmp){
                cert = current->cert_sha1;
                if(cert_index > 0){
                    strncat(cert_chain, ";", (cert_chain_len - strlen(cert_chain) - 1));
                }
                strncat(cert_chain, cert, (cert_chain_len - strlen(cert_chain) - 1));
                cert_index++;
            }
            if(cert != NULL){
                canonical_client_id = _mosquitto_strdup(cert);
                if(!canonical_client_id)
                    return MOSQ_ERR_NOMEM;
                size_t canonical_len = strlen(canonical_client_id);
                size_t id_len = strlen(client_id);
                char* id = (char*)_mosquitto_malloc(canonical_len + id_len + 2);
                if(!id)
                    return MOSQ_ERR_NOMEM;
                strcpy(id, canonical_client_id);
                id[canonical_len] = ':';
                strcpy(id + canonical_len + 1, client_id);
                _mosquitto_free(client_id);
                client_id = id;
            }
        }
    }

    if(is_multi_tenant && strcmp(context->dxl_tenant_guid, dxl_get_broker_tenant_guid()) == 0){
        // Turning on Ops Flag
        context->dxl_flags |= DXL_FLAG_OPS;
    }

    if(IS_DEBUG_ENABLED)
        _mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "Final client identifier: %s", client_id);
    // DXL end

    if(will){
        will_struct = (struct mosquitto_message *)_mosquitto_calloc(1, sizeof(struct mosquitto_message));
        if(!will_struct){
            mqtt3_context_disconnect(db, context);
            rc = MOSQ_ERR_NOMEM;
            goto handle_connect_error;
        }
        if(_mosquitto_read_string(&context->in_packet, &will_topic)){
            mqtt3_context_disconnect(db, context);
            rc = 1;
            goto handle_connect_error;
        }
        if(strlen(will_topic) == 0){
            mqtt3_context_disconnect(db, context);
            rc = 1;
            goto handle_connect_error;
        }
        if(_mosquitto_topic_wildcard_pos_check(will_topic)){
            mqtt3_context_disconnect(db, context);
            rc = 1;
            goto handle_connect_error;
        }

        if(_mosquitto_read_uint16(&context->in_packet, &will_payloadlen)){
            mqtt3_context_disconnect(db, context);
            rc = 1;
            goto handle_connect_error;
        }
        if(will_payloadlen > 0){
            will_payload = (char *)_mosquitto_malloc(will_payloadlen);
            if(!will_payload){
                mqtt3_context_disconnect(db, context);
                rc = 1;
                goto handle_connect_error;
            }

            rc = _mosquitto_read_bytes(&context->in_packet, will_payload, will_payloadlen);
            if(rc){
                mqtt3_context_disconnect(db, context);
                rc = 1;
                goto handle_connect_error;
            }
        }
    }else{
        if(context->protocol == mosq_p_mqtt311){
            if(will_qos != 0 || will_retain != 0){
                mqtt3_context_disconnect(db, context);
                rc = MOSQ_ERR_PROTOCOL;
                goto handle_connect_error;
            }
        }
    }

    /* Find if this client already has an entry. This must be done *after* any security checks. */
    HASH_FIND_STR(db->clientid_index_hash, client_id, find_cih);
    if(find_cih){
        i = find_cih->db_context_index;
        /* Found a matching client */
        if(db->contexts[i]->sock == -1){
            /* Client is reconnecting after a disconnect */
            /* FIXME - does anything else need to be done here? */
        }else{
            /* Client is already connected, disconnect old version */
            if(db->config->connection_messages == true){
                _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Client %s already connected, closing old connection.", client_id);
            }
        }
        db->contexts[i]->clean_session = (clean_session != 0);
        mqtt3_context_cleanup(db, db->contexts[i], false, true /* DXL */);
        db->contexts[i]->state = mosq_cs_connected;
        db->contexts[i]->address = _mosquitto_strdup(context->address);
        db->contexts[i]->sock = context->sock;
        db->contexts[i]->listener = context->listener;
        db->contexts[i]->last_msg_in = mosquitto_time();
        db->contexts[i]->last_msg_out = mosquitto_time();
        db->contexts[i]->keepalive = context->keepalive;
        db->contexts[i]->pollfd_index = context->pollfd_index;
        db->contexts[i]->epoll_events = context->epoll_events; // EPOLL
        // DXL Begin
        db->contexts[i]->dxl_flags = context->dxl_flags;
        // DXL End
        db->contexts[i]->ssl = context->ssl;
        // DXL begin
        if(context->dxl_client_guid){
            db->contexts[i]->dxl_client_guid = _mosquitto_strdup(context->dxl_client_guid);
        }
        if(context->dxl_tenant_guid){
            db->contexts[i]->dxl_tenant_guid = _mosquitto_strdup(context->dxl_tenant_guid);
        }

        // Copy certs
        struct cert_hashes *current, *tmp;
        HASH_ITER(hh, context->cert_hashes, current, tmp){
            struct cert_hashes* s = (struct cert_hashes*)_mosquitto_malloc(sizeof(struct cert_hashes));
            s->cert_sha1 = strdup(current->cert_sha1);
            HASH_ADD_KEYPTR(hh, db->contexts[i]->cert_hashes, s->cert_sha1, (unsigned int)strlen(s->cert_sha1), s);
        }
        // DXL end
        context->listener = NULL; // DXL
        context->sock = -1;
        context->ssl = NULL;
        context->state = mosq_cs_disconnecting;
        context = db->contexts[i];
        if(context->msgs){
            mqtt3_db_message_reconnect_reset(context);
        }
        mosquitto_epoll_update_context_index(db->contexts[i], i); // EPOLL
        context->numericId = i; // DXL
        context->clean_subs = false; // DXL
    }

    context->id = client_id;
    client_id = NULL;
    context->canonical_id = // DXL
        canonical_client_id ? canonical_client_id : strdup(context->id);
    canonical_client_id = NULL; // DXL
    context->clean_session = (clean_session != 0);
    context->ping_t = 0;
    context->is_dropping = false;
    context->cert_chain = cert_chain; // DXL
    cert_chain = NULL; // DXL

    msg_tail = context->msgs;
    while(msg_tail){
        msg_tail = msg_tail->next;
    }

    // Add the client ID to the DB hash table here
    new_cih = (struct _clientid_index_hash *)_mosquitto_malloc(sizeof(struct _clientid_index_hash));
    if(!new_cih){
        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory.");
        mqtt3_context_disconnect(db, context);
        rc = MOSQ_ERR_NOMEM;
        goto handle_connect_error;
    }
    new_cih->id = context->id;
    new_cih->db_context_index = context->db_index;
    HASH_ADD_KEYPTR(hh, db->clientid_index_hash, context->id, (int)strlen(context->id), new_cih);

    // DXL begin
    mqtt3_add_canonical_client_id(context->canonical_id);
    dxl_on_client_connected(context);
    // DXL end

    if(will_struct){
        context->will = will_struct;
        context->will->topic = will_topic;
        if(will_payload){
            context->will->payload = will_payload;
            context->will->payloadlen = will_payloadlen;
        }else{
            context->will->payload = NULL;
            context->will->payloadlen = 0;
        }
        context->will->qos = will_qos;
        context->will->retain = (will_retain != 0);
    }

    // DXL Begin
    if(!dxl_is_bridge_connect_allowed(context)){
        _mosquitto_send_connack(context, CONNACK_REFUSED_IDENTIFIER_REJECTED);
        mqtt3_context_disconnect(db, context);
        rc = MOSQ_ERR_SUCCESS;
        goto handle_connect_error;
    }
    // DXL End

    if(db->config->connection_messages == true){
        if(context->is_bridge){
            if(IS_NOTICE_ENABLED)
                _mosquitto_log_printf(NULL, MOSQ_LOG_NOTICE, "New bridge connected from %s as %s (c%d, k%d).",
                context->address, context->id, context->clean_session, context->keepalive);
        }else{
            if(IS_NOTICE_ENABLED)
                _mosquitto_log_printf(NULL, MOSQ_LOG_NOTICE, "New client connected from %s as %s (c%d, k%d).",
                context->address, context->id, context->clean_session, context->keepalive);
        }
    }

    context->state = mosq_cs_connected;

    // DXL Begin
    // Notify that a bridge has connected
    if(context->is_bridge){
        dxl_on_bridge_connected(context);
    }
    // DXL End

    return _mosquitto_send_connack(context, CONNACK_ACCEPTED);

handle_connect_error:
    if(client_id) _mosquitto_free(client_id);
    if(will_payload) _mosquitto_free(will_payload);
    if(will_topic) _mosquitto_free(will_topic);
    if(will_struct) _mosquitto_free(will_struct);
    if(canonical_client_id) _mosquitto_free(canonical_client_id); // DXL
    if(cert_chain) _mosquitto_free(cert_chain); // DXL
    return rc;
}

int mqtt3_handle_disconnect(struct mosquitto_db *db, struct mosquitto *context)
{
    if(!context){
        return MOSQ_ERR_INVAL;
    }
    if(context->in_packet.remaining_length != 0){
        return MOSQ_ERR_PROTOCOL;
    }
    if(IS_DEBUG_ENABLED)
        _mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "Received DISCONNECT from %s", context->id);
    if(context->protocol == mosq_p_mqtt311){
        if((context->in_packet.command&0x0F) != 0x00){
            mqtt3_context_disconnect(db, context);
            return MOSQ_ERR_PROTOCOL;
        }
    }
    context->state = mosq_cs_disconnecting;
    mqtt3_context_disconnect(db, context);
    return MOSQ_ERR_SUCCESS;
}


int mqtt3_handle_subscribe(struct mosquitto_db *db, struct mosquitto *context)
{
    int rc = 0;
    int rc2;
    uint16_t mid;
    char *sub;
    uint8_t qos;
    uint8_t *payload = NULL, *tmp_payload;
    uint32_t payloadlen = 0;

    if(!context) return MOSQ_ERR_INVAL;
    if(IS_DEBUG_ENABLED)
        _mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "Received SUBSCRIBE from %s", context->id);
    /* FIXME - plenty of potential for memory leaks here */

    if(context->protocol == mosq_p_mqtt311){
        if((context->in_packet.command&0x0F) != 0x02){
            return MOSQ_ERR_PROTOCOL;
        }
    }
    if(_mosquitto_read_uint16(&context->in_packet, &mid)) return 1;

    while(context->in_packet.pos < context->in_packet.remaining_length){
        sub = NULL;
        if(_mosquitto_read_string(&context->in_packet, &sub)){
            if(payload) _mosquitto_free(payload);
            return 1;
        }

        if(sub){
            if(!strlen(sub)){
                if(IS_INFO_ENABLED)
                    _mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "Empty subscription string from %s, disconnecting.",
                        context->address);
                _mosquitto_free(sub);
                if(payload) _mosquitto_free(payload);
                return 1;
            }
            if(_mosquitto_topic_wildcard_pos_check(sub)){
                if(IS_INFO_ENABLED)
                    _mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "Invalid subscription string from %s, disconnecting.",
                        context->address);
                _mosquitto_free(sub);
                if(payload) _mosquitto_free(payload);
                return 1;
            }

            if(_mosquitto_read_byte(&context->in_packet, &qos)){
                _mosquitto_free(sub);
                if(payload) _mosquitto_free(payload);
                return 1;
            }
            if(qos > 2){
                if(IS_INFO_ENABLED)
                    _mosquitto_log_printf(NULL, MOSQ_LOG_INFO,
                        "Invalid QoS in subscription command from %s, disconnecting.",
                        context->address);
                _mosquitto_free(sub);
                if(payload) _mosquitto_free(payload);
                return 1;
            }
            if(IS_DEBUG_ENABLED)
                _mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "\t%s (QoS %d)", sub, qos);

            if(qos != 0x80){
                rc2 = mqtt3_sub_add(db, context, sub, qos, &db->subs);
                if(rc2 == MOSQ_ERR_SUCCESS){
                    if(mqtt3_retain_queue(db, context, sub, qos)) rc = 1;
                }else if(rc2 != -1){
                    rc = rc2;
                }
                if(IS_SUBSCRIBE_ENABLED)
                    _mosquitto_log_printf(NULL, MOSQ_LOG_SUBSCRIBE, "%s %d %s", context->id, qos, sub);
            }
            _mosquitto_free(sub);

            tmp_payload = (uint8_t *)_mosquitto_realloc(payload, payloadlen + 1);
            if(tmp_payload){
                payload = tmp_payload;
                payload[payloadlen] = qos;
                payloadlen++;
            }else{
                if(payload) _mosquitto_free(payload);

                return MOSQ_ERR_NOMEM;
            }
        }
    }

    if(context->protocol == mosq_p_mqtt311){
        if(payloadlen == 0){
            /* No subscriptions specified, protocol error. */
            return MOSQ_ERR_PROTOCOL;
        }
    }
    if(_mosquitto_send_suback(context, mid, payloadlen, payload)) rc = 1;
    _mosquitto_free(payload);
    
    return rc;
}

int mqtt3_handle_unsubscribe(struct mosquitto_db *db, struct mosquitto *context)
{
    uint16_t mid;
    char *sub;

    if(!context) return MOSQ_ERR_INVAL;
    if(IS_DEBUG_ENABLED)
        _mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "Received UNSUBSCRIBE from %s", context->id);

    if(context->protocol == mosq_p_mqtt311){
        if((context->in_packet.command&0x0F) != 0x02){
            return MOSQ_ERR_PROTOCOL;
        }
    }
    if(_mosquitto_read_uint16(&context->in_packet, &mid)) return 1;

    while(context->in_packet.pos < context->in_packet.remaining_length){
        sub = NULL;
        if(_mosquitto_read_string(&context->in_packet, &sub)){
            return 1;
        }

        if(sub){
            if(!strlen(sub)){
                if(IS_INFO_ENABLED)
                    _mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "Empty unsubscription string from %s, disconnecting.",
                        context->id);
                _mosquitto_free(sub);
                return 1;
            }
            if(_mosquitto_topic_wildcard_pos_check(sub)){
                if(IS_INFO_ENABLED)
                    _mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "Invalid unsubscription string from %s, disconnecting.",
                        context->id);
                _mosquitto_free(sub);
                return 1;
            }

            if(IS_DEBUG_ENABLED)
                _mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "\t%s", sub);
            mqtt3_sub_remove(db, context, sub, &db->subs);
            if(IS_UNSUBSCRIBE_ENABLED)
                _mosquitto_log_printf(NULL, MOSQ_LOG_UNSUBSCRIBE, "%s %s", context->id, sub);
            _mosquitto_free(sub);
        }
    }

    return _mosquitto_send_command_with_mid(context, UNSUBACK, mid, false);
}
