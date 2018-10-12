/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/include/DxlMessageConstants.h"
#include "message/payload/include/ServiceRegistryQueryResponsePayload.h"
#include "message/payload/include/ServiceRegistryQueryResponseServicePayload.h"

using namespace dxl::broker;
using namespace dxl::broker::message;
using namespace dxl::broker::message::payload;
using namespace Json;

/** {@inheritDoc} */
void ServiceRegistryQueryResponsePayload::write( Json::Value& out ) const
{
    Value registrations( Json::objectValue );
    for( auto it = m_registrations.begin(); it != m_registrations.end(); it++ )
    {
        Value jsonObject( Json::objectValue );
        ServiceRegistryQueryResponseServicePayload( **it ).write( jsonObject );
        registrations[ (*it)->getServiceGuid() ] = jsonObject;
    }
    out[ DxlMessageConstants::PROP_SERVICES] = registrations;
}

