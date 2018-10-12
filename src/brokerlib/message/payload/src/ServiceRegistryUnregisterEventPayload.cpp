/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/include/DxlMessageConstants.h"
#include "message/payload/include/ServiceRegistryUnregisterEventPayload.h"

using namespace std;
using namespace dxl::broker::message;
using namespace dxl::broker::message::payload;
using namespace Json;

/** {@inheritDoc} */
bool ServiceRegistryUnregisterEventPayload::operator==( 
    const ServiceRegistryUnregisterEventPayload& rhs ) const
{    
    return m_serviceGuid == rhs.m_serviceGuid;
}

/** {@inheritDoc} */
void ServiceRegistryUnregisterEventPayload::write( Json::Value& out ) const
{
    out[ DxlMessageConstants::PROP_SERVICE_GUID ] = m_serviceGuid;
}

/** {@inheritDoc} */
void ServiceRegistryUnregisterEventPayload::read( const Json::Value& in )
{
    m_serviceGuid = in[ DxlMessageConstants::PROP_SERVICE_GUID ].asString();
}
