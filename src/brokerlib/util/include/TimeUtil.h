/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef TIMEUTIL_H_
#define TIMEUTIL_H_

#include <cstdint>

namespace dxl {
namespace broker {
namespace util {

/**
 * Time utility methods
 */
class TimeUtil
{    
public:
    /**
     * Returns the current time in milliseconds
     *
     * @return  The current time in milliseconds
     */
    static uint64_t getCurrentTimeMillis();

    /**
     * Returns the current time in seconds
     *
     * @return  The current time in seconds
     */
    static uint32_t getCurrentTimeSeconds();
};

} /* namespace util */
} /* namespace broker */
} /* namespace dxl */

#endif /* TIMEUTIL_H_ */
