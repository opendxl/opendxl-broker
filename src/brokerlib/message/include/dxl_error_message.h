/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef _DXL_ERROR_MESSAGE_H_
#define _DXL_ERROR_MESSAGE_H_

#include <stdint.h>

#if defined __cplusplus
extern "C"
{
#endif
    // Use this to create a fabric error codes
    static const uint32_t FABRICERRORCODEMASK = 0x80000000;

    // Application and fabric error codes.  Fabric error codes
    // have the 31st bit set to 1.
    typedef enum { 
        FABRICSERVICEUNAVAILABLE    = 0x80000001, // Fabric - service unavailable
        FABRICSERVICEOVERLOADED     = 0x80000002, // Fabric - service is overloaded
        FABRICTIMEOUTRESPONSE       = 0x80000003, // Internal timeout waiting for response
        FABRICSERVICELIMITEXCEEDED  = 0x80000011, // Tenant service limit exceeded
        FABRICSUBLIMITEXCEEDED      = 0x80000012  // Tenant subscription limit exceeded
    } error_code_t;

    // Get the error messages.  They point to const char *, so
    // don't allow users to change the pointer or the values to
    // which the pointers, point.
    const char* getMessage(error_code_t errorNumber, int *isFabricError);

#if defined __cplusplus
}
#endif
#endif
