/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/BrokerSettings.h"
#include "include/SimpleLog.h"
#include "message/include/DxlMessage.h"
#include "message/include/DxlMessageConstants.h"
#include "message/include/DxlMessageService.h"
#include "core/include/CoreMessageHandlerService.h"
#include <cstring>

using namespace SimpleLogger;
using namespace std;
using namespace dxl::broker::message;
using namespace dxl::broker::core;

/** {@inheritDoc} */
CoreMessageHandlerService& CoreMessageHandlerService::getInstance()
{
    static CoreMessageHandlerService instance;
    return instance;
}

/** Constructor */
CoreMessageHandlerService::CoreMessageHandlerService() :
    m_messageTimingStart(0),
    m_publishMessageCount(0),
    m_destinationMessageCount(0),
    m_publishMessagesPerSecond(0),
    m_destMessagesPerSecond(0)
{
}

/** {@inheritDoc} */
bool CoreMessageHandlerService::onPublishMessage(
    const char* sourceId, const char* canonicalSourceId, bool isBridge, uint8_t contextFlags,
    const char* topic, struct cert_hashes *certHashes ) const
{
    try
    {
        // Update publish count (cast away const-ness)
        (const_cast<CoreMessageHandlerService*>(this))->m_publishMessageCount++;

        // Global handlers
        for( auto it = m_globalOnPublishHandlers.begin(); it != m_globalOnPublishHandlers.end(); it++ )
        {
            // Invoke the callback
            if( !(*it)->onPublishMessage( sourceId, canonicalSourceId, isBridge, contextFlags,
                    topic, certHashes ) )
            {
                // Don't allow publish
                return false;
            }
        }

        // Find on publish handler by topic
        auto it = m_onPublishHandlers.find( topic );
        if( it != m_onPublishHandlers.end() )
        {
            return it->second->onPublishMessage( sourceId, canonicalSourceId, isBridge, contextFlags,
                        topic, certHashes );
        }

        // Success
        return true;
    }
    catch( const exception& ex )
    {
        SL_START << "Error while invoking on publish handlers(" << topic << "), " << ex.what() << SL_ERROR_END;
    }
    catch( ... )
    {
        SL_START << "Error while invoking on publish handlers(" << topic << "), unknown error" << SL_ERROR_END;
    }

    // An error occurred, don't allow publish
    return false;
}

/** {@inheritDoc} */
void CoreMessageHandlerService::fireOnStoreMessage(
    CoreMessageContext* context,
    struct cert_hashes *certHashes,
    const CoreOnStoreMessageHandler* handler ) const
{
    if( handler->isBridgeSourceRequired() && !context->isSourceBridge() )
    {
        SL_START << "A non-broker attempted to publish a broker-only message (" << 
            context->getTopic() << ")" << SL_ERROR_END;

        // Don't allow publish
        context->setMessageInsertEnabled( false );
    }
    else if( !handler->onStoreMessage( context, certHashes ) )
    {
        // Handler specified that the message not be published
        context->setMessageInsertEnabled( false );
    }
}

/** {@inheritDoc} */
void CoreMessageHandlerService::onStoreMessage(
    uint64_t dbId, const char* sourceId, const char* canonicalSourceId, bool isBridge, 
    uint8_t contextFlags, const char* topic,
    uint32_t payloadLen, const void* payload,
    uint32_t *outPayloadLen, void** outPayload,
    const char* sourceTenantGuid, struct cert_hashes *certHashes, const char* certChain )
{
    CoreMessageContext* context =
        new CoreMessageContext( sourceId, canonicalSourceId, isBridge, contextFlags, topic, payloadLen, payload );
    m_contexts[dbId] = context;

    try
    {
        // Clear output values
        *outPayloadLen = 0;
        *outPayload = NULL;

        // Is it a DXL message?
        DxlMessage* dxlMessage = NULL;
        if( context->isDxlMessage() )
        {
            dxlMessage = context->getDxlMessage();

            // Set DXL message fields appropriately (clientId, tenantId, etc.)
            if( !setDxlMessageFields( context, sourceTenantGuid, dxlMessage, certChain ) )
            {
                // Specify that the message should not be published
                context->setMessageInsertEnabled( false );

                // Exit, do not process any on store handlers
                return;
            }
        }

        // Global handlers
        for( auto it = m_globalOnStoreHandlers.begin(); it != m_globalOnStoreHandlers.end(); it++ )
        {
            fireOnStoreMessage( context, certHashes, *it );
        }

        // Topic-specific handler
        auto it = m_onStoreHandlers.find( topic );
        if( it != m_onStoreHandlers.end() )
        {            
            fireOnStoreMessage( context, certHashes, it->second );
        }

        // Rewrite message if it was modified
        if( dxlMessage && dxlMessage->isDirty() )
        {
            if( SL_LOG.isDebugEnabled() )
            {
                SL_START << "Message is dirty, rewriting message." << SL_DEBUG_END;
            }

            size_t outLen;
            DxlMessageService::getInstance().toBytes( 
                *dxlMessage, reinterpret_cast<unsigned char**>(outPayload), &outLen );
            *outPayloadLen = (uint32_t)outLen;
        }

        // Success
        return;
    }
    catch( const exception& ex )
    {
        SL_START << "Error while invoking on store handlers (" << topic << "), " << ex.what() << SL_ERROR_END;
    }
    catch( ... )
    {
        SL_START << "Error while invoking on store handlers (" << topic << "), unknown error" << SL_ERROR_END;
    }

    // An error occurred, don't allow publish
    context->setMessageInsertEnabled( false );
}

/** {@inheritDoc} */
bool CoreMessageHandlerService::onPreInsertPacketQueueExceeded(
    const char* destId, bool isBridge, uint64_t dbId ) const
{
    //
    // TODO: Use a callback/registration mechanism when the queue is exceeded
    //

    CoreMessageContext* ctx = NULL;
    auto iter = m_contexts.find( dbId );
    if( iter != m_contexts.end() )
    {
        ctx = iter->second;
        if( ctx )
        {    
            try
            {
                bool isDxlMessage = ctx->isDxlMessage();

                // Allow the queue operation to succeed if the context is a bridge, it is not a DXL message,
                // or it is a broker-specific message.
                if( isBridge || !isDxlMessage || 
                    !strncmp( ctx->getTopic(), DXL_BROKER_EVENT_PREFIX, DXL_BROKER_EVENT_PREFIX_LEN ) ||
                    !strncmp( ctx->getTopic(), DXL_BROKER_REQUEST_PREFIX, DXL_BROKER_REQUEST_PREFIX_LEN ) ||
                    !strncmp( ctx->getTopic(), DXL_CLIENT_PREFIX, DXL_CLIENT_PREFIX_LEN ) )
                {
                    if( SL_LOG.isDebugEnabled() )
                    {
                        SL_START << "Overriding packet queue full, allowing insert(" << 
                            ctx->getTopic() << "), destId=" << destId << ", isBridge=" << isBridge << 
                            ", isDxlMessage=" << isDxlMessage << SL_DEBUG_END;
                    }

                    return false;
                }

                if( isDxlMessage && ctx->getDxlMessage()->isRequestMessage() )
                {
                    // Disable sending of the service not found message since we are going to 
                    // send a service is overloaded message
                    ctx->setServiceNotFoundMessageEnabled( false );                    

                    // The service is overloaded, send the appropriate error response message
                    DxlMessageService::getInstance()
                        .sendServiceOverloadedErrorMessage( ctx->getDxlRequest() );
                }

                if( SL_LOG.isDebugEnabled() )
                {
                    SL_START << "Packet queue is full, rejecting insert(" << 
                        ctx->getTopic() << "), destId=" << destId << ", isBridge=" << isBridge << 
                        SL_DEBUG_END;
                }

                return true;
            }
            catch( const exception& ex )
            {
                SL_START << "Error while handling insert packet queue exceeded(" << 
                    ctx->getTopic() << "), " << ex.what() << SL_ERROR_END;
            }
            catch( ... )
            {
                SL_START << "Error while handling insert packet queue exceeded(" << 
                    ctx->getTopic() << "), unknown error" << SL_ERROR_END;
            }
        }
    }

    if( !ctx )
    {
        SL_START << "Unable to find message context (packet queue full)." << dbId << SL_ERROR_END;
    }

    return true;        
}

/** {@inheritDoc} */
bool CoreMessageHandlerService::onInsertMessage( const char* destId, const char* canonicalDestId, bool isBridge,
    uint8_t contextFlags, uint64_t dbId, const char* targetTenantGuid, struct cert_hashes *certHashes,
    bool* isClientMessageEnabled, unsigned char** clientMessage, size_t* clientMessageLen ) const
{
    CoreMessageContext* ctx = NULL;
    auto iter = m_contexts.find( dbId );
    if( iter != m_contexts.end() )
    {
        ctx = iter->second;
        if( ctx )
        {    
            if( !ctx->isMessageInsertEnabled() )
            {
                // Sending of this particular message has been disabled during on store
                return false;
            }

            try
            {
                // Global handlers
                for( auto it = m_globalOnInsertHandlers.begin(); it != m_globalOnInsertHandlers.end(); it++ )
                {
                    // Invoke the callback
                    if( !(*it)->onInsertMessage( ctx, destId, canonicalDestId, isBridge, contextFlags, targetTenantGuid, certHashes,
                            isClientMessageEnabled, clientMessage, clientMessageLen) )
                    {
                        // Don't allow insert
                        return false;
                    }
                }

                // Success
                ctx->incrementDestinationCount();
                return true;
            }
            catch( const exception& ex )
            {
                SL_START << "Error while invoking on insert handlers(" << ctx->getTopic() << "), " << ex.what() << SL_ERROR_END;
            }
            catch( ... )
            {
                SL_START << "Error while invoking on insert handlers(" << ctx->getTopic() << "), unknown error" << SL_ERROR_END;
            }

            // An error occurred, don't allow insert
            return false;
        }
    }

    if( !ctx )
    {
        SL_START << "Unable to find message context." << dbId << SL_ERROR_END;
    }

    return false;        
}

/** {@inheritDoc} */
void CoreMessageHandlerService::onFinalizeMessage( uint64_t dbId )
{
    auto iter = m_contexts.find( dbId );
    if( iter != m_contexts.end() )
    {
        CoreMessageContext* ctx = iter->second;
        if( ctx )
        {
            try
            {
                // Update destination count
                m_destinationMessageCount += ctx->getDestinationCount();

                // Global handlers
                for( auto it = m_globalOnFinalizeHandlers.begin(); 
                    it != m_globalOnFinalizeHandlers.end(); it++ )
                {
                    // Invoke the callback
                    (*it)->onFinalizeMessage( ctx );
                }
            }
            catch( const exception& ex )
            {
                SL_START << "Error while invoking on finalize handlers(" << ctx->getTopic() << "), " << ex.what() << SL_ERROR_END;
            }
            catch( ... )
            {
                SL_START << "Error while invoking on finalize handlers(" << ctx->getTopic() << "), unknown error" << SL_ERROR_END;
            }

            m_contexts.erase( dbId );
            delete ctx;
        }
    }
}


/** {@inheritDoc} */
void CoreMessageHandlerService::registerPublishHandler(
    const CoreOnPublishMessageHandler* handler )
{
    m_globalOnPublishHandlers.push_back( handler );
}

/** {@inheritDoc} */
void CoreMessageHandlerService::registerPublishHandler(
    const std::string& topic, const CoreOnPublishMessageHandler* handler )
{
    m_onPublishHandlers[topic] = handler;
}

/** {@inheritDoc} */
void CoreMessageHandlerService::registerStoreHandler(
    const CoreOnStoreMessageHandler* handler )
{
    m_globalOnStoreHandlers.push_back( handler );
}

/** {@inheritDoc} */
void CoreMessageHandlerService::registerInsertHandler(
    const CoreOnInsertMessageHandler* handler )
{
    m_globalOnInsertHandlers.push_back( handler );
}

/** {@inheritDoc} */
void CoreMessageHandlerService::registerStoreHandler(
    const std::string& topic, const CoreOnStoreMessageHandler* handler )
{
    m_onStoreHandlers[topic] = handler;
}

/** {@inheritDoc} */
void CoreMessageHandlerService::registerFinalizeHandler(
    const CoreOnFinalizeMessageHandler* handler )
{
    m_globalOnFinalizeHandlers.push_back( handler );
}

/** {@inheritDoc} */
void CoreMessageHandlerService::onCoreMaintenance( time_t time )
{    
    const int elapsedSecs = (const int)(time - m_messageTimingStart);
    if( ( m_messageTimingStart == 0 ) || ( elapsedSecs >= BrokerSettings::getMessageSampleSecs() ) )
    {
        if( m_messageTimingStart != 0 )
        {
            m_publishMessagesPerSecond = ((float)m_publishMessageCount / elapsedSecs);
            m_destMessagesPerSecond = ((float)m_destinationMessageCount / elapsedSecs);

            if( SL_LOG.isDebugEnabled() )
            {
                SL_START << "Messages: publish=" << m_publishMessageCount << ", dest=" 
                    << m_destinationMessageCount << ", elapsed=" << elapsedSecs << SL_DEBUG_END;
                SL_START << "Publish messages/sec: " << getPublishMessagesPerSecond() << SL_DEBUG_END;
                SL_START << "Dest messages/sec: " << getDestinationMessagesPerSecond() << SL_DEBUG_END;
            }
        }

        m_messageTimingStart = time;
        m_destinationMessageCount = 0;
        m_publishMessageCount = 0;
    }
}

/** {@inheritDoc} */
bool CoreMessageHandlerService::setDxlMessageFields(
    CoreMessageContext* context, const char* sourceTenantGuid, DxlMessage* message,
    const char* certChain ) const
{
    // Whether the message is from a bridge
    bool isSourceBridge = context->isSourceBridge();
    bool isMultiTenant = BrokerSettings::isMultiTenantModeEnabled();

    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "CoreMessageHandlerService::setDxlMessageFields: topic="
            << context->getTopic() << ", isBridge=" << isSourceBridge << SL_DEBUG_END;
    }
        
    if( !isSourceBridge ) // Message from a local client
    {
        // This should only occur on the initial broker (set the source client and broker)
        message->setSourceBrokerGuid( BrokerSettings::getGuid() );
        message->setSourceClientId( context->getCanonicalSourceId() );    
        message->setSourceClientInstanceId( context->getSourceId() );
        if( certChain )
        {
            // Set the certificates
            message->setOtherField( "dxl.certs", certChain );
        }
        
        if( SL_LOG.isDebugEnabled() )
        {
            SL_START << "Set source broker, client id, client instance id: broker="
                << message->getSourceBrokerGuid() 
                << ", clientId=" 
                << message->getSourceClientId()
                << ", clientInstanceId=" 
                << message->getSourceClientInstanceId()
                << SL_DEBUG_END;
        }
            
        // Clear source tenant if it has been specified in on-premise mode
        const char* sourceTenant = message->getSourceTenantGuid();
        if( !isMultiTenant && sourceTenant && strlen( sourceTenant ) > 0 )
        {
            message->setSourceTenantGuid( "" );
        }
    }
    
    if( isMultiTenant ) // Multi-tenant conditions
    {
        if( !isSourceBridge ) // Message from a local client
        {            
            // Sets the tenant GUID of the source
            message->setSourceTenantGuid( sourceTenantGuid );

            if( SL_LOG.isDebugEnabled() )
            {
                SL_START << "Set source tenant id: tenantId="
                    << message->getSourceTenantGuid() 
                    << SL_DEBUG_END;
            }

            // If the message is from a non-ops tenant and destination tenant
            // guids have been specified, apply the tenant rules
            const unordered_set<std::string>* destTenantGuids = message->getDestinationTenantGuids();
            if ( !context->isSourceOps() &&  destTenantGuids != NULL )
            {
                const char* opsTenantGuid = BrokerSettings::getTenantGuid();
                for( auto iter = destTenantGuids->begin(); iter != destTenantGuids->end(); iter++ )
                {
                    const char* guid = iter->c_str();

                    // If the destination tenant is not the same as the source client 
                    // and it is not ops reject the message
                    if( ( strcmp( guid, sourceTenantGuid ) != 0 ) && 
                        ( strcmp( guid, opsTenantGuid ) != 0 ) )
                    {
                        if( SL_LOG.isDebugEnabled() )
                        {
                            SL_START << "Rejecting message due to invalid destination tenant, "
                                << "destination tenantId= " << guid << SL_DEBUG_END;
                        }

                        return false;
                    }
                }
            }                
        }
        else if( strcmp( BrokerSettings::getTenantGuid(), message->getSourceTenantGuid() ) == 0 ) 
        {
            // Since the message is from a bridge and the message originated from an OPs tenant
            // we need to add the OPs flag to the message context
            //
            // This occurs for both messages from other brokers as well as messages from the
            // local broker (both appear as bridge sources).
            context->setContextFlags( context->getContextFlags() | DXL_FLAG_OPS );
        }
    }

    // propagate message
    return true;
}

