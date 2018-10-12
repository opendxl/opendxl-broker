/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include <cstring>
#include "include/BrokerSettings.h"
#include "include/SimpleLog.h"
#include "message/include/DxlMessageConstants.h"
#include "message/include/DxlMessageService.h"
#include "message/handler/include/NoEventDestinationHandler.h"
#include "message/payload/include/EventSubscriberNotFoundEventPayload.h"

using namespace std;
using namespace dxl::broker;
using namespace dxl::broker::message::handler;
using namespace dxl::broker::message::payload;
using namespace dxl::broker::core;

/** {@inheritDoc} */
void NoEventDestinationHandler::onFinalizeMessage(
    CoreMessageContext* context ) const
{
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "NoEventDestinationHandler::onInsertMessage: topic="
            << context->getTopic() << ", isDxlMessage=" << context->isDxlMessage() 
            << ", destCount=" << context->getDestinationCount() << SL_DEBUG_END;
    }

    // If it is a DXL message and it was not sent to a destination
    if( context->getDestinationCount() == 0 && 
        context->isDxlMessage() && 
        context->isMessageInsertEnabled() )
    {
        // Get the DXL message
        DxlMessage* message = context->getDxlMessage();

        const char* topic = context->getTopic();

        // Must be from another broker
        if( strcmp( message->getSourceBrokerGuid(), BrokerSettings::getGuid() ) && 
            // Only check for event messages
            message->isEventMessage() &&
            // Ignore client-based events to response channel
            strncmp( DXL_CLIENT_PREFIX_BRACKET, topic, DXL_CLIENT_PREFIX_BRACKET_LEN ) &&
            // Ignore broker-based events
            strncmp( DXL_BROKER_EVENT_PREFIX, topic, DXL_BROKER_EVENT_PREFIX_LEN ) &&
            // Ignore our event
            strcmp( DxlMessageConstants::CHANNEL_DXL_EVENT_SUBSCRIBER_NOT_FOUND_EVENT, topic ) )
        {
            if( SL_LOG.isDebugEnabled() )
            {
                SL_START << "Unable to find subscriber for event: topic="
                    << context->getTopic() << ", isDxlMessage=" << context->isDxlMessage()  
                    << SL_DEBUG_END;
            }

            // Send the event
            DxlMessageService& service = DxlMessageService::getInstance();
            EventSubscriberNotFoundEventPayload payload( topic );
            auto evt = service.createEvent();
            evt->setPayload( payload );

            service.sendMessage(  
                DxlMessageConstants::CHANNEL_DXL_EVENT_SUBSCRIBER_NOT_FOUND_EVENT, *evt );
        }
    }
}
