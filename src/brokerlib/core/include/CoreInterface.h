/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef COREINTERFACE_H_
#define COREINTERFACE_H_

#include "CoreBridgeConfiguration.h"
#include "CoreMaintenanceListener.h"
#include "CoreOnPublishMessageHandler.h"
#include "CoreBrokerHealth.h"
#include "cert_hashes.h"
#include <ctime>
#include <string>

namespace dxl {
namespace broker {
/** Namespace for declarations related to communicating with the core messaging layer */
namespace core {

/**
 * The level of a particular log message
 */
enum LogLevel
{
    info, debug, warn, error
};

/**
 * The purpose for this interface is to centralize all communication to/from the core messaging
 * layer. Information from this library that will be accessed by core are delivered via this interface.
 * Conversely, any information needed in the broker library via core will be obtained via this
 * interface.
 *
 * Methods that are to be implemented by core are pure virtual.
 *
 * In terms of memory allocation, any data delivered via this interface should only be considered
 * valid for the duration of the invocation. A local copy of data should be created for any 
 * information that is retained.
 */
class CoreInterface : public CoreOnPublishMessageHandler
{
public:
    /** Constructor */
    CoreInterface();

    /** Destructor */
    virtual ~CoreInterface() {}

    /**
     * Returns the GUID for the broker
     *
     * @return  The GUID for the broker
     */
    const char* getBrokerGuid() const;

    /**
     * Returns the GUID for the broker tenant
     *
     * @return  The GUID for the broker tenant
     */
    const char* getBrokerTenantGuid() const;

    /** 
     * Returns the ASN1 NID for the Client GUID in certificates 
     *
     * @return  The ASN1 NID for the Client GUID in certificates
     */
    int getClientGuidNid() const;

    /** 
     * Returns the ASN1 NID for the Tenant GUID in certificates 
     *
     * @return  The ASN1 NID for the Tenant GUID in certificates
     */
    int getTenantGuidNid() const;

    /** 
     * Returns the port of the broker 
     *
     * @return  The port for the GUI
     */
    virtual uint32_t getBrokerPort() const = 0;

    /**
     * Returns the current time based on the way core calculates it
     *
     * @return  The current time based on the way core calculates it
     */
    virtual time_t getCoreTime() const = 0;

    /**
     * Returns the host name for the broker
     * 
     * @return  The host name for the broker
     */
    virtual std::string getBrokerHostname() const = 0;

    /**
     * Sets the bridge configuration for the local broker 
     *
     * @param   config The bridge configuration for the local broker
     */
    void setBridgeConfiguration( const CoreBridgeConfiguration& config );

    /**
     * Forces the current bridge configuration to be re-applied in core.
     * This essentially will force the bridge to be disconnected and reconnected.
     */
    void forceBridgeReconnect();

    /**
     * Method that is invoked (by core) to check whether the specified broker
     * should be allowed to connect to this broker.
     *
     * @param   brokerId The identifier (GUID) of the other broker
     * @return  Whether the specified broker should be allowed to connect to this broker.
     */
    bool isBridgeConnectAllowed( const std::string& brokerId ) const;
        
    /**
     * Method that is invoked (by core) when this broker bridges (connects) to another
     * broker
     *
     * @param   isChild Whether this broker is the child or parent in the bridged connection
     * @param   brokerId The identifier (GUID) of the other broker
     */
    void onBridgeConnected( bool isChild, const std::string& brokerId ) const;

    /**
     * Method that is invoked (by core) when this broker disconnects from another broker
     *
     * @param   isChild Whether this broker was the child or parent in the bridged connection
     * @param   brokerId The identifier (GUID) of the other broker
     */
    void onBridgeDisconnected( bool isChild, const std::string& brokerId ) const;

    /**
     * Method that is invoked (by core) when a client connects to this broker
     *
     * @param   clientId The identifier (GUID) of the client
     */
    void onClientConnected( const std::string& clientId ) const;

    /**
     * Method that is invoked (by core) when a client disconnects from this broker
     *
     * @param   clientId The identifier (GUID) of the client
     */
    void onClientDisconnected( const std::string& clientId ) const;

    /**
     * Sends notification to the rest of the fabric (other brokers) indicating that the fabric
     * has changed. 
     */
    void sendFabricChangeNotification() const;

    /**
     * Sends a notification to the rest of the fabric (other brokers) about the state of the 
     * local broker.
     */
    void sendLocalBrokerStateEvent();

    /**
     * Returns whether the specified certificate is revoked
     *
     * NOTE: This method should only be called on the core thread as it will be accessing
     * thread unsafe internal structures. 
     *
     * @return  Whether the specified certificate is revoked
     */
    bool isCertRevoked( const char* cert ) const;

    /**
     * Returns whether to validate certificates against the connection identifier
     *
     * @return  Whether to validate certificates against the connection identifier
     */
    bool isCertIdentityValidationEnabled();

    /**
     * Returns whether multi-tenant mode is enabled
     *
     * @return  Whether multi-tenant mode is enabled
     */
    bool isMultiTenantModeEnabled() const;

    /**
    * Returns whether test mode is enabled
    * 
    * @return   Whether test mode is enabled
    */
    bool isTestModeEnabled() const;

    /**
     * Returns whether a new tenant client subscription is allowed
     *
     * @param   tenantGuid The guid of the tenant trying to subscribe
     * @param   subscriptionCount The current number of subscriptions the client has
     * @return  whether a new tenant client subscription is allowed
     */
    bool isTenantSubscriptionAllowed( const char* tenantGuid, int subscriptionCount ) const;

    /**
     * Sends a message to the fabric (The message will be sent during the next core maintenance interval).
     *
     * @param   topic The topic for the message
     * @param   payloadLen The length of the payload
     * @param   payload The message payload
     */
    virtual void sendMessage( const char* topic, uint32_t payloadLen, const void* payload ) const = 0;

    /**
     * Sets the keep-alive interval for bridges. This value will not be applied to currently bridge connections.
     * It will be applied the next time they bridge.
     *
     * @param   keepAliveMins The keep alive in minutes
     */
    virtual void setBridgeKeepalive( uint32_t keepAliveMins ) const = 0;

    /**
     * Sets the connection limit for the broker. This value will not be applied to current connections.
     * (It will not disconnect clients if the newly applied limit is exceeded)
     *
     * @param   limit The connection limit
     */
    virtual void setConnectionLimit( uint32_t limit ) const = 0;

    /**
     * Returns whether the specified client is connected.
     *
     * NOTE: This method should only be called on the core messaging thread as it will be accessing
     * thread unsafe internal structures. Therefore, it should only be called during the core-based
     * callbacks (message processing).
     *
     * @param   clientGuid The client guid
     * @return  Whether the specified client is connected.
     */
    virtual bool isClientConnected( const std::string& clientGuid ) const = 0;

    /**
     * Disconnects the specified client
     *
     * NOTE: This method should only be called on the core messaging thread as it will be accessing
     * thread unsafe internal structures. Therefore, it should only be called during the core-based
     * callbacks (message processing).
     *
     * @param   clientGuid The client to disconnect
     */
    virtual void disconnectClient( const std::string& clientGuid ) const = 0;

    /**
     * Revoke certificates. NOTE: This method will free the revoked certificates that are passed
     *
     * @param   revokedCerts The certificates that have been revoked
     */
    virtual void revokeCerts( struct cert_hashes* revokedCerts ) const = 0;

    /**
     * Forces the core listeners to be restarted
     *
     * @param   managedHashes The core listeners are typically restarted due to certificate changes.
     *          If this parameter is non-null it will update core's set of "managed hashes"
     */
    virtual void restartListeners( struct cert_hashes* managedHashes ) const = 0;

    /**
     * Gets and returns broker health information.
     * NOTE: This method is not thread safe. This can only be safely invoked on the core messaging
     * thread (i.e. during message handling callbacks).
     *
     * @param   brokerHealth The returned broker health information
     */
    virtual void getBrokerHealth( CoreBrokerHealth& brokerHealth ) const = 0;

    /** 
     * Stops the broker from running (exits loop)
     *
     * @param   exitCode The exit code for the main loop
     */
    virtual void stopBroker( int exitCode ) const = 0;

    /**
     * Gets the current subscription count for the specified topic
     * NOTE: This method is not thread safe. This can only be safely invoked on the core messaging
     * thread (i.e. during message handling callbacks).
     *
     * @param   topic The topic
     * @param   tenantGuid The tenant guid to get the subscription count for (null for all subs)
     * @return  The number of subscriptions for the specified topic
     */
    virtual uint32_t getSubscriptionCount( const std::string& topic, const char* tenantGuid ) const = 0;

    /**
     * Invoked by core messaging layer for every message that is about to be published
     *
     * @param   sourceId The core context identifier that is publishing the message
     * @param   canonicalSourceId The canonical source identifier
     * @param   isBridge Whether the source context is a bridge
     * @param   contextFlags The context-specific flags
     * @param   topic The message topic
     * @param   certHashes The certificate hashes associated with the source context
     * @return  Whether the message should be allowed to be published
     */
    bool onPublishMessage( const char* sourceId, const char* canonicalSourceId, bool isBridge,
        uint8_t contextFlags, const char* topic, struct cert_hashes *certHashes ) const;

    /**
     * Invoked by core messaging layer after a message to publish has been stored.
     *
     * All of the information provided in this method is valid until after the <code>onFinalizeMessage()</code>
     * method has been invoked. 
     *
     * @param   dbId The core database identifier
     * @param   sourceId The core context identifier that is publishing the message
     * @param   canonicalSourceId The canonical source identifier
     * @param   isBridge Whether the source context is a bridge
     * @param   contextFlags The context-specific flags
     * @param   topic The message topic
     * @param   payloadLen The length of the message payload
     * @param   payload The message payload
     * @param   outPayloadLen The length of the message payload (0 if not rewritten)
     * @param   outPayload The output payload (NULL if not rewritten)
     * @param   sourceTenantGuid The core context source tenant identifier
     * @param   certHashes The certificate hashes associated with the source context
     * @param   certChain The certificate chain associated with the source context
     */
    void onStoreMessage(
        uint64_t dbId, const char* sourceId, const char* canonicalSourceId, bool isBridge, 
        uint8_t contextFlags, const char* topic,
        uint32_t payloadLen, const void* payload,
        uint32_t* outPayloadLen, void** outPayload,
        const char* sourceTenantGuid,
        struct cert_hashes *certHashes,
        const char* certChain ) const;

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
     * Invoked by core for every message that is about to be inserted for delivery
     *
     * @param   destId The core context identifier that will receive the message
     * @param   canonicalDestId The canonical destination identifier
     * @param   isBridge Whether the destination context is a bridge
     * @param   contextFlags The context specific flags
     * @param   dbId The core database identifier
     * @param   targetTenantGuid The core context tenant identifier that will receive the message
     * @param   certHashes The certificate hashes associated with the destination context
     * @param   isClientMessageEnabled Whether to use the client message (or bridge) (out)
     * @param   clientMessage A message that should be sent to clients versus bridges (if applicable) (out)
     * @param   clientMessageLen The length of the message that should be sent to clients versus bridges
     *          (if applicable) (out)
     * @return  Whether the message should be allowed to be inserted for delivery
     */
    bool onInsertMessage( const char* destId, const char* canonicalDestId, bool isBridge, uint8_t contextFlags,
        uint64_t dbId, const char* targetTenantGuid, struct cert_hashes *certHashes,
        bool* isClientMessageEnabled, unsigned char** clientMessage, size_t* clientMessageLen ) const;

    /**
     * Invoked prior to a message being finalized (all work has been completed).
     *
     * After this method has been invoked, the information provided in <code>onStoreMessage()</code> is no
     * longer valid.
     *
     * @param   dbId The core database identifier
     */
    void onFinalizeMessage( uint64_t dbId ) const;

    /**
     * Invoked when core does its maintenance
     *
     * @param   time The time of the core maintenance (in seconds). This value is in
     *          terms of "core time" (For example getCoreTime())
     */
    void onCoreMaintenance( time_t time );

    /**
     * Invoked by core when a new topic has been subscribed to on the broker.
     * (the first time it is subscribed to across the connected clients).
     *
     * @param   topic The topic
     */
    void onTopicAddedToBroker( const char *topic ) const;

    /**
     * Invoked by core when a topic is no longer being subscribed to on the broker.
     * (no subscriptions remain across all of the connected clients).
     *
     * @param   topic The topic
     */
    void onTopicRemovedFromBroker( const char *topic ) const;

    /**
     * Updates the count of bytes sent
     *
     * @param   tenantId The tenant identifier
     * @param   byteCount The count of bytes being sent
     * @return  Whether the tenant has exceeded its limit
     */
    bool updateTenantSentByteCount( const char* tenantId, uint32_t byteCount ) const;

    /**
     * Updates the connection count
     *
     * @param   tenantId The tenant identifier
     * @param   adjCount The adjustment to the connection count
     */
    void updateTenantConnectionCount( const char* tenantId, int adjCount ) const;

    /**
     * Returns whether the tenant connection is allowed
     *
     * @param   tenantId The tenant identifier
     * @return  Whether the tenant connection is allowed
     */
    bool isTenantConnectionAllowed( const char* tenantId ) const;

    /**
     * Adds a listener that is notified when core maintenance is performed
     *
     * @param   listener The listener to add
     */
    void addMaintenanceListener( CoreMaintenanceListener* listener );

    /**
     * Removes a listener that is notified when core maintenance is performed
     *
     * @param   listener The listener to remove
     */
    void removeMaintenanceListener( CoreMaintenanceListener* listener );

    /**
     * Method invoked by core to log via the broker library
     *
     * @param   level The log level
     * @param   message The log message
     */
    void log( LogLevel level, const char* message ) const;

protected:

    /**
     * Invoked when the bridge configuration for the local broker has changed
     *
     * @param   config The updated bridge configuration for the local broker
     */
    virtual void onBridgeConfigurationChanged( const CoreBridgeConfiguration& config ) = 0;
    
private:
    /** The bridge configuration for the local broker */
    CoreBridgeConfiguration m_bridgeConfig;

    /** Registered listeners for core maintenance */
    std::vector<CoreMaintenanceListener*> m_maintListeners;

    /** The last time the local broker state was sent */
    time_t m_lastLocalBrokerStateSend;

};

} /* namespace core */
} /* namespace broker */
} /* namespace dxl */

#endif /* COREINTERFACE_H_ */
