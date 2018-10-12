/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/include/dxl_error_message.h"

static int isSet(error_code_t errorNumber)
{
    return (errorNumber & FABRICERRORCODEMASK) == FABRICERRORCODEMASK;
}

const char* getMessage(error_code_t errorNumber, int *isFabricError)
{
    if(isFabricError) *isFabricError = isSet(errorNumber);

    switch(errorNumber)
    {
        case FABRICSERVICEUNAVAILABLE: return "unable to locate service for request";
        case FABRICSERVICEOVERLOADED: return "service is overloaded";
        case FABRICTIMEOUTRESPONSE: return "response timeout";
        default: return "unknown error code";
    };
}
