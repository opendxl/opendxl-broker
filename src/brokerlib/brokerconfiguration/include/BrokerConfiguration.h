/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERCONFIGURATION_H_
#define BROKERCONFIGURATION_H_

#include "include/unordered_map.h"
#include <memory>
#include <string>
#include <vector>
#include "brokerconfiguration/include/Hub.h"
#include "brokerconfiguration/include/ConfigBroker.h"
#include "brokerconfiguration/include/ConfigNode.h"

namespace dxl {
namespace broker {

typedef unordered_map<std::string, std::shared_ptr<ConfigNode>> ConfigNodeMap;

/**
 * The broker fabric configuration. This configuration is used to determine
 * how the local broker will bridge, how service zones are to be processed,
 * etc.
 */
class BrokerConfiguration
{
public:
    /**
     * Loads the configuration from the specified file
     *
     * @param   configFile The file to load the configuration from
     */
    static std::shared_ptr<BrokerConfiguration> CreateFromJsonConfigFile(
        const std::string& configFile );

    /**
     * Constructs the configuration
     *
     * @param   hubs The hubs in the configuration
     * @param   brokers The brokers in the configuration
     */
    BrokerConfiguration( const std::vector<std::shared_ptr<Hub>>& hubs,
        const std::vector<std::shared_ptr<ConfigBroker>>& brokers );

    /** Destructor */
    virtual ~BrokerConfiguration();

    /** {@inheritDoc} */
    std::shared_ptr<Hub> getContainingHub( const std::string& brokerId ) const;

    /** {@inheritDoc} */
    std::shared_ptr<ConfigNode> getConfigNode( const std::string& id ) const;

    /** {@inheritDoc} */
    ConfigNodeMap getConfigNodeMap() const { return m_nodeMap; }

    /** Config equality */
    bool operator==( const BrokerConfiguration& rhs ) const;
    /** Config inequality */
    bool operator!=( const BrokerConfiguration& rhs ) const;

protected:    
    /** A vector containing the hubs in the configuration */
    std::vector<std::shared_ptr<Hub>> m_hubs;
    /** A vector containing the brokers in the configuration */
    std::vector<std::shared_ptr<ConfigBroker>> m_brokers;
    /** A map containing the configuration nodes (brokers/hubs) by identifier */
    ConfigNodeMap m_nodeMap;
};

}
}

#endif /* BROKERCONFIGURATION_H_ */
