/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/SimpleLog.h"
#include "include/BrokerSettings.h"
#include "message/include/DxlMessageConstants.h"
#include "message/handler/include/AuthorizationHandler.h"
#include "DxlFlags.h"
#include <cstring>

using namespace std;
using namespace dxl::broker;
using namespace dxl::broker::message::handler;
using namespace dxl::broker::core;

/** {@inheritDoc} */
bool AuthorizationHandler::onPublishMessage(
    const char* /* sourceId */, const char* canonicalSourceId, bool isBridge, uint8_t contextFlags, const char* topic,
    struct cert_hashes *certHashes ) const
{
    // Swap the identifier if applicable for local connections
    const char* clientSourceId = ( (contextFlags & DXL_FLAG_LOCAL) ? BrokerSettings::getGuid() : canonicalSourceId );


    bool authorized = ( isBridge || 
        ( ( contextFlags & DXL_FLAG_MANAGED ) ?
            m_authService->isAuthorizedToPublish( clientSourceId, topic ) :
            m_authService->isAuthorizedToPublish( certHashes, topic ) ) );

    if( !authorized ) 
    {
        if( SL_LOG.isInfoEnabled() )
        {
            SL_START << "Not authorized for send: " << clientSourceId << " (" <<
                topic << ")" << SL_INFO_END;
        }
    }

    return authorized;
}

/** {@inheritDoc} */
bool AuthorizationHandler::onInsertMessage(
    CoreMessageContext* context, const char* /*destId*/, const char* canonicalDestId, bool isBridge,
    uint8_t contextFlags, const char* /*targetTenantGuid*/, struct cert_hashes *certHashes,
    bool* /*isClient*/, unsigned char** /*clientMessage*/, size_t* /*clientMessageLen*/ ) const
{
    // Swap the identifier if applicable for local connections
    const char* clientDestId = ( (contextFlags & DXL_FLAG_LOCAL) ? BrokerSettings::getGuid() : canonicalDestId );

        
    bool authorized = ( isBridge || 
        ( ( contextFlags & DXL_FLAG_MANAGED ) ?
            m_authService->isAuthorizedToSubscribe( clientDestId, context->getTopic() ) :
            m_authService->isAuthorizedToSubscribe( certHashes, context->getTopic() ) ) );

    if( !authorized ) 
    {
        if( SL_LOG.isInfoEnabled() )
        {
            SL_START << "Not authorized for receive: " << clientDestId << " (" <<
                context->getTopic() << ")" << SL_INFO_END;
        }
    }

    return authorized;
}
