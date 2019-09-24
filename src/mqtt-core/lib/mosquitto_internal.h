/*
Copyright (c) 2010,2011,2013 Roger Light <roger@atchoo.org>
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

#ifndef _MOSQUITTO_INTERNAL_H_
#define _MOSQUITTO_INTERNAL_H_

#include "config.h"


#include <openssl/ssl.h>
// DXL Begin
#include "../../common/include/cert_hashes.h"
typedef enum CertType {unknown, broker, client} CertType;
// DXL End
#include <stdlib.h>

#    include <stdint.h>

#include "mosquitto.h"
#include "time_mosq.h"
struct mosquitto_client_msg;

enum mosquitto_msg_direction {
    mosq_md_in = 0,
    mosq_md_out = 1
};

enum mosquitto_msg_state {
    mosq_ms_invalid = 0,
    mosq_ms_publish_qos0 = 1,
    mosq_ms_publish_qos1 = 2,
    mosq_ms_wait_for_puback = 3,
    mosq_ms_publish_qos2 = 4,
    mosq_ms_wait_for_pubrec = 5,
    mosq_ms_resend_pubrel = 6,
    mosq_ms_wait_for_pubrel = 7,
    mosq_ms_resend_pubcomp = 8,
    mosq_ms_wait_for_pubcomp = 9,
    mosq_ms_send_pubrec = 10,
    mosq_ms_queued = 11
};

enum mosquitto_client_state {
    mosq_cs_new = 0,
    mosq_cs_connected = 1,
    mosq_cs_disconnecting = 2,
    mosq_cs_connect_async = 3,
    mosq_cs_connect_pending = 4,
    mosq_cs_connect_srv = 5,
    mosq_cs_ws_dead = 6
};

enum _mosquitto_protocol {
    mosq_p_invalid = 0,
    mosq_p_mqtt31 = 1,
    mosq_p_mqtt311 = 2,
    mosq_p_mqtts = 3
};

enum _mosquitto_transport {
    mosq_t_invalid = 0,
    mosq_t_tcp = 1,
    mosq_t_ws = 2,
    mosq_t_sctp = 3
};

struct _mosquitto_packet{
    uint8_t command;
    uint8_t have_remaining;
    uint8_t remaining_count;
    uint8_t is_ws_packet;
    uint16_t mid;
    uint32_t remaining_mult;
    uint32_t remaining_length;
    uint32_t packet_length;
    uint32_t to_process;
    uint32_t pos;
    uint8_t *payload;
    struct _mosquitto_packet *next;
};

struct mosquitto_message_all{
    struct mosquitto_message_all *next;
    time_t timestamp;
    //enum mosquitto_msg_direction direction;
    enum mosquitto_msg_state state;
    bool dup;
    struct mosquitto_message msg;
};

struct mosquitto {
    // DXL Begin
    uint32_t numericId;
    bool clean_subs;
    // DXL End
    int sock;
    int ws_sock;
    enum _mosquitto_protocol protocol;
    char *address;
    char *id;
    char *canonical_id; // DXL
    char *cert_chain; // DXL
    uint16_t keepalive;
    bool clean_session;
    enum mosquitto_client_state state;
    time_t last_msg_in;
    time_t last_msg_out;
    time_t ping_t;
    uint16_t last_mid;
    struct _mosquitto_packet in_packet;
    struct _mosquitto_packet *current_out_packet;
    struct _mosquitto_packet *out_packet;
    struct mosquitto_message *will;
    SSL *ssl;
    SSL_CTX *ssl_ctx;
    char *tls_cafile;
    char *tls_capath;
    char *tls_certfile;
    char *tls_keyfile;
    int (*tls_pw_callback)(char *buf, int size, int rwflag, void *userdata);
    int tls_cert_reqs;
    char *tls_version;
    char *tls_ciphers;
    bool tls_insecure;
    // DXL begin
    CertType tls_certtype;
    char* dxl_client_guid;
    char* dxl_tenant_guid;
    struct cert_hashes* cert_hashes;
    // DXL end
    bool want_write;
    bool is_bridge;
    struct _mqtt3_bridge *bridge;
    struct mosquitto_client_msg *msgs;
    struct mosquitto_client_msg *last_msg;
    int msg_count;
    int msg_count12;
    struct _mqtt3_listener *listener;
    time_t disconnect_t;
    int pollfd_index;
    int db_index;
    struct _mosquitto_packet *out_packet_last;
    bool is_dropping;
#ifdef PACKET_COUNT
    uint64_t packet_count;
#endif
    /* The EPOLL events */
    uint32_t epoll_events;
    // DXL Begin
    uint8_t dxl_flags;
    int subscription_count;
    // DXL End
    void* wsi; // Websocket instance
};

// Helper macro to check if context has valid connection information or not
#define IS_CONTEXT_INVALID(mosq) ((mosq->sock == INVALID_SOCKET) && !mosq->wsi)

#endif
