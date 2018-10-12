/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/BrokerSettings.h"
#include "message/include/DxlMessageConstants.h"
#include "message/payload/include/BrokerTopicQueryResponsePayload.h"

using namespace dxl::broker;
using namespace dxl::broker::message;
using namespace dxl::broker::message::payload;
using namespace Json;

/** {@inheritDoc} */
void BrokerTopicQueryResponsePayload::write( Json::Value& out ) const
{
    out[ DxlMessageConstants::PROP_COUNT ] = m_topicCount;
    out[ DxlMessageConstants::PROP_EXISTS ] = m_topicsExist;
}

