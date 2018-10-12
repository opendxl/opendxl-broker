/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "util/include/Base64Util.h"
#include <limits.h>

using namespace dxl::broker::util;

/** {@inheritDoc} */
size_t Base64Util::decode( 
    const char* source, size_t sourceLength, void* _dest, size_t maxDestSize,
    int* _state, size_t *sourceCharsProcessed )
{
    // decode base64 encoded data, every valid input character translates to
    // six significant bits, four valid input characters generates up to three
    // output bytes
    size_t destSize = 0;
    int state = 0;
    unsigned char partial = 0;
    size_t sourceIndex = 0;
    unsigned char* dest = (unsigned char*)( _dest );

    if( sourceLength >= INT_MAX )
    {
        sourceLength = strlen( source );
    }

    if( _state )
    {
        state = (*_state) & 0x3;
        partial = (unsigned char)( ( *_state ) & 0xFC );
    }

    while( sourceIndex < sourceLength && destSize < maxDestSize )
    {
        // 'A'-'Z' = 0-25  or 0x00-0x19
        // 'a'-'z' = 26-51 or 0x1A-0x33
        // '0'-'9' = 52-61 or 0x34-0x3D
        // '+'     = 62    or 0x3E
        // '/'     = 63    or 0x3F
        // '='     = pad, resets state
        // all other characters are ignored
        char code = source[sourceIndex];
        unsigned char decode = 0xFF;
        switch( code )
        {
        case '=':
            state = 0;
            break;
        case '+':
            decode = 62;
            break;
        case '/':
            decode = 63;
            break;
        default:
            if( code >= '0' )
            {
                if( code <= '9' )
                {
                    decode = (unsigned char)( code )+52 - '0';
                }
                else if( code >= 'A' )
                {
                    if( code <= 'Z' )
                    {
                        decode = (unsigned char)( code ) - 'A';
                    }
                    else if( code >= 'a' && code <= 'z' )
                    {
                        decode = (unsigned char)( code ) + 26 - 'a';
                    }
                }
            }
        }
        if( decode != 0xFF )
        {
            if( !state )
            {
                partial = decode << 2;
                state = 1;
            }
            else
            {
                unsigned char data;
                switch( state )
                {
                    case 1:
                        data = partial | ( decode >> 4 );
                        partial = decode << 4;
                        state = 2;
                        break;
                    case 2:
                        data = partial | ( decode >> 2 );
                        partial = decode << 6;
                        state = 3;
                        break;
                        // case 3:
                    default:
                        data = partial | decode;
                        state = 0;
                        break;
                }
                dest[destSize] = data;
                destSize++;
            }
        }
        sourceIndex++;
    }

    if( _state )
    {
        *_state = partial | state;
    }

    if( sourceCharsProcessed )
    {
        *sourceCharsProcessed = sourceIndex;
    }

    return destSize;
}
