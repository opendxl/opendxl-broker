/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/include/DxlErrorResponse.h"

using namespace dxl::broker::message;

/** {@inheritDoc} */
DxlErrorResponse::DxlErrorResponse( const DxlRequest* request, dxl_message_t* msg ) :
    DxlResponse( request, msg )
{
}

/** {@inheritDoc} */
DxlErrorResponse::DxlErrorResponse( dxl_message_t* msg ) : 
    DxlResponse( msg )
{
}

/** {@inheritDoc} */
DxlErrorResponse::~DxlErrorResponse()
{
}

/** {@inheritDoc} */
uint32_t DxlErrorResponse::getErrorCode() const
{
    return getMessage()->dxl_message_specificData.responseErrorData->code;
}

/** {@inheritDoc} */
const char* DxlErrorResponse::getDestinationServiceId() const
{
    return getMessage()->dxl_message_specificData.responseErrorData->serviceInstanceId;
}

