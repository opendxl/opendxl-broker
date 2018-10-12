/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef COREMESSAGEHANDLERSERVICE_H_
#define COREMESSAGEHANDLERSERVICE_H_

#include "core/include/CoreMaintenanceListener.h"
#include "core/include/CoreMessageContext.h"
#include "core/include/CoreOnFinalizeMessageHandler.h"
#include "core/include/CoreOnInsertMessageHandler.h"
#include "core/include/CoreOnPublishMessageHandler.h"
#include "core/include/CoreOnStoreMessageHandler.h"
#include "include/unordered_map.h"
#include <memory>
#include <string>
#include <stdint.h>
#include <vector>

namespace dxl {
namespace broker {
namespace core {

/**
 * Service for registering message handler instances. These handlers will be called prior to a 
 * message being published and/or stored for sending to a particular recipient on a specified 
 * message topic.
 *
 * The flow of a message is as follows:
 *
 *   onPublish - The message is about to be published. At this point, the publishing can be
 *               vetoed.
 *
 *   onStore - The message has been stored in the core database for sending to recipients.
 *             At this point a <code>CoreMessageContext</code> object will be created. This message
 *             context object will exist until after the finalize method has been invoked.
 *
 *   onInsert - This is called for each of the possible recipients of the message (this queues for
 *              delivery to the recipient). The insertion can be vetoed.
 * 
 *   onFinalize - The message sending process has completed. After this message is invoked, the
 *                message context object will be destroyed.
 */
class CoreMessageHandlerService :
    public CoreOnPublishMessageHandler,
    public CoreMaintenanceListener
{
public:
    /** Destructor */
    virtual ~CoreMessageHandlerService() {}

    /**
     * Returns the single service instance
     * 
     * @return  The single service instance
     */
    static CoreMessageHandlerService& getInstance();

    /**
     * Registers a "On publish" message handler (will be invoked prior to the message being 
     * published) for all topics.
     *
     * @param   handler The handler to invoke
     */
    void registerPublishHandler( const CoreOnPublishMessageHandler* handler );

    /**
     * Registers a "On publish" message handler (will be invoked prior to the message being 
     * published) for the specified topic.
     *
     * @param   topic The message topic
     * @param   handler The handler to invoke
     */
    void registerPublishHandler( const std::string& topic, const CoreOnPublishMessageHandler* handler );

    /**
     * Registers a "On store" message handler (will be invoked after the message has been 
     * stored) for all topics.
     *
     * @param   handler The handler to invoke
     */
    void registerStoreHandler( const CoreOnStoreMessageHandler* handler );

    /**
     * Registers a "On store" message handler (will be invoked after the message has been 
     * stored) for the specified topic.
     *
     * @param   topic The message topic
     * @param   handler The handler to invoke
     */
    void registerStoreHandler( const std::string& topic, const CoreOnStoreMessageHandler* handler );

    /**
     * Registers an "On insert" message handler (will be invoked prior to a message being 
     * inserted for delivery to a destination).
     *
     * @param   handler The handler to invoke
     */
    void registerInsertHandler( const CoreOnInsertMessageHandler* handler );

    /**
     * Registers an "On finalize" message handler (will be invoked prior to a message being 
     * finalized, done with message).
     *
     * @param   handler The handler to invoke
     */
    void registerFinalizeHandler( const CoreOnFinalizeMessageHandler* handler );

    /** {@inheritDoc} */
    bool onPublishMessage( const char* sourceId, const char* canonicalSourceId, bool isBridge,
        uint8_t contextFlags, const char* topic, struct cert_hashes *certHashes ) const;

    /**
     * Invoked when a message to be published has been stored
     *
     * @param   dbId The core database identifier
     * @param   sourceId The source identifier (the context publishing the message)
     * @param   canonicalSourceId The canonical source identifier (the context publishing the message)
     * @param   isBridge Whether the source is a bridge
     * @param   contextFlags The context-specific flags
     * @param   topic The message topic
     * @param   payloadLen The length of the message payload
     * @param   payload The message payload
     * @param   outPayloadLen The length of the output payload (0 if not rewritten)
     * @param   outPayload <code>NULL</code> if not rewritten
     * @param   sourceTenantGuid The tenant source identifier of the client
     * @param   certHashes The certificate hashes associated with the source context
     * @param   certChain The certificate chain associated with the source context
     */
    void onStoreMessage(
        uint64_t dbId, const char* sourceId, const char* canonicalSourceId, bool isBridge, 
        uint8_t contextFlags, const char* topic,
        uint32_t payloadLen, const void* payload,
        uint32_t *outPayloadLen, void** outPayload,
        const char* sourceTenantGuid, struct cert_hashes *certHashes, const char* certChain );

    /**
     * Invoked by core when the queue of packets for a context exceeds the maximum
     * value and a message is attempting to be inserted for delivery.
     *
     * @param   destId The core context identifier that the message is about to be
     *          inserted for
     * @param   isBridge Whether the destination context is a bridge
     * @param   dbId The core database identifier
     * @return  True if the message should be rejected. False if the insert should be allowed
     *          even though the maximum queue size is exceeded.
     */
    bool onPreInsertPacketQueueExceeded( const char* destId, bool isBridge, uint64_t dbId ) const;

    /**
     * Invoked by the core messaging layer for every message that is about to be inserted for delivery
     *
     * @param   destId The core context identifier that will receive the message
     * @param   canonicalDestId The canonical destination identifier
     * @param   isBridge Whether the destination context is a bridge
     * @param   contextFlags The context-specific flags
     * @param   dbId The core database identifier
     * @param   targetTenantGuid The core context tenant identifier that will receive the message
     * @param   certHashes The certificate hashes associated with the destination context
     * @param   isClientMessageEnabled Whether to use the client message (or bridge) (out)
     * @param   clientMessage A message that should be sent to clients versus bridges (if applicable) (out)
     * @param   clientMessageLen The length of the message that should be sent to clients versus bridges
     *          (if applicable) (out)
     * @return  Whether the message should be allowed to be inserted for delivery
     */
    bool onInsertMessage( const char* destId, const char* canonicalDestId, bool isBridge, uint8_t contextFlags, uint64_t dbId,
        const char* targetTenantGuid, struct cert_hashes *certHashes,
        bool* isClientMessageEnabled, unsigned char** clientMessage, size_t* clientMessageLen) const;

    /**
     * Invoked prior to a message being finalized (all work has been completed).
     *
     * After this method has been invoked, the information provided in <code>onStoreMessage()</code> is no
     * longer valid.
     *
     * @param   dbId The core database identifier
     */
    void onFinalizeMessage( uint64_t dbId );

    /** {@inheritDoc} */
    void onCoreMaintenance( time_t time );

    /**
     * Returns the count of publish messages received by the broker per second
     *
     * @return  The count of publish messages received by the broker per second
     */
    float getPublishMessagesPerSecond() { return m_publishMessagesPerSecond; }

    /**
     * Returns the count of messages sent to destinations by the broker per second
     *
     * @return  The count of messages sent to destinations by the broker per second
     */
    float getDestinationMessagesPerSecond() { return m_destMessagesPerSecond; }

private:
    /** Constructor */
    CoreMessageHandlerService();

    /**
     * Fires the "on store" event to the specified handler
     *
     * @param   context The message context
     * @param   certHashes The certificate hashes associated with the source context
     * @param   handler The handler to fire the event to
     */
    void fireOnStoreMessage(
        CoreMessageContext* context, struct cert_hashes *certHashes,
        const CoreOnStoreMessageHandler* handler ) const;

    /**
     * Sets the default values in the DXL message (client id, tenant id, etc.)
     *
     * @param   context The message context
     * @param   sourceTenantGuid The tenant GUID of the source context
     * @param   message The DXL message
     * @param   certChain The certificate chain of the source context
     */
    bool setDxlMessageFields( CoreMessageContext* context,
        const char* sourceTenantGuid, dxl::broker::message::DxlMessage* message,
        const char* certChain ) const;

    /** On publish handlers (topic-based) */
    unordered_map<std::string, const CoreOnPublishMessageHandler*> m_onPublishHandlers;

    /** On store handlers (top-based) */
    unordered_map<std::string, const CoreOnStoreMessageHandler*> m_onStoreHandlers;

    /** On publish handlers (all topics) */
    std::vector<const CoreOnPublishMessageHandler*> m_globalOnPublishHandlers;

    /** On store handlers (all topics) */
    std::vector<const CoreOnStoreMessageHandler*> m_globalOnStoreHandlers;

    /** On insert handlers (all topics) */
    std::vector<const CoreOnInsertMessageHandler*> m_globalOnInsertHandlers;

    /** On finalize handlers (all topics) */
    std::vector<const CoreOnFinalizeMessageHandler*> m_globalOnFinalizeHandlers;

    /** The map of message contexts */
    unordered_map<uint64_t, CoreMessageContext*> m_contexts;

    /** The start of the sample time for message timing */
    time_t m_messageTimingStart;

    /** The current count of publish messages */
    int m_publishMessageCount;

    /** The current count of destinations */
    int m_destinationMessageCount;

    /** The publish messages per second */
    float m_publishMessagesPerSecond;

    /** The destination messages per second */
    float m_destMessagesPerSecond;
};

} /* namespace core */
} /* namespace broker */
} /* namespace dxl */

#endif /* COREMESSAGEHANDLERSERVICE_H_ */
