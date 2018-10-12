/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/include/DxlMessageConstants.h"
#include "message/payload/include/BrokerHealthResponsePayload.h"

using namespace dxl::broker::message::payload;
using namespace Json;

/** {@inheritDoc} */
void BrokerHealthResponsePayload::write( Json::Value& out ) const
{
    out[ DxlMessageConstants::PROP_CONNECTED_CLIENTS ] = m_brokerHealth.getConnectedClients();
    out[ DxlMessageConstants::PROP_INCOMING_MSGS ] = static_cast<double>(m_brokerHealth.getIncomingMsgs());
    out[ DxlMessageConstants::PROP_OUTGOING_MSGS ] = static_cast<double>(m_brokerHealth.getOutgoingMsgs());
    out[ DxlMessageConstants::PROP_LOCAL_SVC_COUNTER ] = static_cast<Json::Value::UInt>(m_brokerHealth.getLocalServicesCounter());
    out[ DxlMessageConstants::PROP_START_TIME ] = static_cast<Json::Value::UInt>(m_brokerHealth.getStartUpTime());
}
