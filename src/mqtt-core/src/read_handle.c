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
#include <stdio.h>
#include <string.h>

#include <config.h>

#include <mosquitto_broker.h>
#include <mqtt3_protocol.h>
#include <memory_mosq.h>
#include <read_handle.h>
#include <send_mosq.h>
#include <util_mosq.h>

#include "dxl.h"

int mqtt3_packet_handle(struct mosquitto_db *db, struct mosquitto *context)
{
    if(!context) return MOSQ_ERR_INVAL;

    switch((context->in_packet.command)&0xF0){
        case PINGREQ:
            return _mosquitto_handle_pingreq(context);
        case PINGRESP:
            return _mosquitto_handle_pingresp(context);
        case PUBACK:
            return _mosquitto_handle_pubackcomp(context, "PUBACK");
        case PUBCOMP:
            return _mosquitto_handle_pubackcomp(context, "PUBCOMP");
        case PUBLISH:
        {
            int result = mqtt3_handle_publish(db, context);

            return result;
        }            
        case PUBREC:
            return _mosquitto_handle_pubrec(context);
        case PUBREL:
            return _mosquitto_handle_pubrel(db, context);
        case CONNECT:
            return mqtt3_handle_connect(db, context);
        case DISCONNECT:
            return mqtt3_handle_disconnect(db, context);
        case SUBSCRIBE:
            return mqtt3_handle_subscribe(db, context);
        case UNSUBSCRIBE:
            return mqtt3_handle_unsubscribe(db, context);
        case CONNACK:
            return mqtt3_handle_connack(db, context);
        case SUBACK:
            return _mosquitto_handle_suback(context);
        case UNSUBACK:
            return _mosquitto_handle_unsuback(context);
        default:
            /* If we don't recognise the command, return an error straight away. */
            return MOSQ_ERR_PROTOCOL;
    }
}

int mqtt3_handle_publish(struct mosquitto_db *db, struct mosquitto *context)
{
    char *topic;
    void *payload = NULL;
    uint32_t payloadlen;
    uint8_t dup, qos, retain;
    uint16_t mid = 0;
    int rc = 0;
    uint8_t header = context->in_packet.command;
    int res = 0;
    struct mosquitto_msg_store *stored = NULL;
    int len;
    char *topic_temp;
    int i;
    struct _mqtt3_bridge_topic *cur_topic;
    bool match;

#ifndef DXL
    dup = (header & 0x08)>>3;
#endif
    qos = (header & 0x06)>>1;
    if(qos == 3){
        _mosquitto_log_printf(NULL, MOSQ_LOG_INFO,
                "Invalid QoS in PUBLISH from %s, disconnecting.", context->id);
        return 1;
    }

#ifndef DXL
    retain = (header & 0x01);
#endif

#ifdef DXL
    dup = 0;
    retain = 0;
    if(qos > 0){
        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "QoS > 0 is not supported, level specified=%d", qos);
        return  MOSQ_ERR_SUCCESS;
    }
#endif

    if(_mosquitto_read_string(&context->in_packet, &topic)) return 1;
    if(strlen(topic) == 0){
        /* Invalid publish topic, disconnect client. */
        _mosquitto_free(topic);
        return 1;
    }
    if(!strlen(topic)){
        _mosquitto_free(topic);
        return 1;
    }
    if(context->bridge && context->bridge->topics && context->bridge->topic_remapping){
        for(i=0; i<context->bridge->topic_count; i++){
            cur_topic = &context->bridge->topics[i];
            if((cur_topic->direction == bd_both || cur_topic->direction == bd_in) 
                    && (cur_topic->remote_prefix || cur_topic->local_prefix)){

                /* Topic mapping required on this topic if the message matches */

                rc = mosquitto_topic_matches_sub(cur_topic->remote_topic, topic, &match);
                if(rc){
                    _mosquitto_free(topic);
                    return rc;
                }
                if(match){
                    if(cur_topic->remote_prefix){
                        /* This prefix needs removing. */
                        if(!strncmp(cur_topic->remote_prefix, topic, strlen(cur_topic->remote_prefix))){
                            topic_temp = _mosquitto_strdup(topic+strlen(cur_topic->remote_prefix));
                            if(!topic_temp){
                                _mosquitto_free(topic);
                                return MOSQ_ERR_NOMEM;
                            }
                            _mosquitto_free(topic);
                            topic = topic_temp;
                        }
                    }

                    if(cur_topic->local_prefix){
                        /* This prefix needs adding. */
                        len = (int)(strlen(topic) + strlen(cur_topic->local_prefix) + 1);
                        topic_temp = (char *)_mosquitto_calloc(len+1, sizeof(char));
                        if(!topic_temp){
                            _mosquitto_free(topic);
                            return MOSQ_ERR_NOMEM;
                        }
                        snprintf(topic_temp, len, "%s%s", cur_topic->local_prefix, topic);
                        _mosquitto_free(topic);
                        topic = topic_temp;
                    }
                    break;
                }
            }
        }
    }
    if(_mosquitto_topic_wildcard_len_check(topic) != MOSQ_ERR_SUCCESS){
        /* Invalid publish topic, just swallow it. */
        _mosquitto_free(topic);
        return 1;
    }

    if(qos > 0){
        if(_mosquitto_read_uint16(&context->in_packet, &mid)){
            _mosquitto_free(topic);
            return 1;
        }
    }

    payloadlen = context->in_packet.remaining_length - context->in_packet.pos;

    if(payloadlen){
        if(db->config->message_size_limit && payloadlen > (uint32_t)db->config->message_size_limit){
            if(IS_DEBUG_ENABLED)
                _mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG,
                    "Dropped too large PUBLISH from %s (d%d, q%d, r%d, m%d, '%s', ... (%ld bytes))",
                    context->id, dup, qos, retain, mid, topic, (long)payloadlen);
            goto process_bad_message;
        }
        payload = _mosquitto_calloc(payloadlen+1, sizeof(uint8_t));
        if(!payload){
            _mosquitto_free(topic);
            return 1;
        }
        if(_mosquitto_read_bytes(&context->in_packet, payload, payloadlen)){
            _mosquitto_free(topic);
            _mosquitto_free(payload);
            return 1;
        }
    }

    // DXL Begin
    if(!dxl_on_publish_message(context, topic, payloadlen, payload, context->cert_hashes)){
        if(IS_DEBUG_ENABLED)
            _mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "DXL Denied PUBLISH from %s", context->id);
        goto process_bad_message;        
    }
    // DXL End

    if(IS_DEBUG_ENABLED)
        _mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG,
            "Received PUBLISH from %s (d%d, q%d, r%d, m%d, '%s', ... (%ld bytes))",
            context->id, dup, qos, retain, mid, topic, (long)payloadlen);
    if(qos > 0){
        mqtt3_db_message_store_find(context, mid, &stored);
    }
    if(!stored){
        dup = 0;
        if(mqtt3_db_message_store(db, context, context->id, mid, topic, qos, payloadlen, payload, retain, &stored, 0)){
            _mosquitto_free(topic);
            if(payload) _mosquitto_free(payload);
            return 1;
        }
    }else{
        dup = 1;
    }
    switch(qos){
        case 0:
        {
            if(mqtt3_db_messages_queue(db, context->id, topic, qos, retain, stored)) rc = 1;
        }
            break;
        case 1:
            if(mqtt3_db_messages_queue(db, context->id, topic, qos, retain, stored)) rc = 1;
            if(_mosquitto_send_puback(context, mid)) rc = 1;
            break;
        case 2:
            if(!dup){
                res = mqtt3_db_message_insert(db, context, mid, mosq_md_in, qos, (retain != 0), stored);
            }else{
                res = 0;
            }
            /* mqtt3_db_message_insert() returns 2 to indicate dropped message
             * due to queue. This isn't an error so don't disconnect them. */
            if(!res){
                if(_mosquitto_send_pubrec(context, mid)) rc = 1;
            }else if(res == 1){
                rc = 1;
            }
            break;
    }
    _mosquitto_free(topic);
    if(payload) _mosquitto_free(payload);

    return rc;
process_bad_message:
    _mosquitto_free(topic);
    if(payload) _mosquitto_free(payload);
    switch(qos){
        case 0:
            return MOSQ_ERR_SUCCESS;
        case 1:
            return _mosquitto_send_puback(context, mid);
        case 2:
            mqtt3_db_message_store_find(context, mid, &stored);
            if(!stored){
                if(mqtt3_db_message_store(db, context, context->id, mid, NULL, qos, 0, NULL, false, &stored, 0)){
                    return 1;
                }
                res = mqtt3_db_message_insert(db, context, mid, mosq_md_in, qos, false, stored);
            }else{
                res = 0;
            }
            if(!res){
                res = _mosquitto_send_pubrec(context, mid);
            }
            return res;
    }
    return 1;
}

