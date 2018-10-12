/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "SetDefaultBridgeKeepaliveRunner.h"
#include "mosquitto_broker.h"

using namespace dxl::broker::core;

/** {@inheritDoc} */
void SetDefaultBridgeKeepaliveRunner::run()
{
    if( IS_DEBUG_ENABLED )
        _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG, "SetDefaultBridgeKeepaliveRunner::run" );    
    mqtt3_set_default_bridge_keepalive( m_keepAliveMins * 60 ); // In seconds                
}
