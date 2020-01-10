/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/BrokerSettings.h"
#include "include/Configuration.h"
#include "include/SimpleLog.h"
#include "serviceregistry/include/ServiceRegistry.h"
#include "message/include/DxlMessageConstants.h"
#include "message/include/DxlMessageService.h"
#include "message/handler/include/ServiceLookupHandler.h"
#include "message/payload/include/MultiServiceRequestResponsePayload.h"
#include "core/include/CoreUtil.h"
#include "message/include/dxl_error_message.h"
#include <cstring>

using namespace std;
using namespace dxl::broker;
using namespace dxl::broker::message;
using namespace dxl::broker::message::handler;
using namespace dxl::broker::message::payload;
using namespace dxl::broker::core;
using namespace dxl::broker::service;


/** {@inheritDoc} */
bool ServiceLookupHandler::onStoreMessage(
    CoreMessageContext* context, struct cert_hashes* /*certHashes*/ ) const
{
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "ServiceLookupHandler::onStoreMessage" << SL_DEBUG_END;
    }

    if( context->isDxlMessage() )
    {
        DxlMessage* dxlMessage = context->getDxlMessage();

        if( dxlMessage->isRequestMessage() && !context->isDxlBrokerServiceRequest() )
        {
            // Handle request messages unless they are being sent to local broker services
            return handleRequest( context, context->getDxlRequest() );
        }
        else if( ServiceRegistry::getInstance().isEventToRequestEnabled() &&
            dxlMessage->isEventMessage() && 
            !strcmp( dxlMessage->getSourceBrokerGuid(), BrokerSettings::getGuid() ) )
        {
            // If a service is registered that supports the transformation of an event into
            // a request, and this is the first broker visited, attempt to invoke the
            // service method (transform event, find service, invoke).
            return handleEvent( context, context->getDxlEvent() );
        }
        else if( dxlMessage->isErrorMessage() )
        {
            return handleErrorResponse( context, context->getDxlErrorResponse() );
        }
    }
    
    return true;
}

/** {@inheritDoc} */
serviceRegistrationPtr_t ServiceLookupHandler::findServiceForTopic(
    const char* topic, const char* targetServiceTenantGuid ) const
{
    // Get the service registry
    ServiceRegistry& serviceRegistry = ServiceRegistry::getInstance();

    // Lookup the service for the specified topic
    serviceRegistrationPtr_t service = serviceRegistry.getNextService( topic, targetServiceTenantGuid );

    //
    // TODO: If no services are registered with wildcards, we can eliminate the following
    // code (optimization).
    //

    // Wildcard based service lookup
    if( !service.get() )
    {
        // Attempt to find service based on wildcards (# at the end)
        char* wcTopic = CoreUtil::iterateWildcardBegin( topic );
        while( CoreUtil::iterateWildcardNext( wcTopic ) )
        {
            service = serviceRegistry.getNextService( wcTopic, targetServiceTenantGuid );
            if( service.get() )
            {
                // Found a service, stop looking
                break;
            }
        }
        CoreUtil::iterateWildcardEnd( wcTopic );
    }
    
    return service;
}

/** {@inheritDoc} */
void ServiceLookupHandler::handleMultiServiceRequest( 
    const CoreMessageContext* context, DxlRequest* dxlRequest, const char* clientTenantGuid ) const
{
    // TODO: Does not currently take into consideration wildcards
    // TODO: The underlying lookup methods need to be updated to correctly round-robin
    //       multi-service requests
    //
    //       To accomplish this, the following order would be ideal:
    //
    //        Types: (ordered service types)
    //            svctype_a 
    //            svctype_b
    //            svctype_c
    //        Services in registry: (Services arranged to distribute groups of types)
    //            svc1: svctype_a
    //            svc2: svctype_b
    //            svc3: svstype_c
    //            svc4: svctype_a
    //            svc5: svctype_b
    //            svc6: svctype_c
    //            ...

    // Determine unique types for the request    
    const topicServiceTypes_t *types = 
        ServiceRegistry::getInstance().getServiceTypes( context->getTopic() );

    if( types )
    {        
        unordered_map<string, serviceRegistrationPtr_t> servicesByRequestId;

        // Iterate the different types attempting to locate valid services for
        // each one        
        for( auto iter = types->begin(); iter != types->end(); iter++ )
        {
            if( SL_LOG.isDebugEnabled() )
            {
                SL_START << "Looking up service for type: " << (*iter) << SL_DEBUG_END;
            }

            serviceRegistrationPtr_t service; 
            service = ServiceRegistry::getInstance().getNextService( 
                context->getTopic(), clientTenantGuid, iter->c_str() );

            if( service.get() )
            {
                if( SL_LOG.isDebugEnabled() )
                {
                    SL_START << "Found service: "<< service->getServiceGuid() <<
                        ", type: " << service->getServiceType() << SL_DEBUG_END;
                }

                // Send the request
                DxlRequest req(*dxlRequest);
                req.assignNewMessageId( dxlRequest->getMessageId() );
                req.setDestinationBrokerGuid( service->getBrokerGuid().c_str() );
                req.setDestinationClientGuid( service->getClientInstanceGuid().c_str() );
                req.setDestinationServiceId( service->getServiceGuid().c_str() );
                req.setMultiServiceRequest( false );
                DxlMessageService::getInstance().sendMessage( context->getTopic(), req );

                // Add the service to the list of services
                servicesByRequestId[ req.getMessageId() ] = service;
            }
        }                

        if( !servicesByRequestId.empty() )
        {
            // Create the response
            const shared_ptr<DxlResponse> response = DxlMessageService::getInstance().createResponse( dxlRequest );
            // Set the payload
            MultiServiceRequestResponsePayload responsePayload( servicesByRequestId );
            response->setPayload( responsePayload );
            // Send the response
            DxlMessageService::getInstance().sendMessage( dxlRequest->getReplyToTopic(), *response );
            return;
        }
    }

    DxlMessageService::getInstance().sendServiceNotFoundErrorMessage( dxlRequest );
}

/** {@inheritDoc} */
bool ServiceLookupHandler::handleRequest( const CoreMessageContext* context, DxlRequest* dxlRequest ) const
{
    const char* serviceId = dxlRequest->getDestinationServiceId();
        
    // Determine whether the request has a service identifier
    bool hasServiceId = ( serviceId && ( strlen( serviceId ) > 0 ) );
            
    // Determine if the request has routing information
    bool hasRoutingInformation = hasServiceId &&
        dxlRequest->getDestinationClientGuids() &&
        dxlRequest->getDestinationBrokerGuids();
    
    if( !hasRoutingInformation )
    {
        // The tenant GUID to find the service for.
        const char* clientTenantGuid = dxlRequest->getSourceTenantGuid();

        // Handle multi-service requests
        if( dxlRequest->isMultiServiceRequest() )
        {
            // Generate the multi service requests
            handleMultiServiceRequest( context, dxlRequest, clientTenantGuid );

            // We create new requests, so do not forward
            return false;                        
        }

        serviceRegistrationPtr_t service;    
        if( hasServiceId )
        {
            // A service identifier was specified, but the routing information has not been 
            // set. Lookup the service taking into account the tenant GUID.
            service = ServiceRegistry::getInstance().findService( serviceId, clientTenantGuid );
            if( !service.get() )
            {
                if( BrokerSettings::isMultiTenantModeEnabled() )
                {
                    // This is a bit of a hack. In multi-tenant mode, it is possible that you
                    // could have the ability to invoke a service, but specify a service 
                    // identifier that is associated with another tenant. If this occurs, 
                    // the service would be unregistered. Clearing the service identifier when
                    // specified explicitly will prevent this from happening.
                    dxlRequest->setDestinationServiceId( "" );
                }
            }
        }
        else
        {
            // Get the next service for the topic and tenant GUID.
            service = findServiceForTopic( context->getTopic(), clientTenantGuid );
        }

        if( service.get() )
        {
            // Set the routing information
            dxlRequest->setDestinationBrokerGuid( service->getBrokerGuid().c_str() );
            dxlRequest->setDestinationClientGuid( service->getClientInstanceGuid().c_str() );
            dxlRequest->setDestinationServiceId( service->getServiceGuid().c_str() );                
        }
        else
        {
            // Send service not found error message
            DxlMessageService::getInstance().sendServiceNotFoundErrorMessage( dxlRequest );
            return false;
        }
    }

    return true;
}

/** {@inheritDoc} */
bool ServiceLookupHandler::handleEvent( CoreMessageContext* context, DxlEvent* dxlEvent ) const
{
    // Get the service registry
    ServiceRegistry& serviceRegistry = ServiceRegistry::getInstance();

    // Attempt to find prefix for the event
    std::string prefix = serviceRegistry.getRequestPrefixForEvent( context->getTopic() );
    if( prefix.empty() )
    {
        char* wcTopic = CoreUtil::iterateWildcardBegin( context->getTopic() );
        while( CoreUtil::iterateWildcardNext( wcTopic ) )
        {
            prefix = serviceRegistry.getRequestPrefixForEvent( wcTopic );
            if( !prefix.empty() )
            {
                // Found a prefix, stop looking
                break;
            }
        }
        CoreUtil::iterateWildcardEnd( wcTopic );
    }

    // If we found a prefix, attempt to find a service
    if( !prefix.empty() )
    {
        const char* clientTenantGuid = dxlEvent->getSourceTenantGuid();
        std::string requestTopic = prefix + context->getTopic();

        serviceRegistrationPtr_t service = findServiceForTopic( requestTopic.c_str(), clientTenantGuid );

        if( service.get() )
        {
            // Create the response topic for the invoking client
            string responseTopic( DXL_CLIENT_PREFIX );
            responseTopic += dxlEvent->getSourceClientId();

            // Create request for event
            std::shared_ptr<DxlRequest> request = 
                DxlMessageService::getInstance().createRequest( dxlEvent->getMessageId(), responseTopic.c_str() );
            request->setSourceBrokerGuid( dxlEvent->getSourceBrokerGuid() );
            request->setSourceClientId( dxlEvent->getSourceClientId() );
            request->setSourceClientInstanceId( dxlEvent->getSourceClientInstanceId() );
            request->setSourceTenantGuid( dxlEvent->getSourceTenantGuid() );
            request->setDestinationBrokerGuid( service->getBrokerGuid().c_str() );
            request->setDestinationClientGuid( service->getClientInstanceGuid().c_str() );
            request->setDestinationServiceId( service->getServiceGuid().c_str() );

            const unsigned char* payload;
            size_t payloadLen;
            dxlEvent->getPayload( &payload, &payloadLen );
            request->setPayload( payload, payloadLen );

            // Send the request
            DxlMessageService::getInstance().sendMessage( requestTopic.c_str(), *(request.get()) );
        }
    }

    return true;
}

/** {@inheritDoc} */
bool ServiceLookupHandler::handleErrorResponse( 
    CoreMessageContext* /* context */, DxlErrorResponse* errorResponse ) const
{
    // If the fabric service is unavailable, remove from the service registry
    if( errorResponse->getErrorCode() == FABRICSERVICEUNAVAILABLE )
    {
        const char* serviceId = errorResponse->getDestinationServiceId();
        if( serviceId && ( strlen( serviceId ) > 0 ) )
        {
            // Unregister the service
            ServiceRegistry::getInstance().unregisterService( serviceId );
        }
    }

    return true;
}
