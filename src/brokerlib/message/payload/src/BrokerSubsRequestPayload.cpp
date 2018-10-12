/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/include/DxlMessageConstants.h"
#include "message/payload/include/BrokerSubsRequestPayload.h"

using namespace dxl::broker::message::payload;

/** {@inheritDoc} */
void BrokerSubsRequestPayload::read( const Json::Value& in )
{
    m_topic = in[ DxlMessageConstants::PROP_TOPIC ].asString();    
}
