/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERCONFIGURATIONSERVICE_H_
#define BROKERCONFIGURATIONSERVICE_H_

#include <vector>
#include "include/unordered_map.h"
#include <memory>
#include <mutex>
#include <string>
#include "brokerconfiguration/include/BrokerConfiguration.h"
#include "brokerconfiguration/include/Hub.h"
#include "brokerconfiguration/include/ConfigBroker.h"
#include "brokerconfiguration/include/BrokerConfiguration.h"
#include "brokerconfiguration/include/BrokerConfigurationServiceListener.h"

namespace dxl {
namespace broker {

/**
 * Service that contains the "current" fabric configuration. The "current" 
 * configuration will change as policy is received, etc.
 * The fabric configuration is used to determine how the local broker will bridge, 
 * how service zones are to be processed, etc.
 */
class BrokerConfigurationService 
{
public:
    /**
     * Returns the single instance of the service
     *
     * @return  The single instance of the service
     */
    static const std::shared_ptr<BrokerConfigurationService> Instance();

    /** Destructor */     
    virtual ~BrokerConfigurationService();

    /**
     * Sets the "current" broker configuration to the specified configuration
     *
     * @param   config The configuration that is to become the "current" configuration
     * @return  Whether the current configuration was updated
     */
    bool setBrokerConfiguration( std::shared_ptr<BrokerConfiguration> config );

    /**
     * Returns the "current" broker configuration
     *
     * @return  The "current" broker configuration
     */
    std::shared_ptr<const BrokerConfiguration> getBrokerConfiguration();

    /**
     * Adds a listener that is notified when changes are made to the broker configuration
     *
     * @param   listener The listener to add
     */
    void addListener( BrokerConfigurationServiceListener* listener );

    /**
     * Removes a listener that is notified when changes are made to the broker configuration
     *
     * @param   listener The listener to remove
     */
    void removeListener( BrokerConfigurationServiceListener* listener );

protected:
    /** Constructor */
    BrokerConfigurationService();

    /**
     * Fires a configuration change event to registered listeners.
     *
     * @param   config The configuration with changes
     */
    void fireConfigurationChanged( std::shared_ptr<BrokerConfiguration> config );

    /** Registered listeners for change events */
    std::vector<BrokerConfigurationServiceListener*> m_listeners;
    /** Mutex used to control writes (when new configuration are set, etc.) to the service */
    std::mutex m_writeMutex;
    /** The "current" configuration */
    std::shared_ptr<BrokerConfiguration> m_config;
};

}  // namespace broker
}  // namespace dxl

#endif  // BROKERCONFIGURATIONSERVICE_H_
