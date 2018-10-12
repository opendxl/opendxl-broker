/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERTOPICEVENTHANDLER_H_
#define BROKERTOPICEVENTHANDLER_H_

#include "core/include/CoreOnStoreMessageHandler.h"

namespace dxl {
namespace broker {
namespace message {
namespace handler {

/**
 * Handler for "Broker topic event" messages
 */
class BrokerTopicEventHandler : public dxl::broker::core::CoreOnStoreMessageHandler
{
public:
    /** Constructor */
    BrokerTopicEventHandler() {}

    /** Destructor */
    virtual ~BrokerTopicEventHandler() {}

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

#endif /* BROKERSTATEEVENTHANDLER_H_ */
