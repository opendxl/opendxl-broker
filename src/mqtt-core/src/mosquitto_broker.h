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

#ifndef MQTT3_H
#define MQTT3_H

#include "dxlcommon.h"

#include <config.h>
#include <stdio.h>

#include <mosquitto_internal.h>
#include <mosquitto.h>
#include "tls_mosq.h"
#include "uthash.h"

#ifndef __GNUC__
#define __attribute__(attrib)
#endif

/* Log destinations */
#define MQTT3_LOG_NONE 0x00
#define MQTT3_LOG_FILE 0x02
#define MQTT3_LOG_STDOUT 0x04
#define MQTT3_LOG_STDERR 0x08
#define MQTT3_LOG_ALL 0xFF

// DXL
#define CONTEXT_REALLOC_SIZE 0xFFF

/* Flag to track listener contexts */
#define MOSQUITTO_EPOLL_DATA_LISTENER_FLAG   0x0000000080000000

/* Flag for tracking websockets */
#define MOSQUITTO_EPOLL_DATA_WEBSOCKETS_FLAG 0x0000000100000000

typedef uint64_t dbid_t;

struct _mqtt3_listener {
    int fd;
    char *host;
    uint16_t port;
    int max_connections;
    int *socks;
    int sock_count;
    int client_count;
    char *cafile;
    char *capath;
    char *certfile;
    char *keyfile;
    char *ciphers;
    bool require_certificate;
    SSL_CTX *ssl_ctx;
    char *crlfile;
    char *tls_version;
};

struct mqtt3_config {
    char *config_file;
    bool allow_duplicate_messages;
    bool connection_messages;
    bool daemon;
    struct _mqtt3_listener default_listener;
    struct _mqtt3_listener *listeners;
    int listener_count;
    int log_dest;
    int log_type;
    unsigned int log_category_mask;
    bool log_timestamp;
    char *log_file;
    FILE *log_fptr;
    int message_size_limit;
    char *pid_file;
    bool queue_qos0_messages;
    int retry_interval;
    int store_clean_interval;
    bool upgrade_outgoing_qos;
    char *user;
    bool verbose;
    struct _mqtt3_bridge *bridges;
    int bridge_count;
    bool ws_enabled;
    int ws_port;
};

struct _mosquitto_subleaf {
    struct _mosquitto_subleaf *prev;
    struct _mosquitto_subleaf *next;
    struct mosquitto *context;
    int qos;
};

struct _mosquitto_subhier {
    struct _mosquitto_subhier *children;
    struct _mosquitto_subhier *next;
    struct _mosquitto_subleaf *subs;
    char *topic;
    struct mosquitto_msg_store *retained;
    // Search OPT Begin
    struct subheir_hash *children_hash_table;
    int hash_table_has_pound_wild_card;
    int hash_table_has_plus_wild_card;
    // Search OPT End
};

struct mosquitto_msg_store{
    struct mosquitto_msg_store *next;
    dbid_t db_id;
    int ref_count;
    char *source_id;
    char **dest_ids;
    int dest_id_count;
    uint16_t source_mid;
    struct mosquitto_message msg;
};

struct mosquitto_client_msg{
    struct mosquitto_client_msg *next;
    struct mosquitto_msg_store *store;
    uint16_t mid;
    int qos;
    bool retain;
    time_t timestamp;
    enum mosquitto_msg_direction direction;
    enum mosquitto_msg_state state;
    bool dup;
    bool client_message; // DXL
};

struct _clientid_index_hash{
    /* this is the key */
    char *id;
    /* this is the index where the client ID exists in the db->contexts array */
    int db_context_index;
    UT_hash_handle hh;
};

struct mosquitto_db{
    dbid_t last_db_id;
    struct _mosquitto_subhier subs;
    struct mosquitto **contexts;
    struct _clientid_index_hash *clientid_index_hash;
    int context_count;
    struct mosquitto_msg_store *msg_store;
    int msg_store_count;
    struct mqtt3_config *config;
    int subscription_count;
    int retained_count;
};

enum mqtt3_bridge_direction{
    bd_out = 0,
    bd_in = 1,
    bd_both = 2
};

enum mosquitto_bridge_start_type{
    bst_automatic = 0,
    bst_lazy = 1,
    bst_manual = 2,
    bst_once = 3
};

struct _mqtt3_bridge_topic{
    char *topic;
    int qos;
    enum mqtt3_bridge_direction direction;
    char *local_prefix;
    char *remote_prefix;
    char *local_topic; /* topic prefixed with local_prefix */
    char *remote_topic; /* topic prefixed with remote_prefix */
};

struct bridge_address{
    char *address;
    int port;
};

struct _mqtt3_bridge{
    // DXL Begin
    int cur_primary_address;
    int primary_address_count;
    // DXL End
    char *name;
    struct bridge_address *addresses;
    int cur_address;
    int address_count;
    time_t primary_retry;
    bool round_robin;
    char *clientid;
    int keepalive;
    bool clean_session;
    struct _mqtt3_bridge_topic *topics;
    int topic_count;
    bool topic_remapping;
    time_t restart_t;
    enum mosquitto_bridge_start_type start_type;
    int idle_timeout;
    int restart_timeout;
    int threshold;
    bool lazy_reconnect;
    bool try_private;
    bool try_private_accepted;
    char *tls_cafile;
    char *tls_capath;
    char *tls_certfile;
    char *tls_keyfile;
    bool tls_insecure;
    char *tls_version;
};

#include <net_mosq.h>

/* ============================================================
 * Main functions
 * ============================================================ */
// DXL Begin
int mosquitto_epoll_init();
void mosquitto_epoll_destroy();
bool mosquitto_epoll_flag_is_listener(struct epoll_event *event);
bool mosquitto_epoll_flag_is_websocket(struct epoll_event *event);
int mosquitto_main_loop(struct mosquitto_db *db);
int mosquitto_get_listensock_count();
int* mosquitto_get_listensocks();
void mosquitto_set_listensocks(int* socks, int sock_count);
void mosquitto_add_new_clients_set(struct mosquitto* context);
void mosquitto_add_new_msgs_set(struct mosquitto* context);
void mosquitto_update_context(uint32_t ctx_idx, struct mosquitto *context);
void mosquitto_epoll_update_context_index(struct mosquitto *context, uint32_t new_ctx_idx);
int mosquitto_epoll_restart_listeners();
void mosquitto_remove_context(struct mosquitto *context);
// DXL End

struct mosquitto_db *_mosquitto_get_db(void);

/* ============================================================
 * Config functions
 * ============================================================ */
/* Initialise config struct to default values. */
void mqtt3_config_init(struct mqtt3_config *config);
/* Parse command line options into config. */
int mqtt3_config_parse_args(struct mqtt3_config *config, int argc, char *argv[]);
/* Read configuration data from config->config_file into config.
 * If reload is true, don't process config options that shouldn't be reloaded (listeners etc)
 * Returns 0 on success, 1 if there is a configuration error or if a file cannot be opened.
 */
int mqtt3_config_read(struct mqtt3_config *config, bool reload);
/* Free all config data. */
void mqtt3_config_cleanup(struct mqtt3_config *config);

// DXL Begin

/**
 * Adds a bridge to the current configuration.
 *
 * @param   config The Mosquitto config
 * @param   name The name of the bridge
 * @param   addresses The ordered list of broker addresses for the bridge
 * @param   addressCount The count of addresses
 * @param   bridge The bridge that was added to the configuration (out)
 * @return  Whether the operation was successful
 */
int mqtt3_config_add_bridge(
    struct mqtt3_config *config, 
    const char* name,
    struct bridge_address* addresses, 
    int addressCount,
    struct _mqtt3_bridge** bridge );

/**
 * Clears the current bridges from the configuration
 *
 * @return    Whether the operation was successful
 */
int mqtt3_config_clear_bridges(struct mqtt3_config *config);

/**
 * Sets the managed certs (SHA-1)
 *
 * @param    managedHashes The managed certs in a UT hash
 */
void mqtt3_config_set_managed_certs(struct cert_hashes* managedHashes);

/**
 * Updates the TLS settings
 *
 * @param   config The Mosquitto config
 * @param   tlsEnabled Whether TLS is enabled (via brokerlib)
 * @param   tlsBridgingInsecure Whether TLS bridging is insecure
 * @param   clientCertChainFile The client certificate chain file
 * @param   brokerCertChainFile The broker certificate chain file
 * @param   brokerKeyFile The broker private key
 * @param   brokerCertFile The broker certificate
 * @param   ciphers The ciphers to restrict to
 * @param   brokerCertsUtHash List of broker certificate hashes (SHA-1)
 */
int mqtt3_config_update_tls(
    struct mqtt3_config *config,
    bool tlsEnabled,
    bool tlsBridgingInsecure,
    const char* clientCertChainFile,
    const char* brokerCertChainFile,
    const char* brokerKeyFile,
    const char* brokerCertFile,
    const char* ciphers,
    struct cert_hashes* brokerCertsUtHash );

/**
 * Determines whether the specified certificate is a broker cert
 *
 * @param   certSha1 The certificate sha1
 * @return  Whether the specified certificate is a broker cert
 */
bool mqtt3_config_is_broker_cert(const char* certSha1);
// DXL End

/* ============================================================
 * Server send functions
 * ============================================================ */
int _mosquitto_send_connack(struct mosquitto *context, int result);
int _mosquitto_send_suback(struct mosquitto *context, uint16_t mid, uint32_t payloadlen, const void *payload);

/* ============================================================
 * Network functions
 * ============================================================ */
int mqtt3_socket_accept(struct mosquitto_db *db, int listensock);
int _mosquitto_socket_get_address(int sock, char *buf, int len, int* port);
int mqtt3_socket_listen(struct _mqtt3_listener *listener);

/* ============================================================
 * Read handling functions
 * ============================================================ */
int mqtt3_packet_handle(struct mosquitto_db *db, struct mosquitto *context);
int mqtt3_handle_connack(struct mosquitto_db *db, struct mosquitto *context);
int mqtt3_handle_connect(struct mosquitto_db *db, struct mosquitto *context);
int mqtt3_handle_disconnect(struct mosquitto_db *db, struct mosquitto *context);
int mqtt3_handle_publish(struct mosquitto_db *db, struct mosquitto *context);
int mqtt3_handle_subscribe(struct mosquitto_db *db, struct mosquitto *context);
int mqtt3_handle_unsubscribe(struct mosquitto_db *db, struct mosquitto *context);
// DXL Begin
int mqtt3_dxl_get_max_connect_count();
void mqtt3_dxl_set_max_connect_count(int count);
// DXL End

/* ============================================================
 * Database handling
 * ============================================================ */
int mqtt3_db_open(struct mqtt3_config *config, struct mosquitto_db *db);
int mqtt3_db_close(struct mosquitto_db *db);
// DXL Begin
void mqtt3_add_canonical_client_id(const char* clientid );
void mqtt3_db_remove_canonical_client_id(const char* clientid );
int mqtt3_db_get_canonical_client_id_count(const char* clientid);
int mqtt3_dxl_db_client_count(struct mosquitto_db *db);
void mqtt3_dxl_set_max_packet_buffer_size(uint64_t maxBufferSize );
int mqtt3_db_add_new_context(struct mosquitto_db* db, struct mosquitto* new_context);
// DXL End
int mqtt3_db_client_count(struct mosquitto_db *db, unsigned int *count, unsigned int *inactive_count);
void mqtt3_db_limits_set(int inflight, int queued);
/* Return the number of in-flight messages in count. */
int mqtt3_db_message_count(int *count);
int mqtt3_db_message_delete(struct mosquitto *context, uint16_t mid, enum mosquitto_msg_direction dir);
int mqtt3_db_message_insert(struct mosquitto_db *db, struct mosquitto *context, uint16_t mid,
    enum mosquitto_msg_direction dir, int qos, bool retain, struct mosquitto_msg_store *stored);
int mqtt3_db_message_release(struct mosquitto_db *db, struct mosquitto *context, uint16_t mid,
    enum mosquitto_msg_direction dir);
int mqtt3_db_message_update(struct mosquitto *context, uint16_t mid, enum mosquitto_msg_direction dir,
    enum mosquitto_msg_state state);
int mqtt3_db_message_write(struct mosquitto *context);
int mqtt3_db_messages_delete(struct mosquitto *context);
int mqtt3_db_messages_easy_queue(struct mosquitto_db *db, struct mosquitto *context, const char *topic, int qos,
    uint32_t payloadlen, const void *payload, int retain);
int mqtt3_db_messages_queue(struct mosquitto_db *db, const char *source_id, const char *topic, int qos, int retain,
    struct mosquitto_msg_store *stored);
int mqtt3_db_message_store(struct mosquitto_db *db, struct mosquitto* context /*DXL*/, const char *source,
    uint16_t source_mid, const char *topic, int qos, uint32_t payloadlen, const void *payload, int retain,
    struct mosquitto_msg_store **stored, dbid_t store_id);
int mqtt3_db_message_store_find(struct mosquitto *context, uint16_t mid, struct mosquitto_msg_store **stored);
/* Check all messages waiting on a client reply and resend if timeout has been exceeded. */
int mqtt3_db_message_timeout_check(struct mosquitto *context /* DXL */, unsigned int timeout);
int mqtt3_db_message_reconnect_reset(struct mosquitto *context);
int mqtt3_retain_queue(struct mosquitto_db *db, struct mosquitto *context, const char *sub, int sub_qos);
void mqtt3_db_store_clean(struct mosquitto_db *db);
void mqtt3_db_sys_update(struct mosquitto_db *db, int interval, time_t start_time);
void mqtt3_db_vacuum(void);
bool mqtt3_db_lookup_client(const char* clientId, struct mosquitto_db *db); // DXL

/* ============================================================
 * Subscription functions
 * ============================================================ */
int mqtt3_sub_add(struct mosquitto_db *db, struct mosquitto *context, const char *sub, int qos,
    struct _mosquitto_subhier *root);
int mqtt3_sub_remove(struct mosquitto_db *db, struct mosquitto *context, const char *sub,
    struct _mosquitto_subhier *root);
int mqtt3_sub_search(struct mosquitto_db *db, struct _mosquitto_subhier *root, const char *source_id,
    const char *topic, int qos, int retain, struct mosquitto_msg_store *stored);
int mqtt3_subs_clean_session(struct mosquitto_db *db, struct _mosquitto_subhier *root);
// DXL Begin
int mqtt3_sub_count(struct mosquitto_db *db, const char *topic, int *count, const char* tenant_guid);
int mqtt3_sub_init();
void mqtt3_sub_cleanup();
// DXL End

/* ============================================================
 * Context functions
 * ============================================================ */
struct mosquitto *mqtt3_context_init(int sock);
void mqtt3_context_cleanup(struct mosquitto_db *db, struct mosquitto *context,
    bool do_free, bool clean_subs /* DXL */ );
void mqtt3_context_disconnect(struct mosquitto_db *db, struct mosquitto *context);
// DXL Begin
void mqtt3_context_disconnect_byid(struct mosquitto_db *db, const char* contextId);
void mqtt3_context_cleanup_certs(struct mosquitto *context);
// DXL End

/* ============================================================
 * Logging functions
 * ============================================================ */
// DXL Begin
extern int log_priorities;
#define IS_DEBUG_ENABLED (log_priorities & MOSQ_LOG_DEBUG)
#define IS_INFO_ENABLED (log_priorities & MOSQ_LOG_INFO)
#define IS_NOTICE_ENABLED (log_priorities & MOSQ_LOG_NOTICE)
#define IS_WARNING_ENABLED (log_priorities & MOSQ_LOG_WARNING)
#define IS_SUBSCRIBE_ENABLED (log_priorities & MOSQ_LOG_SUBSCRIBE)
#define IS_UNSUBSCRIBE_ENABLED (log_priorities & MOSQ_LOG_UNSUBSCRIBE)
#define IS_CATEGORY_ENABLED(C) (log_category_mask & (C))
// DXL End
int mqtt3_log_init(int level, int destinations, unsigned int category_mask);
int mqtt3_log_close(void);
int _mosquitto_log_printf(struct mosquitto *mosq, int level, const char *fmt, ...) __attribute__((format(printf, 3, 4)));

/* ============================================================
 * Bridge functions
 * ============================================================ */
void mqtt3_set_default_bridge_keepalive(int keepalive); // DXL
int mqtt3_bridge_new(struct mosquitto_db *db, struct _mqtt3_bridge *bridge);
int mqtt3_bridge_connect(struct mosquitto_db *db, struct mosquitto *context);
void mqtt3_bridge_packet_cleanup(struct mosquitto *context);

/* ============================================================
 * Websockets functions
 * ============================================================ */
void mosquitto_ws_init(struct mqtt3_config *config);
void mosquitto_ws_destroy();
void mosquitto_ws_request_writeable_callback(struct mosquitto *mosq);
int mosquitto_ws_write(struct mosquitto *mosq, uint8_t* buf, uint32_t len);
int mosquitto_ws_add_fd(int sock, struct epoll_event* event);
int mosquitto_ws_mod_fd(int sock, struct epoll_event* event);
int mosquitto_ws_del_fd(int sock);
void mosquitto_ws_handle_poll(struct epoll_event* event);
void mosquitto_ws_do_maintenance();

/* ============================================================
 * SSL functions
 * ============================================================ */
int mosquitto_process_client_certificate(X509_STORE_CTX *ctx, struct mosquitto *context);


/* ============================================================
 * Window service related functions
 * ============================================================ */
#if defined(WIN32) || defined(__CYGWIN__)
void service_install(void);
void service_uninstall(void);
void service_run(void);
#endif

#endif
