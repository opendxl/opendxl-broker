/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERREGISTRYQUERYREQUESTHANDLER_H_
#define BROKERREGISTRYQUERYREQUESTHANDLER_H_

#include "core/include/CoreOnStoreMessageHandler.h"

namespace dxl {
namespace broker {
namespace message {
namespace handler {

/**
 * Handler for "BrokerRegistry : Query" request messages
 */
class BrokerRegistryQueryRequestHandler : public dxl::broker::core::CoreOnStoreMessageHandler
{
public:
    /** Constructor */
    BrokerRegistryQueryRequestHandler() {}

    /** Destructor */
    virtual ~BrokerRegistryQueryRequestHandler() {}

    /** {@inheritDoc} */
    bool onStoreMessage( dxl::broker::core::CoreMessageContext* context,
        struct cert_hashes *certHashes ) const;
};

} /* namespace handler */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* BROKERREGISTRYQUERYREQUESTHANDLER_H_ */
