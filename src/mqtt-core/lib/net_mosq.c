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
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#ifdef __ANDROID__
#include <linux/in.h>
#include <linux/in6.h>
#include <sys/endian.h>
#endif

#ifdef __FreeBSD__
#  include <netinet/in.h>
#endif

#ifdef __SYMBIAN32__
#include <netinet/in.h>
#endif

#ifdef __QNX__
#ifndef AI_ADDRCONFIG
#define AI_ADDRCONFIG 0
#endif
#include <net/netbyte.h>
#include <netinet/in.h>
#endif

#include <openssl/err.h>
#include <tls_mosq.h>

#include <mosquitto_broker.h>
// DXL Begin
#include "dxl.h"
// DXL End

#include "logging_mosq.h"
#include "memory_mosq.h"
#include "mqtt3_protocol.h"
#include "net_mosq.h"
#include "time_mosq.h"
#include "util_mosq.h"

int tls_ex_index_mosq = -1;

// Size of extra buffer that LWS requires prior to user data buffer.
extern int g_ws_pre_buffer_size;

void _mosquitto_net_init(void)
{

    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    if(tls_ex_index_mosq == -1){
        tls_ex_index_mosq = SSL_get_ex_new_index(0, (void *)"client context", NULL, NULL, NULL);
    }
}

void _mosquitto_net_cleanup(void)
{
    ERR_free_strings();
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();

}

/* DXL begin */
int _mosquitto_fips_enable(int mode)
{
    if( FIPS_mode() != mode )
    {
        _mosquitto_log_printf( NULL, MOSQ_LOG_INFO, "Setting FIPS mode to %d.", mode );
        return( FIPS_mode_set( mode ) );
    }
    else
    {
        _mosquitto_log_printf( NULL, MOSQ_LOG_INFO, "FIPS mode already in correct state." );
        return 1;
    }
}

const char *_mosquitto_ssl_version()
{
    return SSLeay_version(SSLEAY_VERSION);
}
/* DXL end */

void _mosquitto_packet_cleanup(struct _mosquitto_packet *packet)
{
    if(!packet) return;

    /* Free data and reset values */
    packet->command = 0;
    packet->have_remaining = 0;
    packet->remaining_count = 0;
    packet->remaining_mult = 1;
    packet->remaining_length = 0;    
    if(packet->payload) {
        if(packet->is_ws_packet){
            // Reset buffer to the original allocated value.
            packet->payload = packet->payload - g_ws_pre_buffer_size;
        }
        _mosquitto_free(packet->payload);
        packet->payload = NULL;
    }
    packet->to_process = 0;
    packet->pos = 0;
}

int _mosquitto_packet_queue(struct mosquitto *mosq, struct _mosquitto_packet *packet)
{
    assert(mosq);
    assert(packet);

    packet->pos = 0;
    packet->to_process = packet->packet_length;

    packet->next = NULL;
    if(mosq->out_packet){
        mosq->out_packet_last->next = packet;
    }else{
        mosq->out_packet = packet;
    }
    mosq->out_packet_last = packet;

#ifdef PACKET_COUNT
    mosq->packet_count++;
#endif

    if(mosq->wsi){
        // For websockets, we can only write from LWS callbacks.
        // So, request for one.
        mosquitto_ws_request_writeable_callback(mosq);
        //lws_service_fd(lws_ctx, &p);
        return 0;
    }

    return _mosquitto_packet_write(mosq);
}

/* Close a socket associated with a context and set it to -1.
 * Returns 1 on failure (context is NULL)
 * Returns 0 on success.
 */
int _mosquitto_socket_close(struct mosquitto *mosq)
{
    int rc = 0;

    assert(mosq);
    if(mosq->wsi){        
        // LWS does not provide a way to close the client socket from the server side.
        // So, set the state as dead in the context and request for callback.
        // In the callback, return an error to close the connection.
        mosq->state = mosq_cs_ws_dead;
        mosquitto_ws_request_writeable_callback(mosq);
        return rc;
    }
    if(mosq->ssl){
        // DXL Start
        if(!SSL_in_init(mosq->ssl)){
            SSL_shutdown(mosq->ssl);
        }
        // DXL Stop
        SSL_free(mosq->ssl);
        mosq->ssl = NULL;
    }
    if(mosq->ssl_ctx){
        SSL_CTX_free(mosq->ssl_ctx);
        mosq->ssl_ctx = NULL;
    }

    if(mosq->sock != INVALID_SOCKET){
        rc = COMPAT_CLOSE(mosq->sock);
        mosq->sock = INVALID_SOCKET;
    }

    return rc;
}

int _mosquitto_try_connect(const char *host, uint16_t port, int *sock, const char *bind_address, bool blocking)
{
    struct addrinfo hints;
    struct addrinfo *ainfo, *rp;
    struct addrinfo *ainfo_bind, *rp_bind;
    int s;
    int rc;

    *sock = INVALID_SOCKET;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = PF_UNSPEC;
    hints.ai_flags = AI_ADDRCONFIG;
    hints.ai_socktype = SOCK_STREAM;

    s = getaddrinfo(host, NULL, &hints, &ainfo);
    if(s){
        errno = s;
        return MOSQ_ERR_EAI;
    }

    if(bind_address){
        s = getaddrinfo(bind_address, NULL, &hints, &ainfo_bind);
        if(s){
            freeaddrinfo(ainfo);
            errno = s;
            return MOSQ_ERR_EAI;
        }
    }

    for(rp = ainfo; rp != NULL; rp = rp->ai_next){
        *sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(*sock == INVALID_SOCKET) continue;
        
        if(rp->ai_family == PF_INET){
            ((struct sockaddr_in *)rp->ai_addr)->sin_port = htons(port);
        }else if(rp->ai_family == PF_INET6){
            ((struct sockaddr_in6 *)rp->ai_addr)->sin6_port = htons(port);
        }else{
            continue;
        }

        if(bind_address){
            for(rp_bind = ainfo_bind; rp_bind != NULL; rp_bind = rp_bind->ai_next){
                if(bind(*sock, rp_bind->ai_addr, (int)rp_bind->ai_addrlen) == 0){
                    break;
                }
            }
            if(!rp_bind){
                COMPAT_CLOSE(*sock);
                continue;
            }
        }

        if(!blocking){
            /* Set non-blocking */
            if(_mosquitto_socket_nonblock(*sock)){
                COMPAT_CLOSE(*sock);
                continue;
            }
        }

        rc = connect(*sock, rp->ai_addr, (int)rp->ai_addrlen);
        if(rc == 0 || errno == EINPROGRESS || errno == COMPAT_EWOULDBLOCK){
            if(blocking){
                /* Set non-blocking */
                if(_mosquitto_socket_nonblock(*sock)){
                    COMPAT_CLOSE(*sock);
                    continue;
                }
            }
            break;
        }

        COMPAT_CLOSE(*sock);
        *sock = INVALID_SOCKET;
    }
    freeaddrinfo(ainfo);
    if(bind_address){
        freeaddrinfo(ainfo_bind);
    }
    if(!rp){
        return MOSQ_ERR_ERRNO;
    }
    return MOSQ_ERR_SUCCESS;
}

/* Create a socket and connect it to 'ip' on port 'port'.
 * Returns -1 on failure (ip is NULL, socket creation/connection error)
 * Returns sock number on success.
 */
int _mosquitto_socket_connect(struct mosquitto *mosq, const char *host, uint16_t port,
    const char *bind_address, bool blocking)
{
    int sock = INVALID_SOCKET;
    int rc;
    int ret;
    BIO *bio;

    if(!mosq || !host || !port) return MOSQ_ERR_INVAL;

    if(mosq->tls_cafile || mosq->tls_capath){
        blocking = true;
    }

    rc = _mosquitto_try_connect(host, port, &sock, bind_address, blocking);
    if(rc != MOSQ_ERR_SUCCESS) {
        if(sock != INVALID_SOCKET){
            COMPAT_CLOSE(sock);
        }
        return rc;
    }

    if(mosq->tls_cafile || mosq->tls_capath){
#if OPENSSL_VERSION_NUMBER >= 0x10001000L
        if(!mosq->tls_version || !strcmp(mosq->tls_version, "tlsv1.2")){
            mosq->ssl_ctx = SSL_CTX_new(TLSv1_2_client_method());
        }else if(!strcmp(mosq->tls_version, "tlsv1.1")){
            mosq->ssl_ctx = SSL_CTX_new(TLSv1_1_client_method());
        }else if(!strcmp(mosq->tls_version, "tlsv1")){
            mosq->ssl_ctx = SSL_CTX_new(TLSv1_client_method());
        }else{
            _mosquitto_log_printf(mosq, MOSQ_LOG_ERR, "Error: Protocol %s not supported.", mosq->tls_version);
            COMPAT_CLOSE(sock);
            return MOSQ_ERR_INVAL;
        }
#else
        if(!mosq->tls_version || !strcmp(mosq->tls_version, "tlsv1")){
            mosq->ssl_ctx = SSL_CTX_new(TLSv1_client_method());
        }else{
            _mosquitto_log_printf(mosq, MOSQ_LOG_ERR, "Error: Protocol %s not supported.", mosq->tls_version);
            COMPAT_CLOSE(sock);
            return MOSQ_ERR_INVAL;
        }
#endif
        if(!mosq->ssl_ctx){
            _mosquitto_log_printf(mosq, MOSQ_LOG_ERR, "Error: Unable to create TLS context.");
            COMPAT_CLOSE(sock);
            return MOSQ_ERR_TLS;
        }

#if OPENSSL_VERSION_NUMBER >= 0x10000000
        /* Disable compression */
        SSL_CTX_set_options(mosq->ssl_ctx, SSL_OP_NO_COMPRESSION);
#endif
#ifdef SSL_MODE_RELEASE_BUFFERS
            /* Use even less memory per SSL connection. */
            SSL_CTX_set_mode(mosq->ssl_ctx, SSL_MODE_RELEASE_BUFFERS);
#endif

        if(mosq->tls_ciphers){
            ret = SSL_CTX_set_cipher_list(mosq->ssl_ctx, mosq->tls_ciphers);
            if(ret == 0){
                _mosquitto_log_printf(mosq, MOSQ_LOG_ERR,
                    "Error: Unable to set TLS ciphers. Check cipher list \"%s\".", mosq->tls_ciphers);
                //COMPAT_CLOSE(sock);
                _mosquitto_socket_close(mosq); // DXL
                return MOSQ_ERR_TLS;
            }
        }
        if(mosq->tls_cafile || mosq->tls_capath){
            ret = SSL_CTX_load_verify_locations(mosq->ssl_ctx, mosq->tls_cafile, mosq->tls_capath);
            if(ret == 0){
                if(mosq->tls_cafile && mosq->tls_capath){
                    _mosquitto_log_printf(mosq, MOSQ_LOG_ERR,
                        "Error: Unable to load CA certificates, check bridge_cafile \"%s\" and bridge_capath \"%s\".",
                        mosq->tls_cafile, mosq->tls_capath);
                }else if(mosq->tls_cafile){
                    _mosquitto_log_printf(mosq, MOSQ_LOG_ERR,
                        "Error: Unable to load CA certificates, check bridge_cafile \"%s\".", mosq->tls_cafile);
                }else{
                    _mosquitto_log_printf(mosq, MOSQ_LOG_ERR,
                        "Error: Unable to load CA certificates, check bridge_capath \"%s\".", mosq->tls_capath);
                }
                //COMPAT_CLOSE(sock);
                _mosquitto_socket_close(mosq); // DXL
                return MOSQ_ERR_TLS;
            }
            if(mosq->tls_cert_reqs == 0){
                SSL_CTX_set_verify(mosq->ssl_ctx, SSL_VERIFY_NONE, NULL);
            }else{
                SSL_CTX_set_verify(mosq->ssl_ctx, SSL_VERIFY_PEER, _mosquitto_server_certificate_verify);
            }

            if(mosq->tls_pw_callback){
                SSL_CTX_set_default_passwd_cb(mosq->ssl_ctx, mosq->tls_pw_callback);
                SSL_CTX_set_default_passwd_cb_userdata(mosq->ssl_ctx, mosq);
            }

            if(mosq->tls_certfile){
                ret = SSL_CTX_use_certificate_chain_file(mosq->ssl_ctx, mosq->tls_certfile);
                if(ret != 1){
                    _mosquitto_log_printf(mosq, MOSQ_LOG_ERR,
                        "Error: Unable to load client certificate, check bridge_certfile \"%s\".", mosq->tls_certfile);
                    //COMPAT_CLOSE(sock);
                    _mosquitto_socket_close(mosq); // DXL
                    return MOSQ_ERR_TLS;
                }
            }
            if(mosq->tls_keyfile){
                ret = SSL_CTX_use_PrivateKey_file(mosq->ssl_ctx, mosq->tls_keyfile, SSL_FILETYPE_PEM);
                if(ret != 1){
                    _mosquitto_log_printf(mosq, MOSQ_LOG_ERR,
                        "Error: Unable to load client key file, check bridge_keyfile \"%s\".", mosq->tls_keyfile);
                    //COMPAT_CLOSE(sock);
                    _mosquitto_socket_close(mosq); // DXL
                    return MOSQ_ERR_TLS;
                }
                ret = SSL_CTX_check_private_key(mosq->ssl_ctx);
                if(ret != 1){
                    _mosquitto_log_printf(mosq, MOSQ_LOG_ERR, "Error: Client certificate/key are inconsistent.");
                    //COMPAT_CLOSE(sock);
                    _mosquitto_socket_close(mosq); // DXL
                    return MOSQ_ERR_TLS;
                }
            }
        }

        mosq->ssl = SSL_new(mosq->ssl_ctx);
        if(!mosq->ssl){
            //COMPAT_CLOSE(sock);
            _mosquitto_socket_close(mosq); // DXL
            return MOSQ_ERR_TLS;
        }
        SSL_set_ex_data(mosq->ssl, tls_ex_index_mosq, mosq);
        bio = BIO_new_socket((int)sock, BIO_NOCLOSE);
        if(!bio){
            //COMPAT_CLOSE(sock);
            _mosquitto_socket_close(mosq); // DXL
            return MOSQ_ERR_TLS;
        }
        SSL_set_bio(mosq->ssl, bio, bio);

        ret = SSL_connect(mosq->ssl);
        if(ret != 1){
            ret = SSL_get_error(mosq->ssl, ret);
            if(ret == SSL_ERROR_WANT_READ){
                /* We always try to read anyway */
            }else if(ret == SSL_ERROR_WANT_WRITE){
                mosq->want_write = true;
            }else{
                //COMPAT_CLOSE(sock);
                _mosquitto_socket_close(mosq); // DXL
                return MOSQ_ERR_TLS;
            }
        }
    }

    mosq->sock = sock;

    return MOSQ_ERR_SUCCESS;
}

int _mosquitto_read_byte(struct _mosquitto_packet *packet, uint8_t *byte)
{
    assert(packet);
    if(packet->pos+1 > packet->remaining_length) return MOSQ_ERR_PROTOCOL;

    *byte = packet->payload[packet->pos];
    packet->pos++;

    return MOSQ_ERR_SUCCESS;
}

void _mosquitto_write_byte(struct _mosquitto_packet *packet, uint8_t byte)
{
    assert(packet);
    assert(packet->pos+1 <= packet->packet_length);

    packet->payload[packet->pos] = byte;
    packet->pos++;
}

int _mosquitto_read_bytes(struct _mosquitto_packet *packet, void *bytes, uint32_t count)
{
    assert(packet);
    if(packet->pos+count > packet->remaining_length) return MOSQ_ERR_PROTOCOL;

    memcpy(bytes, &(packet->payload[packet->pos]), count);
    packet->pos += count;

    return MOSQ_ERR_SUCCESS;
}

void _mosquitto_write_bytes(struct _mosquitto_packet *packet, const void *bytes, uint32_t count)
{
    assert(packet);
    assert(packet->pos+count <= packet->packet_length);

    memcpy(&(packet->payload[packet->pos]), bytes, count);
    packet->pos += count;
}

int _mosquitto_read_string(struct _mosquitto_packet *packet, char **str)
{
    uint16_t len;
    int rc;

    assert(packet);
    rc = _mosquitto_read_uint16(packet, &len);
    if(rc) return rc;

    if(packet->pos+len > packet->remaining_length) return MOSQ_ERR_PROTOCOL;

    *str = (char *)_mosquitto_calloc(len+1, sizeof(char));
    if(*str){
        memcpy(*str, &(packet->payload[packet->pos]), len);
        packet->pos += len;
    }else{
        return MOSQ_ERR_NOMEM;
    }

    return MOSQ_ERR_SUCCESS;
}

void _mosquitto_write_string(struct _mosquitto_packet *packet, const char *str, uint16_t length)
{
    assert(packet);
    _mosquitto_write_uint16(packet, length);
    _mosquitto_write_bytes(packet, str, length);
}

int _mosquitto_read_uint16(struct _mosquitto_packet *packet, uint16_t *word)
{
    uint8_t msb, lsb;

    assert(packet);
    if(packet->pos+2 > packet->remaining_length) return MOSQ_ERR_PROTOCOL;

    msb = packet->payload[packet->pos];
    packet->pos++;
    lsb = packet->payload[packet->pos];
    packet->pos++;

    *word = (msb<<8) + lsb;

    return MOSQ_ERR_SUCCESS;
}

void _mosquitto_write_uint16(struct _mosquitto_packet *packet, uint16_t word)
{
    _mosquitto_write_byte(packet, MOSQ_MSB(word));
    _mosquitto_write_byte(packet, MOSQ_LSB(word));
}

ssize_t _mosquitto_net_read(struct mosquitto *mosq, void *buf, size_t count)
{
    int ret;
    int err;
    char ebuf[256];
    unsigned long e;
    assert(mosq);
    errno = 0;
    if(mosq->ssl){
        ret = SSL_read(mosq->ssl, buf, (int)count);
        if(ret <= 0){
            err = SSL_get_error(mosq->ssl, ret);
            if(err == SSL_ERROR_WANT_READ){
                ret = -1;
                errno = EAGAIN;
            }else if(err == SSL_ERROR_WANT_WRITE){
                ret = -1;
                mosq->want_write = true;
                errno = EAGAIN;
            }else{
                e = ERR_get_error();
                while(e){
                    // DXL: Handshake failures only appear in debug due to the fact that we use a socket connect
                    // to emulate ping-like functionality from the clients.
                    if(((e == 336150757L && mosq->state == 0) || /*handshake failure*/
                          (e == 336105606L && 
                                (dxl_update_sent_byte_count(mosq, 0) ||
                                 !dxl_is_tenant_connection_allowed(mosq))) /*certificate verify failed*/)) {
                        if(IS_DEBUG_ENABLED) {
                            char peer_addr[256] = {0};
                            int peer_port = 0;
                            _mosquitto_socket_get_address(mosq->sock, peer_addr, 256, &peer_port);
                            _mosquitto_log_printf(mosq, MOSQ_LOG_DEBUG, 
                                "OpenSSL Error: %s. Peer: %s:%d", ERR_error_string(e, ebuf), peer_addr, peer_port);
                        }
                    }
                    else {
                        char peer_addr[256] = {0};
                        int peer_port = 0;
                        _mosquitto_socket_get_address(mosq->sock, peer_addr, 256, &peer_port);
                        _mosquitto_log_printf(mosq, MOSQ_LOG_ERR, 
                            "OpenSSL Error: %s. Peer: %s:%d", ERR_error_string(e, ebuf), peer_addr, peer_port);
                    }
                    e = ERR_get_error();
                }
                errno = EPROTO;
            }
        }
        return (ssize_t )ret;
    }else{
        /* Call normal read/recv */
    return read(mosq->sock, buf, count);
    }
}

ssize_t _mosquitto_net_write(struct mosquitto *mosq, void *buf, size_t count)
{
    int ret;
    int err;
    char ebuf[256];
    unsigned long e;
    assert(mosq);

    errno = 0;
    if(mosq->ssl){
        ret = SSL_write(mosq->ssl, buf, (int)count);
        if(ret < 0){
            err = SSL_get_error(mosq->ssl, ret);
            if(err == SSL_ERROR_WANT_READ){
                ret = -1;
                errno = EAGAIN;
            }else if(err == SSL_ERROR_WANT_WRITE){
                ret = -1;
                mosq->want_write = true;
                errno = EAGAIN;
            }else{
                e = ERR_get_error();
                while(e){
                    char peer_addr[256] = {0};
                    int peer_port = 0;
                    _mosquitto_socket_get_address(mosq->sock, peer_addr, 256, &peer_port);
                    _mosquitto_log_printf(mosq, MOSQ_LOG_ERR, 
                        "OpenSSL Error: %s. Peer: %s:%d", ERR_error_string(e, ebuf), peer_addr, peer_port);
                    e = ERR_get_error();
                }
                errno = EPROTO;
            }
        }
        return (ssize_t )ret;
    }else{
        /* Call normal write/send */

    return write(mosq->sock, buf, count);
    }
}

int _mosquitto_packet_write(struct mosquitto *mosq)
{
    ssize_t write_length;
    struct _mosquitto_packet *packet;

    if(!mosq) return MOSQ_ERR_INVAL;
    if(IS_CONTEXT_INVALID(mosq)) return MOSQ_ERR_NO_CONN;

    if(mosq->out_packet && !mosq->current_out_packet){
        mosq->current_out_packet = mosq->out_packet;
        mosq->out_packet = mosq->out_packet->next;
        if(!mosq->out_packet){
            mosq->out_packet_last = NULL;
        }
    }

    while(mosq->current_out_packet){
        packet = mosq->current_out_packet;


        while(packet->to_process > 0){
            if(mosq->wsi){
                write_length = mosquitto_ws_write(mosq, &(packet->payload[packet->pos]), packet->to_process);
            }
            else{
                write_length = _mosquitto_net_write(mosq, &(packet->payload[packet->pos]), packet->to_process);
            }

            if(write_length > 0){
                if(mosq->wsi) {                    
                    // LWS will internally buffer any data that is not sent and retry sending it later.
                    // So, advance the packet as though all data has been written.
                    packet->pos += packet->to_process;
                    packet->to_process = 0;
                }
                else{
                    packet->to_process -= write_length;
                    packet->pos += write_length;
                }

            }else{                
                if(mosq->wsi){
                    if(write_length == 0){
                        // LWS internally buffered the data. So, advance the packet position.
                        packet->pos += packet->to_process;
                        packet->to_process = 0;

                        // But, LWS could not write anymore and would have requested for POLLOUT. 
                        // Stop trying for this iteration.
                        return MOSQ_ERR_SUCCESS;
                    }
                    else{
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                            "[%p] lws_write failed. Return value [%d]", mosq->wsi, write_length);
                        return MOSQ_ERR_ERRNO;
                    }
                }

                if(errno == EAGAIN || errno == COMPAT_EWOULDBLOCK){
                    // Update context, will add EPOLLOUT if packets remain
                    mosquitto_update_context(mosq->numericId, mosq);
                    return MOSQ_ERR_SUCCESS;
                }else{
                    switch(errno){
                        case COMPAT_ECONNRESET:
                            return MOSQ_ERR_CONN_LOST;
                        default:
                            return MOSQ_ERR_ERRNO;
                    }
                }
            }
        }


        /* Free data and reset values */
        mosq->current_out_packet = mosq->out_packet;
        if(mosq->out_packet){
            mosq->out_packet = mosq->out_packet->next;
            if(!mosq->out_packet){
                mosq->out_packet_last = NULL;
            }
        }

#ifdef PACKET_COUNT
        mosq->packet_count--;
#endif
        _mosquitto_packet_cleanup(packet);
        _mosquitto_free(packet);

        mosq->last_msg_out = mosquitto_time();
    }
    return MOSQ_ERR_SUCCESS;
}

int _mosquitto_packet_read(struct mosquitto_db *db, struct mosquitto *mosq,
                           uint8_t* ws_buf, uint32_t ws_len)
{
    uint32_t ws_pos = 0;
    int rc = 0;

    if(!mosq) return MOSQ_ERR_INVAL;
    if(IS_CONTEXT_INVALID(mosq)) return MOSQ_ERR_NO_CONN;

    do{
        uint8_t byte;
        ssize_t read_length;    
        rc = 0;

        /* This gets called if pselect() indicates that there is network data
         * available - ie. at least one byte.  What we do depends on what data we
         * already have.
         * If we've not got a command, attempt to read one and save it. This should
         * always work because it's only a single byte.
         * Then try to read the remaining length. This may fail because it is may
         * be more than one byte - will need to save data pending next read if it
         * does fail.
         * Then try to read the remaining payload, where 'payload' here means the
         * combined variable header and actual payload. This is the most likely to
         * fail due to longer length, so save current data and current position.
         * After all data is read, send to _mosquitto_handle_packet() to deal with.
         * Finally, free the memory and reset everything to starting conditions.
         */
        if(!mosq->in_packet.command){
            if(ws_buf){
                if(ws_pos < ws_len){
                    byte = ws_buf[ws_pos++];
                    read_length = 1;                
                }
                else{
                    // No more data available.
                    read_length = -1;
                    errno = EAGAIN;
                }
            }
            else{
                read_length = _mosquitto_net_read(mosq, &byte, 1);
            }

            if(read_length == 1){
                mosq->in_packet.command = byte;

                /* Clients must send CONNECT as their first command. */
                if(!(mosq->bridge) && mosq->state == mosq_cs_new && (byte&0xF0) != CONNECT) return MOSQ_ERR_PROTOCOL;
            }else{
                if(read_length == 0) return MOSQ_ERR_CONN_LOST; /* EOF */
                if(errno == EAGAIN || errno == COMPAT_EWOULDBLOCK){
                    return MOSQ_ERR_SUCCESS;
                }else{
                    switch(errno){
                        case COMPAT_ECONNRESET:
                            return MOSQ_ERR_CONN_LOST;
                        default:
                            return MOSQ_ERR_ERRNO;
                    }
                }
            }
        }
        if(!mosq->in_packet.have_remaining){
            do{
                if(ws_buf){
                    if(ws_pos < ws_len){
                        byte = ws_buf[ws_pos++];
                        read_length = 1;                
                    }
                    else{
                        // No more data available.
                        read_length = -1;
                        errno = EAGAIN;
                    }
                }
                else{
                    read_length = _mosquitto_net_read(mosq, &byte, 1);
                }

                if(read_length == 1){
                    mosq->in_packet.remaining_count++;
                    /* Max 4 bytes length for remaining length as defined by protocol.
                     * Anything more likely means a broken/malicious client.
                     */
                    if(mosq->in_packet.remaining_count > 4) return MOSQ_ERR_PROTOCOL;

                    mosq->in_packet.remaining_length += (byte & 127) * mosq->in_packet.remaining_mult;
                    mosq->in_packet.remaining_mult *= 128;
                }else{
                    if(read_length == 0) return MOSQ_ERR_CONN_LOST; /* EOF */
                    if(errno == EAGAIN || errno == COMPAT_EWOULDBLOCK){
                        return MOSQ_ERR_SUCCESS;
                    }else{
                        switch(errno){
                            case COMPAT_ECONNRESET:
                                return MOSQ_ERR_CONN_LOST;
                            default:
                                return MOSQ_ERR_ERRNO;
                        }
                    }
                }
            }while((byte & 128) != 0);

            // DXL Begin
            uint32_t payloadLen = mosq->in_packet.remaining_length;

            // Disconnect if payload length exceeds maximum or
            // The count of sent bytes exceeds the limit for the client
            if((((mosq->in_packet.command)&0xF0)!=CONNECT) && (
                (db->config->message_size_limit && (payloadLen > (uint32_t)db->config->message_size_limit )) ||            
                dxl_update_sent_byte_count(mosq, mosq->in_packet.remaining_length)))
            {
                mqtt3_context_disconnect(db, mosq);
            }
            // DXL End

            if(mosq->in_packet.remaining_length > 0){
                mosq->in_packet.payload = (uint8_t *)_mosquitto_malloc(mosq->in_packet.remaining_length*sizeof(uint8_t));
                if(!mosq->in_packet.payload) return MOSQ_ERR_NOMEM;
                mosq->in_packet.to_process = mosq->in_packet.remaining_length;
            }
            mosq->in_packet.have_remaining = 1;
        }
        while(mosq->in_packet.to_process>0){

            if(ws_buf){
                if(ws_pos < ws_len){
                    if(ws_pos + mosq->in_packet.to_process < ws_len){
                        read_length = mosq->in_packet.to_process;
                        memcpy(&(mosq->in_packet.payload[mosq->in_packet.pos]), &(ws_buf[ws_pos]), read_length);
                        ws_pos = ws_pos + read_length;
                    }
                    else{
                        read_length = ws_len - ws_pos;
                        memcpy(&(mosq->in_packet.payload[mosq->in_packet.pos]), &(ws_buf[ws_pos]), read_length);
                        ws_pos = ws_len;
                    }
                }
                else{
                    // No more data available.
                    read_length = -1;
                    errno = EAGAIN;
                }
            }
            else{
                read_length = _mosquitto_net_read(
                        mosq, 
                        &(mosq->in_packet.payload[mosq->in_packet.pos]), 
                        mosq->in_packet.to_process);
            }
            if(read_length > 0){
                mosq->in_packet.to_process -= read_length;
                mosq->in_packet.pos += read_length;
            }else{
                if(errno == EAGAIN || errno == COMPAT_EWOULDBLOCK){
                    if(mosq->in_packet.to_process > 1000){
                        /* Update last_msg_in time if more than 1000 bytes left to
                         * receive. Helps when receiving large messages.
                         * This is an arbitrary limit, but with some consideration.
                         * If a client can't send 1000 bytes in a second it
                         * probably shouldn't be using a 1 second keep alive. */
                        mosq->last_msg_in = mosquitto_time();
                    }
                    return MOSQ_ERR_SUCCESS;
                }else{
                    switch(errno){
                        case COMPAT_ECONNRESET:
                            return MOSQ_ERR_CONN_LOST;
                        default:
                            return MOSQ_ERR_ERRNO;
                    }
                }
            }
        }


        /* All data for this packet is read. */
        mosq->in_packet.pos = 0;
        rc = mqtt3_packet_handle(db, mosq);

        /* Free data and reset values */
        _mosquitto_packet_cleanup(&mosq->in_packet);

        mosq->last_msg_in = mosquitto_time();
    }
    while(ws_buf && (ws_pos < ws_len)); // For websockets, the supplied buffer can have more than one packet. 
                                        // So, loop again for any remaining data in the buffer. 

    if(mosq->ssl){
        int pending = SSL_pending(mosq->ssl);
		if(pending > 0){
			mosquitto_add_pending_bytes_set(mosq);
		} else {
			mosquitto_remove_pending_bytes_set(mosq);
		}
	}

    return rc;
}

int _mosquitto_socket_nonblock(int sock)
{
    int opt;
    /* Set non-blocking */
    opt = fcntl(sock, F_GETFL, 0);
    if(opt == -1){
        COMPAT_CLOSE(sock);
        return 1;
    }
    if(fcntl(sock, F_SETFL, opt | O_NONBLOCK) == -1){
        /* If either fcntl fails, don't want to allow this client to connect. */
        COMPAT_CLOSE(sock);
        return 1;
    }
    return 0;
}
