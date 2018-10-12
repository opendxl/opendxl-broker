/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "SetConnectionLimitRunner.h"
#include "mosquitto_broker.h"

using namespace dxl::broker::core;

/** {@inheritDoc} */
void SetConnectionLimitRunner::run()
{
    if( IS_DEBUG_ENABLED )
        _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG, "SetConnectionLimitRunner::run" );    
    mqtt3_dxl_set_max_connect_count( m_limit );
}
