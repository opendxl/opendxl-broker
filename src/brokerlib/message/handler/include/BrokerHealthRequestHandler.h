/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERHEALTHREQUESTHANDLER_H_
#define BROKERHEALTHREQUESTHANDLER_H_

#include "core/include/CoreOnStoreMessageHandler.h"

namespace dxl {
namespace broker {
namespace message {
namespace handler {

/**
 * Handler for BrokerHealth request messages
 */
class BrokerHealthRequestHandler : public dxl::broker::core::CoreOnStoreMessageHandler
{
public:
    /** Constructor */
    BrokerHealthRequestHandler() {}

    /** Destructor */
    virtual ~BrokerHealthRequestHandler() {}

    /** {@inheritDoc} */
    bool onStoreMessage( dxl::broker::core::CoreMessageContext* context,
        struct cert_hashes *certHashes ) const;
};

} /* namespace handler */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* BROKERHEALTHREQUESTHANDLER_H_ */
