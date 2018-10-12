/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/BrokerSettings.h"
#include "include/SimpleLog.h"
#include "message/include/DxlMessageService.h"
#include "message/handler/include/BrokerDisableTestModeRequestHandler.h"
#include "DxlFlags.h"

using namespace std;
using namespace dxl::broker::json;
using namespace dxl::broker::message::handler;
using namespace dxl::broker::core;

/** {@inheritDoc} */
bool BrokerDisableTestModeRequestHandler::onStoreMessage(
    CoreMessageContext* context, struct cert_hashes* /*certHashes*/ ) const
{
    if( SL_LOG.isInfoEnabled() )
    {
        SL_START << "BrokerDisableTestModeRequestHandler::onStoreMessage" << SL_INFO_END;
    }

    BrokerSettings::setTestModeEnabled( false );

    // Send an empty response
    DxlMessageService& messageService = DxlMessageService::getInstance();
    DxlRequest* request = context->getDxlRequest();
    const shared_ptr<DxlResponse> response = messageService.createResponse( request );    
    messageService.sendMessage( request->getReplyToTopic(), *(response.get()) );

    // Do not propagate the request, we send an event to the other brokers
    return false;
}
