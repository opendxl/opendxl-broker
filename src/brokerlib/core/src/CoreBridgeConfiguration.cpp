/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "core/include/CoreBridgeConfiguration.h"
#include "include/SimpleLog.h"
#include "brokerconfiguration/include/ConfigBroker.h"

using namespace std;
using namespace dxl::broker::core;

/** {@inheritDoc} */
string CoreBridgeConfiguration::getBridgeId() const
{
    string bridgeId;
    if( isEnabled() )
    {
        bridgeId.append( m_guid );
        for( auto it = m_brokers.begin(); it != m_brokers.end(); ++it ) 
        {
            bridgeId.append( ":" );
            bridgeId.append( it->getGuid() );
        }
    }

    return bridgeId;
}

/** {@inheritDoc} */
void CoreBridgeConfiguration::setBrokerProperties(
    const string& hostName, const string& ipAddress, uint32_t port )
{
    m_hostName = hostName;
    m_ipAddress = ipAddress;
    m_port = port;
}

/** {@inheritDoc} */
uint16_t CoreBridgeConfiguration::getInitialBrokerIndex() const
{
    auto brokerCount = getBrokerCount();
    // Randomize the initial bridge broker instance if we are round-robining
    return (uint16_t)(( isRoundRobin() && brokerCount > 1 ) ? rand() % brokerCount : 0);
}

/** {@inheritDoc} */
bool CoreBridgeConfiguration::operator==( const CoreBridgeConfiguration &rhs ) const
{ 
    return ( 
        ( m_guid == rhs.m_guid ) && 
        ( m_brokers == rhs.m_brokers ) && 
        ( m_primaryBrokerCount == rhs.m_primaryBrokerCount ) 
    ); 
}

/** {@inheritDoc} */
bool CoreBridgeConfiguration::Broker::operator==( const Broker &rhs ) const
{ 
    return ( 
        ( m_guid == rhs.m_guid ) && 
        ( m_host == rhs.m_host ) && 
        ( m_port == rhs.m_port )
    ); 
}
