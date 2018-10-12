/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "MqttCoreInterface.h"
#include "dxl.h"
#include "MqttWorkQueue.h"
#include "BridgeConfigurationRunner.h"
#include "SendMessageRunner.h"
#include "SetConnectionLimitRunner.h"
#include "SetDefaultBridgeKeepaliveRunner.h"
#include "RestartMqttListenersRunner.h"
#include "RevokeCertsRunner.h"
#include "logging_mosq.h"
#include "DxlFlags.h"
#include <iostream>
#include <stdexcept>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

using namespace std;
using namespace dxl::broker::core;

extern int run;
extern int loop_exit_code;

/** {@inheritDoc} */
uint32_t MqttCoreInterface::getBrokerPort() const
{
    struct mosquitto_db *db = _mosquitto_get_db();
    if( db->config->listener_count != 1 )
    {
        throw runtime_error( "Exactly one listener must be configured." );
    }

    return db->config->listeners[0].port;
}

/** {@inheritDoc} */
std::string MqttCoreInterface::getBrokerHostname() const
{

    char hostname[1024];

    // Get the host name
    if( !gethostname( hostname, sizeof( hostname ) ) )
    {
        struct addrinfo hints, *info, *p;
        memset( &hints, 0, sizeof( hints ) );
        hints.ai_family = AF_UNSPEC; /*either IPV4 or IPV6*/
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_CANONNAME;

        // Get the fully qualified domain name (FQDN)
        if( !getaddrinfo( hostname, NULL, &hints, &info ) ) 
        {
            for( p = info; p != NULL; p = p->ai_next ) 
            {    
                if( p->ai_canonname != NULL )
                {
                    strncpy( hostname, p->ai_canonname, sizeof( hostname ) );
                    hostname[ sizeof( hostname ) - 1 ] = '\0';
                    break;
                }
            }

            freeaddrinfo( info );
        }        

        return hostname;
    }

    throw runtime_error( "Unable to determine hostname" );
}

/** {@inheritDoc} */
bool MqttCoreInterface::isBridgeConnectAllowed( const struct mosquitto* context ) const
{
    bool isChild; 
    string bridgeBrokerId;

    getBridgeBrokerIdFromContext( context, isChild, bridgeBrokerId );
    return CoreInterface::isBridgeConnectAllowed( bridgeBrokerId );
}

/** {@inheritDoc} */
void MqttCoreInterface::onBridgeConnected( const struct mosquitto* context ) const
{
    bool isChild; 
    string bridgeBrokerId;

    getBridgeBrokerIdFromContext( context, isChild, bridgeBrokerId );
    CoreInterface::onBridgeConnected( isChild, bridgeBrokerId );
}
        
/** {@inheritDoc} */
void MqttCoreInterface::onBridgeDisconnected( const struct mosquitto* context ) const
{
    bool isChild; 
    string bridgeBrokerId;    
        
    getBridgeBrokerIdFromContext( context, isChild, bridgeBrokerId );
    CoreInterface::onBridgeDisconnected( isChild, bridgeBrokerId );
}

/** {@inheritDoc} */
void MqttCoreInterface::onClientConnected( const struct mosquitto* context ) const
{
    CoreInterface::onClientConnected( context->id );
    updateTenantConnectionCount( context, 1 );
}

/** {@inheritDoc} */
void MqttCoreInterface::onClientDisconnected( const struct mosquitto* context ) const
{
    CoreInterface::onClientDisconnected( context->id );
    updateTenantConnectionCount( context, -1 );
}

/** {@inheritDoc} */
bool MqttCoreInterface::onPublishMessage(
    const struct mosquitto* sourceContext, const char* topic, uint32_t /*payloadLen*/, const void* /*payload*/,
    struct cert_hashes *certHashes ) const
{
    if( sourceContext->is_bridge )
    {
        bool isChild; 
        string bridgeBrokerId;

        getBridgeBrokerIdFromContext( sourceContext, isChild, bridgeBrokerId );
        return CoreInterface::onPublishMessage( bridgeBrokerId.c_str(), bridgeBrokerId.c_str(),
            true, sourceContext->dxl_flags, topic, certHashes );
    }
    else
    {
        return CoreInterface::onPublishMessage( sourceContext->id, sourceContext->canonical_id,
            false, sourceContext->dxl_flags, topic, certHashes );
    }
}

/** {@inheritDoc} */
void MqttCoreInterface::onStoreMessage(
    struct mosquitto* sourceContext, struct mosquitto_msg_store *message,
    uint32_t* outPayloadSize, void** outPayload, struct cert_hashes *certHashes,
    const char* certChain )
{     
    const char* sourceId = message->source_id;
    const char* canonSourceId = sourceContext ? sourceContext->canonical_id : NULL;
    bool isBridge = false;
    uint8_t dxl_flags = 0;
    const char* sourceTenantId = "";
    bool isMultiTenant = dxl_is_multi_tenant_mode_enabled();

    if( sourceContext == NULL )
    {
        if( strlen( sourceId ) == 0 )
        {
            // Is a message from the local broker
            sourceId = getBrokerGuid();    
            canonSourceId = getBrokerGuid();
            isBridge = true;

            if( isMultiTenant )
            {
                sourceTenantId = getBrokerTenantGuid();
                // Turning on Ops Flag
                dxl_flags |= DXL_FLAG_OPS;
            }
        }
        else
        {
            throw runtime_error( "Message stored without context and non-local source identifier" );
        }
    }            
    else
    {
        // Set parameters with the source context ones.
        dxl_flags = sourceContext->dxl_flags;
        if( isMultiTenant && sourceContext->dxl_tenant_guid )
        {
            sourceTenantId = sourceContext->dxl_tenant_guid;
        }

        if( sourceContext->is_bridge )
        {
            bool isChild; 
            string bridgeBrokerId;

            if( isMultiTenant )
            {
                // Context tenant information is not relevant when source is a bridge.
                // Tenant information will be read from message.
                sourceTenantId = "";
                // Turning off Ops Flag
                dxl_flags &= ~DXL_FLAG_OPS;
            }

            getBridgeBrokerIdFromContext( sourceContext, isChild, bridgeBrokerId );
            CoreInterface::onStoreMessage(
                message->db_id, bridgeBrokerId.c_str(), bridgeBrokerId.c_str(), true, 
                dxl_flags, message->msg.topic,
                message->msg.payloadlen, message->msg.payload,
                outPayloadSize, outPayload, sourceTenantId, NULL, NULL);
            // Return, we called the broker library method for a bridge
            return;
        }
    }

    // Call the broker library method
    CoreInterface::onStoreMessage(
        message->db_id, sourceId, canonSourceId, isBridge, 
        dxl_flags, message->msg.topic,
        message->msg.payloadlen, message->msg.payload,
        outPayloadSize, outPayload,
        sourceTenantId, certHashes, certChain );        
}

/** {@inheritDoc} */
bool MqttCoreInterface::onPreInsertPacketQueueExceeded(
    struct mosquitto* destContext, struct mosquitto_msg_store *message )
{
    if( destContext->is_bridge )
    {
        bool isChild; 
        string bridgeBrokerId;

        getBridgeBrokerIdFromContext( destContext, isChild, bridgeBrokerId );
        return CoreInterface::onPreInsertPacketQueueExceeded(
            bridgeBrokerId.c_str(), true, message->db_id );
    }
    else
    {
        return CoreInterface::onPreInsertPacketQueueExceeded(
            destContext->id, false, message->db_id );
    }
}

/** {@inheritDoc} */
bool MqttCoreInterface::onInsertMessage(
    struct mosquitto* destContext, struct mosquitto_msg_store *message, struct cert_hashes *certHashes,
    bool* isClientMessageEnabled, unsigned char** clientMessage, size_t* clientMessageLen)
{
    // Get the tenant id assuming destContext != NULL.
    const char* targetTenantGuid = (destContext->dxl_tenant_guid ? destContext->dxl_tenant_guid : "");

    if( destContext->is_bridge )
    {
        bool isChild; 
        string bridgeBrokerId;

        getBridgeBrokerIdFromContext( destContext, isChild, bridgeBrokerId );
        return CoreInterface::onInsertMessage(
            bridgeBrokerId.c_str(), bridgeBrokerId.c_str(), true, destContext->dxl_flags, message->db_id,
            targetTenantGuid, certHashes, isClientMessageEnabled, clientMessage, clientMessageLen );
    }
    else
    {
        return CoreInterface::onInsertMessage(
            destContext->id, destContext->canonical_id, false, destContext->dxl_flags, message->db_id,
            targetTenantGuid, certHashes, isClientMessageEnabled, clientMessage, clientMessageLen );
    }
}

/** {@inheritDoc} */
void MqttCoreInterface::onBridgeConfigurationChanged( const CoreBridgeConfiguration& config )
{
    // Add bridge configuration runner (will update Mosquitto to reflect the new bridge configuration).
    MqttWorkQueue::getInstance().add(
        shared_ptr<BridgeConfigurationRunner>( new BridgeConfigurationRunner( config ) ) );
}

/** {@inheritDoc} */
void MqttCoreInterface::sendMessage( const char* topic, uint32_t payloadLen, const void* payload ) const
{
    MqttWorkQueue::getInstance().add(
        shared_ptr<SendMessageRunner>( 
            new SendMessageRunner( topic, payloadLen, payload ) ) );
}

/** {@inheritDoc} */
void MqttCoreInterface::setBridgeKeepalive( uint32_t keepAliveMins ) const
{
    MqttWorkQueue::getInstance().add(
        shared_ptr<SetDefaultBridgeKeepaliveRunner>( 
            new SetDefaultBridgeKeepaliveRunner( keepAliveMins ) ) );
}

/** {@inheritDoc} */
void MqttCoreInterface::setConnectionLimit( uint32_t limit ) const
{
    MqttWorkQueue::getInstance().add(
        shared_ptr<SetConnectionLimitRunner>( 
            new SetConnectionLimitRunner( limit ) ) );
}

/** {@inheritDoc} */
void MqttCoreInterface::getBridgeBrokerIdFromContext(
    const struct mosquitto* context, bool& isChild, string& bridgeBrokerId ) const
{    
    //
    // Bridged identifiers take the following form:
    // {hostname}.{child broker id}:{parent broker id 1}:...
    //

    // Ensure it is a bridged context
    if( !context->is_bridge ) throw runtime_error( "Context is not a bridge" );

    // Get the identifier of this broker
    const string& brokerId = this->getBrokerGuid();

    // Ensure we have a context identifier
    if( !context->id ) throw runtime_error( "No context identifier found" );

    // Find the ".", Mosquitto places this after the host name of bridged connections
    const char* start = strrchr( context->id, '.' );
    if( !start ) throw runtime_error( "Unable to find host delimiter" );
    start++;

    // Find the first ":". The first semicolon separates the identifier of the local broker's 
    // identifier from the candidate list of possible bridged connection identifiers.
    const char* end = strchr( start, ':' );
    if( !end ) throw runtime_error( "Unable to find first broker delimiter" );

    int len = (int)(end - start);

    if( brokerId.compare( 0, len, start, len ) != 0 )
    {        
        // If the current bridge is not the first identifier, we are a parent connecting to the
        // first identifier
        bridgeBrokerId.assign( start, len );
        isChild = false;        
    }
    else
    {
        // We are the child, ensure we have information on the current bridge connection (so 
        // we can determine the current index)
        if( !context->bridge ) throw runtime_error( "Unable to find bridge structure" );
        // Move to the appropriate bridged connection (use the current address index)
        start = end + 1;
        for( int i = 0; i < context->bridge->cur_address; i++ )
        {
            start = strchr( start, ':' );
            if( !start ) throw runtime_error( "Unable to find broker delimiter for parent" );
            start++;
        }
        end = strchr( start, ':' );
        if( end )
        {
            // Capture the bridged identifier, it is not the last one in the candidates
            bridgeBrokerId.assign( start, end - start );            
        }
        else
        {
            // Capture the bridged identifier, it is the last candidate
            bridgeBrokerId.assign( start );            
        }
        
        isChild = true;
    }

    // Ensure we have an identifier
    if( bridgeBrokerId.empty() ) throw runtime_error( "Broker identifier is empty" );
}

/** {@inheritDoc} */
bool MqttCoreInterface::isClientConnected( const std::string& clientGuid ) const
{
    return dxl_is_client_connected( clientGuid.c_str() );
}

/** {@inheritDoc} */
void MqttCoreInterface::disconnectClient( const std::string& clientGuid ) const
{
    return dxl_disconnect_client( clientGuid.c_str() );
}

/** {@inheritDoc} */
void MqttCoreInterface::restartListeners( struct cert_hashes* managedHashes ) const
{
    MqttWorkQueue::getInstance().add(
        shared_ptr<RestartMqttListenersRunner>(
            new RestartMqttListenersRunner( managedHashes ) ) );
}

/** {@inheritDoc} */
void MqttCoreInterface::getBrokerHealth( CoreBrokerHealth& brokerHealth ) const
{
    struct mosquitto_db *db = _mosquitto_get_db();
    int connectedClients = mqtt3_dxl_db_client_count( db );
    // these calls are not thread safe and should only be invoked on Mosquitto thread
    brokerHealth.setConnectedClients( connectedClients );
}

/** {@inheritDoc} */
uint32_t MqttCoreInterface::getSubscriptionCount( const std::string& topic, const char* tenantGuid ) const
{
    int count = 0;
    struct mosquitto_db *db = _mosquitto_get_db();
    mqtt3_sub_count( db, topic.c_str(), &count, tenantGuid );
    return count;
}

/** {@inheritDoc} */
bool MqttCoreInterface::isTenantLimited( const struct mosquitto* context ) const
{
    if( isMultiTenantModeEnabled() &&
        !( context->dxl_flags & DXL_FLAG_OPS ) &&
        !( context->is_bridge ) )
    {
        return context->dxl_tenant_guid != NULL;
    }
    return false;
}


/** {@inheritDoc} */
bool MqttCoreInterface::updateTenantSentByteCount( struct mosquitto* context, uint32_t byteCount ) const
{
    if( isTenantLimited( context ) )
    {
        return CoreInterface::updateTenantSentByteCount(
            context->dxl_tenant_guid, byteCount );
    }        

    return false;
}

/** {@inheritDoc} */
void MqttCoreInterface::updateTenantConnectionCount( const struct mosquitto* context, int adjCount ) const
{
    if( isTenantLimited( context ) )
    {
        CoreInterface::updateTenantConnectionCount(
            context->dxl_tenant_guid, adjCount );
    }        
}

/** {@inheritDoc} */
bool MqttCoreInterface::isTenantConnectionAllowed( const struct mosquitto* context ) const
{
    if( isTenantLimited( context ) )
    {
        return CoreInterface::isTenantConnectionAllowed( context->dxl_tenant_guid );
    }        

    return true;
}


/** {@inheritDoc} */
void MqttCoreInterface::revokeCerts( struct cert_hashes* revokedCerts ) const
{
    MqttWorkQueue::getInstance().add(
        shared_ptr<RevokeCertsRunner>( 
            new RevokeCertsRunner( revokedCerts ) ) );
}

/** {@inheritDoc} */
void MqttCoreInterface::stopBroker( int exitCode ) const
{
    run = 0;
    loop_exit_code = exitCode;
}
