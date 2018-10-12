/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef SETDEFAULTBRIDGEKEEPALIVERUNNER_H_
#define SETDEFAULTBRIDGEKEEPALIVERUNNER_H_

#include "MqttWorkQueue.h"
#include "logging_mosq.h"

namespace dxl {
namespace broker {
namespace core {

/**
 * Sets the default bridge keep-alive
 */
class SetDefaultBridgeKeepaliveRunner : public MqttWorkQueue::Runnable
{    
public:

    /**
     * Constructs the runner
     *
     * @param   keepAliveMins The keep alive time in minutes
     */
    SetDefaultBridgeKeepaliveRunner( uint32_t keepAliveMins ) : m_keepAliveMins( keepAliveMins )
    {
        if( IS_DEBUG_ENABLED )
            _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG,
                "SetDefaultBridgeKeepaliveRunner::SetDefaultBridgeKeepaliveRunner()" );
    };

    /** Destructor */
    virtual ~SetDefaultBridgeKeepaliveRunner()
    {
        if( IS_DEBUG_ENABLED )
            _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG,
                "SetDefaultBridgeKeepaliveRunner::~SetDefaultBridgeKeepaliveRunner()" );
    }
    
    /** {@inheritDoc} */
    void run();

private:

    /** The keep alive time in minutes */
    uint32_t m_keepAliveMins;
};

} /* namespace core */
} /* namespace broker */
} /* namespace dxl */

#endif /* SETDEFAULTBRIDGEKEEPALIVERUNNER_H_ */
