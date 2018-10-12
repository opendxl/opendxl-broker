/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/SimpleLog.h"
#include "message/handler/include/TenantLimitResetEventHandler.h"
#include "metrics/include/TenantMetricsService.h"
#include "core/include/CoreMessageContext.h"

using namespace std;
using namespace dxl::broker::message::handler;
using namespace dxl::broker::metrics;
using namespace dxl::broker::core;

/** {@inheritDoc} */
bool TenantLimitResetEventHandler::onStoreMessage(
    CoreMessageContext* /*context*/, struct cert_hashes* /*certHashes*/ ) const
{
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "TenantLimitResetEventHandler::onStoreMessage" << SL_DEBUG_END;
    }

    // Reset tenant limits
    TenantMetricsService::getInstance().resetTenantByteCounts();

    // propagate event
    return true;
}

