/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/include/DxlMessageConstants.h"
#include "message/payload/include/BrokerTopicEventPayload.h"

using namespace dxl::broker::message::payload;
using namespace Json;

/** {@inheritDoc} */
void BrokerTopicEventPayload::write( Json::Value& out ) const
{
    out[ DxlMessageConstants::PROP_TOPIC ] = m_topic;
}

/** {@inheritDoc} */
void BrokerTopicEventPayload::read( const Json::Value& in )
{
    m_topic = in[ DxlMessageConstants::PROP_TOPIC ].asString();
}