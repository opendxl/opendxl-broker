/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/BrokerSettings.h"
#include "include/SimpleLog.h"
#include "json/include/JsonService.h"
#include "message/include/DxlEvent.h"
#include "message/include/DxlMessageService.h"
#include "message/include/DxlMessageConstants.h"
#include "message/handler/include/ServiceRegistryRegisterRequestHandler.h"
#include "message/payload/include/ServiceRegistryRegisterEventPayload.h"
#include "serviceregistry/include/ServiceRegistry.h"
#include "DxlFlags.h"

using namespace std;
using namespace dxl::broker::json;
using namespace dxl::broker::message::handler;
using namespace dxl::broker::message::payload;
using namespace dxl::broker::core;
using namespace dxl::broker::service;

/** {@inheritDoc} */
bool ServiceRegistryRegisterRequestHandler::onStoreMessage(
    CoreMessageContext* context, struct cert_hashes* certHashes ) const
{
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "ServiceRegistryRegisterRequestHandler::onStoreMessage" << SL_DEBUG_END;
    }

    // Get the DXL request
    DxlRequest* request = context->getDxlRequest();

    // Get the payload
    ServiceRegistryRegisterEventPayload registerPayload;
    JsonService::getInstance().fromJson( request->getPayloadStr(), registerPayload );

    // Update the payload with information from the local broker, current connection
    registerPayload.setBrokerGuid( BrokerSettings::getGuid() );
    registerPayload.setClientGuid( 
        ( ( context->getContextFlags() & DXL_FLAG_LOCAL ) ?
            BrokerSettings::getGuid() : context->getCanonicalSourceId() ) );
    registerPayload.setClientInstanceGuid( 
        ( ( context->getContextFlags() & DXL_FLAG_LOCAL ) ?
            BrokerSettings::getInstanceGuid() : context->getSourceId() ) );
    bool isManaged = (context->getContextFlags() & DXL_FLAG_MANAGED) != 0;
    registerPayload.setManagedClient( isManaged );
    if( !isManaged )
    {
        unordered_set<std::string> certs;
        struct cert_hashes *current, *tmp;
        HASH_ITER( hh, certHashes, current, tmp ) 
        {
            certs.insert( current->cert_sha1 );
        }        
        registerPayload.setCertificates( certs );
    }

    if( BrokerSettings::isMultiTenantModeEnabled() )
    {
        const std::string& clientTenantGuid = request->getSourceTenantGuid();
        const unordered_set<std::string>* destTenantGuids = request->getDestinationTenantGuids();

        unordered_set<std::string> targetTenantGuids;

        // If the client is not OPs
        if( !context->isSourceOps() )
        {
            // Service registration from a regular client only available for its tenants
            targetTenantGuids.insert( clientTenantGuid );
        }
        // If the client is OPs and the destination tenants are not NULL, the destinations are allowed
        else if( destTenantGuids != NULL )
        {
            targetTenantGuids = *destTenantGuids;
        }

        // Set the service tenant GUID to that of the client that registered the service.
        registerPayload.setClientTenantGuid( clientTenantGuid );

        // Set the target tenant GUIDs for this service.
        registerPayload.setTargetTenantGuids( targetTenantGuids );
    }

    // Create the registration
    shared_ptr<ServiceRegistration> reg( 
        new ServiceRegistration( registerPayload.getServiceRegistration() ) );    
    reg->setBrokerService( context->getContextFlags() & DXL_FLAG_LOCAL );

    // Register the service
    ServiceRegistry::getInstance().registerService( reg );

    // Send a response to the invoking client
    DxlMessageService& messageService = DxlMessageService::getInstance();    
    shared_ptr<DxlResponse> response = messageService.createResponse( request );
    messageService.sendMessage( request->getReplyToTopic(), *response );

    // Do not propagate the request, we send an event to the other brokers
    return false;
}
