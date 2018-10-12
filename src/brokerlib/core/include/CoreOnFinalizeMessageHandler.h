/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef COREONFINALIZEMESSAGEHANDLER_H_
#define COREONFINALIZEMESSAGEHANDLER_H_

#include "core/include/CoreMessageContext.h"

namespace dxl {
namespace broker {
namespace core {
    
/**
 * Interface to be implemented by objects that will be notified when a message is about to be 
 * finalized (all work has been completed). (see CoreMessageHandlerService)
 */
class CoreOnFinalizeMessageHandler
{
public:
    /** Destructor */
    virtual ~CoreOnFinalizeMessageHandler() {}

    /**
     * Invoked by the core messaging layer for every message that is about to be about
     * to be finalized (all work has been completed)
     *
     * @param   context The message context
     */
    virtual void onFinalizeMessage( CoreMessageContext* context ) const = 0;
};

} /* namespace core */
} /* namespace broker */
} /* namespace dxl */

#endif /* COREONFINALIZEMESSAGEHANDLER_H_ */
