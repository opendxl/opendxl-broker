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

#include <config.h>

#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#ifdef WITH_WRAP
#include <tcpd.h>
#endif

#ifdef __FreeBSD__
#  include <netinet/in.h>
#  include <sys/socket.h>
#endif

#ifdef __QNX__
#include <netinet/in.h>
#include <net/netbyte.h>
#include <sys/socket.h>
#endif

#include <mosquitto_broker.h>
#include <mqtt3_protocol.h>
#include <memory_mosq.h>
#include <net_mosq.h>
#include <util_mosq.h>

#include "tls_mosq.h"
#include <openssl/err.h>
// DXL Begin
#include "dxl.h"
#include "DxlFlags.h"
#include <openssl/ssl.h>
#include <openssl/x509_vfy.h>
// DXL End
static int tls_ex_index_context = -1;
static int tls_ex_index_listener = -1;

int mqtt3_socket_accept(struct mosquitto_db *db, int listensock)
{
    int i;
    int j;
    int new_sock = -1;
    struct mosquitto **tmp_contexts = NULL;
    struct mosquitto *new_context;
    BIO *bio;
    int rc;
    char ebuf[256];
    unsigned long e;
#ifdef WITH_WRAP
    struct request_info wrap_req;
    char address[1024];
#endif

    new_sock = accept(listensock, NULL, 0);
    if(new_sock == INVALID_SOCKET) return -1;

    if(_mosquitto_socket_nonblock(new_sock)){
        return INVALID_SOCKET;
    }

#ifdef WITH_WRAP
    /* Use tcpd / libwrap to determine whether a connection is allowed. */
    request_init(&wrap_req, RQ_FILE, new_sock, RQ_DAEMON, "mosquitto", 0);
    fromhost(&wrap_req);
    if(!hosts_access(&wrap_req)){
        /* Access is denied */
        if(!_mosquitto_socket_get_address(new_sock, address, 1024)){
            if(IS_NOTICE_ENABLED)
                _mosquitto_log_printf(NULL, MOSQ_LOG_NOTICE,
                    "Client connection from %s denied access by tcpd.", address);
        }
        COMPAT_CLOSE(new_sock);
        return -1;
    }else{
#endif
        new_context = mqtt3_context_init(new_sock);
        if(!new_context){
            COMPAT_CLOSE(new_sock);
            return -1;
        }
        for(i=0; i<db->config->listener_count; i++){
            for(j=0; j<db->config->listeners[i].sock_count; j++){
                if(db->config->listeners[i].socks[j] == listensock){
                    new_context->listener = &db->config->listeners[i];
                    new_context->listener->client_count++;
                    break;
                }
            }
        }
        if(!new_context->listener){
            mqtt3_context_cleanup(NULL, new_context, true, true /* DXL */);
            return -1;
        }

        if(new_context->listener->max_connections > 0 &&
            new_context->listener->client_count > new_context->listener->max_connections){
            if(IS_NOTICE_ENABLED)
                _mosquitto_log_printf(NULL, MOSQ_LOG_NOTICE,
                    "Client connection from %s denied: max_connections exceeded.", new_context->address);
            mqtt3_context_cleanup(NULL, new_context, true, true /* DXL */);
              return -1;
        }

        /* TLS init */
        for(i=0; i<db->config->listener_count; i++){
            for(j=0; j<db->config->listeners[i].sock_count; j++){
                if(db->config->listeners[i].socks[j] == listensock){
                    if(db->config->listeners[i].ssl_ctx){
                        new_context->ssl = SSL_new(db->config->listeners[i].ssl_ctx);
                        if(!new_context->ssl){
                            mqtt3_context_cleanup(NULL, new_context, true, true /* DXL */);
                            return -1;
                        }
                        SSL_set_ex_data(new_context->ssl, tls_ex_index_context, new_context);
                        SSL_set_ex_data(new_context->ssl, tls_ex_index_listener, &db->config->listeners[i]);
                        new_context->want_write = true;
                        bio = BIO_new_socket((int)new_sock, BIO_NOCLOSE);
                        SSL_set_bio(new_context->ssl, bio, bio);
                        rc = SSL_accept(new_context->ssl);
                        if(rc != 1){
                            rc = SSL_get_error(new_context->ssl, rc);
                            if(rc == SSL_ERROR_WANT_READ){
                                /* We always want to read. */
                            }else if(rc == SSL_ERROR_WANT_WRITE){
                                new_context->want_write = true;
                            }else{
                                e = ERR_get_error();
                                while(e){
                                    if(IS_NOTICE_ENABLED)
                                        _mosquitto_log_printf(NULL, MOSQ_LOG_NOTICE,
                                            "Client connection from %s failed: %s.",
                                            new_context->address, ERR_error_string(e, ebuf));
                                    e = ERR_get_error();
                                }
                                mqtt3_context_cleanup(NULL, new_context, true, true /* DXL */);
                                return -1;
                            }
                        }
                    }
                }
            }
        }

        if(IS_NOTICE_ENABLED)
            _mosquitto_log_printf(NULL, MOSQ_LOG_NOTICE,
                "New connection from %s on port %d.", new_context->address, new_context->listener->port);
        for(i=0; i<db->context_count; i++){
            if(db->contexts[i] == NULL){
                db->contexts[i] = new_context;
                new_context->numericId = i; // DXL
                mosquitto_add_new_clients_set(new_context); // Loop OPT
                break;
            }
        }
        if(i==db->context_count){
            tmp_contexts = (struct mosquitto **)_mosquitto_realloc(
                db->contexts, sizeof(struct mosquitto*)*(db->context_count+CONTEXT_REALLOC_SIZE));
            if(tmp_contexts){
                memset(tmp_contexts+db->context_count, 0, (CONTEXT_REALLOC_SIZE*sizeof(struct mosquitto*)));  
                db->context_count+=CONTEXT_REALLOC_SIZE;
                db->contexts = tmp_contexts;
                db->contexts[i] = new_context;
                new_context->numericId = i; // DXL
                mosquitto_add_new_clients_set(new_context); // Loop OPT
            }else{
                // Out of memory
                mqtt3_context_cleanup(NULL, new_context, true, true /* DXL */);
                return -1;
            }
        }
        // If we got here then the context's DB index is "i" regardless of how we got here
        new_context->db_index = i;

#ifdef WITH_WRAP
    }
#endif
    return new_sock;
}

// DXL Begin
static struct mosquitto *get_client_context(X509_STORE_CTX *ctx, int index)
{
    struct mosquitto *retVal = 0;
    
    SSL *ssl = 0;
    int idx = SSL_get_ex_data_X509_STORE_CTX_idx();
    if((ssl = (SSL *)X509_STORE_CTX_get_ex_data(ctx, idx)) != 0){
        retVal = (struct mosquitto *)SSL_get_ex_data(ssl, index);
    }

    return retVal;
}

static int process_client_certificate(X509_STORE_CTX *ctx, struct mosquitto *context)
{
    // Whether certificate validation succeeded
    int succeeded = 1;

    // Get the certificate from the context
    X509 *cert = 0;
    if((cert = X509_STORE_CTX_get_current_cert(ctx)) != 0){
        if(true){ // Check currently disabled
            unsigned int fprint_size;
            unsigned char fprint[EVP_MAX_MD_SIZE];
            char sha1str[41] = {0};

            if(X509_digest(cert, EVP_sha1(), fprint, &fprint_size)){
                char* p = sha1str;
                for(unsigned int i = 0; i < fprint_size; i++){
                    p += sprintf(p, "%02x", fprint[i]);
                }
                if(true){ // Check currently disabled
                    // Check to see if the certificate has been revoked
                    if(dxl_is_cert_revoked(sha1str)){
                        succeeded = 0;
                    }

                    // Add thumbprint for certificate to current set of hashes
                    struct cert_hashes* s = NULL;
                    HASH_FIND_STR(context->cert_hashes, sha1str, s);
                    if(!s){
                        s = (struct cert_hashes*)_mosquitto_malloc(sizeof(struct cert_hashes));
                        s->cert_sha1 = strdup(sha1str);
                        HASH_ADD_KEYPTR(hh, context->cert_hashes, s->cert_sha1, (unsigned int)strlen(s->cert_sha1), s);
                    }
                }

                if(IS_DEBUG_ENABLED){
                    _mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG,
                        "Certificate digest during connect, context: %p, sha1: '%s'",
                        context, sha1str);
                }
            }
        }

        if(NID_dxlClientGuid != 0){
            X509_CINF *cert_inf = cert->cert_info;
            STACK_OF(X509_EXTENSION) *ext_list = NULL;
            if(cert_inf){
                ext_list = cert_inf->extensions;
            }

            if(ext_list){
                for(int i = 0; i < sk_X509_EXTENSION_num(ext_list); i++){
                    ASN1_OBJECT *obj;
                    X509_EXTENSION *ext;
                    ext = sk_X509_EXTENSION_value(ext_list, i);
                    if(!ext) continue;
                    obj = X509_EXTENSION_get_object(ext);
                    if(!obj) continue;
                    int nid = OBJ_obj2nid(obj);
                    if(nid != 0 && (nid == NID_dxlClientGuid || nid == NID_dxlTenantGuid)){
                        ASN1_OCTET_STRING* octet_str = X509_EXTENSION_get_data(ext);
                        if(octet_str){
                            const unsigned char* octet_str_data = octet_str->data;
                            if(octet_str_data){
                                long xlen;
                                int tag, xclass;
                                /*int ret =*/ ASN1_get_object(&octet_str_data, &xlen, &tag, &xclass, octet_str->length);
                                if(nid == NID_dxlClientGuid){
                                    context->dxl_client_guid = _mosquitto_strdup((char*)octet_str_data);
                                }else{
                                    context->dxl_tenant_guid = _mosquitto_strdup((char*)octet_str_data);

                                    // Check to see if tenant limit has been exceeded
                                    if(dxl_update_sent_byte_count(context, 0) ||
                                        !dxl_is_tenant_connection_allowed(context)){
                                        // Has exceeded limit
                                        succeeded = 0;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return succeeded;
}

static int client_certificate_verify(int preverify_ok, X509_STORE_CTX *ctx)
{
    // Get the mosquitto context
    struct mosquitto *context = get_client_context(ctx, tls_ex_index_context);
    if(context){
        if(!process_client_certificate(ctx, context)){
            preverify_ok = 0;
        }
    }

    /* Preverify should check expiry, revocation. */
    return preverify_ok;
}
// DXL End

/* Creates a socket and listens on port 'port'.
 * Returns 1 on failure
 * Returns 0 on success.
 */
int mqtt3_socket_listen(struct _mqtt3_listener *listener)
{
    int sock = -1;
    struct addrinfo hints;
    struct addrinfo *ainfo, *rp;
    char service[10];
    int ss_opt = 1;
    int rc;
    X509_STORE *store;
    X509_LOOKUP *lookup;
    int ssl_options = 0;
    char buf[256];

#ifdef WITH_EC
#if OPENSSL_VERSION_NUMBER >= 0x10000000L && OPENSSL_VERSION_NUMBER < 0x10002000L
    EC_KEY *ecdh = NULL;
#endif
#endif

    if(!listener) return MOSQ_ERR_INVAL;

    snprintf(service, 10, "%d", listener->port);
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = PF_UNSPEC;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;

    if(getaddrinfo(listener->host, service, &hints, &ainfo)) return INVALID_SOCKET;

    listener->sock_count = 0;
    listener->socks = NULL;

    for(rp = ainfo; rp; rp = rp->ai_next){
        if(rp->ai_family == AF_INET){
            if(IS_INFO_ENABLED)
                _mosquitto_log_printf(NULL, MOSQ_LOG_INFO,
                    "Opening ipv4 listen socket on port %d.", ntohs(((struct sockaddr_in *)rp->ai_addr)->sin_port));
        }else if(rp->ai_family == AF_INET6){
            if(IS_INFO_ENABLED)
                _mosquitto_log_printf(NULL, MOSQ_LOG_INFO,
                    "Opening ipv6 listen socket on port %d.", ntohs(((struct sockaddr_in6 *)rp->ai_addr)->sin6_port));
        }else{
            continue;
        }

        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(sock == -1){
            // g++ compiler automatically defines _GNU_SOURCE
            if(IS_WARNING_ENABLED)
                _mosquitto_log_printf(NULL, MOSQ_LOG_WARNING, "Warning: %s", strerror_r(errno, buf, 256));
            continue;
        }
        listener->sock_count++;
        listener->socks = (int *)_mosquitto_realloc(listener->socks, sizeof(int)*listener->sock_count);
        if(!listener->socks){
            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory.");
            return MOSQ_ERR_NOMEM;
        }
        listener->socks[listener->sock_count-1] = sock;

        ss_opt = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &ss_opt, sizeof(ss_opt));
        ss_opt = 1;
        setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &ss_opt, sizeof(ss_opt));

        if(_mosquitto_socket_nonblock(sock)){
            return 1;
        }

        if(bind(sock, rp->ai_addr, (int)rp->ai_addrlen) == -1){
            // g++ compiler automatically defines _GNU_SOURCE
            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: %s", strerror_r(errno, buf, 256));
            COMPAT_CLOSE(sock);
            return 1;
        }

        if(listen(sock, 100) == -1){
            // g++ compiler automatically defines _GNU_SOURCE
            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: %s", strerror_r(errno, buf, 256));
            COMPAT_CLOSE(sock);
            return 1;
        }
    }
    freeaddrinfo(ainfo);

    /* We need to have at least one working socket. */
    if(listener->sock_count > 0){
        if((listener->cafile || listener->capath) && listener->certfile && listener->keyfile){
#if OPENSSL_VERSION_NUMBER >= 0x10001000L
            if(listener->tls_version == NULL){
                listener->ssl_ctx = SSL_CTX_new(TLSv1_2_server_method());
            }else if(!strcmp(listener->tls_version, "tlsv1.2")){
                listener->ssl_ctx = SSL_CTX_new(TLSv1_2_server_method());
            }else if(!strcmp(listener->tls_version, "tlsv1.1")){
                listener->ssl_ctx = SSL_CTX_new(TLSv1_1_server_method());
            }else if(!strcmp(listener->tls_version, "tlsv1")){
                listener->ssl_ctx = SSL_CTX_new(SSLv23_server_method());
            }
#else
            listener->ssl_ctx = SSL_CTX_new(SSLv23_server_method());
#endif
            if(!listener->ssl_ctx){
                _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Unable to create TLS context.");
                COMPAT_CLOSE(sock);
                return 1;
            }

            /* Don't accept SSLv2 or SSLv3 */
            ssl_options = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3;
#ifdef SSL_OP_NO_COMPRESSION
            /* Disable compression */
            ssl_options |= SSL_OP_NO_COMPRESSION;
#endif
#ifdef SSL_OP_CIPHER_SERVER_PREFERENCE
            /* Server chooses cipher */
            ssl_options |= SSL_OP_CIPHER_SERVER_PREFERENCE;
#endif
            SSL_CTX_set_options(listener->ssl_ctx, ssl_options);
            // Disable caching of sessions (DXL)
            SSL_CTX_set_session_cache_mode(listener->ssl_ctx, SSL_SESS_CACHE_OFF);

#ifdef SSL_MODE_RELEASE_BUFFERS
            /* Use even less memory per SSL connection. */
            SSL_CTX_set_mode(listener->ssl_ctx, SSL_MODE_RELEASE_BUFFERS);
#endif

#ifdef WITH_EC
#if OPENSSL_VERSION_NUMBER >= 0x10002000L && OPENSSL_VERSION_NUMBER < 0x10100000L
            SSL_CTX_set_ecdh_auto(listener->ssl_ctx, 1);
#elif OPENSSL_VERSION_NUMBER >= 0x10000000L && OPENSSL_VERSION_NUMBER < 0x10002000L
            ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
            if(!ecdh){
                _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Unable to create TLS ECDH curve.");
                return 1;
            }
            SSL_CTX_set_tmp_ecdh(listener->ssl_ctx, ecdh);
            EC_KEY_free(ecdh);
#endif
#endif

            snprintf(buf, 256, "mosquitto-%d", listener->port);
            SSL_CTX_set_session_id_context(listener->ssl_ctx, (unsigned char *)buf, (unsigned int)strlen(buf));

            if(listener->ciphers){
                rc = SSL_CTX_set_cipher_list(listener->ssl_ctx, listener->ciphers);
                if(rc == 0){
                    _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                        "Error: Unable to set TLS ciphers. Check cipher list \"%s\".", listener->ciphers);
                    COMPAT_CLOSE(sock);
                    return 1;
                }
            }else{
                rc = SSL_CTX_set_cipher_list(listener->ssl_ctx, "DEFAULT:!aNULL:!eNULL:!LOW:!EXPORT:!SSLv2:@STRENGTH");
                if(rc == 0){
                    _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                        "Error: Unable to set TLS ciphers. Check cipher list \"%s\".", listener->ciphers);
                    COMPAT_CLOSE(sock);
                    return 1;
                }
            }
            rc = SSL_CTX_load_verify_locations(listener->ssl_ctx, listener->cafile, listener->capath);
            if(rc == 0){
                if(listener->cafile && listener->capath){
                    _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                        "Error: Unable to load CA certificates. Check cafile \"%s\" and capath \"%s\".",
                        listener->cafile, listener->capath);
                }else if(listener->cafile){
                    _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                        "Error: Unable to load CA certificates. Check cafile \"%s\".", listener->cafile);
                }else{
                    _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                        "Error: Unable to load CA certificates. Check capath \"%s\".", listener->capath);
                }
                COMPAT_CLOSE(sock);
                return 1;
            }
            /* FIXME user data? */
            if(listener->require_certificate){
                SSL_CTX_set_verify(listener->ssl_ctx,
                    SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, client_certificate_verify);
            }else{
                SSL_CTX_set_verify(listener->ssl_ctx, SSL_VERIFY_NONE, client_certificate_verify);
            }
            rc = SSL_CTX_use_certificate_chain_file(listener->ssl_ctx, listener->certfile);
            if(rc != 1){
                _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                    "Error: Unable to load server certificate \"%s\". Check certfile.", listener->certfile);
                COMPAT_CLOSE(sock);
                return 1;
            }
            rc = SSL_CTX_use_PrivateKey_file(listener->ssl_ctx, listener->keyfile, SSL_FILETYPE_PEM);
            if(rc != 1){
                _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                    "Error: Unable to load server key file \"%s\" (%d). Check keyfile.", listener->keyfile, rc);
                COMPAT_CLOSE(sock);
                return 1;
            }
            rc = SSL_CTX_check_private_key(listener->ssl_ctx);
            if(rc != 1){
                _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Server certificate/key are inconsistent.");
                COMPAT_CLOSE(sock);
                return 1;
            }
            /* Load CRLs if they exist. */
            if(listener->crlfile){
                store = SSL_CTX_get_cert_store(listener->ssl_ctx);
                if(!store){
                    _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Unable to obtain TLS store.");
                    COMPAT_CLOSE(sock);
                    return 1;
                }
                lookup = X509_STORE_add_lookup(store, X509_LOOKUP_file());
                rc = X509_load_crl_file(lookup, listener->crlfile, X509_FILETYPE_PEM);
                if(rc != 1){
                    _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                        "Error: Unable to load certificate revocation file \"%s\". Check crlfile.", listener->crlfile);
                    COMPAT_CLOSE(sock);
                    return 1;
                }
                X509_STORE_set_flags(store, X509_V_FLAG_CRL_CHECK);
            }
            // DXL Begin
            if(tls_ex_index_context == -1){
                tls_ex_index_context = SSL_get_ex_new_index(0, (void *)"client context", NULL, NULL, NULL);
            }
            if(tls_ex_index_listener == -1){
                tls_ex_index_listener = SSL_get_ex_new_index(0, (void *)"listener", NULL, NULL, NULL);
            }
            // DXL End
        }
        return 0;
    }else{
        return 1;
    }
}

int _mosquitto_socket_get_address(int sock, char *buf, int len)
{
    struct sockaddr_storage addr;
    socklen_t addrlen;

    addrlen = sizeof(addr);
    if(!getpeername(sock, (struct sockaddr *)&addr, &addrlen)){
        if(addr.ss_family == AF_INET){
            if(inet_ntop(AF_INET, &((struct sockaddr_in *)&addr)->sin_addr.s_addr, buf, len)){
                return 0;
            }
        }else if(addr.ss_family == AF_INET6){
            if(inet_ntop(AF_INET6, &((struct sockaddr_in6 *)&addr)->sin6_addr.s6_addr, buf, len)){
                return 0;
            }
        }
    }
    return 1;
}
