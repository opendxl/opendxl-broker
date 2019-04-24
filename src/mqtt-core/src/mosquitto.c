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

/* For initgroups() */
#  include <unistd.h>
#  include <grp.h>

#include <pwd.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#ifdef WITH_WRAP
#include <tcpd.h>
#endif

#include <mosquitto_broker.h>
#include <memory_mosq.h>
#include "util_mosq.h"

#include "dxl.h"

struct mosquitto_db int_db;

int run;
#ifdef WITH_WRAP
#include <syslog.h>
int allow_severity = LOG_INFO;
int deny_severity = LOG_INFO;
#endif

// DXL Begin
static int* mosq_listensock = NULL;
static int mosq_listensock_count = 0;
// DXL End

int drop_privileges(struct mqtt3_config *config);
void handle_sigint(int signal);
void handle_sigusr1(int signal);
void handle_sigusr2(int signal);

struct mosquitto_db *_mosquitto_get_db(void)
{
    return &int_db;
}

/* mosquitto shouldn't run as root.
 * This function will attempt to change to an unprivileged user and group if
 * running as root. The user is given in config->user.
 * Returns 1 on failure (unknown user, setuid/setgid failure)
 * Returns 0 on success.
 * Note that setting config->user to "root" does not produce an error, but it
 * strongly discouraged.
 */
int drop_privileges(struct mqtt3_config *config)
{
#if !defined(__CYGWIN__) && !defined(WIN32)
    struct passwd *pwd;
    char err[256];

    if(geteuid() == 0){
        if(config->user && strcmp(config->user, "root")){
            pwd = getpwnam(config->user);
            if(!pwd){
                _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Invalid user '%s'.", config->user);
                return 1;
            }
            if(initgroups(config->user, pwd->pw_gid) == -1){
                // g++ compiler automatically defines _GNU_SOURCE
                _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error setting groups whilst dropping privileges: %s.", strerror_r(errno, err, 256));
                return 1;
            }
            if(setgid(pwd->pw_gid) == -1){
                // g++ compiler automatically defines _GNU_SOURCE
                _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error setting gid whilst dropping privileges: %s.", strerror_r(errno, err, 256));
                return 1;
            }
            if(setuid(pwd->pw_uid) == -1){
                // g++ compiler automatically defines _GNU_SOURCE
                _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error setting uid whilst dropping privileges: %s.", strerror_r(errno, err, 256));
                return 1;
            }
        }
        if(geteuid() == 0 || getegid() == 0){
            if(IS_WARNING_ENABLED)
                _mosquitto_log_printf(NULL, MOSQ_LOG_WARNING, "Warning: Mosquitto should not be run as root/administrator.");
        }
    }
#endif
    return MOSQ_ERR_SUCCESS;
}

#ifdef SIGHUP
/* Signal handler for SIGHUP - flag a config reload. */
void handle_sighup(int UNUSED(signal))
{
}
#endif

/* Signal handler for SIGINT and SIGTERM - just stop gracefully. */
void handle_sigint(int UNUSED(signal))
{
    run = 0;
}

/* Signal handler for SIGUSR1 - backup the db. */
void handle_sigusr1(int UNUSED(signal))
{
}

/* Signal handler for SIGUSR2 - vacuum the db. */
void handle_sigusr2(int UNUSED(signal))
{
}

// DXL Begin
/**
 * Simple class that cleans up the broker library when Mosquitto main exits.
 */
class DxlBrokerlibCleanup
{
public:
    virtual ~DxlBrokerlibCleanup() { dxl_brokerlib_cleanup(); }
};

int mosquitto_get_listensock_count()
{
    return mosq_listensock_count;
}

int* mosquitto_get_listensocks()
{
    return mosq_listensock;
}

void mosquitto_set_listensocks(int* socks, int sock_count)
{
    if(mosq_listensock){
        _mosquitto_free(mosq_listensock);
    }
    mosq_listensock = socks;
    mosq_listensock_count = sock_count;
}
// DXL End

int main(int argc, char *argv[])
{
    int *listensock = NULL;
    int listensock_count = 0;
    int listensock_index = 0;
    struct mqtt3_config config;
    int i, j;
    FILE *pid;
    int listener_max;
    int rc;    
    char err[256];
    struct timeval tv;

#if defined(WIN32) || defined(__CYGWIN__)
    if(argc == 2){
        if(!strcmp(argv[1], "run")){
            service_run();
            return 0;
        }else if(!strcmp(argv[1], "install")){
            service_install();
            return 0;
        }else if(!strcmp(argv[1], "uninstall")){
            service_uninstall();
            return 0;
        }
    }
#endif

    // DXL Begin
    DxlBrokerlibCleanup dxlCleanup;

    if(!mqtt3_sub_init()){
        // Failed to init subscriptions, exit.
        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Subscriptions init failed.");
        return 1;
    }
    // DXL End

    gettimeofday(&tv, NULL);
    srand(tv.tv_sec + tv.tv_usec);

    memset(&int_db, 0, sizeof(struct mosquitto_db));

    _mosquitto_net_init();

    mqtt3_config_init(&config);
    rc = mqtt3_config_parse_args(&config, argc, argv);
    if(rc != MOSQ_ERR_SUCCESS) return rc;
    int_db.config = &config;

    if(config.daemon){
        switch(fork()){
            case 0:
                break;
            case -1:
                // g++ compiler automatically defines _GNU_SOURCE
                _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error in fork: %s", strerror_r(errno, err, 256));
                return 1;
            default:
                return MOSQ_ERR_SUCCESS;
        }
    }

    if(config.daemon && config.pid_file){
        pid = _mosquitto_fopen(config.pid_file, "wt");
        if(pid){
            fprintf(pid, "%d", getpid());
            fclose(pid);
        }else{
            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Unable to write pid file.");
            return 1;
        }
    }

    // DXL Begin
    bool tlsEnabled = false;
    bool tlsBridgingInsecure = true;
    bool fipsEnabled = false;
    const char* clientCertChainFile = NULL;
    const char* brokerCertChainFile = NULL;
    const char* brokerKeyFile = NULL;
    const char* brokerCertFile = NULL; 
    const char* ciphers = NULL;
    int listenPort = 8883;
    uint64_t maxPacketBufferSize;
    int mosquittoLogType = 0;
    int messageSizeLimit = 1048576;
    char* user = NULL;
    struct cert_hashes* brokerCertsUtHash = NULL;
    bool webSocketsEnabled = false;
    int webSocketsPort = 443;
    const char *sslver;

    if(!dxl_main(argc, argv,
        &tlsEnabled,
        &tlsBridgingInsecure,
        &fipsEnabled,
        &clientCertChainFile, 
        &brokerCertChainFile, 
        &brokerKeyFile, 
        &brokerCertFile, 
        &ciphers,
        &maxPacketBufferSize,
        &listenPort,
        &mosquittoLogType,
        &messageSizeLimit,
        &user,
        &brokerCertsUtHash,
        &webSocketsEnabled,
        &webSocketsPort)){

        // Failed to start the broker library, exit.
        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Broker library forced exit (main).");
        return 1;
    }

    // DXL: Configure FIPS mode
    sslver = _mosquitto_ssl_version();
    if(sslver){
        _mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "OpenSSL version %s", sslver);
    }

    if(fipsEnabled){
        if(_mosquitto_fips_enable(1)){
            _mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "FIPS Mode Enabled.");
        }else{
            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "FIPS Mode FAILED to enable.");
        }
    }else{
        _mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "FIPS Mode is disabled.");
    }

    // DXL: Set the user
    if(config.user){
        _mosquitto_free(config.user);
        config.user = NULL;
    }
    config.user = (strlen(user) > 0) ? user : NULL;

    // Update Websockets settings
    config.ws_enabled = webSocketsEnabled;
    config.ws_port = webSocketsPort;

    // Update TLS settings
    mqtt3_config_update_tls(
        &config, tlsEnabled, tlsBridgingInsecure,
        clientCertChainFile, brokerCertChainFile,
        brokerKeyFile, brokerCertFile, ciphers,
        brokerCertsUtHash);


    // Set the maximum packet buffer size
    mqtt3_dxl_set_max_packet_buffer_size(maxPacketBufferSize);
    // DXL End

    rc = mqtt3_db_open(&config, &int_db);
    if(rc != MOSQ_ERR_SUCCESS){
        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Couldn't open database.");
        return rc;
    }

    /* Initialise logging only after initialising the database in case we're
     * logging to topics */
    mqtt3_log_init(config.log_type, config.log_dest);

    if(IS_INFO_ENABLED)
        _mosquitto_log_printf(NULL, MOSQ_LOG_INFO,"OpenDXL Broker %s.%s.%s (build %s) (core %s) (%s)",
            SOMAJVER, SOMINVER, SOSUBMINVER, SOBLDNUM, VERSION, TIMESTAMP);

    if(config.config_file){
        //_mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "Config loaded from %s.", config.config_file);
        if(IS_INFO_ENABLED)
            _mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "Core config loaded from %s.", config.config_file); // DXL
    }else{
        //_mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "Using default config.");
        if(IS_INFO_ENABLED)
            _mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "Using default core config."); // DXL
    }

    // DXL: Set mosquitto log types
    config.log_type = mosquittoLogType;
    mqtt3_log_init(config.log_type, config.log_dest);
    // DXL End

    // DXL: Set message size limit
    config.message_size_limit = messageSizeLimit;
    
    // DXL: Set the default listener port
    if(IS_INFO_ENABLED)
        _mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "Setting listener port: %d", listenPort);
    config.default_listener.port = listenPort;
    if(config.listener_count > 0){
        config.listeners[0].port = listenPort;
    }
    // DXL End

    listener_max = -1;
    listensock_index = 0;
    for(i=0; i<config.listener_count; i++){
        if(mqtt3_socket_listen(&config.listeners[i])){
            _mosquitto_free(int_db.contexts);
            mqtt3_db_close(&int_db);
            if(config.pid_file){
                remove(config.pid_file);
            }
            return 1;
        }
        listensock_count += config.listeners[i].sock_count;
        listensock = (int *)_mosquitto_realloc(listensock, sizeof(int)*listensock_count);
        if(!listensock){
            _mosquitto_free(int_db.contexts);
            mqtt3_db_close(&int_db);
            if(config.pid_file){
                remove(config.pid_file);
            }
            return 1;
        }
        for(j=0; j<config.listeners[i].sock_count; j++){
            if(config.listeners[i].socks[j] == INVALID_SOCKET){
                _mosquitto_free(int_db.contexts);
                mqtt3_db_close(&int_db);
                if(config.pid_file){
                    remove(config.pid_file);
                }
                return 1;
            }
            listensock[listensock_index] = config.listeners[i].socks[j];
            if(listensock[listensock_index] > listener_max){
                listener_max = listensock[listensock_index];
            }
            listensock_index++;
        }
    }

    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);
#ifdef SIGHUP
    signal(SIGHUP, handle_sighup);
#endif
    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);
    signal(SIGPIPE, SIG_IGN);

    // Initialize epoll
    mosquitto_epoll_init();

    // DXL Begin
    // Initialize the broker library
    if(!dxl_brokerlib_init()){
        // Failed to init the broker library, exit.
        _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Broker library forced exit (init).");
        return 1;
    }
    // DXL End

    // Initialize WebSockets
    if(webSocketsEnabled){
        mosquitto_ws_init(&config);
    }

    // Drop root privileges, now that the socket is open
    rc = drop_privileges(&config);
    if(rc != MOSQ_ERR_SUCCESS) return rc;
    
    
    for(i=0; i<config.bridge_count; i++){
        if(mqtt3_bridge_new(&int_db, &(config.bridges[i]))){
            if(IS_WARNING_ENABLED)
                _mosquitto_log_printf(NULL, MOSQ_LOG_WARNING, "Warning: Unable to connect to bridge %s.", 
                    config.bridges[i].name);
        }
    }

    mosquitto_set_listensocks(listensock, listensock_count); // DXL

    run = 1;
    rc = mosquitto_main_loop(&int_db);

    // DXL Begin
    listensock = mosquitto_get_listensocks();
    listensock_count = mosquitto_get_listensock_count();
    // DXL End

    if(IS_INFO_ENABLED)
        _mosquitto_log_printf(NULL, MOSQ_LOG_INFO, "DXL Broker terminating");
    mqtt3_log_close();

    // DXL Begin
    
    if(webSocketsEnabled){
        mosquitto_ws_destroy();
    }

    mosquitto_epoll_destroy();

    for(i=0; i<int_db.context_count; i++){
        if(int_db.contexts[i]){
            int_db.contexts[i]->clean_subs = true;
        }
    }
    mqtt3_subs_clean_session(&int_db, &int_db.subs);
    for(i=0; i<int_db.context_count; i++){
        if(int_db.contexts[i]){
            mqtt3_context_cleanup(&int_db, int_db.contexts[i], true, false);
        }
    }
    // DXL End

    _mosquitto_free(int_db.contexts);
    int_db.contexts = NULL;
    mqtt3_db_close(&int_db);

    if(listensock){
        for(i=0; i<listensock_count; i++){
            if(listensock[i] != INVALID_SOCKET){
                close(listensock[i]);
            }
        }
        _mosquitto_free(listensock);
    }

    if(config.pid_file){
        remove(config.pid_file);
    }

    _mosquitto_net_cleanup();
    mqtt3_config_cleanup(int_db.config);
    mqtt3_sub_cleanup(); // DXL
    
    return rc;
}

