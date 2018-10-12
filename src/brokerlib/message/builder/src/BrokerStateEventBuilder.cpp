/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/BrokerSettings.h"
#include "brokerregistry/include/brokerregistry.h"
#include "brokerregistry/include/brokerstate.h"
#include "message/builder/include/BrokerStateEventBuilder.h"
#include "message/payload/include/BrokerStateEventPayload.h"
#include "message/include/DxlMessageConstants.h"
#include "message/include/DxlMessageService.h"
#include <stdexcept>

using namespace dxl::broker;
using namespace dxl::broker::message;
using namespace dxl::broker::message::builder;
using namespace dxl::broker::message::payload;
using namespace std;

/** {@inheritDoc} */
const shared_ptr<DxlMessage> BrokerStateEventBuilder::buildMessage() const
{
    const BrokerState* brokerStatePtr =
        BrokerRegistry::getInstance().getBrokerStatePtr( BrokerSettings::getGuid() );

    if( !brokerStatePtr )
    {
        throw runtime_error( "Unable to find local broker in registry" );
    }

    DxlMessageService& messageService = DxlMessageService::getInstance();
    const shared_ptr<DxlEvent> changeEvent = messageService.createEvent();
    changeEvent->setPayload( BrokerStateEventPayload( *brokerStatePtr ) );
    return changeEvent;
}