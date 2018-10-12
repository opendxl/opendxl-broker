/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef COREBRIDGECONFIGURATIONFACTORY_H_
#define COREBRIDGECONFIGURATIONFACTORY_H_

#include "core/include/CoreBridgeConfiguration.h"
#include "brokerconfiguration/include/BrokerConfigurationService.h"

namespace dxl {
namespace broker {
namespace core {

/**
 * Factory for creating CoreBridgeConfiguration instances.
 *
 * We use a factory pattern for creating configuration instances due to the fact that we don't want
 * to introduce additional broker library dependencies in the core. The configuration object is
 * simply a data transfer object.
 */
class CoreBridgeConfigurationFactory
{
public:

    /**
     * Creates a bridge configuration corresponding to the specified broker configuration
     *
     * @param   guid The guid of the local broker
     * @param   brokerConfig The broker configuration service
     * @return  A bridge configuration corresponding to the specified broker configuration
     */
    static CoreBridgeConfiguration createConfiguration(
        const std::string& guid,
        std::shared_ptr<const dxl::broker::BrokerConfiguration> brokerConfig );
    /**
     * Creates an empty bridge configuration corresponding to the specified broker configuration
     *
     * @return  An empty configuration
     */
    static CoreBridgeConfiguration createEmptyConfiguration();

private:

    /**
     * Adds the primary hub (if applicable) to the list of brokers that may be bridged from 
     * the local broker.
     *
     * @param   config The bridge configuration
     * @param   brokerConfig The broker configuration service
     * @param   localBroker The local broker (the one associated with the configuration)
     */
    static void addPrimaryHub( 
        CoreBridgeConfiguration& config,
        std::shared_ptr<const dxl::broker::BrokerConfiguration> brokerConfig,
        std::shared_ptr<const dxl::broker::ConfigBroker> localBroker );

    /**
     * Adds the parent brokers to the list of brokers that may be bridged from the local 
     * broker.
     *
     * @param   config The bridge configuration
     * @param   brokerConfig The broker configuration service
     * @param   localBroker The local broker (the one associated with the configuration)
     */
    static void addParents( 
        CoreBridgeConfiguration& config,
        std::shared_ptr<const dxl::broker::BrokerConfiguration> brokerConfig,
        std::shared_ptr<const dxl::broker::ConfigBroker> localBroker );

    /**
     * Adds the specified broker to the list of brokers that may be bridged from the local 
     * broker.
     *
     * @param   config The bridge configuration
     * @param   brokerConfig The broker configuration service
     * @param   brokerId The identifier of the broker to add
     */
    static void addBroker( 
        CoreBridgeConfiguration& config,
        std::shared_ptr<const dxl::broker::BrokerConfiguration> brokerConfig,
        const std::string& brokerId );
};

} /* namespace core */
} /* namespace broker */
} /* namespace dxl */

#endif /* COREBRIDGECONFIGURATIONFACTORY_H_ */
