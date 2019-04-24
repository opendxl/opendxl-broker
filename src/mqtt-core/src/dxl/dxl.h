/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef DXL_H_
#define DXL_H_

/**
 * This header exposes C-style methods that will be invoked by Mosquitto for communication with
 * the DXL broker library.
 */

#include "cert_hashes.h"
#include "mosquitto.h"
#include "mosquitto_internal.h"
#include "mosquitto_broker.h"
#include <ctime>

/** The numeric ID for the Client GUID (In certificates) */
extern int NID_dxlClientGuid;

/** The numeric ID for the Tenant GUID (In certificates) */
extern int NID_dxlTenantGuid;

/**
 * Invoked immediatedly from the Mosquitto main method
 *
 * @param   argc The argument count
 * @param   argv The arguments
 * @param   tlsEnabled Whether TLS is enabled (out)
 * @param   tlsBridgingInsecure Whether TLS bridging is insecure
 * @param   fipsEnabled Whether FIPS mode is enabled (out)
 * @param   clientCertChainFile The client certificate chain file (out)
 * @param   brokerCertChainFile The broker certificate chain file (out)
 * @param   brokerKeyFile The broker key file (out)
 * @param   brokerCertFile The broker certificate file (out)
 * @param   ciphers The ciphers to restrict to (out)
 * @param   maxPacketBufferSize The maximum packet buffer size (out)
 * @param   listenPort The broker listener port (out)
 * @param   mosquittoLogType The mosquitto log types (out)
 * @param   messageSizeLimit The mosquitto message size limit (out)
 * @param   user The user to run the broker as (out)
 * @param   brokerCertsUtHash List of broker certificate hashes (SHA-1) (out)
 * @param   webSocketsEnabled Whether WebSockets is enabled (out)
 * @param   webSocketsListenPort The broker WebSockets listen port (out)
 * @return  Whether Mosquitto should continue starting
 */
bool dxl_main(
    int argc, char *argv[], 
    bool* tlsEnabled, bool* tlsBridgingInsecure, bool* fipsEnabled,
    const char** clientCertChainFile, const char** brokerCertChainFile,
    const char** brokerKeyFile, const char** brokerCertFile, const char** ciphers,
    uint64_t* maxPacketBufferSize, int* listenPort, int* mosquittoLogType,
    int* messageSizeLimit, char** user,
    struct cert_hashes** brokerCertsUtHash,
    bool *webSocketsEnabled, int* webSocketsListenPort );

/**
 * Invoked when the broker library can be initialized.
 *
 * @return  Whether Mosquitto should continue starting
 */
bool dxl_brokerlib_init();

/**
 * Whether the broker library has been initialized
 *
 * @return  Whether the broker library has been initialized
 */
bool dxl_is_brokerlib_initialized();

/**
 * Invoked when the broker library can be cleaned up.
 */
void dxl_brokerlib_cleanup();

/**
 * Checks whether the specified context (representing a bridge) should be allowed to 
 * connect to this broker.
 *
 * @param   context The bridge context
 * @return  Whether the specified context (representing a bridge) should be allowed to
 *          connect to this broker.
 */
bool dxl_is_bridge_connect_allowed( struct mosquitto* context );

/**
 * Invoked when a bridge is connected to the broker
 *
 * @param   context The context associated with the bridged connection
 */
void dxl_on_bridge_connected( struct mosquitto* context );

/**
 * Invoked when a bridge is disconnected from the broker
 *
 * @param   context The context associated with the bridged connection
 */
void dxl_on_bridge_disconnected( struct mosquitto* context );

/**
 * Invoked when a client is connected to the broker
 *
 * @param   context The context associated with the connection
 */
void dxl_on_client_connected( struct mosquitto* context );

/**
 * Invoked when a client is disconnected from the broker
 *
 * @param   context The context associated with the connection
 */
void dxl_on_client_disconnected( struct mosquitto* context );

/**
 * Invoked when a message is about to be published
 *
 * @param   sourceContext The source context
 * @param   topic The message topic
 * @param   payloadLen The length of the payload
 * @param   payload The message payload
 * @param   certHashes The certificate hashes associated with the source context
 * @return  Whether the publish should be allowed
 */
bool dxl_on_publish_message(
    struct mosquitto* sourceContext, const char* topic, uint32_t payloadLen, const void* payload, 
    struct cert_hashes *certHashes );

/**
 * Invoked when a message is stored in the Mosquitto database for publishing
 *
 * @param   sourceContext The source context (can be null)
 * @param   message The message that was stored
 * @param   outPayloadLen The output payload length (0 if not rewritten)
 * @param   outPayload The output payload (NULL if not rewritten)
 * @param   certHashes The certificate hashes associated with the source context
 * @param   certChain The certificate chain associated with the source context
 */
void dxl_on_store_message(
    struct mosquitto* sourceContext, struct mosquitto_msg_store *message,
    uint32_t* outPayloadLen, void** outPayload, struct cert_hashes *certHashes,
    const char* certChain );


/**
 * Invoked by when the queue of packets for a context exceeds the maximum
 * value and a message is attempting to be inserted for delivery.
 *
 * @param   destContext The destination context
 * @param   message The message that was stored
 * @return  True if the message should be rejected. False if the insert should be allowed
 *          even though the maximum queue size is exceeded.
 */
bool dxl_on_pre_insert_message_packet_queue_exceeded(
    struct mosquitto* destContext, struct mosquitto_msg_store *message );

/**
 * Invoked when a message is stored in the Mosquitto database for publishing
 *
 * @param   destContext The destination context
 * @param   message The message that was stored
 * @param   certHashes The certificate hashes associated with the destination context
 * @param   isClientMessageEnabled Whether to use the client message (or bridge) (out)
 * @param   clientMessage A message that should be sent to clients versus bridges (if applicable) (out)
 * @param   clientMessageLen The length of the message that should be sent to clients versus bridges (out)
 *          (if applicable)
 * @return  Whether to allow the insert to occur
 */
bool dxl_on_insert_message( struct mosquitto* destContext, struct mosquitto_msg_store *message,
    struct cert_hashes *certHashes, bool* isClientMessageEnabled, unsigned char** clientMessage, size_t* clientMessageLen );

/**
 * Invoked when a message is about to be finalized (all operations completed)
 *
 * @param   dbId The database identifier
 */
void dxl_on_finalize_message( dbid_t dbId );

/**
 * Invoked when a message is inserted for a destination
 *
 * @param   destContext The destination context
 * @param   topic The message topic
 * @param   payloadLen The length of the payload
 * @param   payload The message payload
 * @param   certHashes The certificate hashes associated with the destination context
 * @return  Whether the insert should be allowed
 */
bool dxl_on_insert_message(
    struct mosquitto* destContext, const char* topic, uint32_t payloadLen, const void* payload,
    struct cert_hashes *certHashes );

/** 
 * Invoked when Mosquitto runs its maintenance task
 *
 * @param   time The time of the maintenance task (in seconds)
 */
void dxl_on_maintenance( time_t time );

/** 
 * Checks to see if the specified client is connected
 *
 * @param   clientId The client identifier
 * @return  Whether the specified client is connected
 */
bool dxl_is_client_connected( const char* clientId );

/** 
 * Disconnects the specified client
 *
 * @param   clientId The client identifier to disconnect
 */
void dxl_disconnect_client( const char* clientId );

/**
 * Invoked by Mosquitto when a new topic has been subscribed to on the broker.
 * (the first time it is subscribed to across the connected clients).
 *
 * @param   topic The topic
 */
void dxl_on_topic_added_to_broker( const char *topic );

/**
 * Invoked by Mosquitto when a topic is no longer being subscribed to on the broker.
 * (no subscriptions remain across all of the connected clients).
 *
 * @param   topic The topic
 */
void dxl_on_topic_removed_from_broker( const char *topic );

/**
 * Runs any pending tasks. Tasks are typically queued by the broker library for execution
 * on the main Mosquitto thread (see <code>MqttWorkQueue</code>).
 */
void dxl_run_work_queue();

/**
 * Invoked to log via the DXL broker library
 *
 * @param   priority The Mosquitto log priority
 * @param   message The log message
 */
void dxl_log( int priority, const char* message );

/**
 * Updates the count of bytes sent
 *
 * @param   context The context that is sending the bytes
 * @param   byteCount The count of bytes being sent
 * @return  Whether the context has exceeded its limit
 */
bool dxl_update_sent_byte_count( struct mosquitto* context, uint32_t byteCount);

/**
 * Returns whether a connection for the specified tenant is allowed
 *
 * @param   context The connection context
 * @return  Whether a connection for the specified tenant is allowed
 */
bool dxl_is_tenant_connection_allowed( struct mosquitto* context );

/**
 * Returns whether multi-tenant mode is enabled
 *
 * @return  Whether multi-tenant mode is enabled
 */
bool dxl_is_multi_tenant_mode_enabled();

/**
 * Returns whether the broker is running in test mode
 * 
 * @return  Whether test mode is enabled
 */
bool dxl_is_test_mode_enable();

/**
 * Returns the GUID for the broker tenant
 *
 * @return  The GUID for the broker tenant
 */
const char* dxl_get_broker_tenant_guid();

/**
 * Returns whether the specified certificate has been revoked
 *
 * @return  Whether the specified certificate has been revoked
 */
bool dxl_is_cert_revoked( const char* cert );

// used by dxl_check_connection_get_status
#define dxl_check_connect_invalid -1
#define dxl_check_connect_nothing 0
#define dxl_check_connect_working 1
#define dxl_check_connect_has_result 2

struct dxl_check_connection_result {
    char* host;
    int port;
    int result; // Mosquitto error status
    int extended_result; // error status returned by getaddrinfo if result == MOSQ_ERR_EAI
};

size_t dxl_check_connection_create_id();
void dxl_check_connection_push(size_t connection_id, const char* host, uint16_t port);
int dxl_check_connection_get_status(size_t connection_id);
struct dxl_check_connection_result* dxl_check_connection_pop(size_t connection_id);
void dxl_check_connection_free_result(struct dxl_check_connection_result* result);


#endif /* DXL_H_ */
