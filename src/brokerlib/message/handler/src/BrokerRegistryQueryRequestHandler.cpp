/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/SimpleLog.h"
#include "include/BrokerSettings.h"
#include "brokerregistry/include/brokerregistry.h"
#include "json/include/JsonService.h"
#include "message/include/DxlMessageService.h"
#include "message/include/DxlRequest.h"
#include "message/handler/include/BrokerRegistryQueryRequestHandler.h"
#include "message/payload/include/BrokerRegistryQueryRequestPayload.h"
#include "message/payload/include/BrokerRegistryQueryResponsePayload.h"

using namespace std;
using namespace dxl::broker::json;
using namespace dxl::broker::message::handler;
using namespace dxl::broker::message::payload;
using namespace dxl::broker::core;


/** {@inheritDoc} */
bool BrokerRegistryQueryRequestHandler::onStoreMessage(
    CoreMessageContext* context, struct cert_hashes* /*certHashes*/ ) const
{
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "BrokerRegistryQueryRequestHandler::onStoreMessage" << SL_DEBUG_END;
    }

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
        BrokerRegistryQueryRequestPayload requestPayload;
        JsonService::getInstance().fromJson( request->getPayloadStr(), requestPayload );

        BrokerRegistry& brokerRegistry = BrokerRegistry::getInstance();

        const string brokerGuid = requestPayload.getBrokerGuid();

        std::vector<const BrokerState*> brokers;

        // Lookup by broker GUID if applicable
        if( brokerGuid.length() > 0 )
        {
            const BrokerState* statePtr = brokerRegistry.getBrokerStatePtr( brokerGuid );

            if( statePtr )
            {
                brokers.push_back( statePtr );
            }
        }    
        else
        {
            brokers = brokerRegistry.getAllBrokerStatePtrs();
        }

        // Create the response    
        const shared_ptr<DxlResponse> response = messageService.createResponse( request );    

        // Set the payload
        BrokerRegistryQueryResponsePayload responsePayload( brokers );
        response->setPayload( responsePayload );

        // Send the response
        messageService.sendMessage( request->getReplyToTopic(), *(response.get()) );

        // Only propagate beyond this broker if other destination broker GUIDs were specified
        return ( destBrokerGuids != NULL && destBrokerGuids->size() > 1 );
    }
}
