/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef DXLMESSAGE_H_
#define DXLMESSAGE_H_

#include "message/include/messageinterface.h"
#include "json/include/JsonWriter.h"
#include "include/unordered_map.h"
#include "include/unordered_set.h"

namespace dxl {
namespace broker {
/** Namespace for DXL message-related declarations */
namespace message {

/** Forward class reference */
class DxlMessageService;

/**
 * Represents a DXL message. This is a wrapper around an underlying "dxl_message_t" structure. This 
 * class provides convenience methods for setting and receiving payloads, as well as simplifying 
 * publishing and receiving events via the "DxlMessageService".
 */
class DxlMessage
{    
/** The message service is our friend. */
friend class dxl::broker::message::DxlMessageService;

public:
    /** Destructor */
    virtual ~DxlMessage();

    /**
     * Whether this message is a DXL request  message
     *
     * @return  Whether this message is a request message
     */
    bool isRequestMessage() const { return m_msg->messageType == DXLMP_REQUEST; }

    /**
     * Whether this message is a DXL response message
     *
     * @return  Whether this message is a response message
     */
    bool isResponseMessage() const { return m_msg->messageType == DXLMP_RESPONSE; }

    /**
     * Whether this message is a DXL error message
     *
     * @return  Whether this message is a error message
     */
    bool isErrorMessage() const { return m_msg->messageType == DXLMP_RESPONSE_ERROR; }

    /**
     * Whether this message is a DXL event message
     *
     * @return  Whether this message is a event message
     */
    bool isEventMessage() const { return m_msg->messageType == DXLMP_EVENT; }

    /**
     * Returns the message id
     *
     * @return  The message id
     */
    const char* getMessageId() const;

    /**
     * Returns the source broker guid
     *
     * @return  The source broker guid
     */
    const char* getSourceBrokerGuid() const;

    /**
     * Sets the source broker guid
     *
     * @param   sourceBrokerGuid The source broker guid
     */
    void setSourceBrokerGuid( const char* sourceBrokerGuid );

    /**
     * Returns the source client id
     *
     * @return  The source client id
     */
    const char* getSourceClientId() const;

    /**
     * Sets the source client id
     *
     * @param   sourceClientId The source client id
     */
    void setSourceClientId( const char* sourceClientId );

    /** 
     * Returns the source client instance id
     *
     * @return  The source client instance id
     */
    const char* getSourceClientInstanceId() const;

    /**
     * Sets the source client instance id
     *
     * @param   sourceClientInstanceId The source client instance id
     */
    void setSourceClientInstanceId( const char* sourceClientInstanceId );

    /**
     * Sets the payload via the specified string
     *
     * @param   payload The payload as a string
     */
    void setPayload( const std::string& payload );

    /**
     * Sets the payload as the specified bytes
     *
     * @param   bytes The payload bytes
     * @param   size The payload size
     */
    void setPayload( const unsigned char* bytes, size_t size );

    /**
     * Sets the payload via the specified JSON writer
     *
     * @param   writer The JSON write which will write the payload
     * @param   log Whether to log the JSON being written (if debug is enabled)
     */
    void setPayload( const dxl::broker::json::JsonWriter& writer, bool log = true );

    /**
     * Returns the message payload
     *
     * @param   bytes The payload bytes
     * @param   size The payload size
     */
    void getPayload( const unsigned char** bytes, size_t* size ) const;

    /**
     * Returns the payload as a string
     *
     * @return  The payload as a string
     */
    std::string getPayloadStr() const;

    /**
     * Returns the destination broker guids, or NULL if no destinations have been set.
     *
     * @return  The destination broker guids, or NULL if no destinations have been set.
     */
    const unordered_set<std::string>* getDestinationBrokerGuids();

    /**
     * Sets the destination broker guid
     *
     * @param   brokerGuid The destination broker guid
     */
    void setDestinationBrokerGuid( const char* brokerGuid );

    /**
     * Returns the destination client guids, or NULL if no destinations have been set.
     *
     * @return  The destination client guids, or NULL if no destinations have been set.
     */
    const unordered_set<std::string>* getDestinationClientGuids();

    /**
     * Sets the value of an "other" field (name-value pair)
     *
     * @param   name The name
     * @param   value The value
     *
     * NOTE: Added as of version 2.0 of the broker (Version 1 of the DXL message format)
     */
    void setOtherField( const std::string& name, const std::string& value );

    /**
     * Retrieves the value of an "other" field (name-value pair)
     *
     * @param   name The name
     * @param   value The value (out)
     * @return  Whether the name-value pair exists
     *
     * NOTE: Added as of version 2.0 of the broker (Version 1 of the DXL message format)
     */
    bool getOtherField( const std::string& name, std::string& value );

    /**
     * Returns the other fields (name-value pairs) or NULL if no other fields have been set
     *
     * @return  The other fields (name-value pairs) or NULL if no other fields have been set
     *
     * NOTE: Added as of version 2.0 of the broker (Version 1 of the DXL message format)
     */
    const unordered_map<std::string, std::string>* getOtherFields();

    /**
     * Sets the destination client guid
     *
     * @param   clientGuid The destination client guid
     */
    void setDestinationClientGuid( const char* clientGuid );

    /**
     * Returns the source tenant GUID.
     *
     * @return  The source tenant GUID.
     */
    const char* getSourceTenantGuid() const;

    /**
     * Sets the source tenant GUID.
     *
     * @param   sourceTenantGuid The source tenant GUID.
     */
    void setSourceTenantGuid(const char* sourceTenantGuid);

    /**
     * Returns the destination tenant GUIDs, or NULL if no destinations have been set.
     *
     * @return The destination tenant GUIDs, or NULL if no destinations have been set.
     */
    const unordered_set<std::string>* getDestinationTenantGuids();

    /**
     * Sets the destination tenant GUIDs.
     *
     * @param   tenantGuids The destination tenant GUIDs.
     * @param   tenantGuidCount The size of destination tenant GUIDs.
     */
    void setDestinationTenantGuids( const char* tenantGuids[], size_t tenantGuidCount );

    /**
     * Returns the guids for the next brokers in the path to the destination brokers.
     * The "next" broker guids are the guids to route the message to from this broker's 
     * perspective (the next hop on the way to the final destination brokers). 
     *
     * @return  The guids for the next brokers in the path to the destination brokers.
     *          A "NULL" value indicates that the message is being sent everywhere (unrestricted).
     *          An "empty" set indicates that destinations have been specified, but there are no
     *          valid "next" hops from this broker's perspective. Therefore, the message should not be
     *          routed.
     */
    const unordered_set<std::string>* getNextBrokerGuids();

    /**
     * Whether the message is dirty (has been updated)
     * 
     * @return  Whether the message is dirty (has been updated)
     */
    bool isDirty() const { return m_isDirty; }

    /**
     * Invoked prior to this object being converted to bytes. 
     */
    virtual void onPreToBytes();

protected:
    /** 
     * Constructor 
     *
     * @param   msg The underlying message structure
     */
    DxlMessage( dxl_message_t* msg );

    /**
     * Returns the underlying message structure
     * 
     * @return  The underlying message structure
     */
    dxl_message_t* getMessage() const;

    /**
     * Sets that the message is dirty 
     */
    void markDirty() { m_isDirty = true; }

private:
    /** The underlying message structure */
    dxl_message_t* m_msg;
    /** Whether the message is dirty (has been updated) */
    bool m_isDirty;
    /** The destination broker guids */
    unordered_set<std::string>* m_destBrokerGuids;
    /** The destination client guids */
    unordered_set<std::string>* m_destClientGuids;
    /** The next broker guids */
    unordered_set<std::string>* m_nextBrokerGuids;
    /** The other fields */
    unordered_map<std::string, std::string>* m_otherFields;
    /** Whether the "other" field writes are pending */
    bool m_otherFieldsPending;
    /** The destination tenant GUIDs. */
    unordered_set<std::string>* m_destTenantGuids;
};

} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* DXLMESSAGE_H_ */
