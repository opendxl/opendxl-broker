/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/include/DxlMessageConstants.h"
#include "message/payload/include/BrokerSubsResponsePayload.h"

using namespace dxl::broker::message::payload;
using namespace Json;

/** {@inheritDoc} */
void BrokerSubsResponsePayload::write( Json::Value& out ) const
{
    out[ DxlMessageConstants::PROP_COUNT ] = m_subCount;
}
