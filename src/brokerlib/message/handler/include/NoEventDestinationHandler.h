/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef NOEVENTDESTINATIONHANDLER_H_
#define NOEVENTDESTINATIONHANDLER_H_

#include "core/include/CoreOnFinalizeMessageHandler.h"

namespace dxl {
namespace broker {
namespace message {
namespace handler {

/**
 * Handler that sends a notification event if an event is delivered toa broker that does not
 * have a current subscriber. 
 *
 * NOTE: This handler should be used in "test mode" only to validate that topic-based routing
 * is working as expected.
 */
class NoEventDestinationHandler : public dxl::broker::core::CoreOnFinalizeMessageHandler
{
public:
    /** Constructor */
    NoEventDestinationHandler() {}

    /** Destructor */
    virtual ~NoEventDestinationHandler() {}

    /** {@inheritDoc} */
    void onFinalizeMessage(
        dxl::broker::core::CoreMessageContext* context ) const;
};

} /* namespace handler */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* NOEVENTDESTINATIONHANDLER_H_ */
