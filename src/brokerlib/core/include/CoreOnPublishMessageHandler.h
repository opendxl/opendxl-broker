/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef COREONPUBLISHMESSAGEHANDLER_H_
#define COREONPUBLISHMESSAGEHANDLER_H_

#include <stdint.h>
#include "cert_hashes.h"

namespace dxl {
namespace broker {
namespace core {
    
/**
 * Interface to be implemented by objects that will be notified when particular messages are 
 * about to be published (see CoreMessageHandlerService).
 */
class CoreOnPublishMessageHandler
{
public:
    /** Destructor */
    virtual ~CoreOnPublishMessageHandler() {}

    /**
     * Invoked when a message is about to be published
     *
     * @param   sourceId The source identifier (the context publishing the message)
     * @param   canonicalSourceId The canonical source identifier
     * @param   isBridge Whether the source is a bridge
     * @param   contextFlags The context-specific flags
     * @param   topic The message topic
     * @param   certHashes The certificate hashes associated with the source context
     * @return  Whether the message should be allowed to be published
     */
    virtual bool onPublishMessage( const char* sourceId, const char* canonicalSourceId,
        bool isBridge, uint8_t contextFlags, const char* topic, struct cert_hashes *certHashes ) const = 0;
};

} /* namespace core */
} /* namespace broker */
} /* namespace dxl */

#endif /* COREONPUBLISHMESSAGEHANDLER_H_ */
