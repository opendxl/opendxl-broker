/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/include/DxlMessageConstants.h"
#include "message/payload/include/EventSubscriberNotFoundEventPayload.h"

using namespace dxl::broker::message::payload;
using namespace Json;

/** {@inheritDoc} */
void EventSubscriberNotFoundEventPayload::write( Json::Value& out ) const
{
    out[ DxlMessageConstants::PROP_TOPIC ] = m_topic;
}

/** {@inheritDoc} */
void EventSubscriberNotFoundEventPayload::read( const Json::Value& in )
{
    m_topic = in[ DxlMessageConstants::PROP_TOPIC ].asString();
}