/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/SimpleLog.h"
#include "message/include/DxlMessageConstants.h"
#include "message/payload/include/BrokerStateEventPayload.h"

using namespace dxl::broker::message;
using namespace dxl::broker::message::payload;
using namespace Json;

/** {@inheritDoc} */
bool BrokerStateEventPayload::operator==( const BrokerStateEventPayload& rhs ) const
{
    return 
        m_brokerGuid == rhs.m_brokerGuid &&
        m_brokerHostname == rhs.m_brokerHostname &&
        m_brokerPort == rhs.m_brokerPort &&
        m_brokerTtlMins == rhs.m_brokerTtlMins &&
        m_connections == rhs.m_connections &&     
        m_childConnections == rhs.m_childConnections &&
        m_startTime == rhs.m_startTime &&
        m_brokerPolicyHostname == rhs.m_brokerPolicyHostname &&
        m_brokerPolicyIpAddress == rhs.m_brokerPolicyIpAddress &&
        m_brokerPolicyHubName == rhs.m_brokerPolicyHubName &&
        m_brokerPolicyPort == rhs.m_brokerPolicyPort &&
        m_brokerVersion == rhs.m_brokerVersion &&
        m_connectionLimit == rhs.m_connectionLimit &&
        m_topicRoutingEnabled == rhs.m_topicRoutingEnabled;
}

/** {@inheritDoc} */
void BrokerStateEventPayload::write( Json::Value& out ) const
{
    out[ DxlMessageConstants::PROP_GUID ] = m_brokerGuid;
    out[ DxlMessageConstants::PROP_HOSTNAME ] = m_brokerHostname;
    out[ DxlMessageConstants::PROP_PORT ] = m_brokerPort;
    out[ DxlMessageConstants::PROP_WEBSOCKET_PORT ] = m_brokerWebSocketPort;
    out[ DxlMessageConstants::PROP_TTL_MINS ] = m_brokerTtlMins;
    out[ DxlMessageConstants::PROP_BROKER_VERSION] = m_brokerVersion;
    out[ DxlMessageConstants::PROP_START_TIME ] = (UInt)m_startTime;
    out[ DxlMessageConstants::PROP_POLICY_HOSTNAME ] = m_brokerPolicyHostname;
    out[ DxlMessageConstants::PROP_POLICY_IP_ADDRESS ] = m_brokerPolicyIpAddress;
    out[ DxlMessageConstants::PROP_POLICY_HUB ] = m_brokerPolicyHubName;
    out[ DxlMessageConstants::PROP_POLICY_PORT ] = m_brokerPolicyPort;
    out[ DxlMessageConstants::PROP_CONNECTION_LIMIT ] = m_connectionLimit;
    out[ DxlMessageConstants::PROP_TOPIC_ROUTING ] = m_topicRoutingEnabled;
    Value connections( arrayValue );
    for( auto it = m_connections.begin(); it != m_connections.end(); ++it )
    {
        connections.append( *it );
    }
    out[ DxlMessageConstants::PROP_BRIDGES ] = connections; 
    Value childConnections( arrayValue );
    for( auto it = m_childConnections.begin(); it != m_childConnections.end(); ++it )
    {
        childConnections.append( *it );
    }
    out[ DxlMessageConstants::PROP_BRIDGE_CHILDREN ] = childConnections;
}

/** {@inheritDoc} */
void BrokerStateEventPayload::read( const Json::Value& in )
{
    m_brokerGuid = in[ DxlMessageConstants::PROP_GUID ].asString();
    m_brokerHostname = in[ DxlMessageConstants::PROP_HOSTNAME ].asString();
    m_brokerPort = in[ DxlMessageConstants::PROP_PORT ].asUInt();
    m_brokerWebSocketPort = in[ DxlMessageConstants::PROP_WEBSOCKET_PORT ].asUInt();
    m_brokerTtlMins = in[ DxlMessageConstants::PROP_TTL_MINS ].asUInt();
    m_startTime = in[ DxlMessageConstants::PROP_START_TIME ].asUInt();
    m_brokerPolicyHostname = in[ DxlMessageConstants::PROP_POLICY_HOSTNAME ].asString();
    m_brokerPolicyIpAddress = in[ DxlMessageConstants::PROP_POLICY_IP_ADDRESS ].asString();
    m_brokerPolicyHubName = in[ DxlMessageConstants::PROP_POLICY_HUB ].asString();
    m_brokerPolicyPort = in[ DxlMessageConstants::PROP_POLICY_PORT ].asUInt();
    m_brokerVersion = in[ DxlMessageConstants::PROP_BROKER_VERSION].asString();
    m_connectionLimit = in[ DxlMessageConstants::PROP_CONNECTION_LIMIT ].asUInt();
    m_topicRoutingEnabled = in[ DxlMessageConstants::PROP_TOPIC_ROUTING ].asBool();    

    Json::Value connections = in[ DxlMessageConstants::PROP_BRIDGES ];
    for( Value::iterator itr = connections.begin(); itr != connections.end(); itr++ )
    {
        m_connections.insert( (*itr).asString() );
    }
    Json::Value childConnections = in[ DxlMessageConstants::PROP_BRIDGE_CHILDREN ];
    for( Value::iterator itr = childConnections.begin(); itr != childConnections.end(); itr++ )
    {
        m_childConnections.insert( (*itr).asString() );
    }
}
