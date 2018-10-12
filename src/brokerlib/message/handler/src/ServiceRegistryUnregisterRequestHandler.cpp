/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/BrokerSettings.h"
#include "include/SimpleLog.h"
#include "json/include/JsonService.h"
#include "message/include/DxlMessageService.h"
#include "message/include/DxlMessageConstants.h"
#include "message/handler/include/ServiceRegistryUnregisterRequestHandler.h"
#include "message/payload/include/ServiceRegistryUnregisterEventPayload.h"
#include "serviceregistry/include/ServiceRegistry.h"

using namespace std;
using namespace dxl::broker;
using namespace dxl::broker::json;
using namespace dxl::broker::message::handler;
using namespace dxl::broker::message::payload;
using namespace dxl::broker::core;
using namespace dxl::broker::service;

/** {@inheritDoc} */
bool ServiceRegistryUnregisterRequestHandler::onStoreMessage(
    CoreMessageContext* context, struct cert_hashes* /*certHashes*/ ) const
{
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "ServiceRegistryUnregisterRequestHandler::onStoreMessage" << SL_DEBUG_END;
    }

    // Get the DXL request
    DxlRequest* request = context->getDxlRequest();

    // Get the payload
    ServiceRegistryUnregisterEventPayload unregisterPayload;
    JsonService::getInstance().fromJson( request->getPayloadStr(), unregisterPayload );

    // Determine the client guid
    string clientGuid = 
        ( ( context->getContextFlags() & DXL_FLAG_LOCAL ) ?
            BrokerSettings::getGuid() : context->getCanonicalSourceId() );

    // Unregister the service
    //
    // Since we are receiving this unregistration message from an endpoint, we need to validate
    // that the client and tenant guids match the service that is being unregistered
    ServiceRegistry::getInstance().unregisterService( 
        unregisterPayload.getServiceGuid(), clientGuid, request->getSourceTenantGuid() );

    // Send a response to the invoking client
    DxlMessageService& messageService = DxlMessageService::getInstance();    
    shared_ptr<DxlResponse> response = messageService.createResponse( request );
    messageService.sendMessage( request->getReplyToTopic(), *response );

    // Do not propagate the request, we send an event to the other brokers
    return false;
}
