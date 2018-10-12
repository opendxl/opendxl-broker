/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/SimpleLog.h"
#include "json/include/JsonService.h"
#include "message/include/DxlEvent.h"
#include "message/include/DxlMessageService.h"
#include "message/handler/include/ServiceRegistryUnregisterEventHandler.h"
#include "message/payload/include/ServiceRegistryUnregisterEventPayload.h"
#include "serviceregistry/include/ServiceRegistry.h"

using namespace std;
using namespace dxl::broker::json;
using namespace dxl::broker::message::handler;
using namespace dxl::broker::message::payload;
using namespace dxl::broker::core;
using namespace dxl::broker::service;

/** {@inheritDoc} */
bool ServiceRegistryUnregisterEventHandler::onStoreMessage(
    CoreMessageContext* context, struct cert_hashes* /*certHashes*/ ) const
{
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "ServiceRegistryUnregisterEventHandler::onStoreMessage" << SL_DEBUG_END;
    }

    // Only process the event if are not the broker that sent it
    if( !context->isLocalBrokerSource() )
    {
        // Get the DXL event
        DxlEvent* evt = context->getDxlEvent();

        // Get the payload
        ServiceRegistryUnregisterEventPayload unregisterEventPayload;
        JsonService::getInstance().fromJson( evt->getPayloadStr(), unregisterEventPayload );

        // Unregister the service
        //
        // Since this event is only ever sent by other brokers, we don't need to validate the 
        // client guid and client tenant guid prior to unregistration.
        ServiceRegistry::getInstance().unregisterService( unregisterEventPayload.getServiceGuid() );
    }

    // propagate event
    return true;
}
