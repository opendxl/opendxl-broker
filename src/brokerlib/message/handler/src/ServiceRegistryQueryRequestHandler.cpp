/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/BrokerSettings.h"
#include "include/SimpleLog.h"
#include "brokerregistry/include/brokerregistry.h"
#include "json/include/JsonService.h"
#include "message/include/DxlMessageService.h"
#include "message/include/DxlRequest.h"
#include "message/handler/include/ServiceRegistryQueryRequestHandler.h"
#include "message/payload/include/ServiceRegistryQueryRequestPayload.h"
#include "message/payload/include/ServiceRegistryQueryResponsePayload.h"
#include "serviceregistry/include/ServiceRegistry.h"

using namespace std;
using namespace dxl::broker::json;
using namespace dxl::broker::message::handler;
using namespace dxl::broker::message::payload;
using namespace dxl::broker::core;
using namespace dxl::broker::service;

/** {@inheritDoc} */
bool ServiceRegistryQueryRequestHandler::onStoreMessage(
    CoreMessageContext* context, struct cert_hashes* /*certHashes*/ ) const
{
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "ServiceRegistryQueryRequestHandler::onStoreMessage" << SL_DEBUG_END;
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
        ServiceRegistryQueryRequestPayload requestPayload;
        JsonService::getInstance().fromJson( request->getPayloadStr(), requestPayload );

        // Service registry
        ServiceRegistry& serviceRegistry = ServiceRegistry::getInstance();

        // The registrations that were found
        vector<serviceRegistrationPtr_t> registrations;
    
        const string serviceGuid = requestPayload.getServiceGuid();
        const string serviceType = requestPayload.getServiceType();
        const char* clientTenantGuid = request->getSourceTenantGuid();

        // Lookup by service GUID if applicable
        if( serviceGuid.length() > 0 )
        {
            serviceRegistrationPtr_t reg = serviceRegistry.findService( serviceGuid, clientTenantGuid );
            if( reg.get() )
            {
                registrations.push_back( reg );
            }
        }    
        else if( serviceType.length() > 0 )
        {
            // Return by service type if applicable
            registrations = serviceRegistry.findServicesByType( serviceType, clientTenantGuid );
        }                    
        else
        {
            // Return all services
            registrations = serviceRegistry.getAllServices( clientTenantGuid );
        }

        // Create the response    
        const shared_ptr<DxlResponse> response = messageService.createResponse( request );    

        // Set the payload
        ServiceRegistryQueryResponsePayload responsePayload( registrations );
        response->setPayload( responsePayload );

        // Send the response
        messageService.sendMessage( request->getReplyToTopic(), *(response.get()) );

        // Only propagate beyond this broker if other destination broker GUIDs were specified
        return ( destBrokerGuids != NULL && destBrokerGuids->size() > 1 );
    }
}
