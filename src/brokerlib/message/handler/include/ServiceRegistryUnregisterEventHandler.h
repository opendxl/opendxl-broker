/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef SERVICEREGISTRYUNREGISTEREVENTHANDLER_H_
#define SERVICEREGISTRYUNREGISTEREVENTHANDLER_H_

#include "core/include/CoreOnStoreMessageHandler.h"

namespace dxl {
namespace broker {
namespace message {
namespace handler {

/**
 * Handler for "ServiceRegistryUnregisterEvent" messages
 */
class ServiceRegistryUnregisterEventHandler : 
    public dxl::broker::core::CoreOnStoreMessageHandler
{
public:
    /** Constructor */
    ServiceRegistryUnregisterEventHandler() {}

    /** Destructor */
    virtual ~ServiceRegistryUnregisterEventHandler() {}

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

#endif /* SERVICEREGISTRYUNREGISTEREVENTHANDLER_H_ */
