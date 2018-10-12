/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef _BROKER_H_
#define _BROKER_H_

#include "include/BrokerBase.h"
#include <string>
#include <iostream>

namespace dxl {
namespace broker {

namespace registry
{
    /** Default Time to Live */
    const uint32_t DEFAULTTTL = 60;        
}

/**
 * The Broker class is used to store information related to a broker.
 */
class Broker : public BrokerBase
{
    /** Allow registry to access private members */
    friend class BrokerRegistry;

public:

    /*! 
     * Constructs a broker
     *
     * @param   brokerId The broker's id.
     * @param   hostname The broker's host name.
     * @param   port The broker's port
     * @param   ttl Time to live value.
     * @param   startTime The start time of the broker
     * @param   policyHostname The host name of the broker as reported in policy
     * @param   policyIpAddress The IP address as reported in policy
     * @param   policyHubName The hub name as reported in policy
     * @param   policyPort The port as reported in policy
     * @param   brokerVersion The version of the broker
     * @param   connectionLimit The connection limit for the broker
     * @param   topicRoutingEnabled Whether topic routing is enabled
     */
    explicit Broker(
        const std::string& brokerId = "",
        const std::string& hostname = "",
        uint32_t port = DEFAULTPORT, 
        uint32_t ttl = registry::DEFAULTTTL, 
        uint32_t startTime = 0,
        const std::string& policyHostname = "",
        const std::string& policyIpAddress = "",
        const std::string& policyHubName = "",
        uint32_t policyPort = DEFAULTPORT,
        const std::string& brokerVersion = "",
        uint32_t connectionLimit = DEFAULTCONNLIMIT,
        bool topicRoutingEnabled = false );

    /**
     * Sets the time to live value 
     *
     * @param   ttl New time to live value.
     */
    inline void updateTtl( uint32_t ttl ) { m_ttl = ttl; }

    /** 
     * Returns the broker's time to live value 
     *
     * @return  The broker's time to live value
     */
    inline uint32_t getTtl() const { return m_ttl; }

    /**
     * Returns the broker's version
     *
     * @return  The broker's version
     */
    std::string getBrokerVersion() const;

    /**
     * Sets the broker's version
     *
     * @param   version The broker's version
     */
    void setBrokerVersion( const std::string& version ) { m_brokerVersion = version; }

    /**
     * Returns the broker's start time  
     *
     * @return  The broker's start time
     */
    inline uint32_t getStartTime() const { return m_startTime; }

    /**
     * Sets the broker's start time  
     *
     * @param   time The broker's start time
     */
    void setStartTime( uint32_t time ) { m_startTime = time; }

    /**
     * Returns the broker's host name as reported in policy 
     *
     * @return  The broker's host name as reported in policy
     */
    std::string getPolicyHostName() const;

    /**
     * Sets the broker's host name as reported in policy 
     *
     * @param   name The broker's host name as reported in policy
     */
    void setPolicyHostName( const std::string& name ) { m_policyHostname = name; }

    /**
     * Return the broker's IP address as reported in policy 
     *
     * @return  The broker's IP address as reported in policy
     */
    std::string getPolicyIpAddress() const;

    /**
     * Sets the broker's IP address as reported in policy 
     *
     * @param   address The broker's IP address as reported in policy
     */
    void setPolicyIpAddress( const std::string& address ) { m_policyIpAddress = address; }

    /**
     * Returns the broker's hub name as reported in policy 
     *
     * @return  The broker's hub name as reported in policy
     */
    std::string getPolicyHubName() const;

    /**
     * Sets the broker's hub name as reported in policy 
     *
     * @param   hubName The broker's hub name as reported in policy
     */
    void setPolicyHubName( const std::string hubName ) { m_policyHubName = hubName; }

    /**
     * Returns the broker's port as reported in policy
     *
     * @return  The broker's port as reported in policy
     */
    uint32_t getPolicyPort() const;

    /**
     * Sets the broker's port as reported in policy
     *
     * @param   port The broker's port as reported in policy
     */
    void setPolicyPort( uint32_t port ) { m_policyPort = port; }

    /**
     * Returns the broker's connection limit
     *
     * @return  The broker's connection limit
     */    
    uint32_t getConnectionLimit() const;

    /**
     * Sets the broker's connection limit
     *
     * @param   limit The broker's connection limit
     */    
    void setConnectionLimit( uint32_t limit ) { m_connectionLimit = limit; }
    
    /**
     * Returns whether this broker is the local broker
     *
     * @return  Whether this broker is the local broker
     */    
    bool isLocalBroker() const;
    
    /**
     * Sets the managing ePO name
     *
     * @param   name The managing ePO name
     */    
    void setManagingEpoName( const std::string& name ) { m_managingEpoName = name; }
    
    /**
     * Returns whether topic routing is enabled
     *
     * @return  Whether topic routing is enabled
     */    
    bool isTopicRoutingEnabled() const;

    /**
     * Sets whether topic routing is enabled
     *
     * @param   enabled Whether topic routing is enabled
     */    
    void setTopicRoutingEnabled( bool enabled ) { m_topicRoutingEnabled = enabled; }

    /** operator== */
    inline bool operator==(const Broker &rhs) const { return (BrokerBase::operator==(rhs) && (m_ttl == rhs.m_ttl)); }
    /** operator!= */
    inline bool operator!=(const Broker &rhs) const { return !(*this == rhs); }

private:
    /** Broker's time-to-live */
    uint32_t m_ttl;
    /** Broker's start time */
    uint32_t m_startTime;
    /** Broker's host name (*/
    std::string m_policyHostname;
    /** Broker's IP address */
    std::string m_policyIpAddress;
    /** Broker's Hub name */
    std::string m_policyHubName;
    /** Broker's port */
    uint32_t m_policyPort;
    /** Broker's version */
    std::string m_brokerVersion;
    /** Managing ePO name */
    std::string m_managingEpoName;
    /** Broker's connection limit */
    uint32_t m_connectionLimit;
    /** Whether topic-based routing is enabled */
    bool m_topicRoutingEnabled;
};

/** Print contents*/
std::ostream & operator <<( std::ostream &out, const Broker &broker );

} /* namespace broker */ 
} /* namespace dxl */ 

#endif
