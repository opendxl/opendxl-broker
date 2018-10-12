/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef SERVICEREGISTRYQUERYREQUESTHANDLER_H_
#define SERVICEREGISTRYQUERYREQUESTHANDLER_H_

#include "core/include/CoreOnStoreMessageHandler.h"

namespace dxl {
namespace broker {
namespace message {
namespace handler {

/**
 * Handler for "ServiceRegistry : Query" request messages
 */
class ServiceRegistryQueryRequestHandler : public dxl::broker::core::CoreOnStoreMessageHandler
{
public:
    /** Constructor */
    ServiceRegistryQueryRequestHandler() {}

    /** Destructor */
    virtual ~ServiceRegistryQueryRequestHandler() {}

    /** {@inheritDoc} */
    bool onStoreMessage( dxl::broker::core::CoreMessageContext* context,
        struct cert_hashes *certHashes ) const;
};

} /* namespace handler */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* SERVICEREGISTRYQUERYREQUESTHANDLER_H_ */
