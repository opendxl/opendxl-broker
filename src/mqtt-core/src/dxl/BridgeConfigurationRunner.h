/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BRIDGECONFIGURATIONRUNNER_H_
#define BRIDGECONFIGURATIONRUNNER_H_

#include "MqttWorkQueue.h"
#include "core/include/CoreBridgeConfiguration.h"
#include "mosquitto_broker.h"
#include "logging_mosq.h"

namespace dxl {
namespace broker {
namespace core {

/**
 * Runner that updates Mosquitto to reflect the specified bridge configuration 
 * (CoreBridgeConfiguration).
 */
class BridgeConfigurationRunner : public MqttWorkQueue::Runnable
{    
public:

    /**
     * Constructs the runner
     *
     * @param   config The bridge configuration that Mosquitto will be updated to reflect.
     */
    BridgeConfigurationRunner( const CoreBridgeConfiguration config ) : m_config( config )
    {
        if( IS_DEBUG_ENABLED )
            _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG,
                "BridgeConfigurationRunner::BridgeConfigurationRunner()" );
    };

    /** Destructor */
    virtual ~BridgeConfigurationRunner()
    {
        if( IS_DEBUG_ENABLED )
            _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG,
                "BridgeConfigurationRunner::~BridgeConfigurationRunner()" );
    }
    
    /** {@inheritDoc} */
    void run();

private:

    /**
     * Disconnects and clears any existing bridges
     *
     * @param   db The Mosquitto database
     */
    void clearCurrentBridges( struct mosquitto_db *db );

    /** The bridge configuration that Mosquitto will be updated to reflect */
    CoreBridgeConfiguration m_config;
};

} /* namespace core */
} /* namespace broker */
} /* namespace dxl */

#endif /* BRIDGECONFIGURATIONRUNNER_H_ */
