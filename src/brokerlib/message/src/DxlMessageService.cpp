/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/include/DxlErrorResponse.h"
#include "message/include/DxlMessageService.h"
#include "message/include/DxlRequest.h"
#include "message/include/DxlResponse.h"
#include "message/include/messageinterface.h"
#include "include/brokerlib.h"
#include "include/BrokerSettings.h"
#include "message/include/dxl_error_message.h"

#include <sstream>
#include <stdexcept>

using namespace std;
using namespace dxl::broker;
using namespace dxl::broker::message;

/** {@inheritDoc} */
DxlMessageService& DxlMessageService::getInstance()
{
    static DxlMessageService instance;
    return instance;
}

/** {@inheritDoc} */
const shared_ptr<DxlEvent> DxlMessageService::createEvent() const
{
    dxl_message_t *msg;
    if( !createEventMessage( BrokerSettings::getGuid(), BrokerSettings::getGuid(), NULL, NULL, &msg ) )
    {
        throw runtime_error( "Error creating event message" );
    }

    shared_ptr<DxlEvent> evt( new DxlEvent( msg ) );
    evt->setSourceBrokerGuid( BrokerSettings::getGuid() );
    evt->setSourceTenantGuid( BrokerSettings::getTenantGuid( false ) );

    return evt;
}

/** {@inheritDoc} */
const shared_ptr<DxlRequest> DxlMessageService::createRequest( const char* messageId, const char* replyToTopic ) const
{
    dxl_message_t *msg;
    if( !createRequestMessage( BrokerSettings::getGuid(), BrokerSettings::getGuid(), messageId, NULL, replyToTopic, &msg ) )
    {
        throw runtime_error( "Error creating request message" );
    }    

    shared_ptr<DxlRequest> req( new DxlRequest( msg ) );
    req->setSourceBrokerGuid( BrokerSettings::getGuid() );
    req->setSourceTenantGuid( BrokerSettings::getTenantGuid( false ) );
    return req;
}

/** {@inheritDoc} */
const shared_ptr<DxlResponse> DxlMessageService::createResponse( const DxlRequest* request ) const
{
    dxl_message_t *msg;
    if( !createResponseMessage( 
        BrokerSettings::getGuid(), BrokerSettings::getGuid(), NULL, NULL, request->getMessageId(), &msg ) )
    {
        throw runtime_error( "Error creating response message" );
    }
    shared_ptr<DxlResponse> res( new DxlResponse( request, msg ) );
    res->setSourceBrokerGuid( BrokerSettings::getGuid() );
    res->setSourceTenantGuid( BrokerSettings::getTenantGuid( false ) );
    return res;
}

/** {@inheritDoc} */
const shared_ptr<DxlErrorResponse> DxlMessageService::createErrorResponse( 
    const DxlRequest* request, int errorCode, const char* errorMessage ) const
{
    error_t errorInfo = { errorMessage, errorCode };

    dxl_message_t *msg;
    if( !createErrorResponseMessage( 
        BrokerSettings::getGuid(), BrokerSettings::getGuid(), NULL, NULL, request->getMessageId(), &errorInfo, &msg ) )
    {
        throw runtime_error( "Error creating error response message" );
    }
    shared_ptr<DxlErrorResponse> res( new DxlErrorResponse( request, msg ) );
    res->setSourceBrokerGuid( BrokerSettings::getGuid() );
    res->setSourceTenantGuid( BrokerSettings::getTenantGuid( false ) );
    return res;
}

/** {@inheritDoc} */
void DxlMessageService::toBytes( const DxlMessageBuilder& builder, unsigned char** bytes, size_t* size ) const
{
    toBytes( *(builder.buildMessage().get()), bytes, size );
}

/** {@inheritDoc} */
void DxlMessageService::toBytes( DxlMessage& message, unsigned char** bytes, size_t* size, 
    bool stripClientGuids  ) const
{
    // Inform the message that it is about to be converted to bytes
    message.onPreToBytes();

    dxl_message_error_t result = dxlMessageToBytes( NULL, bytes, size, message.getMessage(), (stripClientGuids ? 1 : 0) );        
    if( result != DXLMP_OK )
    {
        stringstream errMsg;
        errMsg << "Error converting message to bytes: " << result;
        throw runtime_error( errMsg.str() );        
    }
}

/** {@inheritDoc} */
shared_ptr<DxlMessage> DxlMessageService::fromBytes( 
    const unsigned char* bytes, size_t size ) const
{
    dxl_message_t* message;
    dxl_message_error_t result = createDxlMessageFromBytes( NULL, bytes, size, &message );
    if( result != DXLMP_OK )
    {
        stringstream errMsg;
        errMsg << "Error creating message from bytes: " << result;
        throw runtime_error( errMsg.str() );        
    }

    switch( message->messageType )
    {
        case DXLMP_EVENT:
            return shared_ptr<DxlEvent>( new DxlEvent( message ) );
        case DXLMP_REQUEST:
            return shared_ptr<DxlRequest>( new DxlRequest( message ) );
        case DXLMP_RESPONSE:
            return shared_ptr<DxlResponse>( new DxlResponse( message ) );
        case DXLMP_RESPONSE_ERROR:
            return shared_ptr<DxlErrorResponse>( new DxlErrorResponse( message ) );
        default:
            stringstream errMsg;
            errMsg << "Message type from bytes not supported: " << message->messageType;
            freeDxlMessage( NULL, message );
            throw runtime_error( errMsg.str() );        
    }
}

/** {@inheritDoc} */
void DxlMessageService::sendMessage( const char* channel, const DxlMessageBuilder& builder ) const
{
    sendMessage( channel, *(builder.buildMessage().get()) );
}

/** {@inheritDoc} */
void DxlMessageService::sendMessage( const char* channel, DxlMessage& message ) const
{
    if( !m_coreInterface )
    {
        throw runtime_error( "Core interface has not been set" );
    }

    unsigned char* bytes;
    size_t size;
    toBytes( message, &bytes, &size );    
    m_coreInterface->sendMessage( channel, (uint32_t)size, bytes );
    free( bytes );
}

/** {@inheritDoc} */
void DxlMessageService::sendServiceNotFoundErrorMessage( const DxlRequest* request ) const
{    
    int isFabricError;
    const shared_ptr<DxlErrorResponse> errorResponse =
        createErrorResponse(
            request, 
            FABRICSERVICEUNAVAILABLE,
            getMessage( FABRICSERVICEUNAVAILABLE, &isFabricError )
        );    
    
    // Send the error message
    sendMessage( request->getReplyToTopic(), *errorResponse );
}

/** {@inheritDoc} */
void DxlMessageService::sendServiceOverloadedErrorMessage( const DxlRequest* request ) const
{    
    int isFabricError;
    const shared_ptr<DxlErrorResponse> errorResponse =
        createErrorResponse(
            request, 
            FABRICSERVICEOVERLOADED,
            getMessage( FABRICSERVICEOVERLOADED, &isFabricError )
        );    
    
    // Send the error message
    sendMessage( request->getReplyToTopic(), *errorResponse );
}