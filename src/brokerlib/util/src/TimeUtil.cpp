/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "util/include/TimeUtil.h"
#include <cstddef>
#include <cstdint>
#include <sys/time.h>

using namespace dxl::broker::util;

/** {@inheritDoc} */
uint64_t TimeUtil::getCurrentTimeMillis()
{    
    struct timeval curtime;    
    gettimeofday( &curtime, NULL );
    return ((curtime.tv_sec) * 1000 + curtime.tv_usec/1000.0) + 0.5;
}

/** {@inheritDoc} */
uint32_t TimeUtil::getCurrentTimeSeconds()
{
    struct timeval curtime;    
    gettimeofday( &curtime, NULL );
    return curtime.tv_sec;
}
