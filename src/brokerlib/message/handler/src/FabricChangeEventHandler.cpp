/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/SimpleLog.h"
#include "include/brokerlib.h"
#include "message/builder/include/BrokerStateEventBuilder.h"
#include "message/handler/include/FabricChangeEventHandler.h"
#include "message/include/DxlMessageConstants.h"
#include "message/include/DxlMessageService.h"
#include "serviceregistry/include/ServiceRegistry.h"

using namespace std;
using namespace dxl::broker::message;
using namespace dxl::broker::message::builder;
using namespace dxl::broker::message::handler;
using namespace dxl::broker::core;
using namespace dxl::broker::service;

/** {@inheritDoc} */
bool FabricChangeEventHandler::onStoreMessage(
        CoreMessageContext* /*context*/, struct cert_hashes* /*certHashes*/ ) const
{
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "FabricChangeEventHandler::onStoreMessage" << SL_DEBUG_END;
    }

    // Send the broker state event
    getCoreInterface()->sendLocalBrokerStateEvent();

    // Re-register local services
    ServiceRegistry::getInstance().sendServiceRegistrationEvents();

    // propagate event
    return true;
}
