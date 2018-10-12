/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/include/DxlMessageConstants.h"
#include "message/payload/include/BrokerRegistryQueryRequestPayload.h"

using namespace dxl::broker;
using namespace dxl::broker::message;
using namespace dxl::broker::message::payload;
using namespace Json;

/** {@inheritDoc} */
void BrokerRegistryQueryRequestPayload::read( const Json::Value& in )
{
    m_brokerGuid = in[ DxlMessageConstants::PROP_BROKER_GUID ].asString();
}

