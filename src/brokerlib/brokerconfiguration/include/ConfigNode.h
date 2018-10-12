/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef CONFIGNODE_H_
#define CONFIGNODE_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace dxl {
namespace broker {

/** Type definition for a service zone list */
typedef std::vector<std::string> ServiceZoneList;

/** External reference */
class BrokerConfiguration;

/**
 * Common interface for configuration brokers and hubs
 */
class ConfigNode
{
    /** Hub as a friend */
    friend class Hub;
    /** Config broker as a friend */
    friend class ConfigBroker;
    /** The configuration as a friend */
    friend class BrokerConfiguration;

public:
    /** Destructor */
    virtual ~ConfigNode() {}

    /** 
     * Returns the node identifier
     *
     * @return  The node identifier
     */
    virtual const std::string getId() const = 0;

    /**
     * Returns the parent node identifier
     *
     * @return  The parent node identifier
     */
    virtual const std::string getParentId() const = 0;

    /**
     * Returns if this node is a hub
     *
     * @return  If this node is a hub
     */
    virtual bool isHub() const = 0;

    /**
     * String representation of this node
     *
     * @return  String representation of this node
     */
    virtual std::string toString() const = 0;

    /**
     * Returns the ordered list of service zones for this particular node
     *
     * @return  The ordered list of service zones for this particular node
     */
    virtual ServiceZoneList getServiceZoneList() const = 0;

private:
    /**
     * Generates the ordered list of service zones for this particular node. 
     *
     * @param   brokerConfig the fabric configuration
     * @return  A reference to the generated service zone list
     */
    virtual const ServiceZoneList& generateServiceZoneList( 
        const BrokerConfiguration& brokerConfig ) = 0;
};

}  // namespace broker
}  // namespace dxl

#endif  // CONFIGNODE_H_
