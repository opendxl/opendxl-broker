/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/include/DxlMessageConstants.h"
#include "message/payload/include/TenantExceedsLimitEventPayload.h"

using namespace std;
using namespace dxl::broker::message;
using namespace dxl::broker::message::payload;
using namespace Json;

// Value constants for the type
const char* TenantExceedsLimitEventPayload::TENANT_LIMIT_BYTE = "byte";
const char* TenantExceedsLimitEventPayload::TENANT_LIMIT_SUBSCRIPTIONS = "subscriptions";
const char* TenantExceedsLimitEventPayload::TENANT_LIMIT_SERVICES = "services";
const char* TenantExceedsLimitEventPayload::TENANT_LIMIT_CONNECTIONS = "connections";

/** {@inheritDoc} */
bool TenantExceedsLimitEventPayload::operator==(
    const TenantExceedsLimitEventPayload& rhs ) const
{    
    return ( m_tenantId == rhs.m_tenantId && m_type == rhs.m_type );
}

/** {@inheritDoc} */
void TenantExceedsLimitEventPayload::write( Json::Value& out ) const
{
    out[ DxlMessageConstants::PROP_CLIENT_TENANT_GUID ] = m_tenantId;
    out[ DxlMessageConstants::PROP_TENANT_LIMIT_TYPE ] = m_type;
}

/** {@inheritDoc} */
void TenantExceedsLimitEventPayload::read( const Json::Value& in )
{
    m_tenantId = in[ DxlMessageConstants::PROP_CLIENT_TENANT_GUID ].asString();
    m_type = in[ DxlMessageConstants::PROP_TENANT_LIMIT_TYPE ].asString();
}
