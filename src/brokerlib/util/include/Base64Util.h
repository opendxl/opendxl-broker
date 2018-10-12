/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BASE64UTIL_H_
#define BASE64UTIL_H_

#include <cstring>

namespace dxl {
namespace broker {
namespace util {

/**
 * Base 64 utility methods
 */
class Base64Util
{    
public:
    /**
     * Decodes the specified source string
     *
     * @param   source The source string
     * @param   sourceLength The source length
     * @param   dest The destination buffer
     * @param   maxDestSize The maximum to write in the dest buffer
     * @param   state The state of the decoding (used for multiple invocations, when the dest is full)
     * @param   sourceCharsProcessed The number of source characters that were processed
     * @return  The size of the writes to the destination
     */
    static size_t decode( const char* source, size_t sourceLength, void* dest, size_t maxDestSize,
        int* state, size_t *sourceCharsProcessed );
};

} /* namespace util */
} /* namespace broker */
} /* namespace dxl */

#endif /* BASE64UTIL_H_ */
