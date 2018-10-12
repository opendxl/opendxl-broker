/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef UTIL_STRINGUTIL_H_
#define UTIL_STRINGUTIL_H_

#include <string>
#include <cstdint>

namespace dxl {
namespace broker {
namespace util {

/**
 * String utility methods
 */
class StringUtil
{    
public:
    /**
     * Returns the string representation of the specified unsigned 32-bit
     * integer
     *
     * @param   value The unsigned 32-bit value to convert to a string
     * @return  The string representation of the specified unsigned 32-bit
     *          integer
     */
    static std::string toString( uint32_t value );

    /**
     * Returns the unsigned 32-bit value corresponding to the specified string
     *
     * @param   value String representation of unsigned 32-bit integer
     * @return  The unsigned 32-bit value corresponding to the specified string
     */
    static uint32_t asUint32( const std::string& value );
};

} /* namespace util */
} /* namespace broker */
} /* namespace dxl */

#endif /* UTIL_STRINGUTIL_H_ */
