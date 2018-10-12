/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef COREBRIDGECONFIGURATION_H_
#define COREBRIDGECONFIGURATION_H_

#include <stdint.h>
#include <string>
#include <vector>

namespace dxl {
namespace broker {
namespace core {

/** Forward reference to factory used to create configuration instances */
class CoreBridgeConfigurationFactory;

/**
 * Contains the bridge configuration for the broker. This includes the list of candidate brokers 
 * to bridge with, what type of bridging algorithm to utilize (round-robin vs. prioritized), etc.
 *
 * NOTE: This class is used as a data transfer object between the broker library and the core layer.
 * As such, it does not have dependencies on any other portions of the broker library. 
 */
class CoreBridgeConfiguration
{
    /** The factory is our friend */
    friend class dxl::broker::core::CoreBridgeConfigurationFactory;

public:

    /** Forward reference to the broker class */
    class Broker;            

    /** Destructor */
    virtual ~CoreBridgeConfiguration() {}

    /**
     * Returns the guid of the broker associated with the configuration
     * 
     * @return  The guid of the broker associated with the configuration
     */
    std::string getGuid() const { return m_guid; }

    /**
     * Returns the hostname of the broker associated with the configuration
     * 
     * @return  The hostname of the broker associated with the configuration
     */
    std::string getHostname() const { return m_hostName; }

    /**
     * Returns the IP address of the broker associated with the configuration
     * 
     * @return  The IP address of the broker associated with the configuration
     */
    std::string getIpAddress() const { return m_ipAddress; }

    /**
     * Returns the port of the broker associated with the configuration
     * 
     * @return  The port of the broker associated with the configuration
     */
    uint32_t getPort() const { return m_port; }

    /**
     * Sets the hub name for the broker associated with the configuration
     * 
     * @param   hubName The hub name for the broker associated with the configuration
     */
    void setHubName( const std::string& hubName ) { m_hubName = hubName; }    

    /**
     * Returns the hub name for the broker associated with the configuration
     * 
     * @return  The hub name for the broker associated with the configuration
     */
    std::string getHubName() const { return m_hubName; }

    /**
     * Returns whether this broker is configured to initiate a bridge with another broker
     *
     * @return  Whether this broker is configured to initiate a bridge with another broker
     */
    bool isEnabled() const { return !m_brokers.empty(); }

    /**
     * Whether we are going to round-robin the bridge candidates. Otherwise, we will treat the list
     * of brokers as a prioritized list (always attempt to connect to the first, fallback to the 
     * others).
     *
     * @return  Whether we are going to round-robin the bridge candidates
     */
    bool isRoundRobin() const { return m_primaryBrokerCount == 0; }

    /** 
     * The count of broker objects that represent the primary broker in the hub. Multiple 
     * "brokers" can be added to allow for hostnames versus IP address for the primary
     * broker.
     *
     * @return  The count of broker objects that represent the primary broker in the hub
     */
    uint32_t getPrimaryBrokerCount() { return m_primaryBrokerCount; }

    /** 
     * Returns the index of the broker to initially attempt to bridge to.
     *
     * @return  The index of the broker to initially attempt to bridge to.
     */
     uint16_t getInitialBrokerIndex() const;

    /**
     * The list of bridge candidate brokers (brokers to bridge to)
     *
     * @return  The list of bridge candidate brokers
     */
    std::vector<Broker> getBrokers() const { return m_brokers; }

    /**
     * Returns the count of brokers
     *
     * @return  The count of brokers
     */
    std::vector<Broker>::size_type getBrokerCount() const { return m_brokers.size(); }

    /**
     * Returns the identifier to be used within core to identify this bridge
     * configuration.
     *
     * @return  The identifier to be used within core to identify this bridge
     *          configuration.
     */
    std::string getBridgeId() const;

    /**
     * Sets information about broker associated with this bridge configuration
     *
     * @param   hostName The host name
     * @param   ipAddress The IP address
     * @param   port The port
     */
    void setBrokerProperties( const std::string& hostName, const std::string& ipAddress, uint32_t port );

    /**
     * Equal to operator
     *
     * @param   rhs The configuration to compare to
     * @return  Whether the two configurations are equal
     */
    bool operator==( const CoreBridgeConfiguration &rhs ) const;

    /**
     * Not equal to operator
     *
     * @param   rhs The configuration to compare to
     * @return  Whether the two configurations are not equal
     */
    inline bool operator!=( const CoreBridgeConfiguration &rhs ) const { return ! ( *this == rhs ); }

private:


    /** 
     * Constructor
     *
     * @param   guid The guid of the local broker
     */
    CoreBridgeConfiguration( const std::string& guid ) : m_guid( guid ), m_port( 0 ), m_primaryBrokerCount( 0 ) {};

    /** 
     * Sets the count of broker objects that represent the primary broker in the hub. Multiple
     * "brokers" can be added to allow for host names versus IP address for the primary
     * broker.
     *
     * @param   count The count of broker objects that represent the primary broker in the hub
     */
    void setPrimaryBrokerCount( uint32_t count ) { m_primaryBrokerCount = count; }

    /** 
     * Adds the specified broker to the list of bridge candidates
     *
     * @param   broker The broker to add to the list of bridge candidates
     */
    void addBroker( const Broker& broker ) { m_brokers.push_back( broker ); }

    /** The list of bridge candidates */
    std::vector<Broker> m_brokers;    

    /** The guid of the local broker */
    std::string m_guid;

    /** The port as reported via policy */
    uint32_t m_port;

    /** The host name as reported via policy */
    std::string m_hostName;

    /** The IP address as reported via policy */
    std::string m_ipAddress;

    /** The hub name */
    std::string m_hubName;

    /** 
     * The count of broker objects that represent the primary broker in the hub. Multiple 
     * "brokers" can be added to allow for host names versus IP address for the primary
     * broker. 
     */
    uint32_t m_primaryBrokerCount;

public:

    /**
     * Contains information regarding a broker to bridge to
     */
    class Broker
    {
    public:
        /**
         * Constructs the broker
         *
         * @param   guid The guid of the broker
         * @param   host The host of the broker
         * @param   port The port of the broker
         */
        Broker( const std::string& guid, const std::string& host, uint32_t port ) :
          m_guid( guid ), m_host( host ), m_port( port ) {}

        /**
         * Returns the guid of the broker
         * 
         * @return  The guid of the broker
         */
        std::string getGuid() const { return m_guid; }

        /**
         * Returns the host of the broker
         *
         * @return  The host of the broker
         */
        std::string getHost() const { return m_host; }

        /**
         * Returns the port of the broker
         *
         * @return  The port of the broker
         */
        int getPort() const { return m_port; }

        /**
         * Equal to operator
         *
         * @param   rhs The broker to compare to
         * @return  Whether the two brokers are equal
         */
        bool operator==( const Broker &rhs ) const;

        /**
         * Not equal to operator
         *
         * @param   rhs The broker to compare to
         * @return  Whether the two brokers are not equal
         */
        inline bool operator!=( const Broker &rhs ) const { return ! ( *this == rhs ); }

    private:
        /** The guid of the broker */
        std::string m_guid;
        /** The host of the broker */
        std::string m_host;
        /** The port of the broker */
        uint32_t m_port;
    };
};

} /* namespace core */
} /* namespace broker */
} /* namespace dxl */

#endif /* COREBRIDGECONFIGURATION_H_ */
