/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/SimpleLog.h"
#include "json/include/JsonService.h"
#include "message/handler/include/TenantExceedsByteLimitEventHandler.h"
#include "message/payload/include/TenantExceedsByteLimitEventPayload.h"
#include "metrics/include/TenantMetricsService.h"
#include "core/include/CoreMessageContext.h"

using namespace std;
using namespace dxl::broker::json;
using namespace dxl::broker::message::handler;
using namespace dxl::broker::message::payload;
using namespace dxl::broker::metrics;
using namespace dxl::broker::core;

/** {@inheritDoc} */
bool TenantExceedsByteLimitEventHandler::onStoreMessage(
    CoreMessageContext* context, struct cert_hashes* /*certHashes*/ ) const
{
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "TenantExceedsByteLimitEventHandler::onStoreMessage" << SL_DEBUG_END;
    }

    // Only process the event if we are not the broker that sent it
    if( !context->isLocalBrokerSource() )
    {
        // Get the DXL event
        DxlEvent* evt = context->getDxlEvent();

        // Get the payload
        TenantExceedsByteLimitEventPayload eventPayload;
        JsonService::getInstance().fromJson( evt->getPayloadStr(), eventPayload );

        // Mark tenant as exceeding payload
        TenantMetricsService::getInstance().markTenantExceedsByteCount( 
            eventPayload.getTenantId().c_str() );
    }

    // propagate event
    return true;
}

