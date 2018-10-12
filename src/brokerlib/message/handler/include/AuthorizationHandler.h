/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef AUTHORIZATIONHANDLER_H_
#define AUTHORIZATIONHANDLER_H_

#include "core/include/CoreOnPublishMessageHandler.h"
#include "core/include/CoreOnInsertMessageHandler.h"
#include "topicauthorization/include/topicauthorizationservice.h"

namespace dxl {
namespace broker {
namespace message {
namespace handler {

/**
 * Handler for authorizing sending and receiving of messages
 */
class AuthorizationHandler : 
    public dxl::broker::core::CoreOnPublishMessageHandler,
    public dxl::broker::core::CoreOnInsertMessageHandler
{
public:
    /** Constructor */
    AuthorizationHandler() : 
      m_authService( dxl::broker::TopicAuthorizationService::Instance() ) {}

    /** Destructor */
    virtual ~AuthorizationHandler() {}

    /** {@inheritDoc} */
    bool onPublishMessage( const char* sourceId, const char* canonicalSourceId, bool isBridge,
        uint8_t contextFlags, const char* topic, struct cert_hashes *certHashes ) const;

    /** {@inheritDoc} */
    bool onInsertMessage(
        dxl::broker::core::CoreMessageContext* context, const char* destId,
        const char* canonicalDestId, bool isBridge,
        uint8_t contextFlags, const char* targetTenantGuid, struct cert_hashes *certHashes,
        bool* isClientMessageEnabled, unsigned char** clientMessage, size_t* clientMessageLen) const;

private:
    /** The topic authorization service */
    std::shared_ptr<dxl::broker::TopicAuthorizationService> m_authService;
};

} /* namespace handler */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* AUTHORIZATIONHANDLER_H_ */
