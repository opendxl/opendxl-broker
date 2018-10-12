/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/


#ifndef COREMESSAGECONTEXT_H_
#define COREMESSAGECONTEXT_H_

#include <memory>
#include <string>
#include <stdint.h>
#include "message/include/DxlMessage.h"
#include "message/include/DxlRequest.h"
#include "message/include/DxlErrorResponse.h"
#include "message/include/DxlEvent.h"
#include "DxlFlags.h"

namespace dxl {
namespace broker {
namespace core {

/** Forward class reference */
class CoreMessageHandlerService;

/**
 * Contextual information about a message that is being processed by the core messaging layer.
 * The flow of the message is: publish, store, insert (per recipient) and finalize.
 */
class CoreMessageContext
{
/** The message service is our friend. */
friend class dxl::broker::core::CoreMessageHandlerService;

public:
    /** 
     * Constructor 
     *
     * @param   sourceId The identifier associated with the source of the message
     * @param   canonicalSourceId The canonical identifier associated with the source of the message
     * @param   isSourceBridge Whether the source is a bridge
     * @param   contextFlags The context-specific flags
     * @param   topic The message topic
     * @param   payloadLen The payload length
     * @param   payload The payload
     */
    CoreMessageContext(
        const char* sourceId, const char* canonicalSourceId, bool isSourceBridge, uint8_t contextFlags,
        const char* topic, uint32_t payloadLen, const void* payload
    );

    /** Destructor */
    virtual ~CoreMessageContext();

    /**
     * Returns the message topic
     * 
     * @return  The message topic
     */
    const char* getTopic() const { return m_topic; }

    /** 
     * Returns the source identifier
     *
     * @return  The source identifier
     */
    const char* getSourceId() const { return m_sourceId.c_str(); }

    /** 
     * Returns the canonical source identifier
     *
     * @return  The canonical source identifier
     */
    const char* getCanonicalSourceId() const { return m_canonicalSourceId.c_str(); }

    /**
      * Returns the context flags
      * 
      * @return The context flags
      */
    uint8_t getContextFlags() const { return m_contextFlags; }
    
    /**
      * Set the context flags
      * 
      * @param  contextFlags The context-specific flags
      */
    void setContextFlags( uint8_t contextFlags ) { m_contextFlags = contextFlags; }

    /**
     * Returns whether the source of the message OPS
     *
     * @return  Whether the source of the message OPS
     */
    bool isSourceOps() const { return (m_contextFlags & DXL_FLAG_OPS) != 0; }

    /**
     * Returns whether the local broker is the source of the message
     *
     * @return  Whether the local broker is the source of the message
     */
    bool isLocalBrokerSource() const;

    /**
     * Whether the source is a bridge
     *
     * @return  Whether the source is a bridge
     */
    bool isSourceBridge() const { return m_isSourceBridge; }

    /**
     * Returns whether the underlying payload is a DXL message
     *
     * @return  Whether the underlying payload is a DXL message
     */
    bool isDxlMessage();

    /**
     * Returns the DXL message associated with the payload.
     *
     * An exception will be thrown if the underlying payload does not represent a DXL
     * message.
     *
     * @return  The DXL message associated with the payload.
     */
    dxl::broker::message::DxlMessage* getDxlMessage();

    /**
     * Returns the DXL request message associated with the payload.
     *
     * An exception will be thrown if the underlying payload does not represent a DXL
     * request message.
     *
     * @return  The DXL request message associated with the payload.
     */
    dxl::broker::message::DxlRequest* getDxlRequest();

    /**
     * Returns the DXL event message associated with the payload.
     *
     * An exception will be thrown if the underlying payload does not represent a DXL
     * event message.
     *
     * @return  The DXL event message associated with the payload.
     */
    dxl::broker::message::DxlEvent* getDxlEvent();

    /**
     * Returns the DXL error message associated with the payload.
     *
     * An exception will be thrown if the underlying payload does not represent a DXL
     * error message.
     *
     * @return  The DXL error message associated with the payload.
     */
    dxl::broker::message::DxlErrorResponse* getDxlErrorResponse();
    
    /**
     * Whether the message should be allowed to be sent to recipients
     *
     * @return  Whether the message should be allowed to be sent to recipients
     */
    bool isMessageInsertEnabled() const { return m_messageInsertEnabled; }

    /**
     * Increments the number of times the message has been inserted for delivery to a destination
     */
    void incrementDestinationCount() { m_destCount++; }

    /**
     * Returns the number of times the message was inserted for delivery to a destination
     *
     * @return  The number of times the message was inserted for delivery to a destination
     */
    int getDestinationCount() const { return m_destCount; }

    /**
     * Whether the message is a request that is targeted for the broker (request to the 
     * service registry, broker registry, etc.).
     *
     * @return  Whether the message is a request that is targeted for the broker (request to the
     *          service registry, broker registry, etc.).
     */
    bool isDxlBrokerServiceRequest() const;

    /**
     * Whether the service not found message is enabled for this message
     *
     * @return  Whether the service not found message is enabled for this message
     */
    bool isServiceNotFoundMessageEnabled() const { return m_serviceNotFoundEnabled; }

    /**
     * Sets whether the service not found message is enabled for this message
     *
     * @param   enabled Whether the service not found message is enabled for this message
     */
    void setServiceNotFoundMessageEnabled( bool enabled )
    {
        m_serviceNotFoundEnabled = enabled;
    }

    /**
     * Whether a client specific message has already been generated
     *
     * @return  Whether a client specific message has already been generated
     */
    bool isClientSpecificMessageGenerated() const { return m_clientSpecificMessageGenerated; }

    /**
     * Sets whether a client specific message has already been generated
     *
     * @param   generated Whether a client specific message has already been generated
     */
    void setClientSpecificMessageGenerated( bool generated )
    {
        m_clientSpecificMessageGenerated = generated;
    }

private:
    /**
     * Sets whether the message should be allowed to be sent to recipients
     *
     * @param   enabled Whether the message should be allowed to be sent to recipients
     */
    void setMessageInsertEnabled( bool enabled ) { m_messageInsertEnabled = enabled; }

    /**
     * Parses the payload into a DXL message
     */
    void parseDxlMessage();

    /** The identifier associated with the source of the message */
    std::string m_sourceId;
    /** The canonical identifier associated with the source of the message */
    std::string m_canonicalSourceId;
    /** Whether the source is a bridge */
    bool m_isSourceBridge;
    /** The message topic */
    const char* m_topic;
    /** The original payload length */
    uint32_t m_originalPayloadLen;
    /** The original payload */
    const void* m_originalPayload;
    /** Whether the DXL message has been parsed */
    bool m_dxlMessageParsed;
    /** The DXL message created from the payload */
    std::shared_ptr<dxl::broker::message::DxlMessage> m_dxlMessage;
    /** Whether the message should be allowed to be sent to recipients */
    bool m_messageInsertEnabled;
    /** Whether the service not found message is enabled */
    bool m_serviceNotFoundEnabled;
    /** Destination count */
    int m_destCount;
    /** The context flags */
    uint8_t m_contextFlags;
    /** Whether to use a client-specific message */
    bool m_clientSpecificMessageGenerated;
};

} /* namespace core */
} /* namespace broker */
} /* namespace dxl */

#endif /* COREMESSAGECONTEXT_H_ */
