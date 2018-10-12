/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef HUB_H_
#define HUB_H_

#include <functional>
#include <string>
#include "brokerconfiguration/include/ConfigNode.h"

namespace dxl {
namespace broker {

/**
 * A hub as specified in the fabric configuration 
 */
class Hub : public ConfigNode
{
    /** Config broker as a friend */
    friend class ConfigBroker;

public:
    /**
     * Function used to determine if a hub contains a broker with the specified identifier
     */
    struct PtrHubContainsId : public std::binary_function< std::shared_ptr<Hub>, std::string, bool >
    {
        /** 
         * Returns true if the hub contains a broker with the specified identifier
         *
         * @param   lhs The hub pointer
         * @param   rhs The broker identifier
         * @return  True if the hub contains a broker with the specified identifier
         */
        bool operator() ( const std::shared_ptr<Hub> lhs, const std::string& rhs ) const;
    };

public:
    /**
     * Constructs the hub
     *
     * @param   id The hub identifier
     * @param   primaryBrokerId The primary broker identifier
     * @param   secondaryBrokerId The secondary broker identifier
     * @param   parentId The parent node identifier
     * @param   serviceZone A service zone associated with the hub (if applicable)
     * @param   name The name of the hub
     */
    Hub( const std::string& id, const std::string& primaryBrokerId,
         const std::string& secondaryBrokerId, const std::string& parentId,
         const std::string& serviceZone, const std::string& name );

    /** Destructor */
    virtual ~Hub();

    /** {@inheritDoc} */
    const std::string getId() const;

    /** {@inheritDoc} */
    virtual const std::string getParentId() const;

    /** {@inheritDoc} */
    virtual bool isHub() const;

    /** {@inheritDoc} */
    std::string toString() const;  

    /**
     * The primary broker identifier
     *
     * @return  The primary broker identifier
     */
    const std::string getPrimaryBrokerId() const;

    /**
     * The secondary broker identifier
     *
     * @return  The secondary broker identifier
     */
    const std::string getSecondaryBrokerId() const;

    /**
     * Returns a service zone associated with the broker (if applicable)
     * 
     * @return  A service zone associated with the broker (if applicable)
     */
    const std::string getServiceZone() const;

    /**
     * The name of the hub
     *
     * @return  The name of the hub
     */
    const std::string getName() const;

    /** Equality */
    bool operator==( const std::string& rhs ) const;

    /** Equality */
    bool operator==( const Hub& rhs ) const;

    /** Inequality */
    bool operator!=( const Hub& rhs ) const;

    /** {@inheritDoc} */
    ServiceZoneList getServiceZoneList() const { return m_serviceZones; }

private:  
    /** {@inheritDoc} */
    const ServiceZoneList& generateServiceZoneList( const BrokerConfiguration& brokerConfig );

    /** Whether service zones have been calculated */
    bool m_calculatedServiceZones;
    /** The identifier of the hub */
    std::string m_id;
    /** The primary broker identifier */
    std::string m_primaryBrokerId;
    /** The secondary broker identifier */
    std::string m_secondaryBrokerId;
    /** The parent node identifier */
    std::string m_parentId;
    /** The service zone associated with the hub */
    std::string m_serviceZone;
    /** The name of the hub */
    std::string m_name;
    /** The calculated service zones */
    ServiceZoneList m_serviceZones;
};

}  // namespace broker
}  // namespace dxl

#endif  // HUB_H_
