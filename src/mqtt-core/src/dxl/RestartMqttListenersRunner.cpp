/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "RestartMqttListenersRunner.h"
#include "logging_mosq.h"
#include "mosquitto_broker.h"

using namespace dxl::broker::core;

/** {@inheritDoc} */
void RestartMqttListenersRunner::run()
{
    if( IS_DEBUG_ENABLED )
        _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG, "RestartMqttListenersRunner::run" );

    mosquitto_epoll_restart_listeners();    
}
