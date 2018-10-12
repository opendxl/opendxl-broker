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

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#  include <dirent.h>

#  include <netdb.h>
#  include <sys/socket.h>

#include <mosquitto_broker.h>
#include <memory_mosq.h>
#include "tls_mosq.h"
#include "util_mosq.h"
#include "mqtt3_protocol.h"

struct config_recurse {
    int log_dest;
    int log_dest_set;
    int log_type;
    int log_type_set;
    int max_inflight_messages;
    int max_queued_messages;
};

#if defined(WIN32) || defined(__CYGWIN__)
#include <windows.h>
extern SERVICE_STATUS_HANDLE service_handle;
#endif

static int _conf_parse_bool(char **token, const char *name, bool *value, char *saveptr);
static int _conf_parse_int(char **token, const char *name, int *value, char *saveptr);
static int _conf_parse_string(char **token, const char *name, char **value, char *saveptr);
static int _config_read_file(struct mqtt3_config *config, bool reload, const char *file,
    struct config_recurse *config_tmp, int level, int *lineno);

static int _conf_attempt_resolve(const char *host, const char *text, int log, const char *msg)
{
    struct addrinfo gai_hints;
    struct addrinfo *gai_res;
    int rc;

    memset(&gai_hints, 0, sizeof(struct addrinfo));
    gai_hints.ai_family = PF_UNSPEC;
    gai_hints.ai_flags = AI_ADDRCONFIG;
    gai_hints.ai_socktype = SOCK_STREAM;
    gai_res = NULL;
    rc = getaddrinfo(host, NULL, &gai_hints, &gai_res);
    if(gai_res){
        freeaddrinfo(gai_res);
    }
    if(rc != 0){
        if(rc == EAI_SYSTEM){
            if(errno == ENOENT){
                _mosquitto_log_printf(NULL, log, "%s: Unable to resolve %s %s.", msg, text, host);
            }else{
                _mosquitto_log_printf(NULL, log, "%s: Error resolving %s: %s.", msg, text, strerror(errno));
            }
        }else{
            _mosquitto_log_printf(NULL, log, "%s: Error resolving %s: %s.", msg, text, gai_strerror(rc));
        }
        return MOSQ_ERR_INVAL;
    }
    return MOSQ_ERR_SUCCESS;
}

static void _config_init_reload(struct mqtt3_config *config)
{
    /* Set defaults */
    config->allow_duplicate_messages = false;
    config->connection_messages = true;
    if(config->log_fptr){
        fclose(config->log_fptr);
        config->log_fptr = NULL;
    }
    if(config->log_file){
        _mosquitto_free(config->log_file);
        config->log_file = NULL;
    }
#if defined(WIN32) || defined(__CYGWIN__)
    if(service_handle){
        /* This is running as a Windows service. Default to no logging. Using
         * stdout/stderr is forbidden because the first clients to connect will
         * get log information sent to them for some reason. */
#ifndef DXL
        config->log_dest = MQTT3_LOG_NONE;
#else
        config->log_dest = MQTT3_LOG_STDERR;
#endif
    }else{
        config->log_dest = MQTT3_LOG_STDERR;
    }
#else
    config->log_dest = MQTT3_LOG_STDERR;
    if(config->verbose){
        config->log_type = INT_MAX;
    }else{
        config->log_type = MOSQ_LOG_ERR | MOSQ_LOG_WARNING | MOSQ_LOG_NOTICE | MOSQ_LOG_INFO;
    }
#endif
    config->log_timestamp = true;
    config->queue_qos0_messages = false;
    config->retry_interval = 20;
    config->store_clean_interval = 10;
    config->upgrade_outgoing_qos = false;
}

void mqtt3_config_init(struct mqtt3_config *config)
{
    memset(config, 0, sizeof(struct mqtt3_config));
    _config_init_reload(config);
    config->config_file = NULL;
    config->daemon = false;
    config->default_listener.host = NULL;
    config->default_listener.port = 0;
    config->default_listener.max_connections = -1;
    config->default_listener.socks = NULL;
    config->default_listener.sock_count = 0;
    config->default_listener.client_count = 0;
    config->default_listener.tls_version = NULL;
    config->default_listener.cafile = NULL;
    config->default_listener.capath = NULL;
    config->default_listener.certfile = NULL;
    config->default_listener.keyfile = NULL;
    config->default_listener.ciphers = NULL;
    config->default_listener.require_certificate = false;
    config->default_listener.crlfile = NULL;
    config->listeners = NULL;
    config->listener_count = 0;
    config->pid_file = NULL;
    config->user = NULL;
    config->bridges = NULL;
    config->bridge_count = 0;
    config->verbose = false;
    config->message_size_limit = 0;
}

static void _bridge_cleanup(struct mqtt3_config *config)
{
    int i;
    int j;

    if(config->bridges){
        for(i=0; i<config->bridge_count; i++){
            if(config->bridges[i].name) _mosquitto_free(config->bridges[i].name);
            if(config->bridges[i].addresses){
                for(j=0; j<config->bridges[i].address_count; j++){
                    _mosquitto_free(config->bridges[i].addresses[j].address);
                }
                _mosquitto_free(config->bridges[i].addresses);
            }
            if(config->bridges[i].clientid) _mosquitto_free(config->bridges[i].clientid);
            if(config->bridges[i].topics){
                for(j=0; j<config->bridges[i].topic_count; j++){
                    if(config->bridges[i].topics[j].topic) _mosquitto_free(config->bridges[i].topics[j].topic);
                    if(config->bridges[i].topics[j].local_prefix)
                        _mosquitto_free(config->bridges[i].topics[j].local_prefix);
                    if(config->bridges[i].topics[j].remote_prefix)
                        _mosquitto_free(config->bridges[i].topics[j].remote_prefix);
                    if(config->bridges[i].topics[j].local_topic)
                        _mosquitto_free(config->bridges[i].topics[j].local_topic);
                    if(config->bridges[i].topics[j].remote_topic)
                        _mosquitto_free(config->bridges[i].topics[j].remote_topic);
                }
                _mosquitto_free(config->bridges[i].topics);
            }
        }
        _mosquitto_free(config->bridges);
        config->bridge_count = 0; // DXL
        config->bridges = NULL; // DXL
    }
}

void mqtt3_config_cleanup(struct mqtt3_config *config)
{
    int i;

    if(config->config_file) _mosquitto_free(config->config_file);
    if(config->user) _mosquitto_free(config->user); // DXL
    if(config->listeners){
        for(i=0; i<config->listener_count; i++){
            if(config->listeners[i].host) _mosquitto_free(config->listeners[i].host);
            if(config->listeners[i].socks) _mosquitto_free(config->listeners[i].socks);
            if(config->listeners[i].cafile) _mosquitto_free(config->listeners[i].cafile);
            if(config->listeners[i].capath) _mosquitto_free(config->listeners[i].capath);
            if(config->listeners[i].certfile) _mosquitto_free(config->listeners[i].certfile);
            if(config->listeners[i].keyfile) _mosquitto_free(config->listeners[i].keyfile);
            if(config->listeners[i].ciphers) _mosquitto_free(config->listeners[i].ciphers);
            if(config->listeners[i].crlfile) _mosquitto_free(config->listeners[i].crlfile);
            if(config->listeners[i].tls_version) _mosquitto_free(config->listeners[i].tls_version);
            if(config->listeners[i].ssl_ctx) SSL_CTX_free(config->listeners[i].ssl_ctx);
        }
        _mosquitto_free(config->listeners);
    }

    // DXL: Moved to common method
    _bridge_cleanup(config);

    if(config->log_fptr){
        fclose(config->log_fptr);
        config->log_fptr = NULL;
    }
    if(config->log_file){
        _mosquitto_free(config->log_file);
        config->log_file = NULL;
    }
}

static void print_usage(void)
{
    // DXL Begin
    printf("DXL Broker %s.%s.%s (build %s) (core %s) (%s)\n",
        SOMAJVER, SOMINVER, SOSUBMINVER, SOBLDNUM, VERSION, TIMESTAMP);
    printf("Usage: dxlbroker [--config config_file] [-d] [-h]\n\n");
    printf(" --config : specify the dxlbroker config file.\n");
    printf(" -d       : put the broker into the background after starting.\n");
    printf(" -h       : display this help.\n\n");
    // DXL End
}

int mqtt3_config_parse_args(struct mqtt3_config *config, int argc, char *argv[])
{
    int i;
    int port_tmp;

    for(i=1; i<argc; i++){
        // DXL Begin
        if(!strcmp(argv[i], "--config")){
            if(i<argc-1){
                i++;
                continue;
            }
        }
        if(!strcmp(argv[i], "--mosquitto-config")){
        // DXL End
            if(i<argc-1){
                config->config_file = _mosquitto_strdup(argv[i+1]);
                if(!config->config_file){
                    _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory.");
                    return MOSQ_ERR_NOMEM;
                }

                if(mqtt3_config_read(config, false)){
                    _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Unable to open configuration file.");
                    return MOSQ_ERR_INVAL;
                }
            }else{
                _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: -c argument given, but no config file specified.");
                return MOSQ_ERR_INVAL;
            }
            i++;
        }else if(!strcmp(argv[i], "-d") || !strcmp(argv[i], "--daemon")){
            config->daemon = true;
        }else if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")){
            print_usage();
            return MOSQ_ERR_INVAL;
        }else if(!strcmp(argv[i], "-p") || !strcmp(argv[i], "--port")){
            if(i<argc-1){
                port_tmp = atoi(argv[i+1]);
                if(port_tmp<1 || port_tmp>65535){
                    _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Invalid port specified (%d).", port_tmp);
                    return MOSQ_ERR_INVAL;
                }else{
                    if(config->default_listener.port){
                        if(IS_WARNING_ENABLED)
                            _mosquitto_log_printf(NULL, MOSQ_LOG_WARNING,
                                "Warning: Default listener port specified multiple times. "
                                "Only the latest will be used.");
                    }
                    config->default_listener.port = port_tmp;
                }
            }else{
                _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: -p argument given, but no port specified.");
                return MOSQ_ERR_INVAL;
            }
            i++;
        }else if(!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")){
            config->verbose = true;
        }else{
            fprintf(stderr, "Error: Unknown option '%s'.\n",argv[i]);
            print_usage();
            return MOSQ_ERR_INVAL;
        }
    }

    if(config->listener_count == 0
            || config->default_listener.cafile
            || config->default_listener.capath
            || config->default_listener.certfile
            || config->default_listener.keyfile
            || config->default_listener.ciphers
            || config->default_listener.require_certificate
            || config->default_listener.crlfile
            || config->default_listener.host
            || config->default_listener.port
            || config->default_listener.max_connections != -1){

        config->listener_count++;
        config->listeners = (struct _mqtt3_listener *)_mosquitto_realloc(config->listeners,
            sizeof(struct _mqtt3_listener)*config->listener_count);
        if(!config->listeners){
            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory.");
            return MOSQ_ERR_NOMEM;
        }
        if(config->default_listener.port){
            config->listeners[config->listener_count-1].port = config->default_listener.port;
        }else{
            config->listeners[config->listener_count-1].port = 1883;
        }
        if(config->default_listener.host){
            config->listeners[config->listener_count-1].host = config->default_listener.host;
        }else{
            config->listeners[config->listener_count-1].host = NULL;
        }
        config->listeners[config->listener_count-1].max_connections = config->default_listener.max_connections;
        config->listeners[config->listener_count-1].client_count = 0;
        config->listeners[config->listener_count-1].socks = NULL;
        config->listeners[config->listener_count-1].sock_count = 0;
        config->listeners[config->listener_count-1].client_count = 0;
        config->listeners[config->listener_count-1].tls_version = config->default_listener.tls_version;
        config->listeners[config->listener_count-1].cafile = config->default_listener.cafile;
        config->listeners[config->listener_count-1].capath = config->default_listener.capath;
        config->listeners[config->listener_count-1].certfile = config->default_listener.certfile;
        config->listeners[config->listener_count-1].keyfile = config->default_listener.keyfile;
        config->listeners[config->listener_count-1].ciphers = config->default_listener.ciphers;
        config->listeners[config->listener_count-1].require_certificate = config->default_listener.require_certificate;
        config->listeners[config->listener_count-1].ssl_ctx = NULL;
        config->listeners[config->listener_count-1].crlfile = config->default_listener.crlfile;
    }

    /* Default to drop to mosquitto user if we are privileged and no user specified. */
    if(!config->user){
        config->user = strdup("dxl"); // DXL
    }
    if(config->verbose){
        config->log_type = INT_MAX;
    }
    return MOSQ_ERR_SUCCESS;
}

int mqtt3_config_read(struct mqtt3_config *config, bool reload)
{
    int rc = MOSQ_ERR_SUCCESS;
    struct config_recurse cr;
    int lineno;
    int i;

    cr.log_dest = MQTT3_LOG_NONE;
    cr.log_dest_set = 0;
    cr.log_type = MOSQ_LOG_NONE;
    cr.log_type_set = 0;
    cr.max_inflight_messages = 20;
    cr.max_queued_messages = 100;

    if(!config->config_file) return 0;

    if(reload){
        /* Re-initialise appropriate config vars to default for reload. */
        _config_init_reload(config);
    }
    rc = _config_read_file(config, reload, config->config_file, &cr, 0, &lineno);
    if(rc){
        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error found at %s:%d.", config->config_file, lineno);
        return rc;
    }

    /* Default to drop to mosquitto user if no other user specified. This must
     * remain here even though it is covered in mqtt3_parse_args() because this
     * function may be called on its own. */
    if(!config->user){
        config->user = strdup("dxl"); // DXL
    }

    mqtt3_db_limits_set(cr.max_inflight_messages, cr.max_queued_messages);

    for(i=0; i<config->bridge_count; i++){
        if(!config->bridges[i].name || !config->bridges[i].addresses || !config->bridges[i].topic_count){
            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Invalid bridge configuration.");
            return MOSQ_ERR_INVAL;
        }
    }

    if(cr.log_dest_set){
        config->log_dest = cr.log_dest;
    }
    if(config->verbose){
        config->log_type = INT_MAX;
    }else if(cr.log_type_set){
        config->log_type = cr.log_type;
    }
    return MOSQ_ERR_SUCCESS;
}

int _config_read_file(struct mqtt3_config *config, bool reload, const char *file,
    struct config_recurse *cr, int /*level*/, int *lineno)
{
    FILE *fptr = NULL;
    char buf[1024];
    char *token;
    int port_tmp;
    char *saveptr = NULL;
    struct _mqtt3_bridge *cur_bridge = NULL;
    struct _mqtt3_bridge_topic *cur_topic;
    int len;
    struct _mqtt3_listener *cur_listener = &config->default_listener;
    char *address;
    int i;

    fptr = _mosquitto_fopen(file, "rt");
    if(!fptr){
        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Unable to open config file %s\n", file);
        return 1;
    }

    *lineno = 0;

    while(fgets(buf, 1024, fptr)){
        (*lineno)++;
        if(buf[0] != '#' && buf[0] != 10 && buf[0] != 13){
            while(buf[strlen(buf)-1] == 10 || buf[strlen(buf)-1] == 13){
                buf[strlen(buf)-1] = 0;
            }
            token = strtok_r(buf, " ", &saveptr);
            if(token){
                if(!strcmp(token, "address") || !strcmp(token, "addresses")){
                    if(reload) continue; // FIXME
                    if(!cur_bridge || cur_bridge->addresses){
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Invalid bridge configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                    while((token = strtok_r(NULL, " ", &saveptr))){
                        cur_bridge->address_count++;
                        cur_bridge->addresses = (struct bridge_address *)
                            _mosquitto_realloc(cur_bridge->addresses,
                                sizeof(struct bridge_address)*cur_bridge->address_count);
                        if(!cur_bridge->addresses){
                            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory.");
                            return MOSQ_ERR_NOMEM;
                        }
                        cur_bridge->addresses[cur_bridge->address_count-1].address = token;
                    }
                    for(i=0; i<cur_bridge->address_count; i++){
                        address = strtok_r(cur_bridge->addresses[i].address, ":", &saveptr);
                        if(address){
                            token = strtok_r(NULL, ":", &saveptr);
                            if(token){
                                port_tmp = atoi(token);
                                if(port_tmp < 1 || port_tmp > 65535){
                                    _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                                        "Error: Invalid port value (%d).", port_tmp);
                                    return MOSQ_ERR_INVAL;
                                }
                                cur_bridge->addresses[i].port = port_tmp;
                            }else{
                                cur_bridge->addresses[i].port = 1883;
                            }
                            cur_bridge->addresses[i].address = _mosquitto_strdup(address);
                            _conf_attempt_resolve(address, "bridge address", MOSQ_LOG_WARNING, "Warning");
                        }
                    }
                    if(cur_bridge->address_count == 0){
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Empty address value in configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                }else if(!strcmp(token, "allow_duplicate_messages")){
                    if(_conf_parse_bool(&token, "allow_duplicate_messages", &config->allow_duplicate_messages, saveptr))
                        return MOSQ_ERR_INVAL;
                }else if(!strcmp(token, "bind_address")){
                    if(reload) continue; // Listener not valid for reloading.
                    if(_conf_parse_string(&token, "default listener bind_address",
                        &config->default_listener.host, saveptr)) return MOSQ_ERR_INVAL;
                    if(_conf_attempt_resolve(config->default_listener.host, "bind_address", MOSQ_LOG_ERR, "Error")){
                        return MOSQ_ERR_INVAL;
                    }
                }else if(!strcmp(token, "bridge_cafile")){
                    if(reload) continue; // FIXME
                    if(!cur_bridge){
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Invalid bridge configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                    token = strtok_r(NULL, " ", &saveptr);
                    if(token){
                        if(cur_bridge->tls_cafile){
                            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                                "Error: Duplicate bridge_cafile value in bridge configuration.");
                            return MOSQ_ERR_INVAL;
                        }
                        cur_bridge->tls_cafile = _mosquitto_strdup(token);
                        if(!cur_bridge->tls_cafile){
                            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory");
                            return MOSQ_ERR_NOMEM;
                        }
                    }else{
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                            "Error: Empty bridge_cafile value in configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                }else if(!strcmp(token, "bridge_capath")){
                    if(reload) continue; // FIXME
                    if(!cur_bridge){
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Invalid bridge configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                    token = strtok_r(NULL, " ", &saveptr);
                    if(token){
                        if(cur_bridge->tls_capath){
                            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                                "Error: Duplicate bridge_capath value in bridge configuration.");
                            return MOSQ_ERR_INVAL;
                        }
                        cur_bridge->tls_capath = _mosquitto_strdup(token);
                        if(!cur_bridge->tls_capath){
                            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory");
                            return MOSQ_ERR_NOMEM;
                        }
                    }else{
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                            "Error: Empty bridge_capath value in configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                }else if(!strcmp(token, "bridge_certfile")){
                    if(reload) continue; // FIXME
                    if(!cur_bridge){
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Invalid bridge configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                    token = strtok_r(NULL, " ", &saveptr);
                    if(token){
                        if(cur_bridge->tls_certfile){
                            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                                "Error: Duplicate bridge_certfile value in bridge configuration.");
                            return MOSQ_ERR_INVAL;
                        }
                        cur_bridge->tls_certfile = _mosquitto_strdup(token);
                        if(!cur_bridge->tls_certfile){
                            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory");
                            return MOSQ_ERR_NOMEM;
                        }
                    }else{
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                            "Error: Empty bridge_certfile value in configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                }else if(!strcmp(token, "bridge_insecure")){
                    if(reload) continue; // FIXME
                    if(!cur_bridge){
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Invalid bridge configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                    if(_conf_parse_bool(&token, "bridge_insecure", &cur_bridge->tls_insecure, saveptr))
                        return MOSQ_ERR_INVAL;
                    if(cur_bridge->tls_insecure){
                        if(IS_WARNING_ENABLED)
                            _mosquitto_log_printf(NULL, MOSQ_LOG_WARNING,
                                "Warning: Bridge %s using insecure mode.", cur_bridge->name);
                    }
                }else if(!strcmp(token, "bridge_keyfile")){
                    if(reload) continue; // FIXME
                    if(!cur_bridge){
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Invalid bridge configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                    token = strtok_r(NULL, " ", &saveptr);
                    if(token){
                        if(cur_bridge->tls_keyfile){
                            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                                "Error: Duplicate bridge_keyfile value in bridge configuration.");
                            return MOSQ_ERR_INVAL;
                        }
                        cur_bridge->tls_keyfile = _mosquitto_strdup(token);
                        if(!cur_bridge->tls_keyfile){
                            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory");
                            return MOSQ_ERR_NOMEM;
                        }
                    }else{
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                            "Error: Empty bridge_keyfile value in configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                }else if(!strcmp(token, "bridge_tls_version")){
                    if(reload) continue; // FIXME
                    if(!cur_bridge){
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Invalid bridge configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                    token = strtok_r(NULL, " ", &saveptr);
                    if(token){
                        if(cur_bridge->tls_version){
                            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                                "Error: Duplicate bridge_tls_version value in bridge configuration.");
                            return MOSQ_ERR_INVAL;
                        }
                        cur_bridge->tls_version = _mosquitto_strdup(token);
                        if(!cur_bridge->tls_version){
                            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory");
                            return MOSQ_ERR_NOMEM;
                        }
                    }else{
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                            "Error: Empty bridge_tls_version value in configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                }else if(!strcmp(token, "cafile")){
                    if(reload) continue; // Listeners not valid for reloading.
                    if(_conf_parse_string(&token, "cafile", &cur_listener->cafile, saveptr)) return MOSQ_ERR_INVAL;
                }else if(!strcmp(token, "capath")){
                    if(reload) continue; // Listeners not valid for reloading.
                    if(_conf_parse_string(&token, "capath", &cur_listener->capath, saveptr)) return MOSQ_ERR_INVAL;
                }else if(!strcmp(token, "certfile")){
                    if(reload) continue; // Listeners not valid for reloading.
                    if(_conf_parse_string(&token, "certfile", &cur_listener->certfile, saveptr)) return MOSQ_ERR_INVAL;
                }else if(!strcmp(token, "ciphers")){
                    if(reload) continue; // Listeners not valid for reloading.
                    if(_conf_parse_string(&token, "ciphers", &cur_listener->ciphers, saveptr)) return MOSQ_ERR_INVAL;
                }else if(!strcmp(token, "cleansession")){
                    if(reload) continue; // FIXME
                    if(!cur_bridge){
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Invalid bridge configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                    if(_conf_parse_bool(&token, "cleansession", &cur_bridge->clean_session, saveptr))
                        return MOSQ_ERR_INVAL;
                }else if(!strcmp(token, "connection_messages")){
                    if(_conf_parse_bool(&token, token, &config->connection_messages, saveptr)) return MOSQ_ERR_INVAL;
                }else if(!strcmp(token, "crlfile")){
                    if(reload) continue; // Listeners not valid for reloading.
                    if(_conf_parse_string(&token, "crlfile", &cur_listener->crlfile, saveptr)) return MOSQ_ERR_INVAL;
                }else if(!strcmp(token, "idle_timeout")){
                    if(reload) continue; // FIXME
                    if(!cur_bridge){
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Invalid bridge configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                    if(_conf_parse_int(&token, "idle_timeout", &cur_bridge->idle_timeout, saveptr))
                        return MOSQ_ERR_INVAL;
                    if(cur_bridge->idle_timeout < 1){
                        if(IS_NOTICE_ENABLED)
                            _mosquitto_log_printf(NULL, MOSQ_LOG_NOTICE,
                                "idle_timeout interval too low, using 1 second.");
                        cur_bridge->idle_timeout = 1;
                    }
                }else if(!strcmp(token, "keepalive_interval")){
                    if(reload) continue; // FIXME
                    if(!cur_bridge){
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Invalid bridge configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                    if(_conf_parse_int(&token, "keepalive_interval", &cur_bridge->keepalive, saveptr))
                        return MOSQ_ERR_INVAL;
                    if(cur_bridge->keepalive < 5){
                        if(IS_NOTICE_ENABLED)
                            _mosquitto_log_printf(NULL, MOSQ_LOG_NOTICE,
                                "keepalive interval too low, using 5 seconds.");
                        cur_bridge->keepalive = 5;
                    }
                }else if(!strcmp(token, "keyfile")){
                    if(reload) continue; // Listeners not valid for reloading.
                    if(_conf_parse_string(&token, "keyfile", &cur_listener->keyfile, saveptr)) return MOSQ_ERR_INVAL;
                }else if(!strcmp(token, "listener")){
                    if(reload) continue; // Listeners not valid for reloading.
                    token = strtok_r(NULL, " ", &saveptr);
                    if(token){
                        config->listener_count++;
                        config->listeners = (struct _mqtt3_listener *)_mosquitto_realloc(config->listeners,
                            sizeof(struct _mqtt3_listener)*config->listener_count);
                        if(!config->listeners){
                            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory.");
                            return MOSQ_ERR_NOMEM;
                        }
                        port_tmp = atoi(token);
                        if(port_tmp < 1 || port_tmp > 65535){
                            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Invalid port value (%d).", port_tmp);
                            return MOSQ_ERR_INVAL;
                        }
                        cur_listener = &config->listeners[config->listener_count-1];
                        memset(cur_listener, 0, sizeof(struct _mqtt3_listener));
                        cur_listener->port = port_tmp;
                        token = strtok_r(NULL, " ", &saveptr);
                        if(token){
                            cur_listener->host = _mosquitto_strdup(token);
                        }else{
                            cur_listener->host = NULL;
                        }
                    }else{
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Empty listener value in configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                }else if(!strcmp(token, "log_dest")){
                    token = strtok_r(NULL, " ", &saveptr);
                    if(token){
                        cr->log_dest_set = 1;
                        if(!strcmp(token, "none")){
                            cr->log_dest = MQTT3_LOG_NONE;
                        }else if(!strcmp(token, "stdout")){
                            cr->log_dest |= MQTT3_LOG_STDOUT;
                        }else if(!strcmp(token, "stderr")){
                            cr->log_dest |= MQTT3_LOG_STDERR;
                        }else if(!strcmp(token, "file")){
                            cr->log_dest |= MQTT3_LOG_FILE;
                            if(config->log_fptr || config->log_file){
                                _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Duplicate \"log_dest file\" value.");
                                return MOSQ_ERR_INVAL;
                            }
                            /* Get remaining string. */
                            token = &token[strlen(token)+1];
                            while(token[0] == ' '){
                                token++;
                            }
                            if(token[0]){
                                config->log_file = _mosquitto_strdup(token);
                                if(!config->log_file){
                                    _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory");
                                    return MOSQ_ERR_NOMEM;
                                }
                                config->log_fptr = _mosquitto_fopen(config->log_file, "at");
                                if(!config->log_fptr){
                                    _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                                        "Error: Unable to open log file %s for writing.", config->log_file);
                                    return MOSQ_ERR_INVAL;
                                }
                            }else{
                                _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                                    "Error: Empty \"log_dest file\" value in configuration.");
                                return MOSQ_ERR_INVAL;
                            }
                        }else{
                            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Invalid log_dest value (%s).", token);
                            return MOSQ_ERR_INVAL;
                        }
#if defined(WIN32) || defined(__CYGWIN__)
                        if(service_handle){
                            if(cr->log_dest == MQTT3_LOG_STDOUT || cr->log_dest == MQTT3_LOG_STDERR){
                                _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                                    "Error: Cannot log to stdout/stderr when running as a Windows service.");
                                return MOSQ_ERR_INVAL;
                            }
                        }
#endif
                    }else{
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Empty log_dest value in configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                }else if(!strcmp(token, "log_timestamp")){
                    if(_conf_parse_bool(&token, token, &config->log_timestamp, saveptr)) return MOSQ_ERR_INVAL;
                }else if(!strcmp(token, "log_type")){
                    token = strtok_r(NULL, " ", &saveptr);
                    if(token){
                        cr->log_type_set = 1;
                        if(!strcmp(token, "none")){
                            cr->log_type = MOSQ_LOG_NONE;
                        }else if(!strcmp(token, "information")){
                            cr->log_type |= MOSQ_LOG_INFO;
                        }else if(!strcmp(token, "notice")){
                            cr->log_type |= MOSQ_LOG_NOTICE;
                        }else if(!strcmp(token, "warning")){
                            cr->log_type |= MOSQ_LOG_WARNING;
                        }else if(!strcmp(token, "error")){
                            cr->log_type |= MOSQ_LOG_ERR;
                        }else if(!strcmp(token, "debug")){
                            cr->log_type |= MOSQ_LOG_DEBUG;
                        }else if(!strcmp(token, "subscribe")){
                            cr->log_type |= MOSQ_LOG_SUBSCRIBE;
                        }else if(!strcmp(token, "unsubscribe")){
                            cr->log_type |= MOSQ_LOG_UNSUBSCRIBE;
                        }else if(!strcmp(token, "all")){
                            cr->log_type = INT_MAX;
                        }else{
                            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Invalid log_type value (%s).", token);
                            return MOSQ_ERR_INVAL;
                        }
                    }else{
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Empty log_type value in configuration.");
                    }
                }else if(!strcmp(token, "max_connections")){
                    if(reload) continue; // Listeners not valid for reloading.
                    token = strtok_r(NULL, " ", &saveptr);
                    if(token){
                        cur_listener->max_connections = atoi(token);
                        if(cur_listener->max_connections < 0) cur_listener->max_connections = -1;
                    }else{
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                            "Error: Empty max_connections value in configuration.");
                    }
                }else if(!strcmp(token, "max_inflight_messages")){
                    token = strtok_r(NULL, " ", &saveptr);
                    if(token){
                        cr->max_inflight_messages = atoi(token);
                        if(cr->max_inflight_messages < 0) cr->max_inflight_messages = 0;
                    }else{
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                            "Error: Empty max_inflight_messages value in configuration.");
                    }
                }else if(!strcmp(token, "max_queued_messages")){
                    token = strtok_r(NULL, " ", &saveptr);
                    if(token){
                        cr->max_queued_messages = atoi(token);
                        if(cr->max_queued_messages < 0) cr->max_queued_messages = 0;
                    }else{
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                            "Error: Empty max_queued_messages value in configuration.");
                    }
                }else if(!strcmp(token, "message_size_limit")){
                    if(_conf_parse_int(&token, "message_size_limit", &config->message_size_limit, saveptr))
                        return MOSQ_ERR_INVAL;
                    if(config->message_size_limit < 0 || config->message_size_limit > MQTT_MAX_PAYLOAD){
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                            "Error: Invalid message_size_limit value (%d).", config->message_size_limit);
                        return MOSQ_ERR_INVAL;
                    }
                }else if(!strcmp(token, "pid_file")){
                    if(reload) continue; // pid file not valid for reloading.
                    if(_conf_parse_string(&token, "pid_file", &config->pid_file, saveptr)) return MOSQ_ERR_INVAL;
                }else if(!strcmp(token, "port")){
                    if(reload) continue; // Listener not valid for reloading.
                    if(config->default_listener.port){
                        if(IS_WARNING_ENABLED)
                            _mosquitto_log_printf(NULL, MOSQ_LOG_WARNING,
                                "Warning: Default listener port specified multiple times. "
                                "Only the latest will be used.");
                    }
                    if(_conf_parse_int(&token, "port", &port_tmp, saveptr)) return MOSQ_ERR_INVAL;
                    if(port_tmp < 1 || port_tmp > 65535){
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Invalid port value (%d).", port_tmp);
                        return MOSQ_ERR_INVAL;
                    }
                    config->default_listener.port = port_tmp;
                }else if(!strcmp(token, "queue_qos0_messages")){
                    if(_conf_parse_bool(&token, token, &config->queue_qos0_messages, saveptr)) return MOSQ_ERR_INVAL;
                }else if(!strcmp(token, "require_certificate")){
                    if(reload) continue; // Listeners not valid for reloading.
                    if(_conf_parse_bool(&token, "require_certificate", &cur_listener->require_certificate, saveptr))
                        return MOSQ_ERR_INVAL;
                }else if(!strcmp(token, "restart_timeout")){
                    if(reload) continue; // FIXME
                    if(!cur_bridge){
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Invalid bridge configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                    if(_conf_parse_int(&token, "restart_timeout", &cur_bridge->restart_timeout, saveptr))
                        return MOSQ_ERR_INVAL;
                    if(cur_bridge->restart_timeout < 1){
                        if(IS_NOTICE_ENABLED)
                            _mosquitto_log_printf(NULL, MOSQ_LOG_NOTICE,
                                "restart_timeout interval too low, using 1 second.");
                        cur_bridge->restart_timeout = 1;
                    }
                }else if(!strcmp(token, "retry_interval")){
                    if(_conf_parse_int(&token, "retry_interval", &config->retry_interval, saveptr)) return MOSQ_ERR_INVAL;
                    if(config->retry_interval < 1 || config->retry_interval > 3600){
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                            "Error: Invalid retry_interval value (%d).", config->retry_interval);
                        return MOSQ_ERR_INVAL;
                    }
                }else if(!strcmp(token, "round_robin")){
                    if(reload) continue; // FIXME
                    if(!cur_bridge){
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Invalid bridge configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                    if(_conf_parse_bool(&token, "round_robin", &cur_bridge->round_robin, saveptr)) return MOSQ_ERR_INVAL;
                }else if(!strcmp(token, "start_type")){
                    if(reload) continue; // FIXME
                    if(!cur_bridge){
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Invalid bridge configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                    token = strtok_r(NULL, " ", &saveptr);
                    if(token){
                        if(!strcmp(token, "automatic")){
                            cur_bridge->start_type = bst_automatic;
                        }else if(!strcmp(token, "lazy")){
                            cur_bridge->start_type = bst_lazy;
                        }else if(!strcmp(token, "manual")){
                            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Manual start_type not supported.");
                            return MOSQ_ERR_INVAL;
                        }else if(!strcmp(token, "once")){
                            cur_bridge->start_type = bst_once;
                        }else{
                            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                                "Error: Invalid start_type value in configuration (%s).", token);
                            return MOSQ_ERR_INVAL;
                        }
                    }else{
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Empty start_type value in configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                }else if(!strcmp(token, "store_clean_interval")){
                    if(_conf_parse_int(&token, "store_clean_interval", &config->store_clean_interval, saveptr))
                        return MOSQ_ERR_INVAL;
                    if(config->store_clean_interval < 0 || config->store_clean_interval > 65535){
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                            "Error: Invalid store_clean_interval value (%d).", config->store_clean_interval);
                        return MOSQ_ERR_INVAL;
                    }
                }else if(!strcmp(token, "threshold")){
                    if(reload) continue; // FIXME
                    if(!cur_bridge){
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Invalid bridge configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                    if(_conf_parse_int(&token, "threshold", &cur_bridge->threshold, saveptr)) return MOSQ_ERR_INVAL;
                    if(cur_bridge->threshold < 1){
                        if(IS_NOTICE_ENABLED)
                            _mosquitto_log_printf(NULL, MOSQ_LOG_NOTICE, "threshold too low, using 1 message.");
                        cur_bridge->threshold = 1;
                    }
                }else if(!strcmp(token, "tls_version")){
                    if(reload) continue; // Listeners not valid for reloading.
                    if(_conf_parse_string(&token, "tls_version", &cur_listener->tls_version, saveptr))
                        return MOSQ_ERR_INVAL;
                }else if(!strcmp(token, "topic")){
                    if(reload) continue; // FIXME
                    if(!cur_bridge){
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Invalid bridge configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                    token = strtok_r(NULL, " ", &saveptr);
                    if(token){
                        cur_bridge->topic_count++;
                        cur_bridge->topics = (struct _mqtt3_bridge_topic *)_mosquitto_realloc(cur_bridge->topics, 
                                sizeof(struct _mqtt3_bridge_topic)*cur_bridge->topic_count);
                        if(!cur_bridge->topics){
                            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory");
                            return MOSQ_ERR_NOMEM;
                        }
                        cur_topic = &cur_bridge->topics[cur_bridge->topic_count-1];
                        if(!strcmp(token, "\"\"")){
                            cur_topic->topic = NULL;
                        }else{
                            cur_topic->topic = _mosquitto_strdup(token);
                            if(!cur_topic->topic){
                                _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory");
                                return MOSQ_ERR_NOMEM;
                            }
                        }
                        cur_topic->direction = bd_out;
                        cur_topic->qos = 0;
                        cur_topic->local_prefix = NULL;
                        cur_topic->remote_prefix = NULL;
                    }else{
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Empty topic value in configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                    token = strtok_r(NULL, " ", &saveptr);
                    if(token){
                        if(!strcasecmp(token, "out")){
                            cur_topic->direction = bd_out;
                        }else if(!strcasecmp(token, "in")){
                            cur_topic->direction = bd_in;
                        }else if(!strcasecmp(token, "both")){
                            cur_topic->direction = bd_both;
                        }else{
                            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                                "Error: Invalid bridge topic direction '%s'.", token);
                            return MOSQ_ERR_INVAL;
                        }
                        token = strtok_r(NULL, " ", &saveptr);
                        if(token){
                            cur_topic->qos = atoi(token);
                            if(cur_topic->qos < 0 || cur_topic->qos > 2){
                                _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                                    "Error: Invalid bridge QoS level '%s'.", token);
                                return MOSQ_ERR_INVAL;
                            }

                            token = strtok_r(NULL, " ", &saveptr);
                            if(token){
                                cur_bridge->topic_remapping = true;
                                if(!strcmp(token, "\"\"")){
                                    cur_topic->local_prefix = NULL;
                                }else{
                                    if(_mosquitto_topic_wildcard_len_check(token) != MOSQ_ERR_SUCCESS){
                                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                                            "Error: Invalid bridge topic local prefix '%s'.", token);
                                        return MOSQ_ERR_INVAL;
                                    }
                                    cur_topic->local_prefix = _mosquitto_strdup(token);
                                    if(!cur_topic->local_prefix){
                                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory");
                                        return MOSQ_ERR_NOMEM;
                                    }
                                }

                                token = strtok_r(NULL, " ", &saveptr);
                                if(token){
                                    if(!strcmp(token, "\"\"")){
                                        cur_topic->remote_prefix = NULL;
                                    }else{
                                        if(_mosquitto_topic_wildcard_len_check(token) != MOSQ_ERR_SUCCESS){
                                            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR,
                                                "Error: Invalid bridge topic remote prefix '%s'.", token);
                                            return MOSQ_ERR_INVAL;
                                        }
                                        cur_topic->remote_prefix = _mosquitto_strdup(token);
                                        if(!cur_topic->remote_prefix){
                                            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory");
                                            return MOSQ_ERR_NOMEM;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if(cur_topic->topic == NULL && 
                            (cur_topic->local_prefix == NULL || cur_topic->remote_prefix == NULL)){

                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Invalid bridge remapping.");
                        return MOSQ_ERR_INVAL;
                    }
                    if(cur_topic->local_prefix){
                        if(cur_topic->topic){
                            len = (int)(strlen(cur_topic->topic) + strlen(cur_topic->local_prefix)+1);
                            cur_topic->local_topic = (char *)_mosquitto_calloc(len+1, sizeof(char));
                            if(!cur_topic->local_topic){
                                _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory");
                                return MOSQ_ERR_NOMEM;
                            }
                            snprintf(cur_topic->local_topic, len+1, "%s%s", cur_topic->local_prefix, cur_topic->topic);
                        }else{
                            cur_topic->local_topic = _mosquitto_strdup(cur_topic->local_prefix);
                            if(!cur_topic->local_topic){
                                _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory");
                                return MOSQ_ERR_NOMEM;
                            }
                        }
                    }else{
                        cur_topic->local_topic = _mosquitto_strdup(cur_topic->topic);
                        if(!cur_topic->local_topic){
                            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory");
                            return MOSQ_ERR_NOMEM;
                        }
                    }

                    if(cur_topic->remote_prefix){
                        if(cur_topic->topic){
                            len = (int)(strlen(cur_topic->topic) + strlen(cur_topic->remote_prefix)+1);
                            cur_topic->remote_topic = (char *)_mosquitto_calloc(len+1, sizeof(char));
                            if(!cur_topic->remote_topic){
                                _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory");
                                return MOSQ_ERR_NOMEM;
                            }
                            snprintf(cur_topic->remote_topic, len, "%s%s", cur_topic->remote_prefix, cur_topic->topic);
                        }else{
                            cur_topic->remote_topic = _mosquitto_strdup(cur_topic->remote_prefix);
                            if(!cur_topic->remote_topic){
                                _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory");
                                return MOSQ_ERR_NOMEM;
                            }
                        }
                    }else{
                        cur_topic->remote_topic = _mosquitto_strdup(cur_topic->topic);
                        if(!cur_topic->remote_topic){
                            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory");
                            return MOSQ_ERR_NOMEM;
                        }
                    }
                }else if(!strcmp(token, "try_private")){
                    if(reload) continue; // FIXME
                    if(!cur_bridge){
                        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Invalid bridge configuration.");
                        return MOSQ_ERR_INVAL;
                    }
                    if(_conf_parse_bool(&token, "try_private", &cur_bridge->try_private, saveptr)) return MOSQ_ERR_INVAL;
                }else if(!strcmp(token, "upgrade_outgoing_qos")){
                    if(_conf_parse_bool(&token, token, &config->upgrade_outgoing_qos, saveptr)) return MOSQ_ERR_INVAL;
                }else if(!strcmp(token, "user")){
                    if(reload) continue; // Drop privileges user not valid for reloading.
                    if(_conf_parse_string(&token, "user", &config->user, saveptr)) return MOSQ_ERR_INVAL;
                }else if(!strcmp(token, "trace_level")
                        || !strcmp(token, "ffdc_output")
                        || !strcmp(token, "max_log_entries")
                        || !strcmp(token, "trace_output")){
                    if(IS_WARNING_ENABLED)
                        _mosquitto_log_printf(NULL, MOSQ_LOG_WARNING,
                            "Warning: Unsupported rsmb configuration option \"%s\".", token);
                }else{
                    _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Unknown configuration variable \"%s\".", token);
                    return MOSQ_ERR_INVAL;
                }
            }
        }
    }
    fclose(fptr);

    return MOSQ_ERR_SUCCESS;
}

static int _conf_parse_bool(char **token, const char *name, bool *value, char *saveptr)
{
    *token = strtok_r(NULL, " ", &saveptr);
    if(*token){
        if(!strcmp(*token, "false") || !strcmp(*token, "0")){
            *value = false;
        }else if(!strcmp(*token, "true") || !strcmp(*token, "1")){
            *value = true;
        }else{
            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Invalid %s value (%s).", name, *token);
        }
    }else{
        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Empty %s value in configuration.", name);
        return MOSQ_ERR_INVAL;
    }
    
    return MOSQ_ERR_SUCCESS;
}

static int _conf_parse_int(char **token, const char *name, int *value, char *saveptr)
{
    *token = strtok_r(NULL, " ", &saveptr);
    if(*token){
        *value = atoi(*token);
    }else{
        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Empty %s value in configuration.", name);
        return MOSQ_ERR_INVAL;
    }

    return MOSQ_ERR_SUCCESS;
}

static int _conf_parse_string(char **token, const char *name, char **value, char *saveptr)
{
    *token = strtok_r(NULL, " ", &saveptr);
    if(*token){
        if(*value){
            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Duplicate %s value in configuration.", name);
            return MOSQ_ERR_INVAL;
        }
        *value = _mosquitto_strdup(*token);
        if(!*value){
            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory");
            return MOSQ_ERR_NOMEM;
        }
    }else{
        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Empty %s value in configuration.", name);
        return MOSQ_ERR_INVAL;
    }
    return MOSQ_ERR_SUCCESS;
}

// DXL Begin

/** Whether TLS is enabled (via brokerlib) */
static bool s_tlsEnabled = false;
/** Whether TLS bridging is insecure */
static bool s_tlsBridgingInsecure = true;
/** The client certificate chain file */
static const char* s_clientCertChainFile = NULL;
/** The broker certificate chain file */
static const char* s_brokerCertChainFile = NULL;
/** The broker private key */
static const char* s_brokerKeyFile = NULL;
/** The broker certificate */
static const char* s_brokerCertFile = NULL; 
/** The ciphers */
static const char* s_ciphers = NULL;
/** List of broker certificate hashes (SHA-1) */
struct cert_hashes* s_brokerCerts = NULL;

 int mqtt3_config_update_tls(
    struct mqtt3_config *config,
    bool tlsEnabled,
    bool tlsBridgingInsecure,
    const char* clientCertChainFile,
    const char* brokerCertChainFile,
    const char* brokerKeyFile,
    const char* brokerCertFile,
    const char* ciphers,
    struct cert_hashes* brokerCertsUtHash)
{
    s_tlsEnabled = tlsEnabled;
    s_tlsBridgingInsecure = tlsBridgingInsecure;
    s_clientCertChainFile = clientCertChainFile;
    s_brokerCertChainFile = brokerCertChainFile;
    s_brokerKeyFile = brokerKeyFile;
    s_brokerCertFile = brokerCertFile;
    s_ciphers = ciphers;

    if(tlsEnabled){
        s_brokerCerts = brokerCertsUtHash;
        if(s_brokerCerts){
            if(IS_INFO_ENABLED){
                struct cert_hashes *current, *tmp;
                current = NULL; tmp = NULL;
                HASH_ITER(hh, s_brokerCerts, current, tmp){
                    _mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "Set broker cert sha1: '%s'", current->cert_sha1);
                }
            }
        }
        
        for(int i = 0; i < config->listener_count; i++){
            if(config->listeners[i].cafile) _mosquitto_free(config->listeners[i].cafile);
            config->listeners[i].cafile = _mosquitto_strdup(clientCertChainFile);
            if(config->listeners[i].certfile) _mosquitto_free(config->listeners[i].certfile);
            config->listeners[i].certfile = _mosquitto_strdup(brokerCertFile);
            if(config->listeners[i].keyfile) _mosquitto_free(config->listeners[i].keyfile);
            config->listeners[i].keyfile = _mosquitto_strdup(brokerKeyFile);
            if(strlen(ciphers) > 0){
                if(config->listeners[i].ciphers) _mosquitto_free(config->listeners[i].ciphers);
                config->listeners[i].ciphers = _mosquitto_strdup(ciphers);
            }

            // Require certificates always
            config->listeners[i].require_certificate = true;
        }
    }

    return MOSQ_ERR_SUCCESS;
}

bool mqtt3_config_is_broker_cert(const char* certSha1)
{
    if(!s_brokerCerts) return false;

    struct cert_hashes* s = NULL;
    HASH_FIND_STR(s_brokerCerts, certSha1, s);
    return s != NULL;
}

/**
 * Adds the specified address to the specified bridge
 *
 * @param    cur_bridge The bridge to add the address to
 * @param    address The address to add to the bridge
 * @return    Whether the operation was successful
 */
static int _add_bridge_address(struct _mqtt3_bridge *cur_bridge, struct bridge_address* address)
{
    cur_bridge->address_count++;
    cur_bridge->addresses = (struct bridge_address*)_mosquitto_realloc(
        cur_bridge->addresses, sizeof(struct bridge_address) * cur_bridge->address_count);
    if(!cur_bridge->addresses){
        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory.");
        return MOSQ_ERR_NOMEM;
    }

    cur_bridge->addresses[ cur_bridge->address_count - 1 ].address = _mosquitto_strdup(address->address);
    if(!cur_bridge->addresses[ cur_bridge->address_count - 1 ].address){
        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory.");
        return MOSQ_ERR_NOMEM;
    }
    cur_bridge->addresses[ cur_bridge->address_count - 1 ].port = address->port;

    return MOSQ_ERR_SUCCESS;
}

int mqtt3_config_add_bridge(
    struct mqtt3_config *config, 
    const char* name,
    struct bridge_address* addresses, 
    int addressCount,
    struct _mqtt3_bridge** bridge)
{
    struct _mqtt3_bridge *cur_bridge = NULL;
    struct _mqtt3_bridge_topic *cur_topic;

    config->bridge_count++;
    config->bridges = (struct _mqtt3_bridge*)_mosquitto_realloc(
        config->bridges, config->bridge_count * sizeof(struct _mqtt3_bridge));
    if(!config->bridges){
        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory.");
        return MOSQ_ERR_NOMEM;
    }

    cur_bridge = &(config->bridges[ config->bridge_count - 1 ]);
    memset(cur_bridge, 0, sizeof(struct _mqtt3_bridge));

    // Bridge name
    cur_bridge->name = _mosquitto_strdup(name);
    if(!cur_bridge->name){
        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory.");
        return MOSQ_ERR_NOMEM;
    }

    // Default bridge values
    cur_bridge->keepalive = 60;
    cur_bridge->start_type = bst_automatic;
    cur_bridge->idle_timeout = 60;
    cur_bridge->restart_timeout = 5;
    cur_bridge->threshold = 10;
    cur_bridge->try_private = true;    
    cur_bridge->clean_session = true;    

    cur_bridge->cur_primary_address = 0;
    cur_bridge->primary_address_count = 1;

    if(s_tlsEnabled){
        cur_bridge->tls_cafile = (char*)s_brokerCertChainFile;
        cur_bridge->tls_certfile = (char*)s_brokerCertFile;
        cur_bridge->tls_keyfile = (char*)s_brokerKeyFile;
        cur_bridge->tls_insecure = s_tlsBridgingInsecure;
    }

    // Addresses
    for(int i = 0; i < addressCount; i++){
        _add_bridge_address(cur_bridge, &(addresses[i]));
    }

    //
    // Topics
    //

    cur_bridge->topic_count++;
    cur_bridge->topics = (struct _mqtt3_bridge_topic*)_mosquitto_realloc(
        cur_bridge->topics, 
        sizeof(struct _mqtt3_bridge_topic) * cur_bridge->topic_count);
    if(!cur_bridge->topics){
        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory");
        return MOSQ_ERR_NOMEM;
    }

    cur_topic = &(cur_bridge->topics[ cur_bridge->topic_count - 1 ]);    
    cur_topic->topic = _mosquitto_strdup("#"); // All topics
    if(!cur_topic->topic){
        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory");
        return MOSQ_ERR_NOMEM;
    }

    cur_topic->direction = bd_both;
    cur_topic->qos = 0;
    cur_topic->local_prefix = NULL;
    cur_topic->remote_prefix = NULL;

    cur_topic->local_topic = _mosquitto_strdup(cur_topic->topic);
    if(!cur_topic->local_topic){
        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory");
        return MOSQ_ERR_NOMEM;
    }

    cur_topic->remote_topic = _mosquitto_strdup(cur_topic->topic);
    if(!cur_topic->remote_topic){
        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory");
        return MOSQ_ERR_NOMEM;
    }

    // Return the newly added bridge
    *bridge = cur_bridge;

    return MOSQ_ERR_SUCCESS;
}

int mqtt3_config_clear_bridges(struct mqtt3_config *config)
{
    _bridge_cleanup(config);
    return MOSQ_ERR_SUCCESS;
}

// DXL End

