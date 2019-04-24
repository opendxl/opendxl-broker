/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "MqttCoreInterface.h"
#include "MqttWorkQueue.h"
#include "include/brokerlib.h"
#include "cert_hashes.h"
#include "dxl.h"
#include "logging_mosq.h"
#include "memory_mosq.h"
#include "CheckConnectionRunner.h"
#include <iostream>
#include <stdexcept>
#include <inttypes.h>
#include <vector>
#include <memory>

using namespace std;
using namespace dxl::broker::core;

/** 
 * The single instance of the Mosquitto interface (used to communicate with the broker library). 
 */
static MqttCoreInterface s_dxlInterface;

/** Whether the broker library has been initialized */
static bool s_libInitialized = false;

/** The numeric ID for the Client GUID (In certificates) */
int NID_dxlClientGuid = 0;

/** The numeric ID for the Tenant GUID (In certificates) */
int NID_dxlTenantGuid = 0;

/** {@inheritDoc} */
bool dxl_main(
    int argc, char *argv[], 
    bool* tlsEnabled, bool* tlsBridgingInsecure, bool* fipsEnabled,
    const char** clientCertChainFile, const char** brokerCertChainFile,
    const char** brokerKeyFile, const char** brokerCertFile, const char** ciphers,
    uint64_t* maxPacketBufferSize, int* listenPort, int* mosquittoLogType,
    int* messageSizeLimit, char** user,
    struct cert_hashes** brokerCertsUtHash,
    bool *webSocketsEnabled, int* webSocketsListenPort )
{
    return dxl::broker::brokerlib_main(
        argc, argv, tlsEnabled, tlsBridgingInsecure, fipsEnabled,
        clientCertChainFile, brokerCertChainFile,
        brokerKeyFile, brokerCertFile, ciphers, maxPacketBufferSize, listenPort, 
        mosquittoLogType, messageSizeLimit, user,
        brokerCertsUtHash,
        webSocketsEnabled, webSocketsListenPort );
}

/** {@inheritDoc} */
bool dxl_brokerlib_init()
{    
    bool success = true;
    if( !s_libInitialized )
    {
        success = dxl::broker::init( &s_dxlInterface );
        if( success ) s_libInitialized = true;

        if( s_dxlInterface.isCertIdentityValidationEnabled() ||
            s_dxlInterface.isMultiTenantModeEnabled() )
        {
            NID_dxlClientGuid = s_dxlInterface.getClientGuidNid();
            NID_dxlTenantGuid = s_dxlInterface.getTenantGuidNid();
        }
    }

    return success;
}

/** {@inheritDoc} */
bool dxl_is_brokerlib_initialized()
{
    return s_libInitialized;
}

/** {@inheritDoc} */
void dxl_brokerlib_cleanup()
{
    dxl::broker::cleanup();
}

/** {@inheritDoc} */
bool dxl_is_bridge_connect_allowed( struct mosquitto* context )
{
    if( !context->id || !context->is_bridge )
    {
        // This should never happen
        return true;
    }

    try
    {
        return s_dxlInterface.isBridgeConnectAllowed( context );
    }
    catch( const exception& ex )
    {
        _mosquitto_log_printf( NULL, MOSQ_LOG_ERR, "Error checking if bridge connect allowed: %s", ex.what() );
    }
    catch( ... )
    {
        _mosquitto_log_printf( NULL, MOSQ_LOG_ERR, "Error checking if bridge connect allowed, unknown error" );
    }

    return false;
}


/** {@inheritDoc} */
void dxl_on_bridge_connected( struct mosquitto* context )
{
    if( !context->id || !context->is_bridge )
    {
        return;
    }

    try
    {
        s_dxlInterface.onBridgeConnected( context );
    }
    catch( const exception& ex )
    {
        _mosquitto_log_printf( NULL, MOSQ_LOG_ERR, "Error firing bridge connected: %s", ex.what() );
    }
    catch( ... )
    {
        _mosquitto_log_printf( NULL, MOSQ_LOG_ERR, "Error firing bridge connected, unknown error" );
    }
}

/** {@inheritDoc} */
void dxl_on_bridge_disconnected( struct mosquitto* context )
{
    if( !context->is_bridge || context->state == mosq_cs_new || !context->id )
    {
        return;
    }

    try
    {
        s_dxlInterface.onBridgeDisconnected( context );
    }
    catch( const exception& ex )
    {
        _mosquitto_log_printf( NULL, MOSQ_LOG_ERR, "Error firing bridge disconnected: %s", ex.what() );
    }
    catch( ... )
    {
        _mosquitto_log_printf( NULL, MOSQ_LOG_ERR, "Error firing bridge disconnected, unknown error" );
    }
}

/** {@inheritDoc} */
void dxl_on_client_connected( struct mosquitto* context )
{
    if( !context->id || context->is_bridge )
    {
        return;
    }

    try
    {
        s_dxlInterface.onClientConnected( context );
    }
    catch( const exception& ex )
    {
        _mosquitto_log_printf( NULL, MOSQ_LOG_ERR, "Error firing client connected: %s", ex.what() );
    }
    catch( ... )
    {
        _mosquitto_log_printf( NULL, MOSQ_LOG_ERR, "Error firing client connected, unknown error" );
    }
}

/** {@inheritDoc} */
 void dxl_on_client_disconnected( struct mosquitto* context )
 {
    if( context->is_bridge || context->state == mosq_cs_new || !context->id )
    {
        return;
    }

    try
    {
        s_dxlInterface.onClientDisconnected( context );
    }
    catch( const exception& ex )
    {
        _mosquitto_log_printf( NULL, MOSQ_LOG_ERR, "Error firing client disconnected: %s", ex.what() );
    }
    catch( ... )
    {
        _mosquitto_log_printf( NULL, MOSQ_LOG_ERR, "Error firing client disconnected, unknown error" );
    }
}

/** {@inheritDoc} */
bool dxl_on_publish_message(
    struct mosquitto* sourceContext, const char* topic, uint32_t payloadLen, const void* payload, 
    struct cert_hashes *certHashes )
{
    try
    {
        return s_dxlInterface.onPublishMessage( sourceContext, topic, payloadLen, payload, certHashes );
    }
    catch( const exception& ex )
    {
        _mosquitto_log_printf( NULL, MOSQ_LOG_ERR, "Error firing on publish message: %s, error=%s", topic, ex.what() );
    }
    catch( ... )
    {
        _mosquitto_log_printf( NULL, MOSQ_LOG_ERR, "Error firing on publish message: %s, unknown error", topic );
    }

    // Error occurred, don't allow publish
    return false;
}

/** {@inheritDoc} */
void dxl_on_store_message(
    struct mosquitto* sourceContext, struct mosquitto_msg_store *message,
    uint32_t* outPayloadLen, void** outPayload, struct cert_hashes *certHashes,
    const char* certChain )
{
    try
    {    
        s_dxlInterface.onStoreMessage(
            sourceContext, message, outPayloadLen, outPayload, certHashes, certChain );
    }
    catch( const exception& ex )
    {
        _mosquitto_log_printf( NULL, MOSQ_LOG_ERR, "Error firing on store message: %s, error=%s", 
            message->msg.topic, ex.what() );
    }
    catch( ... )
    {
        _mosquitto_log_printf( NULL, MOSQ_LOG_ERR, "Error firing on store message: %s, unknown error", 
            message->msg.topic );
    }
}

/** {@inheritDoc} */
bool dxl_on_pre_insert_message_packet_queue_exceeded(
    struct mosquitto* destContext, struct mosquitto_msg_store *message )
{
    try
    {        
        return s_dxlInterface.onPreInsertPacketQueueExceeded( destContext, message );
    }
    catch( const exception& ex )
    {
        _mosquitto_log_printf( NULL, MOSQ_LOG_ERR, 
            "Error firing on pre insert packet queue exceeded message: %s, error=%s", 
            message->msg.topic, ex.what() );
    }
    catch( ... )
    {
        _mosquitto_log_printf( NULL, MOSQ_LOG_ERR, 
            "Error firing on pre insert packet queue exceeded message: %s, unknown error", 
            message->msg.topic );
    }

    // Error occurred, prevent insert
    return true;
}

/** {@inheritDoc} */
bool dxl_on_insert_message(
    struct mosquitto* destContext, struct mosquitto_msg_store *message, struct cert_hashes *certHashes,
    bool* isClientMessageEnabled, unsigned char** clientMessage, size_t* clientMessageLen )
{
    try
    {        
        return s_dxlInterface.onInsertMessage(
            destContext, message, certHashes, isClientMessageEnabled, clientMessage, clientMessageLen );
    }
    catch( const exception& ex )
    {
        _mosquitto_log_printf( NULL, MOSQ_LOG_ERR, "Error firing on insert message: %s, error=%s", 
            message->msg.topic, ex.what() );
    }
    catch( ... )
    {
        _mosquitto_log_printf( NULL, MOSQ_LOG_ERR, "Error firing on insert message: %s, unknown error", 
            message->msg.topic );
    }

    // Error occurred, don't allow insert
    return false;
}


/** {@inheritDoc} */
void dxl_on_finalize_message( dbid_t dbId )
{
    try
    {        
        s_dxlInterface.onFinalizeMessage( dbId );
    }
    catch( const exception& ex )
    {
        _mosquitto_log_printf( NULL, MOSQ_LOG_ERR, "Error firing on finalize message: %" PRIu64 ", error=%s",
            dbId, ex.what() );
    }
    catch( ... )
    {
        _mosquitto_log_printf( NULL, MOSQ_LOG_ERR, "Error firing on finalize message: %" PRIu64 ", unknown error",
            dbId );
    }
}

/** {@inheritDoc} */
void dxl_on_maintenance( time_t time )
{
    try
    {        
        s_dxlInterface.onCoreMaintenance( time );
    }
    catch( const exception& ex )
    {
        _mosquitto_log_printf( 
            NULL, MOSQ_LOG_ERR, "Error during on Mosquitto maintenance, error=%s", ex.what() );
    }
    catch( ... )
    {
        _mosquitto_log_printf( 
            NULL, MOSQ_LOG_ERR, "Error during on Mosquitto maintenance, unknown error" );
    }
}

/** {@inheritDoc} */
void dxl_run_work_queue()
{
    // Run the queue
    MqttWorkQueue::getInstance().runQueue();
}

/** {@inheritDoc} */ 
void dxl_on_topic_added_to_broker( const char *topic )
{
    try
    {        
        s_dxlInterface.onTopicAddedToBroker( topic );
    }
    catch( const exception& ex )
    {
        _mosquitto_log_printf( 
            NULL, MOSQ_LOG_ERR, "Error during on topic added to broker, error=%s", ex.what() );
    }
    catch( ... )
    {
        _mosquitto_log_printf( 
            NULL, MOSQ_LOG_ERR, "Error during on topic added to broker, unknown error" );
    }
}

/** {@inheritDoc} */ 
void dxl_on_topic_removed_from_broker( const char *topic )
{
    try
    {        
        s_dxlInterface.onTopicRemovedFromBroker( topic );
    }
    catch( const exception& ex )
    {
        _mosquitto_log_printf( 
            NULL, MOSQ_LOG_ERR, "Error during on topic removed from broker, error=%s", ex.what() );
    }
    catch( ... )
    {
        _mosquitto_log_printf( 
            NULL, MOSQ_LOG_ERR, "Error during on topic removed from broker, unknown error" );
    }
}

/**  {@inheritDoc} */
bool dxl_is_client_connected( const char* clientId )
{
    return (
        mqtt3_db_lookup_client( clientId, _mosquitto_get_db() ) ||
        mqtt3_db_get_canonical_client_id_count( clientId ) > 0 );

}

/** {@inheritDoc} */
void dxl_disconnect_client( const char* clientId )
{
    mqtt3_context_disconnect_byid( _mosquitto_get_db(), clientId );
}

/** {@inheritDoc} */
bool dxl_update_sent_byte_count( struct mosquitto* context, uint32_t byteCount)
{
    return s_dxlInterface.updateTenantSentByteCount( context, byteCount );
}

/** {@inheritDoc} */
bool dxl_is_tenant_connection_allowed( struct mosquitto* context )
{
    return s_dxlInterface.isTenantConnectionAllowed( context );
}

/** {@inheritDoc} */
void dxl_log( int priority, const char* message )
{
    LogLevel level = LogLevel::debug;

    switch( priority )
    {
        case MOSQ_LOG_INFO:
        case MOSQ_LOG_NOTICE:
            level = LogLevel::info;
            break;
        case MOSQ_LOG_ERR:
            level = LogLevel::error;
            break;
        case MOSQ_LOG_WARNING:
            level = LogLevel::warn;
            break;
    }

    s_dxlInterface.log( level, message );
}

static std::vector<std::shared_ptr<CheckConnectionRunner>> s_checkconnrunners;

/** {@inheritDoc} */
size_t dxl_check_connection_create_id()
{
    std::shared_ptr<CheckConnectionRunner> runner( new CheckConnectionRunner() );
    s_checkconnrunners.push_back( runner );
    return s_checkconnrunners.size() - 1;
}

/** {@inheritDoc} */
void dxl_check_connection_push( size_t connection_id, const char* host, uint16_t port )
{
    if( connection_id < s_checkconnrunners.size() ) 
    {
        s_checkconnrunners[connection_id]->add( host, port );
    }    
}

/** {@inheritDoc} */
int dxl_check_connection_get_status( size_t connection_id )
{
    if( connection_id >= s_checkconnrunners.size() ) 
    {
        return dxl_check_connect_invalid;
    }
    return s_checkconnrunners[connection_id]->getStatus();
}

/** {@inheritDoc} */
struct dxl_check_connection_result* dxl_check_connection_pop( size_t connection_id )
{
    if( IS_DEBUG_ENABLED )
        _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG, "dxl_check_connection_pop");
    if( ( connection_id >= s_checkconnrunners.size() ) ||
        ( s_checkconnrunners[connection_id]->getStatus() != CheckConnection::WorkStatus::results_available ) ) 
    {
        if( IS_DEBUG_ENABLED )
            _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG, "dxl_check_connection_pop - false(no results)");
        return NULL;
    }

    CheckConnection::result res;
    if( !s_checkconnrunners[connection_id]->getResult( res ) ) 
    {
        if( IS_DEBUG_ENABLED )
            _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG, "dxl_check_connection_pop - false (results but no get)" );
        return NULL;
    }

    struct dxl_check_connection_result* result = (dxl_check_connection_result*)
        _mosquitto_calloc( 1, sizeof( dxl_check_connection_result ) );
    if( IS_DEBUG_ENABLED )
        _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG, "dxl_check_connection_pop - host = %s", res.host.c_str());
    result->host = _mosquitto_strdup( res.host.c_str() );
    result->port = res.port;
    result->result = res.res;
    result->extended_result = res.EAIresult;
    if( IS_DEBUG_ENABLED )
        _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG, "dxl_check_connection_pop - end" );
    return result;
}

/** {@inheritDoc} */
void dxl_check_connection_free_result(struct dxl_check_connection_result* result)
{
    if( IS_DEBUG_ENABLED )
        _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG, "dxl_check_connection_free_result" );
    _mosquitto_free( result->host );
    _mosquitto_free( result );
    if( IS_DEBUG_ENABLED )
        _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG, "dxl_check_connection_free_result - end" );
}

/** {@inheritDoc} */
bool dxl_is_multi_tenant_mode_enabled()
{
    return s_dxlInterface.isMultiTenantModeEnabled();
}

/** {@inheritDoc} */
bool dxl_is_test_mode_enable()
{
    return s_dxlInterface.isTestModeEnabled();
}

/** {@inheritDoc} */
const char* dxl_get_broker_tenant_guid()
{
    return s_dxlInterface.getBrokerTenantGuid();
}

/** {@inheritDoc} */
bool dxl_is_cert_revoked( const char* cert )
{
    return s_dxlInterface.isCertRevoked( cert );
}

