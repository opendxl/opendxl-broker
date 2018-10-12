/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/include/DxlMessageConstants.h"
#include "message/payload/include/AbstractBrokerTopicEventPayload.h"
#include "util/include/StringUtil.h"

using namespace dxl::broker;
using namespace dxl::broker::message;
using namespace dxl::broker::message::payload;
using namespace dxl::broker::util;

using namespace Json;
using namespace std;

/** {@inheritDoc} */
void AbstractBrokerTopicEventPayload::setMessageHeaderValues( 
    DxlMessage& message, const BrokerState* brokerState )
{
    message.setOtherField(  
        DxlMessageConstants::PROP_START_TIME, 
        StringUtil::toString( brokerState->getBrokerStartTime() ) );
    message.setOtherField(  
        DxlMessageConstants::PROP_CHANGE_COUNT, 
        StringUtil::toString( brokerState->getTopicsChangeCount() ) );
}

/** {@inheritDoc} */
void AbstractBrokerTopicEventPayload::getMessageHeaderValues( 
        DxlMessage& message, uint32_t* brokerStartTime, uint32_t* subsChangeCount )
{
    *brokerStartTime = 0;
    *subsChangeCount = 0;

    string value;
    if( message.getOtherField( DxlMessageConstants::PROP_START_TIME, value ) )
    {
        *brokerStartTime = StringUtil::asUint32( value );
    }
    if( message.getOtherField( DxlMessageConstants::PROP_CHANGE_COUNT, value ) )
    {
        *subsChangeCount = StringUtil::asUint32( value );
    }
}
