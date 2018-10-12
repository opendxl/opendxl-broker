/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef CONFIGBROKER_H_
#define CONFIGBROKER_H_

#include <memory>
#include <string>
#include "include/BrokerBase.h"
#include "brokerconfiguration/include/ConfigNode.h"

namespace dxl {
namespace broker {

/**
 * A broker as specified in the fabric configuration 
 */
class ConfigBroker : public BrokerBase, public ConfigNode
{
    /** Hub as a friend */
    friend class Hub;

public:
    /**
     * Constructs a configuration broker
     *
     * @param   brokerId The broker identifier
     * @param   hostname The broker host name
     * @param   port The broker port
     * @param   serviceZone A service zone associated with the broker (if applicable)
     * @param   parentId The parent node identifier
     * @param   ipAddress The IP address of the broker
     */
    ConfigBroker( const std::string &brokerId, const std::string &hostname,
        uint32_t port, const std::string& serviceZone,
        const std::string& parentId, const std::string& ipAddress );

    /** Destructor */
    virtual ~ConfigBroker();

    /**
     * Returns a service zone associated with the broker (if applicable)
     * 
     * @return  A service zone associated with the broker (if applicable)
     */
    const std::string getServiceZone() const;

    /**
     * Returns the IP address for the broker
     *
     * @return  The IP address for the broker
     */
    const std::string getIpAddress() const;

    /** {@inheritDoc} */
    const std::string getParentId() const;

    /** {@inheritDoc} */
    bool isHub() const;

    /** {@inheritDoc} */
    const std::string getId() const { return BrokerBase::getId(); }

    /** {@inheritDoc} */
    std::string toString() const;

    /** {@inheritDoc} */
    ServiceZoneList getServiceZoneList() const { return m_serviceZones; }

    /** Equality */
    bool operator==( const ConfigBroker& rhs ) const;

    /** Inequality */
    bool operator!=( const ConfigBroker& rhs ) const;

private:  
    /** {@inheritDoc} */
    const ServiceZoneList& generateServiceZoneList( const BrokerConfiguration& brokerConfig );

    /** Whether service zones have been calculated */
    bool m_calculatedServiceZones;
    /** The parent node identifier */
    std::string m_parentId;
    /** A service zone associated with the broker (if applicable) */
    std::string m_serviceZone;
    /** IP address for the broker */
    std::string m_ipAddress;
    /** The calculated service zones for this broker */
    ServiceZoneList m_serviceZones;
};

}  // namespace broker
}  // namespace dxl

#endif  // CONFIGBROKER_H_
