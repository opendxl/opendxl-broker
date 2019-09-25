/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/SimpleLog.h"
#include "include/brokerlib.h"
#include "include/BrokerSettings.h"
#include "brokerregistry/include/brokerregistry.h"
#include "cert/include/BrokerCertsService.h"
#include "cert/include/RevocationService.h"
#include "core/include/CoreInterface.h"
#include "core/include/CoreBridgeConfigurationFactory.h"
#include "core/include/CoreMessageHandlerService.h"
#include "message/include/DxlMessageConstants.h"
#include "message/include/DxlMessageService.h"
#include "message/builder/include/BrokerStateEventBuilder.h"
#include "message/builder/include/FabricChangeEventBuilder.h"
#include "message/payload/include/BrokerStateTopicsEventPayload.h"
#include "message/payload/include/BrokerTopicEventPayload.h"
#include "message/payload/include/ClientRegistryConnectEventPayload.h"
#include "metrics/include/TenantMetricsService.h"

#include <cstring>
#include <iostream>

using namespace std;
using namespace dxl::broker;
using namespace dxl::broker::cert;
using namespace dxl::broker::message;
using namespace dxl::broker::message::builder;
using namespace dxl::broker::message::payload;
using namespace dxl::broker::metrics;
using namespace dxl::broker::core;

/**
 * Class responsible for sending batch topics between brokers when broker state
 * is being sent.
 */
class BrokerStateTopicsSender : public dxl::broker::BrokerState::TopicsCallback
{
public:
    /** {@inheritDoc} */
    void handleBatch( 
        const BrokerState* brokerState, 
        const registry::subscriptions_t& topics, 
        int32_t index, bool isFirst, bool isLast ) const
    {
        uint8_t state = 0;
        if( isFirst ) state |= BrokerStateTopicsEventPayload::STATE_START;
        if( isLast ) state |= BrokerStateTopicsEventPayload::STATE_END;
        BrokerStateTopicsEventPayload pl( state, topics, index );

        DxlMessageService& messageService = DxlMessageService::getInstance();
        shared_ptr<DxlEvent> evt = messageService.createEvent();
        BrokerStateTopicsEventPayload::setMessageHeaderValues( *evt, brokerState );
        evt->setPayload( pl );
        messageService.sendMessage( DxlMessageConstants::CHANNEL_DXL_BROKER_STATE_TOPICS_EVENT, *evt );
    }
};

/** {@inheritDoc} */
CoreInterface::CoreInterface() :
    m_bridgeConfig( CoreBridgeConfigurationFactory::createEmptyConfiguration()  ),
    m_lastLocalBrokerStateSend( 0 )
{
}

/** {@inheritDoc} */
const char* CoreInterface::getBrokerGuid() const
{
    return BrokerSettings::getGuid();
}

/** {@inheritDoc} */
int CoreInterface::getClientGuidNid() const
{
    return BrokerCertsService::getInstance().getClientGuidNid();
}

/** {@inheritDoc} */
int CoreInterface::getTenantGuidNid() const
{
    return BrokerCertsService::getInstance().getTenantGuidNid();
}

/** {@inheritDoc} */
void CoreInterface::sendFabricChangeNotification() const
{
    DxlMessageService& messageService = DxlMessageService::getInstance();

    // Send the fabric change event
    messageService.sendMessage( 
        DxlMessageConstants::CHANNEL_DXL_FABRIC_CHANGE_EVENT,
        FabricChangeEventBuilder() );
}

/** {@inheritDoc} */
void CoreInterface::sendLocalBrokerStateEvent()
{
    // Send the broker state event
    DxlMessageService::getInstance().sendMessage( 
        DxlMessageConstants::CHANNEL_DXL_BROKER_STATE_EVENT,
        BrokerStateEventBuilder() );

    if( BrokerSettings::isTopicRoutingEnabled() )
    {
        // Send the topics (in batches)
        BrokerStateTopicsSender topicsSender;
        BrokerRegistry::getInstance().batchTopics( 
            getBrokerGuid(), BrokerSettings::getBrokerStateTopicsCharsBatchSize(), topicsSender );    
    }

    // Update the last send time
    m_lastLocalBrokerStateSend = getCoreTime();
}

/** {@inheritDoc} */
bool CoreInterface::isBridgeConnectAllowed( const std::string& brokerId ) const
{
    if( SL_LOG.isInfoEnabled() )
        SL_START << "isBridgeConnectAllowed: brokerId=" << brokerId << SL_INFO_END;

    // Check for cycles
    string nextBroker = BrokerRegistry::getInstance().getNextBroker( getBrokerGuid(), brokerId );
    if( !nextBroker.empty() )
    {
        // We found a route to the broker. Therefore adding this additional connection would create
        // a cycle. Reject the bridge, and wait for the current route to disconnect.
        if( SL_LOG.isWarnEnabled() )
            SL_START << "Disallowing bridge connection, would create a cycle: brokerId=" << brokerId << SL_INFO_END;
        return false;
    }

    return true;
}

/** {@inheritDoc} */
void CoreInterface::onBridgeConnected( bool isChild, const string& brokerId ) const
{    
    if( SL_LOG.isInfoEnabled() )
        SL_START << "onBridgeConnected: brokerId=" << brokerId << ", isChild=" << isChild << SL_INFO_END;

    // Update the broker registry with the new connection
    BrokerRegistry::getInstance().addConnection( getBrokerGuid(), brokerId, isChild );

    // Only send notifications if we are the child in the bridge (prevents message from 
    // being sent by both brokers involved in the bridge).
    if( isChild )
    {
        // Notify other brokers that the fabric has changed
        sendFabricChangeNotification();
    }
}

/** {@inheritDoc} */
void CoreInterface::onBridgeDisconnected( bool isChild, const string& brokerId ) const
{
    if( SL_LOG.isInfoEnabled() )
        SL_START << "onBridgeDisconnected: brokerId=" << brokerId << ", isChild=" << isChild << SL_INFO_END;

    // Update the broker registry with the new connection
    BrokerRegistry::getInstance().removeConnection( getBrokerGuid(), brokerId );

    // Notify other brokers that the fabric has changed
    sendFabricChangeNotification();
}

/** {@inheritDoc} */
void CoreInterface::onClientConnected( const string& clientId ) const
{    
    if( SL_LOG.isDebugEnabled() )
        SL_START << "onClientConnected: clientId=" << clientId << SL_DEBUG_END;

    if( BrokerSettings::isSendConnectEventsEnabled() )
    {
        DxlMessageService& messageService = DxlMessageService::getInstance();
        shared_ptr<DxlEvent> evt = messageService.createEvent();
        evt->setPayload( ClientRegistryConnectEventPayload( clientId ) );
        messageService.sendMessage( DxlMessageConstants::CHANNEL_DXL_CLIENTREGISTRY_CONNECT_EVENT, *evt );
    }
}

/** {@inheritDoc} */
void CoreInterface::onClientDisconnected( const string& clientId ) const
{
    if( SL_LOG.isDebugEnabled() )
        SL_START << "onClientDisconnected: clientId=" << clientId << SL_DEBUG_END;

    if( BrokerSettings::isSendConnectEventsEnabled() )
    {
        DxlMessageService& messageService = DxlMessageService::getInstance();
        shared_ptr<DxlEvent> evt = messageService.createEvent();
        evt->setPayload( ClientRegistryConnectEventPayload( clientId ) );
        messageService.sendMessage( DxlMessageConstants::CHANNEL_DXL_CLIENTREGISTRY_DISCONNECT_EVENT, *evt );
    }
}

/** {@inheritDoc} */
void CoreInterface::setBridgeConfiguration( const CoreBridgeConfiguration& config )
{
    // Set the local broker properties in the registry
    BrokerRegistry::getInstance().setLocalBrokerProperties( 
        config.getHostname(), config.getIpAddress(), config.getHubName(),
        config.getPort(), BrokerSettings::getWebSocketsListenPort() );

    if( config != m_bridgeConfig )
    {
        if( SL_LOG.isDebugEnabled() )
            SL_START << "setBridgeConfiguration: Bridge configuration has changed." << SL_DEBUG_END;

        // If the bridge configuration has changed, update core messaging layer
        m_bridgeConfig = config;
        onBridgeConfigurationChanged( m_bridgeConfig );
    }
    else
    {
        if( SL_LOG.isDebugEnabled() )
            SL_START << "setBridgeConfiguration: Bridge configuration is the same." << SL_DEBUG_END;
    }
}

/** {@inheritDoc} */
void CoreInterface::forceBridgeReconnect()
{
    if( SL_LOG.isDebugEnabled() )
        SL_START << "forceBridgeReconnect" << SL_DEBUG_END;

    onBridgeConfigurationChanged( m_bridgeConfig );
}

/** {@inheritDoc} */
bool CoreInterface::onPublishMessage( const char* sourceId, const char* canonicalSourceId,
    bool isBridge, uint8_t contextFlags, const char* topic, struct cert_hashes *certHashes ) const
{
    if( SL_LOG.isDebugEnabled() )
        SL_START << "onPublishMessage: source=" << sourceId << ", isBridge=" << isBridge <<
            ", contextFlags=" << contextFlags << ", topic=" << topic << SL_DEBUG_END;

    return CoreMessageHandlerService::getInstance()
        .onPublishMessage( sourceId, canonicalSourceId, isBridge, contextFlags, topic, certHashes );
}

/** {@inheritDoc} */
bool CoreInterface::onPreInsertPacketQueueExceeded(
    const char* destId, bool isBridge, uint64_t dbId ) const
{
    if( SL_LOG.isDebugEnabled() )
        SL_START << "onPreInsertPacketQueueExceeded: dbId=" << dbId << ", dest=" << destId << ", isBridge=" << 
            isBridge << SL_DEBUG_END;

    return CoreMessageHandlerService::getInstance()
        .onPreInsertPacketQueueExceeded( destId, isBridge, dbId );
}

/** {@inheritDoc} */
bool CoreInterface::onInsertMessage( const char* destId, const char* canonicalDestId, bool isBridge,
    uint8_t targetContextFlags, uint64_t dbId, const char* targetTenantGuid, struct cert_hashes *certHashes,
    bool* isClientMessageEnabled, unsigned char** clientMessage, size_t* clientMessageLen ) const
{
    if( SL_LOG.isDebugEnabled() )
        SL_START << "onInsertMessage: dbId=" << dbId << ", dest=" << destId <<
            ", isBridge=" << isBridge << ", targetContextFlags=" << targetContextFlags <<
            ", targetTenantGuid=" << targetTenantGuid << SL_DEBUG_END;

    return CoreMessageHandlerService::getInstance()
        .onInsertMessage( destId, canonicalDestId, isBridge, targetContextFlags, dbId, targetTenantGuid, certHashes,
            isClientMessageEnabled, clientMessage, clientMessageLen );
}

/** {@inheritDoc} */
void CoreInterface::onStoreMessage(
    uint64_t dbId, const char* sourceId, const char* canonicalSourceId, bool isBridge, 
    uint8_t contextFlags, const char* topic,
    uint32_t payloadLen, const void* payload,
    uint32_t* outPayloadLen, void** outPayload,
    const char* sourceTenantGuid, struct cert_hashes *certHashes, const char* certChain ) const
{
    if( SL_LOG.isDebugEnabled() )
        SL_START << "onStoreMessage: dbId=" << dbId << ", source=" << sourceId << ", isBridge=" << isBridge <<
            ", contextFlags=" << contextFlags << ", topic=" << topic << ", length=" << payloadLen <<
            ", sourceTenantGuid=" << sourceTenantGuid << SL_DEBUG_END;

    CoreMessageHandlerService::getInstance()
        .onStoreMessage(
            dbId, sourceId, canonicalSourceId, isBridge, contextFlags, topic, payloadLen, payload,
            outPayloadLen, outPayload, sourceTenantGuid, certHashes, certChain );
}

/** {@inheritDoc} */
void CoreInterface::onFinalizeMessage( uint64_t dbId ) const
{
    if( SL_LOG.isDebugEnabled() )
        SL_START << "onFinalizeMessage: dbId=" << dbId << SL_DEBUG_END;

    CoreMessageHandlerService::getInstance().onFinalizeMessage( dbId );
}

/** {@inheritDoc} */
void CoreInterface::onCoreMaintenance( time_t time )
{
    if( SL_LOG.isDebugEnabled() )
        SL_START << "onCoreMaintenance: time=" << time << SL_DEBUG_END;

    // Check whether we should send local broker state (TTL)
    if( m_lastLocalBrokerStateSend > 0 )
    {
        if( ( time - m_lastLocalBrokerStateSend ) >= 
            ( BrokerSettings::getBrokerTtlMins() * 60 ) )
        {
            if( SL_LOG.isDebugEnabled() )
                SL_START << "Sending local broker state (TTL has expired)" << time << SL_DEBUG_END;

            sendLocalBrokerStateEvent();
        }
    }

    // Notify registered listeners
    for( size_t i = 0; i < m_maintListeners.size(); i++ ) 
    {
        m_maintListeners[i]->onCoreMaintenance( time );
    }
}

/** {@inheritDoc} */
void CoreInterface::onTopicAddedToBroker( const char *topic ) const
{
    if( SL_LOG.isDebugEnabled() )
        SL_START << "onTopicAddedToBroker: " << topic << SL_DEBUG_END;
    
    // Only notify if topic-routing is enabled
    // Ignore client response channels
    if( BrokerSettings::isTopicRoutingEnabled() &&
        strncmp( DXL_CLIENT_PREFIX_BRACKET, topic, DXL_CLIENT_PREFIX_BRACKET_LEN ) )
    {
        BrokerRegistry& registry = BrokerRegistry::getInstance();
        const char* guid = BrokerSettings::getGuid();

        // Add the topic to the registry
        registry.addTopic( guid, topic );

        const BrokerState* brokerState = 
            BrokerRegistry::getInstance().getBrokerStatePtr( guid );        

        if( brokerState )
        {
            DxlMessageService& messageService = DxlMessageService::getInstance();
            shared_ptr<DxlEvent> evt = messageService.createEvent();
            BrokerTopicEventPayload::setMessageHeaderValues( *evt, brokerState );
            evt->setPayload( BrokerTopicEventPayload( topic ) );
            messageService.sendMessage( DxlMessageConstants::CHANNEL_DXL_BROKER_TOPIC_ADDED_EVENT, *evt );
        }
        else
        {
            SL_START << "Unable to find local broker in registry" << SL_ERROR_END;
        }
    }
}

/** {@inheritDoc} */
void CoreInterface::onTopicRemovedFromBroker( const char *topic ) const
{
    if( SL_LOG.isDebugEnabled() )
        SL_START << "onTopicRemovedFromBroker: " << topic << SL_DEBUG_END;

    // Only notify if topic-routing is enabled
    // Ignore client response channels
    if( BrokerSettings::isTopicRoutingEnabled() &&
        strncmp( DXL_CLIENT_PREFIX_BRACKET, topic, DXL_CLIENT_PREFIX_BRACKET_LEN ) )
    {
        BrokerRegistry& registry = BrokerRegistry::getInstance();
        const char* guid = BrokerSettings::getGuid();

        // Remove the topic from the registry
        registry.removeTopic( guid, topic );

        const BrokerState* brokerState = 
            BrokerRegistry::getInstance().getBrokerStatePtr( guid );

        if( brokerState )
        {
            DxlMessageService& messageService = DxlMessageService::getInstance();
            shared_ptr<DxlEvent> evt = messageService.createEvent();
            BrokerTopicEventPayload::setMessageHeaderValues( *evt, brokerState );
            evt->setPayload( BrokerTopicEventPayload( topic ) );
            messageService.sendMessage( DxlMessageConstants::CHANNEL_DXL_BROKER_TOPIC_REMOVED_EVENT, *evt );
        }
        else
        {
            SL_START << "Unable to find local broker in registry" << SL_ERROR_END;
        }
    }
}

/** {@inheritDoc} */
bool CoreInterface::updateTenantSentByteCount( const char* tenantId, uint32_t byteCount ) const
{
    return TenantMetricsService::getInstance().updateTenantSentByteCount( tenantId, byteCount );
}

/** {@inheritDoc} */
void CoreInterface::updateTenantConnectionCount( const char* tenantId, int adjCount ) const
{
    TenantMetricsService::getInstance().updateTenantConnectionCount( tenantId, adjCount );
}

/** {@inheritDoc} */
bool CoreInterface::isTenantConnectionAllowed( const char* tenantId ) const
{
    return TenantMetricsService::getInstance().isConnectionAllowed( tenantId );
}

/** {@inheritDoc} */
void CoreInterface::addMaintenanceListener( CoreMaintenanceListener* listener )
{
    if( std::find( m_maintListeners.begin(), m_maintListeners.end(), listener ) 
            == m_maintListeners.end() ) 
    {
        m_maintListeners.push_back(listener);
    }
}

/** {@inheritDoc} */
void CoreInterface::removeMaintenanceListener( CoreMaintenanceListener* listener )
{
    auto iter = std::find( m_maintListeners.begin(), m_maintListeners.end(), listener );
    if( iter != m_maintListeners.end() ) 
    {
        m_maintListeners.erase( iter );
    }
}

/** {@inheritDoc} */
void CoreInterface::log( LogLevel level, const char* message ) const
{
    SimpleLogger::LOG_LEVEL slLevel = SimpleLogger::debug;

    switch( level )
    {
        case LogLevel::error:
            slLevel = SimpleLogger::error;
            break;
        case LogLevel::info:
            slLevel = SimpleLogger::info;
            break;
        case LogLevel::warn:
            slLevel = SimpleLogger::warn;
            break;
        case LogLevel::debug:
            slLevel = SimpleLogger::debug;
            break;
    }

    SL_START << message << slLevel; 
}

/** {@inheritDoc} */
bool CoreInterface::isCertRevoked( const char* cert ) const
{
    return RevocationService::getInstance().isRevoked( cert );
}

/** {@inheritDoc} */
bool CoreInterface::isCertIdentityValidationEnabled()
{
    return BrokerSettings::isCertIdentityValidationEnabled();
}

/** {@inheritDoc} */
bool CoreInterface::isMultiTenantModeEnabled() const
{
    return BrokerSettings::isMultiTenantModeEnabled(); 
}

/** {@inheritDoc} */
bool CoreInterface::isTestModeEnabled() const
{
    return BrokerSettings::isTestModeEnabled();
}

/** {@inheritDoc} */
bool CoreInterface::isTenantSubscriptionAllowed( const char* tenantGuid, int subscription_count ) const
{
    return TenantMetricsService::getInstance().isTenantSubscriptionAllowed( tenantGuid, subscription_count );
}

/** {@inheritDoc} */
const char* CoreInterface::getBrokerTenantGuid() const
{
    // In case tenant GUID empty, do not throw any exception.
    const char* guid = BrokerSettings::getTenantGuid( false );
     return guid ? guid : "";
}
