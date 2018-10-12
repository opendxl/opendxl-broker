/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/include/DxlMessageConstants.h"
#include "message/payload/include/ServiceRegistryQueryRequestPayload.h"

using namespace dxl::broker;
using namespace dxl::broker::message;
using namespace dxl::broker::message::payload;
using namespace Json;

/** {@inheritDoc} */
void ServiceRegistryQueryRequestPayload::read( const Json::Value& in )
{
    m_serviceGuid = in[ DxlMessageConstants::PROP_SERVICE_GUID ].asString();
    m_serviceType = in[ DxlMessageConstants::PROP_SERVICE_TYPE ].asString();
}

