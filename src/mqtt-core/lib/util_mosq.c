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
#include <string.h>


#include "mosquitto.h"
#include "memory_mosq.h"
#include "net_mosq.h"
#include "send_mosq.h"
#include "time_mosq.h"
#include "tls_mosq.h"
#include "util_mosq.h"

#include "mosquitto_broker.h"

// Size of extra buffer that LWS requires prior to user data buffer.
extern int g_ws_pre_buffer_size;

int _mosquitto_packet_alloc(struct _mosquitto_packet *packet)
{
    uint8_t remaining_bytes[5], byte;
    uint32_t remaining_length;
    int i;

    assert(packet);

    remaining_length = packet->remaining_length;
    packet->payload = NULL;
    packet->remaining_count = 0;
    do{
        byte = remaining_length % 128;
        remaining_length = remaining_length / 128;
        /* If there are more digits to encode, set the top bit of this digit */
        if(remaining_length > 0){
            byte = byte | 0x80;
        }
        remaining_bytes[packet->remaining_count] = byte;
        packet->remaining_count++;
    }while(remaining_length > 0 && packet->remaining_count < 5);
    if(packet->remaining_count == 5) return MOSQ_ERR_PAYLOAD_SIZE;
    packet->packet_length = packet->remaining_length + 1 + packet->remaining_count;
    if(packet->is_ws_packet) {
        // Libwebsockets lws_send() requires a "pre" region prior to the buffer being used for sending.
        // So, preallocate it right now.
        packet->payload = (uint8_t *)_mosquitto_malloc(
                    sizeof(uint8_t)*packet->packet_length + g_ws_pre_buffer_size);
        if(!packet->payload) return MOSQ_ERR_NOMEM;

        // The pre-allocated region is for LWS internal purpose.
        // Set payload to the region where we are allowed to use.
        packet->payload = packet->payload + g_ws_pre_buffer_size;
    }
    else {
        packet->payload = (uint8_t *)_mosquitto_malloc(sizeof(uint8_t)*packet->packet_length);
        if(!packet->payload) return MOSQ_ERR_NOMEM;
    }
    

    packet->payload[0] = packet->command;
    for(i=0; i<packet->remaining_count; i++){
        packet->payload[i+1] = remaining_bytes[i];
    }
    packet->pos = 1 + packet->remaining_count;

    return MOSQ_ERR_SUCCESS;
}

void _mosquitto_check_keepalive(struct mosquitto *mosq)
{
    time_t last_msg_out;
    time_t last_msg_in;
    time_t now = mosquitto_time();

    assert(mosq);
    /* Check if a lazy bridge should be timed out due to idle. */
    if(mosq->bridge && mosq->bridge->start_type == bst_lazy
                && mosq->sock != INVALID_SOCKET
                && now - mosq->last_msg_out >= mosq->bridge->idle_timeout){

        if(IS_NOTICE_ENABLED)
            _mosquitto_log_printf(NULL, MOSQ_LOG_NOTICE,
                "Bridge connection %s has exceeded idle timeout, disconnecting.", mosq->id);
        _mosquitto_socket_close(mosq);
        return;
    }
    last_msg_out = mosq->last_msg_out;
    last_msg_in = mosq->last_msg_in;
    if(mosq->keepalive && (!IS_CONTEXT_INVALID(mosq)) &&
            (now - last_msg_out >= mosq->keepalive || now - last_msg_in >= mosq->keepalive)){

        if(mosq->state == mosq_cs_connected && mosq->ping_t == 0){
            _mosquitto_send_pingreq(mosq);
            /* Reset last msg times to give the server time to send a pingresp */
            mosq->last_msg_in = now;
            mosq->last_msg_out = now;
        }else{
            if(mosq->listener){
                mosq->listener->client_count--;
                if(mosq->listener->client_count < 0)
                {
                    _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "assert: mosq->listener->client_count >= 0: %d", 
                        mosq->listener->client_count) ;
                }
            }
            mosq->listener = NULL;
            _mosquitto_socket_close(mosq);
        }
    }
}

uint16_t _mosquitto_mid_generate(struct mosquitto *mosq)
{
    assert(mosq);

    mosq->last_mid++;
    if(mosq->last_mid == 0) mosq->last_mid++;
    
    return mosq->last_mid;
}

/* Search for + or # in a topic. Return MOSQ_ERR_INVAL if found.
 * Also returns MOSQ_ERR_INVAL if the topic string is too long.
 * Returns MOSQ_ERR_SUCCESS if everything is fine.
 */
int _mosquitto_topic_wildcard_len_check(const char *str)
{
    int len = 0;
    while(str && str[0]){
        if(str[0] == '+' || str[0] == '#'){
            return MOSQ_ERR_INVAL;
        }
        len++;
        str = &str[1];
    }
    if(len > 65535) return MOSQ_ERR_INVAL;

    return MOSQ_ERR_SUCCESS;
}

/* Search for + or # in a topic, check they aren't in invalid positions such as foo/#/bar, foo/+bar or foo/bar#.
 * Return MOSQ_ERR_INVAL if invalid position found.
 * Also returns MOSQ_ERR_INVAL if the topic string is too long.
 * Returns MOSQ_ERR_SUCCESS if everything is fine.
 */
int _mosquitto_topic_wildcard_pos_check(const char *str)
{
    char c = '\0';
    int len = 0;
    while(str && str[0]){
        if(str[0] == '+'){
            if((c != '\0' && c != '/') || (str[1] != '\0' && str[1] != '/')){
                return MOSQ_ERR_INVAL;
            }
        }else if(str[0] == '#'){
            if((c != '\0' && c != '/')  || str[1] != '\0'){
                return MOSQ_ERR_INVAL;
            }
        }
        len++;
        c = str[0];
        str = &str[1];
    }
    if(len > 65535) return MOSQ_ERR_INVAL;

    return MOSQ_ERR_SUCCESS;
}

/* Does a topic match a subscription? */
int mosquitto_topic_matches_sub(const char *sub, const char *topic, bool *result)
{
    int slen, tlen;
    int spos, tpos;
    bool multilevel_wildcard = false;

    if(!sub || !topic || !result) return MOSQ_ERR_INVAL;

    slen = (int)strlen(sub);
    tlen = (int)strlen(topic);

    if(slen && tlen){
        if((sub[0] == '$' && topic[0] != '$')
                || (topic[0] == '$' && sub[0] != '$')){

            *result = false;
            return MOSQ_ERR_SUCCESS;
        }
    }

    spos = 0;
    tpos = 0;

    while(spos < slen && tpos < tlen){
        if(sub[spos] == topic[tpos]){
            if(tpos == tlen-1){
                /* Check for e.g. foo matching foo/# */
                if(spos == slen-3 
                        && sub[spos+1] == '/'
                        && sub[spos+2] == '#'){
                    *result = true;
                    multilevel_wildcard = true;
                    return MOSQ_ERR_SUCCESS;
                }
            }
            spos++;
            tpos++;
            if(spos == slen && tpos == tlen){
                *result = true;
                return MOSQ_ERR_SUCCESS;
            }else if(tpos == tlen && spos == slen-1 && sub[spos] == '+'){
                spos++;
                *result = true;
                return MOSQ_ERR_SUCCESS;
            }
        }else{
            if(sub[spos] == '+'){
                spos++;
                while(tpos < tlen && topic[tpos] != '/'){
                    tpos++;
                }
                if(tpos == tlen && spos == slen){
                    *result = true;
                    return MOSQ_ERR_SUCCESS;
                }
            }else if(sub[spos] == '#'){
                multilevel_wildcard = true;
                if(spos+1 != slen){
                    *result = false;
                    return MOSQ_ERR_SUCCESS;
                }else{
                    *result = true;
                    return MOSQ_ERR_SUCCESS;
                }
            }else{
                *result = false;
                return MOSQ_ERR_SUCCESS;
            }
        }
    }
    if(multilevel_wildcard == false && (tpos < tlen || spos < slen)){
        *result = false;
    }

    return MOSQ_ERR_SUCCESS;
}

FILE *_mosquitto_fopen(const char *path, const char *mode)
{
    return fopen(path, mode);
}

