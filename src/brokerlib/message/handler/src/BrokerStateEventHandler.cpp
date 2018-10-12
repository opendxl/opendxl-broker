/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/SimpleLog.h"
#include "brokerregistry/include/brokerregistry.h"
#include "json/include/JsonService.h"
#include "message/include/DxlEvent.h"
#include "message/include/DxlMessageService.h"
#include "message/handler/include/BrokerStateEventHandler.h"
#include "message/payload/include/BrokerStateEventPayload.h"
#include "core/include/CoreMessageContext.h"
#include <memory>

using namespace std;
using namespace dxl::broker::json;
using namespace dxl::broker::message::handler;
using namespace dxl::broker::message::payload;
using namespace dxl::broker::core;

/** {@inheritDoc} */
bool BrokerStateEventHandler::onStoreMessage(
    CoreMessageContext* context, struct cert_hashes* /*certHashes*/ ) const
{
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "BrokerStateEventHandler::onStoreMessage" << SL_DEBUG_END;
    }

    if( !context->isLocalBrokerSource() )
    {
        // Get the DXL event
        DxlEvent* evt = context->getDxlEvent();

        // Get the payload
        BrokerStateEventPayload brokerStateEventPayload;
        JsonService::getInstance().fromJson( evt->getPayloadStr(), brokerStateEventPayload );
    
        // Add/update the broker in the registry
        BrokerRegistry& brokerRegistry = BrokerRegistry::getInstance();
        brokerRegistry.addBroker( 
            brokerStateEventPayload.getGuid(),
            brokerStateEventPayload.getHostname(),
            brokerStateEventPayload.getPort(),
            brokerStateEventPayload.getTtlMins(),
            brokerStateEventPayload.getStartTime(),
            brokerStateEventPayload.getPolicyHostname(),
            brokerStateEventPayload.getPolicyIpAddress(),
            brokerStateEventPayload.getPolicyHubName(),
            brokerStateEventPayload.getPolicyPort(),
            brokerStateEventPayload.getBrokerVersion(),
            brokerStateEventPayload.getConnectionLimit(),
            brokerStateEventPayload.isTopicRoutingEnabled() );
        brokerRegistry.setConnections( 
            brokerStateEventPayload.getGuid(),
            brokerStateEventPayload.getConnections(),
            brokerStateEventPayload.getChildConnections() );
        brokerRegistry.updateRegistrationTime( 
            brokerStateEventPayload.getGuid() );
    }

    // propagate event
    return true;
}
