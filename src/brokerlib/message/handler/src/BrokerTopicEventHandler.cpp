/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/BrokerSettings.h"
#include "include/SimpleLog.h"
#include "brokerregistry/include/brokerregistry.h"
#include "json/include/JsonService.h"
#include "message/include/DxlEvent.h"
#include "message/include/DxlMessageConstants.h"
#include "message/include/DxlMessageService.h"
#include "message/handler/include/BrokerTopicEventHandler.h"
#include "message/payload/include/BrokerTopicEventPayload.h"
#include "core/include/CoreMessageContext.h"
#include <memory>

using namespace std;
using namespace dxl::broker::json;
using namespace dxl::broker::message::handler;
using namespace dxl::broker::message::payload;
using namespace dxl::broker::core;

/** {@inheritDoc} */
bool BrokerTopicEventHandler::onStoreMessage(
    CoreMessageContext* context, struct cert_hashes* /*certHashes*/ ) const
{
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "BrokerTopicEventHandler::onStoreMessage: " << context->getTopic() << SL_DEBUG_END;
    }

    // Ignore if it is from the local broker
    if( BrokerSettings::isTopicRoutingEnabled() && !context->isLocalBrokerSource() )
    {
        // Get the DXL event
        DxlEvent* evt = context->getDxlEvent();

        // Get the source broker GUID
        std::string sourceBrokerGuid( evt->getSourceBrokerGuid() );

        // Determine if this is a topic add or remove
        bool isAdd = 
            ( context->getTopic() == 
                std::string( DxlMessageConstants::CHANNEL_DXL_BROKER_TOPIC_ADDED_EVENT ) );

        // Read the message header values
        uint32_t startTime =  0;
        uint32_t changeCount = 0;
        BrokerTopicEventPayload::getMessageHeaderValues( *evt, &startTime, &changeCount );        

        // Parse the payload
        BrokerTopicEventPayload payload;
        JsonService::getInstance().fromJson( evt->getPayloadStr(), payload );
        string payloadTopic = payload.getTopic();
        
        if( SL_LOG.isDebugEnabled() )
        {
            SL_START << "Topic event received: " 
                << sourceBrokerGuid
                << ", topic=" << payloadTopic
                << ", startTime=" << startTime
                << ", changeCount=" << changeCount
                << ", added=" << isAdd
                << SL_DEBUG_END;
        }

        BrokerRegistry& registry = BrokerRegistry::getInstance();    
        const BrokerState* state = registry.getBrokerStatePtr( sourceBrokerGuid );
        bool validState = true;
        if( state )
        {
            // Validate the start time of the broker here
            if( state->getBrokerStartTime() != startTime )
            {
                validState = false;

                if( SL_LOG.isDebugEnabled() )
                {
                    SL_START << "Broker start time is different: " 
                        << sourceBrokerGuid 
                        << ", expected=" << state->getBrokerStartTime() << ", "
                        << ", received=" << startTime 
                        << SL_WARN_END;
                }
            }

            // Validate that the count is one greater than the previous
            if( state->getTopicsChangeCount() != ( changeCount - 1 ) )
            {
                validState = false;

                if( SL_LOG.isDebugEnabled() )
                {
                    SL_START << "Previous change count delta is greater than one: " 
                        << sourceBrokerGuid 
                        << ", oldChangeCount=" << state->getTopicsChangeCount() << ", "
                        << ", newChangeCount=" << changeCount 
                        << SL_WARN_END;
                }
            }                        
        }
        else
        {
            SL_START << "Unable to find broker state: " << sourceBrokerGuid << SL_ERROR_END;            
        }

        // Update the state of the broker to reflect the added or removed topic
        if( isAdd )
        {
            registry.addTopic( sourceBrokerGuid, payloadTopic );
        }
        else
        {
            registry.removeTopic( sourceBrokerGuid, payloadTopic );
        }        

        // Set the topics change count 
        if( validState )
        {
            registry.setTopicsChangeCount( sourceBrokerGuid, changeCount );
        }
    }

    // propagate event
    return true;
}
