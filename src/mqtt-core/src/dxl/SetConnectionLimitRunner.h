/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef SETCONNECTIONLIMITRUNNER_H_
#define SETCONNECTIONLIMITRUNNER_H_

#include "MqttWorkQueue.h"
#include "logging_mosq.h"

namespace dxl {
namespace broker {
namespace core {

/**
 * Sets the connection limit for the broker
 */
class SetConnectionLimitRunner : public MqttWorkQueue::Runnable
{    
public:

    /**
     * Constructs the runner
     *
     * @param   limit The connection limit for the broker
     */
    SetConnectionLimitRunner( uint32_t limit ) : m_limit( limit )
    {
        if( IS_DEBUG_ENABLED )
            _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG, "SetConnectionLimitRunner::SetConnectionLimitRunner()" );    
    };

    /** Destructor */
    virtual ~SetConnectionLimitRunner()
    {
        if( IS_DEBUG_ENABLED )
            _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG, "SetConnectionLimitRunner::~SetConnectionLimitRunner()" );    
    }
    
    /** {@inheritDoc} */
    void run();

private:

    /** The connection limit */
    uint32_t m_limit;
};

} /* namespace core */
} /* namespace broker */
} /* namespace dxl */

#endif /* SETCONNECTIONLIMITRUNNER_H_ */
