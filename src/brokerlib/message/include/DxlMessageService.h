/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef DXLMESSAGESERVICE_H_
#define DXLMESSAGESERVICE_H_

#include <memory>
#include "message/include/DxlErrorResponse.h"
#include "message/include/DxlEvent.h"
#include "message/include/DxlRequest.h"
#include "message/include/DxlResponse.h"
#include "message/include/DxlMessageBuilder.h"
#include "core/include/CoreInterface.h"

namespace dxl {
namespace broker {
namespace message {

/**
 * Service used to create messages, send messages, convert messages to and from bytes, etc.
 */
class DxlMessageService
{    
public:
    /** Destructor */
    virtual ~DxlMessageService() {}

    /**
     * Returns the single service instance 
     * 
     * @return    The single service instance
     */
    static DxlMessageService& getInstance();

    /**
     * Sets the core interface
     *
     * @param   coreInterface The core interface
     */
    void setCoreInterface(
        dxl::broker::core::CoreInterface* coreInterface )
        { m_coreInterface = coreInterface; }

    /**
     * Creates a DXL Event message
     *
     * @return  The new DXL Event message
     */
    const std::shared_ptr<DxlEvent> createEvent() const;

    /**
     * Creates a DXL Request message
     *
     * @param   messageId The identifier for the message (can be NULL)
     * @param   replyToTopic The reply-to topic for the request
     * @return  The new DXL Request message
     */
    const std::shared_ptr<DxlRequest> createRequest( const char* messageId, const char* replyToTopic ) const;

    /**
     * Creates a DXL response message
     *
     * @param   request The request to respond to
     * @return  The new DXL Response message
     */
    const std::shared_ptr<DxlResponse> createResponse( const DxlRequest* request ) const;

    /**
     * Creates a DXL error response message
     *
     * @param   request The request to respond to
     * @param   errorCode The error code
     * @param   errorMessage The error message
     * @return  The new DXL Error Response message
     */
    const std::shared_ptr<DxlErrorResponse> createErrorResponse( 
        const DxlRequest* request, int errorCode, const char* errorMessage ) const;

    /**
     * Returns the bytes for the DXL message built by the specified builder
     *
     * @param   builder The DXL message builder
     * @param   bytes The bytes pointer to output to
     * @param   size The size of the bytes
     */
    void toBytes( const DxlMessageBuilder& builder, unsigned char** bytes, size_t* size ) const;

    /**
     * Returns the bytes for the DXL message
     *
     * @param   message The DXL message
     * @param   bytes The bytes pointer to output to
     * @param   stripClientGuids Whether to strip the client GUIDs from the message
     * @param   size The size of the bytes
     */
    void toBytes( DxlMessage& message, unsigned char** bytes, size_t* size, 
        bool stripClientGuids = false ) const;

    /**
     * Creates and returns a DXL message corresponding to the specified bytes
     *
     * @param   bytes The bytes
     * @param   size The bytes size
     * @return  A DXL message corresponding to the specified bytes
     */
    std::shared_ptr<DxlMessage> fromBytes(
        const unsigned char* bytes, size_t size ) const;

    /**
     * Sends a DXL message as built by the specified builder
     * 
     * @param   channel The channel to send the message to
     * @param   builder The DXL message builder (used to build the message to send)
     */
    void sendMessage( const char* channel, const DxlMessageBuilder& builder ) const;

    /**
     * Sends the specified DXL message
     * 
     * @param   channel The channel to send the message to
     * @param   message The DXL message to send
     */
    void sendMessage( const char* channel, DxlMessage& message ) const;

    /**
     * Sends a service not found error message in response to a request
     * 
     * @param   request The request that was unable to find the service
     */
    void sendServiceNotFoundErrorMessage( const DxlRequest* request ) const;

    /**
     * Sends a service overloaded error message in response to a request
     * 
     * @param   request The request that was made to the service that is overloaded
     */
    void sendServiceOverloadedErrorMessage( const DxlRequest* request ) const;

private:
    /** Constructor */
    DxlMessageService() : m_coreInterface( NULL ) {};

    /** The core interface */
    dxl::broker::core::CoreInterface* m_coreInterface;
};

} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* DXLMESSAGESERVICE_H_ */
