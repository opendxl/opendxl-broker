/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/include/DxlMessageConstants.h"
#include "message/payload/include/ClientRegistryQueryRequestPayload.h"

using namespace dxl::broker;
using namespace dxl::broker::message;
using namespace dxl::broker::message::payload;
using namespace Json;

/** {@inheritDoc} */
void ClientRegistryQueryRequestPayload::read( const Json::Value& in )
{
    m_clientGuid = in[ DxlMessageConstants::PROP_CLIENT_GUID ].asString();
}

