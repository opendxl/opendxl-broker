/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef COREONINSERTMESSAGEHANDLER_H_
#define COREONINSERTMESSAGEHANDLER_H_

#include "cert_hashes.h"
#include "core/include/CoreMessageContext.h"

namespace dxl {
namespace broker {
namespace core {
    
/**
 * Interface to be implemented by objects that will be notified when a message is about to be 
 * inserted by the core messaging layer for delivery to a destination.
 * (see CoreMessageHandlerService).
 */
class CoreOnInsertMessageHandler
{
public:
    /** Destructor */
    virtual ~CoreOnInsertMessageHandler() {}

    /**
     * Invoked by the core messaging layer for every message that is about to be inserted for delivery
     *
     * @param   context The message context
     * @param   destId The core context identifier that will receive the message
     * @param   canonicalDestId The canonical destination identifier
     * @param   isBridge Whether the destination context is a bridge
     * @param   contextFlags The context-specific flags
     * @param   targetTenantGuid The core context tenant identifier that will receive the message
     * @param   certHashes The certificate hashes associated with the destination context
     * @param   isClientMessageEnabled Whether to use the client message (or bridge) (out)
     * @param   clientMessage A message that should be sent to clients versus bridges (if applicable) (out)
     * @param   clientMessageLen The length of the message that should be sent to clients versus bridges
     *          (if applicable) (out)
     * @return  Whether the message should be allowed to be inserted for delivery
     */
    virtual bool onInsertMessage(
        CoreMessageContext* context, const char* destId, const char* canonicalDestId, bool isBridge, uint8_t contextFlags,
        const char* targetTenantGuid, struct cert_hashes *certHashes, 
        bool* isClientMessageEnabled, unsigned char** clientMessage, size_t* clientMessageLen ) const = 0;
};

} /* namespace core */
} /* namespace broker */
} /* namespace dxl */

#endif /* COREONINSERTMESSAGEHANDLER_H_ */
