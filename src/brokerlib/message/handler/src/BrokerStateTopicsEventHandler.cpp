/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/BrokerSettings.h"
#include "include/SimpleLog.h"
#include "brokerregistry/include/brokerregistry.h"
#include "json/include/JsonService.h"
#include "message/include/DxlEvent.h"
#include "message/include/DxlMessageService.h"
#include "message/handler/include/BrokerStateTopicsEventHandler.h"
#include "message/payload/include/BrokerStateTopicsEventPayload.h"
#include "core/include/CoreMessageContext.h"
#include <memory>

using namespace std;
using namespace dxl::broker::json;
using namespace dxl::broker::message::handler;
using namespace dxl::broker::message::payload;
using namespace dxl::broker::core;

/** {@inheritDoc} */
bool BrokerStateTopicsEventHandler::onStoreMessage(
    CoreMessageContext* context, struct cert_hashes* /*certHashes*/ ) const
{
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "BrokerStateTopicsEventHandler::onStoreMessage" << SL_DEBUG_END;
    }

    // Ignore these messages if topic-routing is disabled
    if( BrokerSettings::isTopicRoutingEnabled() && !context->isLocalBrokerSource() )
    {
        // Get the DXL event
        DxlEvent* evt = context->getDxlEvent();

        // Get the source broker GUID
        std::string brokerGuid( evt->getSourceBrokerGuid() );

        BrokerRegistry& brokerRegistry = BrokerRegistry::getInstance();
        const BrokerState* state = brokerRegistry.getBrokerStatePtr( brokerGuid );
        if( state )
        {        
            // Read the message header values
            uint32_t startTime = 0;
            uint32_t changeCount = 0;
            BrokerStateTopicsEventPayload::getMessageHeaderValues( *evt, &startTime, &changeCount );        

            // If the broker start time has changed, or the subscription count has changed
            // Update the topics
            if( ( state->getBrokerStartTime() != startTime ) ||
                ( state->getTopicsChangeCount() != changeCount ) )
            {
                // Get the payload
                BrokerStateTopicsEventPayload brokerStateTopicsEventPayload;
                JsonService::getInstance().fromJson( evt->getPayloadStr(), brokerStateTopicsEventPayload );        

                BrokerStateTopicsEventPayload::topics_t topics = brokerStateTopicsEventPayload.getTopics();

                // Are we starting a new set of topics?
                if( brokerStateTopicsEventPayload.getState() & BrokerStateTopicsEventPayload::STATE_START )
                {
                    brokerRegistry.clearPendingTopics( brokerGuid );
                }

                // Add topics in batch
                brokerRegistry.addPendingTopics( 
                    brokerGuid, 
                    brokerStateTopicsEventPayload.getTopics(),
                    brokerStateTopicsEventPayload.getReadWildcardCount() );

                // We are done with this new set, swap 
                if( brokerStateTopicsEventPayload.getState() & BrokerStateTopicsEventPayload::STATE_END )
                {
                    brokerRegistry.swapPendingTopics( brokerGuid );
                    brokerRegistry.setTopicsChangeCount( brokerGuid, changeCount );
                }    

                if( SL_LOG.isDebugEnabled() )
                {
                    SL_START << "Broker topics have changed: " 
                        << brokerGuid << ", state=" 
                        << brokerStateTopicsEventPayload.getState() << SL_DEBUG_END;
                }
            }
            else
            {
                if( SL_LOG.isDebugEnabled() )
                {
                    SL_START << "Broker topics have not changed, ignoring: " 
                        << brokerGuid << SL_DEBUG_END;
                }
            }
        }
        else
        {
            SL_START << "Unable to find broker state: " << brokerGuid << SL_ERROR_END;
        }
    }

    // propagate event
    return true;
}

