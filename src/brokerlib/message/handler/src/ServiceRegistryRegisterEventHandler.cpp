/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/SimpleLog.h"
#include "json/include/JsonService.h"
#include "message/include/DxlEvent.h"
#include "message/include/DxlMessageService.h"
#include "message/handler/include/ServiceRegistryRegisterEventHandler.h"
#include "message/payload/include/ServiceRegistryRegisterEventPayload.h"
#include "serviceregistry/include/ServiceRegistry.h"

using namespace std;
using namespace dxl::broker::json;
using namespace dxl::broker::message::handler;
using namespace dxl::broker::message::payload;
using namespace dxl::broker::core;
using namespace dxl::broker::service;

/** {@inheritDoc} */
bool ServiceRegistryRegisterEventHandler::onStoreMessage(
    CoreMessageContext* context, struct cert_hashes* /*certHashes*/ ) const
{
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "ServiceRegistryRegisterEventHandler::onStoreMessage" << SL_DEBUG_END;
    }

    // Only process the event if we are not the broker that sent it
    if( !context->isLocalBrokerSource() )
    {
        // Get the DXL event
        DxlEvent* evt = context->getDxlEvent();

        // Get the payload
        ServiceRegistryRegisterEventPayload registerEventPayload;
        JsonService::getInstance().fromJson( evt->getPayloadStr(), registerEventPayload );

        // Register the service
        ServiceRegistry::getInstance().registerService( 
            shared_ptr<ServiceRegistration>( 
                new ServiceRegistration( registerEventPayload.getServiceRegistration() ) ) );
    }

    // propagate event
    return true;
}
