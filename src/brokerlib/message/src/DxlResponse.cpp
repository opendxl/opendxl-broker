/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/include/DxlResponse.h"
#include <cstring>

using namespace dxl::broker::message;

/** {@inheritDoc} */
DxlResponse::DxlResponse( const DxlRequest* request, dxl_message_t* msg ) :
    DxlMessage( msg )
{
    // Set the broker to send the response to
    const char* sourceBrokerGuid = request->getSourceBrokerGuid();
    if( sourceBrokerGuid != NULL && strlen( sourceBrokerGuid ) > 0 )
    {
        setDestinationBrokerGuid( sourceBrokerGuid );
    }

    // Set the client to send the response to
    const char* sourceClientId = request->getSourceClientInstanceId();
    if( sourceClientId != NULL && strlen( sourceClientId ) > 0 )
    {
        setDestinationClientGuid( sourceClientId );
    }

    // Set the service identifier
    const char* serviceId = request->getDestinationServiceId();
    if( serviceId != NULL && strlen( serviceId ) > 0 )
    {
        setDestinationServiceId( serviceId );
    }
}

/** {@inheritDoc} */
DxlResponse::DxlResponse( dxl_message_t* msg ) : 
    DxlMessage( msg )
{
}

/** {@inheritDoc} */
DxlResponse::~DxlResponse()
{
}

/** {@inheritDoc} */
void DxlResponse::setDestinationServiceId( const char* serviceId )
{
    setDxlResponseMessageServiceId( NULL, getMessage(), serviceId );

    // Mark dirty
    markDirty();
}

/** {@inheritDoc} */
const char* DxlResponse::getDestinationServiceId() const
{
    return getMessage()->dxl_message_specificData.responseData->serviceInstanceId;
}