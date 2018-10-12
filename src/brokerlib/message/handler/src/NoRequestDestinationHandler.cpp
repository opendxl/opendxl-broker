/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/BrokerSettings.h"
#include "include/SimpleLog.h"
#include "brokerregistry/include/brokerregistry.h"
#include "message/include/DxlMessageService.h"
#include "message/handler/include/NoRequestDestinationHandler.h"
#include <cstring>
#include <memory>

using namespace std;
using namespace dxl::broker;
using namespace dxl::broker::message::handler;
using namespace dxl::broker::core;

/** {@inheritDoc} */
void NoRequestDestinationHandler::onFinalizeMessage(
    CoreMessageContext* context ) const
{
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "NoRequestDestinationHandler::onInsertMessage: topic="
            << context->getTopic() << ", isDxlMessage=" << context->isDxlMessage() 
            << ", destCount=" << context->getDestinationCount() << SL_DEBUG_END;
    }

    // If it is a DXL message and it was not sent to a destination
    if( context->isServiceNotFoundMessageEnabled() &&
        context->getDestinationCount() == 0 && 
        context->isDxlMessage() && 
        context->isMessageInsertEnabled() )
    {
        // Get the DXL message
        DxlMessage* message = context->getDxlMessage();

        // Only check for request messages
        if( message->isRequestMessage() )
        {
            if( SL_LOG.isDebugEnabled() )
            {
                SL_START << "Unable to find service for channel: topic="
                    << context->getTopic() << ", isDxlMessage=" << context->isDxlMessage()  
                    << SL_DEBUG_END;
            }

            // Send service not found error message                    
            DxlMessageService::getInstance()
                .sendServiceNotFoundErrorMessage( context->getDxlRequest() );
        }
    }
}
