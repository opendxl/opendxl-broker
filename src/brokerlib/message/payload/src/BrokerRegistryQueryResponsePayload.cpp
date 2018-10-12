/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/BrokerSettings.h"
#include "message/include/DxlMessageConstants.h"
#include "message/payload/include/BrokerStateEventPayload.h"
#include "message/payload/include/BrokerRegistryQueryResponsePayload.h"

using namespace dxl::broker;
using namespace dxl::broker::message;
using namespace dxl::broker::message::payload;
using namespace Json;

/** {@inheritDoc} */
void BrokerRegistryQueryResponsePayload::write( Json::Value& out ) const
{
    Value brokers( Json::objectValue );
    for( auto it = m_states.begin(); it != m_states.end(); it++ )
    {
        Value jsonObject( Json::objectValue );
        BrokerStateEventPayload( *(*it) ).write( jsonObject );
        brokers[ (*it)->getBroker().getId() ] = jsonObject;
    }
    out[ DxlMessageConstants::PROP_BROKERS ] = brokers;
}
