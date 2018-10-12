/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef MQTTCOREINTERFACE_H_
#define MQTTCOREINTERFACE_H_

#include "core/include/CoreInterface.h"
#include "mosquitto_broker.h"
#include "mosquitto_internal.h"

namespace dxl {
namespace broker {
namespace core {

/**
 * Concrete implementation of the core interface (used to communicate with the broker library)
 */
class MqttCoreInterface : public CoreInterface
{
public:

    /** 
     * Returns the port of the broker 
     *
     * @return  The port for the GUI
     */
    uint32_t getBrokerPort() const;

    /**
     * Returns the current time based on the way core calculates it
     *
     * @return  The current time based on the way core calculates it
     */
    time_t getCoreTime() const { return mosquitto_time(); };

    /**
     * Returns the host name for the broker
     * 
     * @return  The host name for the broker
     */
    std::string getBrokerHostname() const;

    /**
     * Method that is invoked (by Mosquitto) to check whether the specified context 
     * (representing a bridge) should be allowed to connect to this broker.
     *
     * @param   context The bridge context
     * @return  Whether the specified context (representing a bridge) should be allowed to
     *          connect to this broker.
     */
    bool isBridgeConnectAllowed( const struct mosquitto* context ) const;

    /**
     * Method that is invoked (by Mosquitto) when this broker bridges (connects) to another 
     * broker
     *
     * @param   context The context associated with the bridged connection
     */
    void onBridgeConnected( const struct mosquitto* context ) const;

    /**
     * Method that is invoked (by Mosquitto) when this broker disconnects from another broker
     *
     * @param   context The context associated with the bridged connection
     */
    void onBridgeDisconnected( const struct mosquitto* context ) const;

    /**
     * Method that is invoked (by Mosquitto) when a client connects to this broker
     *
     * @param   context The context associated with the connection
     */
    void onClientConnected( const struct mosquitto* context ) const;

    /**
     * Method that is invoked (by Mosquitto) when a client disconnects from this broker
     *
     * @param   context The context associated with the connection
     */
    void onClientDisconnected( const struct mosquitto* context ) const;

    /**
     * Invoked when a message is about to be published
     *
     * @param   sourceContext The source context
     * @param   topic The message topic
     * @param   payloadLen The length of the payload
     * @param   payload The message payload
     * @param   certHashes The certificate hashes associated with the source context
     * @return  Whether the publish should be allowed
     */
    bool onPublishMessage(
        const struct mosquitto* sourceContext, const char* topic, uint32_t payloadLen, const void* payload, 
        struct cert_hashes *certHashes ) const;

    /**
     * Invoked when a message is stored in the core messaging database for publishing
     *
     * @param   sourceContext The source context (can be null)
     * @param   message The message that was stored
     * @param   outPayloadSize The size of the output message (0 if not rewritten)
     * @param   outPayload The output payload (NULL if not rewritten)
     * @param   certHashes The certificate hashes associated with the source context
     * @param   certChain The certificate chain for the source context
     */
    void onStoreMessage(
        struct mosquitto* sourceContext, struct mosquitto_msg_store *message,
        uint32_t* outPayloadSize, void** outPayload, struct cert_hashes *certHashes,
        const char* certChain );

    /**
     * Invoked when the queue of packets for a context exceeds the maximum
     * value and a message is attempting to be inserted for delivery.
     *
     * @param   destContext The destination context
     * @param   message The message that is going to be inserted
     * @return  True if the message should be rejected. False if the insert should be allowed
     *          even though the maximum queue size is exceeded.
     */
    bool onPreInsertPacketQueueExceeded( 
        struct mosquitto* destContext, struct mosquitto_msg_store *message );

    /**
     * Invoked when a message is about to be inserted for delivery to a destination
     *
     * @param   destContext The destination context
     * @param   message The message that is going to be inserted
     * @param   certHashes The certificate hashes associated with the destination context
     * @param   isClientMessageEnabled Whether to use the client message (or bridge) (out)
     * @param   clientMessage A message that should be sent to clients versus bridges (if applicable) (out)
     * @param   clientMessageLen The length of the message that should be sent to clients versus bridges (out)
     *          (if applicable)
     * @return  Whether to allow the insert to occur
     */
    bool onInsertMessage( struct mosquitto* destContext, struct mosquitto_msg_store *message,
        struct cert_hashes *certHashes, bool* isClientMessageEnabled, unsigned char** clientMessage, size_t* clientMessageLen );

    /** {@inheritDoc} */
    void sendMessage( const char* topic, uint32_t payloadLen, const void* payload ) const;

    /** {@inheritDoc} */
    void setBridgeKeepalive( uint32_t keepAliveMins ) const;

    /** {@inheritDoc} */
    void setConnectionLimit( uint32_t limit ) const;

    /**
     * Returns whether the specified client is connected.
     *
     * NOTE: This method should only be called on the Mosquitto thread as it will be accessing
     * thread unsafe internal structures. Therefore, it should only be called during the mosquitto-based
     * callbacks (message processing).
     *
     * @param   clientGuid The client guid
     * @return  Whether the specified client is connected.
     */
    bool isClientConnected( const std::string& clientGuid ) const;

    /**
     * Disconnects the specified client
     *
     * NOTE: This method should only be called on the Mosquitto thread as it will be accessing
     * thread unsafe internal structures. Therefore, it should only be called during the mosquitto-based
     * callbacks (message processing).
     *
     * @param   clientGuid The client to disconnect
     */
    void disconnectClient( const std::string& clientGuid ) const;

    /** {@inheritDoc} */
    void revokeCerts( struct cert_hashes* revokedCerts ) const;

    /** {@inheritDoc} */
    void restartListeners( struct cert_hashes* managedHashes ) const;

    /**
     * Gets and returns broker health information.
     *
     * @param   brokerHealth The returned broker health information
     */
    void getBrokerHealth( CoreBrokerHealth& brokerHealth ) const;

    /**
     * Gets the current subscription count for the specified topic
     * NOTE: This method is not thread safe. This can only be safely invoked on the Mosquitto thread
     *     (i.e. during message handling callbacks).
     *
     * @param   topic The topic
     * @param   tenantGuid The tenant guid to get the subscription count for (null for all subs)
     * @return  The number of subscriptions for the specified topic
     */
    uint32_t getSubscriptionCount( const std::string& topic, const char* tenantGuid ) const;

    /**
     * Updates the count of bytes sent
     *
     * @param   context The context that is sending the bytes
     * @param   byteCount The count of bytes being sent
     * @return  Whether the context has exceeded its limit
     */
    bool updateTenantSentByteCount( struct mosquitto* context, uint32_t byteCount ) const;

    /**
     * Updates the connection count
     *
     * @param   context The context that is sending the bytes
     * @param   adjCount How to adjust the connection count
     */
    void updateTenantConnectionCount( const struct mosquitto* context, int adjCount ) const;

    /**
     * Returns whether the tenant connection is allowed
     *
     * @param   context The context that is attempting to connect
     * @return  Whether the tenant connection is allowed
     */
    bool isTenantConnectionAllowed( const struct mosquitto* context ) const;

    /**
     * Returns whether the tenant has limits (bytes, connections, etc.)
     *
     * @param   context The connection context
     * @return  Whether the tenant has limits (bytes, connections, etc.)
     */
    bool isTenantLimited( const struct mosquitto* context ) const;

protected:

    /**
     * Invoked when the bridge configuration for the local broker has changed
     *
     * @param   config The updated bridge configuration for the local broker
     */
    void onBridgeConfigurationChanged( const CoreBridgeConfiguration& config );

private:
    /**
     * Obtains and returns the identifier of the "bridged" broker associated with the specified
     * context. The "bridged" broker is another broker that this broker is connected to via a 
     * "bridge" connection. 
     *
     * @param   context The context associated with the bridged connection
     * @param   isChild Returns whether this broker is the child or parent in the bridged (out)
     *          connection.
     * @param   bridgeBridgeId The identifier of the bridged broker. (out)
     */
    void getBridgeBrokerIdFromContext( 
        const struct mosquitto* context, bool& isChild, std::string& bridgeBrokerId ) const;

    /** 
     * Stops the broker from running (exits loop)
     *
     * @param   loopExitCode The exit code for the main loop
     */
    virtual void stopBroker( int loopExitCode ) const;
};

} /* namespace core */
} /* namespace broker */
} /* namespace dxl */

#endif /* MQTTCOREINTERFACE_H_ */
