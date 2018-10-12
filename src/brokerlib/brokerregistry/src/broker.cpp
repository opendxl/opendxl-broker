/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/BrokerSettings.h"
#include "brokerregistry/include/brokerregistry.h"
#include "brokerregistry/include/broker.h"

namespace dxl {
namespace broker {

/** {@inheritDoc} */
Broker::Broker( const std::string &brokerId, const std::string &hostname, uint32_t port,
    uint32_t ttl, uint32_t startTime, const std::string& policyHostname, 
    const std::string& policyIpAddress, const std::string& policyHubName, uint32_t policyPort,
    const std::string& brokerVersion,
    uint32_t connectionLimit,
    bool topicRoutingEnabled ) : BrokerBase( brokerId, hostname, port ), 
    m_ttl(ttl),
    m_startTime(startTime),
    m_policyHostname( policyHostname ),
    m_policyIpAddress( policyIpAddress ),
    m_policyHubName( policyHubName ),
    m_policyPort( policyPort ),
    m_brokerVersion(brokerVersion),
    m_connectionLimit( connectionLimit ),
    m_topicRoutingEnabled( topicRoutingEnabled )
{
}

/** {@inheritDoc} */
bool Broker::isLocalBroker() const
{
    return ( getId() == BrokerSettings::getGuid() );
}

/** {@inheritDoc} */
std::string Broker::getPolicyHostName() const
{
    return isLocalBroker() ? 
        BrokerRegistry::getInstance().getLocalBrokerHostname() : m_policyHostname;
}

/** {@inheritDoc} */
std::string Broker::getPolicyIpAddress() const
{
    return isLocalBroker() ? 
        BrokerRegistry::getInstance().getLocalBrokerIpAddress() : m_policyIpAddress;
}

/** {@inheritDoc} */
std::string Broker::getBrokerVersion() const 
{
    return isLocalBroker() ?
        BrokerRegistry::getInstance().getLocalBrokerVersion() : m_brokerVersion;
}

/** {@inheritDoc} */
std::string Broker::getPolicyHubName() const
{
    return isLocalBroker() ? 
        BrokerRegistry::getInstance().getLocalBrokerHub() : m_policyHubName;
}

/** {@inheritDoc} */
uint32_t Broker::getPolicyPort() const
{
    return isLocalBroker() ?
        BrokerRegistry::getInstance().getLocalBrokerPort() : m_policyPort;
}

/** {@inheritDoc} */
uint32_t Broker::getConnectionLimit() const
{
    return isLocalBroker() ? 
        BrokerRegistry::getInstance().getLocalBrokerConnectionLimit() : m_connectionLimit;
}

/** {@inheritDoc} */
bool Broker::isTopicRoutingEnabled() const
{
    return isLocalBroker() ? 
        BrokerRegistry::getInstance().isLocalBrokerTopicRoutingEnabled() : m_topicRoutingEnabled;
}

/** {@inheritDoc} */
std::ostream & operator <<( std::ostream &out, const Broker &broker )
{
    return out << 
        broker.getId() << ", " << 
        broker.getHostname() << ", " << 
        broker.getPort() << ", " << 
        broker.getTtl() << ", " << 
        broker.getStartTime() << ", " << 
        broker.getPolicyHostName() << ", " <<
        broker.getPolicyIpAddress() << ", " <<
        broker.getPolicyHubName() << ", " <<
        broker.getPolicyPort() << ", " << 
        broker.getBrokerVersion() << ", " <<
        broker.getConnectionLimit() << ", " <<
        broker.isTopicRoutingEnabled();
}

} 
} 
