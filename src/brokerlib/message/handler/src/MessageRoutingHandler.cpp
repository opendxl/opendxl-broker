/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include <cstring>
#include "include/BrokerSettings.h"
#include "include/SimpleLog.h"
#include "message/include/DxlMessageConstants.h"
#include "message/include/DxlMessageService.h"
#include "message/handler/include/MessageRoutingHandler.h"
#include "brokerregistry/include/brokerregistry.h"
#include "DxlFlags.h"

using namespace dxl::broker::message;
using namespace dxl::broker::message::handler;
using namespace dxl::broker::core;

/** {@inheritDoc} */
bool MessageRoutingHandler::onInsertMessage(
    CoreMessageContext* context, const char* destId, const char* canonicalDestId, bool isBridge, uint8_t contextFlags,
    const char* targetTenantGuid, struct cert_hashes* /*certHashes*/, bool* isClientMessageEnabled, 
    unsigned char** clientMessage, size_t* clientMessageLen ) const
{
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "MessageRoutingHandler::onInsertMessage: topic="
            << context->getTopic() << ", isDxlMessage=" << context->isDxlMessage() 
            << ", dest=" << destId << ", isBridge=" << isBridge << ", contextFlags=" << contextFlags
            << ", targetTenantGuid=" << targetTenantGuid << SL_DEBUG_END;
    }

    // Is it a DXL message?
    if( context->isDxlMessage() )
    {
        DxlMessage* message = context->getDxlMessage();

        // If the destination is a bridge
        if( isBridge )
        {
            const unordered_set<std::string>* nextBrokerGuids = 
                message->getNextBrokerGuids();

            // If next broker guids are available, ensure is intermediate broker
            if( nextBrokerGuids != NULL )                
            {
                if( nextBrokerGuids->find( destId ) == nextBrokerGuids->end() )
                {
                    if( SL_LOG.isDebugEnabled() )
                    {
                        SL_START << "Not routing to bridge, not intermediate broker for message destination: "
                            << context->getTopic() << ", dest=" << destId << ", isBridge=" << isBridge << SL_DEBUG_END;
                    }
                    return false;
                }
            }            
            else if( 
                BrokerSettings::isTopicRoutingEnabled() && // Topic-routing must be enabled
                message->isEventMessage() ) // Topic-based routing for events only
            {                    
                // The topic
                const char* topic = context->getTopic();

                return 
                    // Always route client-based events to response channel
                    !strncmp( DXL_CLIENT_PREFIX_BRACKET, topic, DXL_CLIENT_PREFIX_BRACKET_LEN ) ||
                    // Always route broker-based events
                    !strncmp( DXL_BROKER_EVENT_PREFIX, topic, DXL_BROKER_EVENT_PREFIX_LEN ) ||
                    // Normal check for topic-based routing
                    BrokerRegistry::getInstance().isSubscriberInHierarchy(
                        BrokerSettings::getGuid(), destId, topic );
            }
        }        
        else // If it is a Normal client
        {                        
            const unordered_set<std::string>* destBrokerGuids = 
                message->getDestinationBrokerGuids();

            // If dest brokers are available, ensure this broker is a target
            if( destBrokerGuids != NULL &&
                destBrokerGuids->find( BrokerSettings::getGuid() ) == destBrokerGuids->end() )
            {
                if( SL_LOG.isDebugEnabled() )
                {
                    SL_START << "Not routing to client, local broker not message destination: "
                        << context->getTopic() << ", dest=" << destId << ", isBridge=" << isBridge << SL_DEBUG_END;
                }
                return false;
            }

            const unordered_set<std::string>* destClientGuids = 
                message->getDestinationClientGuids();

            // If clients are available, ensure this client is listed
            // If the "local" flag is set, check to see if the broker guid is listed
            // For "local" connections we allow routing on both the client id and the broker id
            if( ( destClientGuids != NULL ) &&
                ( destClientGuids->find( destId ) == destClientGuids->end() ) &&
                ( destClientGuids->find( canonicalDestId ) == destClientGuids->end() ) &&
                ( !( contextFlags & DXL_FLAG_LOCAL ) ||
                  destClientGuids->find( BrokerSettings::getGuid() ) == destClientGuids->end() ) )
            {
                if( SL_LOG.isDebugEnabled() )
                {
                    SL_START << "Not routing to client, not a message destination: "
                        << context->getTopic() << ", dest=" << destId << ", isBridge=" << isBridge << SL_DEBUG_END;
                }
                return false;
            }

            // Filter by tenant only when MT mode is enabled.
            if( BrokerSettings::isMultiTenantModeEnabled() )
            {
                if ( !onInsertMultiTenantMessage(
                        message, contextFlags, context, targetTenantGuid ) )
                {
                    // Target is not allow to recv the message
                    if( SL_LOG.isDebugEnabled() )
                    {
                        SL_START << "Not routing to client, targetTenantGuid= "
                                << targetTenantGuid
                                << "is not a valid destination for sourceTenantId= "
                                << message->getSourceTenantGuid()
                                << SL_DEBUG_END;
                    }
                    return false;
                }
            }

            // Whether to use the client-specific message
            if( destClientGuids != NULL && message->isEventMessage() )
            {
                *isClientMessageEnabled = true;

                // Only generate the client specific message once
                if( !context->isClientSpecificMessageGenerated() )
                {
                    DxlMessageService& messageService = DxlMessageService::getInstance();
                    messageService.toBytes( *message, clientMessage, clientMessageLen, true );                    
                    context->setClientSpecificMessageGenerated( true );
                }
            }
        }
    }
    else if( BrokerSettings::isMultiTenantModeEnabled() )
    {
        // With core messages, we don't have the ability to determine the source tenant identifier.
        // Therefore, we have no choice other than to reject them.
        return false;
    }

    // insert message
    return true;
}

/** {@inheritDoc} */
bool MessageRoutingHandler::onInsertMultiTenantMessage(
    DxlMessage* message, const uint8_t targetContextFlags, CoreMessageContext* context,
    const char* targetTenantGuid ) const
{
    const unordered_set<std::string>* destTenantGuids = message->getDestinationTenantGuids();
 
    // If destination tenants are available, ensure this tenant is a target.
    if( destTenantGuids != NULL && 
        destTenantGuids->find( targetTenantGuid ) == destTenantGuids->end() )
    {        
        return false;
    }
    
    if( destTenantGuids == NULL &&
        !( targetContextFlags & DXL_FLAG_OPS ) &&
        !context->isSourceOps() &&
        strcmp( targetTenantGuid, message->getSourceTenantGuid() ) != 0 )
    {
        // Default case (destTenantsGuids == NULL)
        // Should fail if all following conditions are true:
        // * target is a regular client,
        // * source context is not Ops (regular client or bridged broker),
        // * the message source tenant GUID is not equal to the target tenant.    

        return false;
    }

    return true;
}
