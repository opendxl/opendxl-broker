/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/brokerlib.h"
#include "include/BrokerSettings.h"
#include "include/SimpleLog.h"
#include "json/include/JsonService.h"
#include "message/include/DxlMessageService.h"
#include "message/include/DxlRequest.h"
#include "message/handler/include/ClientRegistryQueryRequestHandler.h"
#include "message/payload/include/ClientRegistryQueryRequestPayload.h"
#include "message/payload/include/ClientRegistryQueryResponsePayload.h"

using namespace std;
using namespace dxl::broker::json;
using namespace dxl::broker::message::handler;
using namespace dxl::broker::message::payload;
using namespace dxl::broker::core;

/** {@inheritDoc} */
bool ClientRegistryQueryRequestHandler::onStoreMessage(
    CoreMessageContext* context, struct cert_hashes* /*certHashes*/ ) const
{
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "ClientRegistryQueryRequestHandler::onStoreMessage" << SL_DEBUG_END;
    }

    // We don't send "service not found message" for client queries due to the fact that they
    // are essentially broadcast "request" messages.
    context->setServiceNotFoundMessageEnabled( false );

    DxlMessageService& messageService = DxlMessageService::getInstance();
    
    // Get the DXL request
    DxlRequest* request = context->getDxlRequest();

    // See if destination GUIDs have been identified
    const unordered_set<std::string>* destBrokerGuids = request->getDestinationBrokerGuids();
    if( destBrokerGuids != NULL &&
        destBrokerGuids->find( BrokerSettings::getGuid() ) == destBrokerGuids->end() )
    {
        // We are not the target for this request, allow it to be propagated to other brokers
        return true;
    }
    else
    {
        ClientRegistryQueryRequestPayload requestPayload;
        JsonService::getInstance().fromJson( request->getPayloadStr(), requestPayload );

        if( getCoreInterface()->isClientConnected( requestPayload.getClientGuid() ) )
        {
            // Create the response    
            const shared_ptr<DxlResponse> response = messageService.createResponse( request );    

            // Set the payload
            ClientRegistryQueryResponsePayload responsePayload;
            response->setPayload( responsePayload );

            // Send the response
            messageService.sendMessage( request->getReplyToTopic(), *(response.get()) );

            // Do not propagate beyond this broker
            return false;
        }

        // Propagate to other brokers if no brokers were specified or more than one
        // broker was specified (besides ourselves)
        return ( destBrokerGuids == NULL || destBrokerGuids->size() > 1 );
    }
}
