/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERSTATETOPICSEVENTHANDLER_H_
#define BROKERSTATETOPICSEVENTHANDLER_H_

#include "core/include/CoreOnStoreMessageHandler.h"

namespace dxl {
namespace broker {
namespace message {
namespace handler {

/**
 * Handler for "BrokerStateTopicsEvent" messages
 */
class BrokerStateTopicsEventHandler : public dxl::broker::core::CoreOnStoreMessageHandler
{
public:
    /** Constructor */
    BrokerStateTopicsEventHandler() {}

    /** Destructor */
    virtual ~BrokerStateTopicsEventHandler() {}

    /** {@inheritDoc} */
    bool isBridgeSourceRequired() const { return true; }

    /** {@inheritDoc} */
    bool onStoreMessage( dxl::broker::core::CoreMessageContext* context,
        struct cert_hashes *certHashes ) const;
};

} /* namespace handler */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* BROKERSTATETOPICSEVENTHANDLER_H_ */
