/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef SERVICEREGISTRYREGISTEREVENTHANDLER_H_
#define SERVICEREGISTRYREGISTEREVENTHANDLER_H_

#include "core/include/CoreOnStoreMessageHandler.h"

namespace dxl {
namespace broker {
namespace message {
namespace handler {

/**
 * Handler for "ServiceRegistryRegisterEvent" messages
 */
class ServiceRegistryRegisterEventHandler : 
    public dxl::broker::core::CoreOnStoreMessageHandler
{
public:
    /** Constructor */
    ServiceRegistryRegisterEventHandler() {}

    /** Destructor */
    virtual ~ServiceRegistryRegisterEventHandler() {}

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

#endif /* SERVICEREGISTRYREGISTEREVENTHANDLER_H_ */
