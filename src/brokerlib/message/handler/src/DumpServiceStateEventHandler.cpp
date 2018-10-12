/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/SimpleLog.h"
#include "serviceregistry/include/ServiceRegistry.h"
#include "message/handler/include/DumpServiceStateEventHandler.h"
#include <sstream>

using namespace std;
using namespace dxl::broker::service;
using namespace dxl::broker::message::handler;

/** {@inheritDoc} */
bool DumpServiceStateEventHandler::onPublishMessage(
    const char* /*sourceId*/, const char* /*canonicalSourceId*/, bool /*isBridge*/, 
    uint8_t /*contextFlags*/, const char* /*topic*/, struct cert_hashes * /*certHashes*/ ) const
{
    if( SL_LOG.isInfoEnabled() )
    {
        stringstream dumpstr;
        dumpstr << ServiceRegistry::getInstance();
        SL_START << "\nService state:\n" << dumpstr.str() << SL_INFO_END;
    }

    // propagate event
    return true;
}
