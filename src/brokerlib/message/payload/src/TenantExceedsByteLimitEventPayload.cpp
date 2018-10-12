/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/include/DxlMessageConstants.h"
#include "message/payload/include/TenantExceedsByteLimitEventPayload.h"

using namespace std;
using namespace dxl::broker::message;
using namespace dxl::broker::message::payload;
using namespace Json;

/** {@inheritDoc} */
bool TenantExceedsByteLimitEventPayload::operator==( 
    const TenantExceedsByteLimitEventPayload& rhs ) const
{    
    return m_tenantId == rhs.m_tenantId;
}

/** {@inheritDoc} */
void TenantExceedsByteLimitEventPayload::write( Json::Value& out ) const
{
    out[ DxlMessageConstants::PROP_CLIENT_TENANT_GUID ] = m_tenantId;
}

/** {@inheritDoc} */
void TenantExceedsByteLimitEventPayload::read( const Json::Value& in )
{
    m_tenantId = in[ DxlMessageConstants::PROP_CLIENT_TENANT_GUID ].asString();
}
