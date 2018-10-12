/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef NOREQUESTDESTINATIONHANDLER_H_
#define NOREQUESTDESTINATIONHANDLER_H_

#include "core/include/CoreOnFinalizeMessageHandler.h"

namespace dxl {
namespace broker {
namespace message {
namespace handler {

/**
 * Handler that ensures that DXL request messages have been sent to a destination. If they have not
 * been sent to a destination, an error message will be sent.
 */
class NoRequestDestinationHandler : public dxl::broker::core::CoreOnFinalizeMessageHandler
{
public:
    /** Constructor */
    NoRequestDestinationHandler() {}

    /** Destructor */
    virtual ~NoRequestDestinationHandler() {}

    /** {@inheritDoc} */
    void onFinalizeMessage(
        dxl::broker::core::CoreMessageContext* context ) const;
};

} /* namespace handler */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* NOREQUESTDESTINATIONHANDLER_H_ */
