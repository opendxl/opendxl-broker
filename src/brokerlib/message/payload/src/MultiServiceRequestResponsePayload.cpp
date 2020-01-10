/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/include/DxlMessageConstants.h"
#include "message/payload/include/MultiServiceRequestResponsePayload.h"

using namespace dxl::broker::message::payload;
using namespace Json;

/** {@inheritDoc} */
void MultiServiceRequestResponsePayload::write( Json::Value& out ) const
{
    Value requests( Json::objectValue );
    for( auto it = m_servicesByMessageId.begin(); it != m_servicesByMessageId.end(); it++ )
    {
        Value jsonObject( Json::objectValue );
        jsonObject[ DxlMessageConstants::PROP_SERVICE_GUID ] = it->second->getServiceGuid();
        requests[ it->first ] = jsonObject;
    }
    out[ DxlMessageConstants::PROP_REQUESTS] = requests;
}
