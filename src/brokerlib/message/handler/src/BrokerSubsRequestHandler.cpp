/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/SimpleLog.h"
#include "json/include/JsonService.h"
#include "include/brokerlib.h"
#include "message/include/DxlMessageService.h"
#include "message/include/DxlRequest.h"
#include "message/handler/include/BrokerSubsRequestHandler.h"
#include "message/payload/include/BrokerSubsRequestPayload.h"
#include "message/payload/include/BrokerSubsResponsePayload.h"
#include "core/include/CoreBrokerHealth.h"
#include "core/include/CoreMessageHandlerService.h"
#include "include/BrokerSettings.h"
#include "serviceregistry/include/ServiceRegistry.h"

using namespace std;
using namespace dxl::broker::json;
using namespace dxl::broker::message::handler;
using namespace dxl::broker::message::payload;
using namespace dxl::broker::core;
using namespace dxl::broker::service;

/** {@inheritDoc} */
bool BrokerSubsRequestHandler::onStoreMessage(
    CoreMessageContext* context, struct cert_hashes* /*certHashes*/ ) const
{
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "BrokerSubsRequestHandler::onStoreMessage" << SL_DEBUG_END;
    }

    // Get the DXL request
    DxlRequest* request = context->getDxlRequest();

    // This request is broadcasted to all brokers so some will naturally have
    // no other broker as target but we don't want to send a "service not found"
    // message in that case
    context->setServiceNotFoundMessageEnabled( false );

    DxlMessageService& messageService = DxlMessageService::getInstance();

    BrokerSubsRequestPayload requestPayload;
    JsonService::getInstance().fromJson( request->getPayloadStr(), requestPayload );
    const std::string topic = requestPayload.getTopic();

    if( !topic.empty() )
    {
        // Create the response    
        DxlRequest* request = context->getDxlRequest();
        const shared_ptr<DxlResponse> response = messageService.createResponse( request );    

        uint32_t subCount = 0;
        if( BrokerSettings::isMultiTenantModeEnabled() )
        {
            auto destTenantGuids = context->isSourceOps() ? request->getDestinationTenantGuids() : NULL;
            if( destTenantGuids )
            {
                for( auto iter = destTenantGuids->begin(); iter != destTenantGuids->end(); iter++ )
                {
                    subCount +=                     
                        getCoreInterface()->getSubscriptionCount( topic, iter->c_str() );
                }
            }
            else
            {
                subCount = 
                    getCoreInterface()->getSubscriptionCount(
                        topic, request->getSourceTenantGuid() );
            }
        }
        else
        {
            // Get the subscription count from core
            subCount = getCoreInterface()->getSubscriptionCount( topic, NULL );
        }
    
        // Set the payload
        BrokerSubsResponsePayload responsePayload( subCount );
        response->setPayload( responsePayload );

        // Send the response
        messageService.sendMessage( request->getReplyToTopic(), *(response.get()) );
    }    

    return true;
}
