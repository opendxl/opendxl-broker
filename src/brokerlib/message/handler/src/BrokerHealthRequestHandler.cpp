/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/SimpleLog.h"
#include "json/include/JsonService.h"
#include "include/brokerlib.h"
#include "message/include/DxlMessageConstants.h"
#include "message/include/DxlMessageService.h"
#include "message/include/DxlRequest.h"
#include "message/handler/include/BrokerHealthRequestHandler.h"
#include "message/payload/include/BrokerHealthRequestPayload.h"
#include "message/payload/include/BrokerHealthResponsePayload.h"
#include "core/include/CoreBrokerHealth.h"
#include "core/include/CoreMessageHandlerService.h"
#include "util/include/BrokerLibThreadPool.h"
#include "include/BrokerSettings.h"
#include "serviceregistry/include/ServiceRegistry.h"
#include <unistd.h> // for usleep

using namespace std;
using namespace dxl::broker::json;
using namespace dxl::broker::message::handler;
using namespace dxl::broker::message::payload;
using namespace dxl::broker::core;
using namespace dxl::broker::service;
using namespace dxl::broker::util;

namespace dxl {
namespace broker {
namespace message {
namespace handler {

/** Runnable to send response runnable */
class HealthResponseRunner : public ThreadPool::Runnable
{
public:
    HealthResponseRunner( const string replyToTopic, const shared_ptr<DxlResponse> response ) :
        m_replyToTopic( replyToTopic ), m_response( response )
    {
    }

    /** Executes the runnable */
    virtual void run()
    {
        DxlMessageService& messageService = DxlMessageService::getInstance();

        // Get the broker health from core
        CoreBrokerHealth brokerHealth;
        getCoreInterface()->getBrokerHealth( brokerHealth );
        CoreMessageHandlerService& m_msvc = CoreMessageHandlerService::getInstance();
    
        brokerHealth.setIncomingMsgs( m_msvc.getPublishMessagesPerSecond() );
        brokerHealth.setOutgoingMsgs( m_msvc.getDestinationMessagesPerSecond() );
        brokerHealth.setLocalServicesCounter( ServiceRegistry::getInstance().getLocalSvcCounter() );

        Broker broker; 
        BrokerRegistry::getInstance().getBroker( BrokerSettings::getGuid(), broker );
        brokerHealth.setStartUpTime( broker.getStartTime() ); 

        // Set the payload
        BrokerHealthResponsePayload responsePayload( brokerHealth );
        m_response->setPayload( responsePayload );

        // Send the response
        messageService.sendMessage( m_replyToTopic.c_str(), *(m_response.get()) );
    }

private:
    /** The reply-to topic */
    const string m_replyToTopic;
    /** The response to send */
    const shared_ptr<DxlResponse> m_response;
};

}}}}

/** {@inheritDoc} */
bool BrokerHealthRequestHandler::onStoreMessage(
    CoreMessageContext* context, struct cert_hashes* /*certHashes*/ ) const
{
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "BrokerHealthRequestHandler::onStoreMessage" << SL_DEBUG_END;
    }

    // This request is broadcasted to all brokers so some will naturally have
    // no other broker as target but we don't want to send a "service not found"
    // message in that case
    context->setServiceNotFoundMessageEnabled( false );

    DxlMessageService& messageService = DxlMessageService::getInstance();
    
    // Create the response    
    DxlRequest* request = context->getDxlRequest();
    const shared_ptr<DxlResponse> response = messageService.createResponse( request );    

    // Add to thread pool for response
    BrokerLibThreadPool::getInstance().addWork( 
        std::shared_ptr<ThreadPool::Runnable>( new HealthResponseRunner( request->getReplyToTopic(), response ) ) );
    
    return true;
}
