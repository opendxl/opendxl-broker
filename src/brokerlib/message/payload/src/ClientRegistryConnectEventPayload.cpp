/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/include/DxlMessageConstants.h"
#include "message/payload/include/ClientRegistryConnectEventPayload.h"

using namespace dxl::broker::message::payload;

/** {@inheritDoc} */
void ClientRegistryConnectEventPayload::write( Json::Value& out ) const
{
    out[ DxlMessageConstants::PROP_CLIENT_GUID ] = m_clientId;
}

/** {@inheritDoc} */
void ClientRegistryConnectEventPayload::read( const Json::Value& in )
{
    m_clientId = in[ DxlMessageConstants::PROP_CLIENT_GUID ].asString();
}
