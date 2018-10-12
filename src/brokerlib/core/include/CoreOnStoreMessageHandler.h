/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef COREONSTOREMESSAGEHANDLER_H_
#define COREONSTOREMESSAGEHANDLER_H_

#include "core/include/CoreMessageContext.h"
#include "cert_hashes.h"

namespace dxl {
namespace broker {
namespace core {
    
/**
 * Interface to be implemented by objects that will be notified when a message is 
 * stored in the core messaging database prior to sending to recipients
 * (see CoreMessageHandlerService).
 */
class CoreOnStoreMessageHandler
{
public:
    /** Destructor */
    virtual ~CoreOnStoreMessageHandler() {}

    /**
     * Whether the source of the message must be a "bridge". If the source is not a bridge,
     * an error will be logged, and the message will not be delivered to any recipients.
     */
    virtual bool isBridgeSourceRequired() const { return false; }

    /**
     * Invoked after a message has been stored in the core database for sending to
     * recipients.
     *
     * @param   context The message context
     * @param   certHashes The certificate hashes associated with the source context
     */
    virtual bool onStoreMessage( CoreMessageContext* context,
        struct cert_hashes *certHashes ) const = 0;
};

} /* namespace core */
} /* namespace broker */
} /* namespace dxl */

#endif /* CORENSTOREMESSAGEHANDLER_H_ */
