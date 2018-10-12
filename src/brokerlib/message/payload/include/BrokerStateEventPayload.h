/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERSTATEEVENTPAYLOAD_H_
#define BROKERSTATEEVENTPAYLOAD_H_

#include <cstdint>
#include "include/SimpleLog.h"
#include "json/include/JsonReader.h"
#include "json/include/JsonWriter.h"
#include "brokerregistry/include/brokerstate.h"
#include "include/unordered_set.h"

namespace dxl {
namespace broker {
namespace message {
/** Namespace for payloads used in DXL messages */
namespace payload {

/**
 * Payload for a "BrokerStateEvent" message
 */
class BrokerStateEventPayload : 
    public dxl::broker::json::JsonReader,
    public dxl::broker::json::JsonWriter
{
public:
    /** 
     * Constructor
     *
     * @param   guid The broker guid
     * @param   hostname The broker hostname
     * @param   port The broker port
     * @param   ttlMins The broker TTL (minutes)
     * @param   connections The brokers current connections
     * @param   childConnections The brokers current child connections
     * @param   startTime The start time of the broker
     * @param   policyHostname The host name (via policy)
     * @param   policyIpAddress  The ip address (via policy)
     * @param   policyHubName The hub name (via policy)
     * @param   policyPort The port (via policy)
     * @param   brokerVersion The broker version (via Broker helpers)
     * @param   managingEpoName The name of the managing ePO
     * @param   connectionLimit The broker connection limit
     * @param   topicRoutingEnabled Whether topic-based routing is enabled
     */
    explicit BrokerStateEventPayload( 
        const std::string& guid = "", const std::string& hostname = "",
        uint32_t port = 0, uint32_t ttlMins = 0, 
        const unordered_set<std::string> connections = unordered_set<std::string>(), 
        const unordered_set<std::string> childConnections = unordered_set<std::string>(), 
        uint32_t startTime = 0, const std::string& policyHostname = "", 
        const std::string& policyIpAddress = "", const std::string policyHubName = "",
        uint32_t policyPort = 0, const std::string& brokerVersion = "",
        const std::string managingEpoName = "", uint32_t connectionLimit = 0,
        bool topicRoutingEnabled = false ) :
        m_brokerGuid( guid ), m_brokerHostname( hostname ), m_brokerPort( port ), 
        m_brokerTtlMins( ttlMins ), m_connections( connections ), 
        m_childConnections( childConnections ),
        m_startTime( startTime ),
        m_brokerPolicyHostname( policyHostname ),
        m_brokerPolicyIpAddress( policyIpAddress ),
        m_brokerPolicyHubName( policyHubName ),
        m_brokerPolicyPort( policyPort ),
        m_brokerVersion(brokerVersion),
        m_managingEpoName( managingEpoName ),
        m_connectionLimit( connectionLimit ),
        m_topicRoutingEnabled( topicRoutingEnabled ) {};     

    /** 
     * Constructor
     *
     * @param   registryBroker The state of a broker from the registry
     */
    explicit BrokerStateEventPayload(
        const dxl::broker::BrokerState& registryBroker ) :
        m_brokerGuid( registryBroker.getBroker().getId() ),  
        m_brokerHostname( registryBroker.getBroker().getHostname() ), 
        m_brokerPort( registryBroker.getBroker().getPort() ), 
        m_brokerTtlMins( registryBroker.getBroker().getTtl() ), 
        m_connections( registryBroker.getConnections() ),
        m_childConnections( registryBroker.getChildConnections() ),
        m_startTime( registryBroker.getBroker().getStartTime() ),
        m_brokerPolicyHostname( registryBroker.getBroker().getPolicyHostName() ),
        m_brokerPolicyIpAddress( registryBroker.getBroker().getPolicyIpAddress() ),
        m_brokerPolicyHubName( registryBroker.getBroker().getPolicyHubName() ),
        m_brokerPolicyPort( registryBroker.getBroker().getPolicyPort() ),
        m_brokerVersion( registryBroker.getBroker().getBrokerVersion() ),
        m_connectionLimit( registryBroker.getBroker().getConnectionLimit() ),
        m_topicRoutingEnabled( registryBroker.getBroker().isTopicRoutingEnabled() ) {}

    /** Destructor */
    virtual ~BrokerStateEventPayload() {}

    /**
     * Returns the broker GUID 
     *
     * @return  The broker GUID
     */
    std::string getGuid() const { return m_brokerGuid; }

    /**
     * Returns the broker hostname 
     *
     * @return  The broker hostname
     */
    std::string getHostname() const { return m_brokerHostname; }

    /**
     * Returns the broker port 
     *
     * @return  The broker port
     */
    uint32_t getPort() const { return m_brokerPort; }

    /**
     * Returns the broker TTL (in minutes)
     *
     * @return  The broker TTL (in minutes)
     */
    uint32_t getTtlMins() const { return m_brokerTtlMins; }

    /**
     * Returns the broker start time 
     *
     * @return  The broker start time
     */
    uint32_t getStartTime() const { return m_startTime; }

    /**
     * Returns the broker host name (via policy)
     *
     * @return  The broker host name (via policy)
     */
    std::string getPolicyHostname() const { return m_brokerPolicyHostname; }

    /**
     * Returns the broker ip address (via policy)
     *
     * @return  The broker ip address (via policy)
     */
    std::string getPolicyIpAddress() const { return m_brokerPolicyIpAddress; }

    /**
     * Returns the broker hub name (via policy)
     *
     * @return  The broker hub name (via policy)
     */
    std::string getPolicyHubName() const { return m_brokerPolicyHubName; }

    /**
     * Returns the broker port (via policy)
     *
     * @return  The broker port (via policy)
     */
    uint32_t getPolicyPort() const { return m_brokerPolicyPort; }

    /**
     * Returns the broker version
     *
     * @return  The broker version
     */
    std::string getBrokerVersion() const { return m_brokerVersion; }


    /**
     * Returns the broker connection limit
     *
     * @return  The broker connection limit
     */
    uint32_t getConnectionLimit() const { return m_connectionLimit; }

    /**
     * Returns whether topic routing is enabled
     *
     * @return  Whether topic routing is enabled
     */
    bool isTopicRoutingEnabled() const { return m_topicRoutingEnabled; }

    /**
     * Returns the broker's connections
     *
     * @return  The broker's connections
     */
    const unordered_set<std::string> getConnections() const { return m_connections; }

    /**
     * Returns the broker's child connections
     *
     * @return  The broker's child connections
     */
    const unordered_set<std::string> getChildConnections() const { return m_childConnections; }

    /** {@inheritDoc} */
    void read( const Json::Value& in );

    /** {@inheritDoc} */
    void write( Json::Value& out ) const;    

    /** Equals operator */
    bool operator==( const BrokerStateEventPayload& rhs ) const;

    /** Not equals operator */
    bool operator!= (const BrokerStateEventPayload& rhs ) const {
        return !( *this == rhs );
    }

private:
    /** The broker GUID */
    std::string m_brokerGuid;
    /** The broker host name */
    std::string m_brokerHostname;
    /** The broker port */
    uint32_t m_brokerPort;
    /** The broker TTL */
    uint32_t m_brokerTtlMins;
    /** The broker's connections */
    unordered_set<std::string> m_connections;
    /** The broker's child connections */
    unordered_set<std::string> m_childConnections;
    /** The broker start time */
    uint32_t m_startTime;
    /** The broker host name (via policy) */
    std::string m_brokerPolicyHostname;
    /** The broker IP address (via policy) */
    std::string m_brokerPolicyIpAddress;
    /** The broker hub name (via policy) */
    std::string m_brokerPolicyHubName;
    /** The broker port (via policy) */
    uint32_t m_brokerPolicyPort;
    /** The broker version (via broker helpers) */
    std::string m_brokerVersion;
    /** The managing ePO name */
    std::string m_managingEpoName;
    /** The broker connection limit */
    uint32_t m_connectionLimit;
    /** Whether topic-based routing is enabled */
    bool m_topicRoutingEnabled;
};

} /* namespace payload */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* BROKERSTATEEVENTPAYLOAD_H_ */
