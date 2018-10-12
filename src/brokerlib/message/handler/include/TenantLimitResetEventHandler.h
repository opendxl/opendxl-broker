/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef TENANTLIMITRESETEVENTHANDLER_H_
#define TENANTLIMITRESETEVENTHANDLER_H_

#include "core/include/CoreOnStoreMessageHandler.h"

namespace dxl {
namespace broker {
namespace message {
namespace handler {

/**
 * Handler for "Tenant reset byte limits" events
 */
class TenantLimitResetEventHandler : public dxl::broker::core::CoreOnStoreMessageHandler
{
public:
    /** Constructor */
    TenantLimitResetEventHandler() {}

    /** Destructor */
    virtual ~TenantLimitResetEventHandler() {}

    /** {@inheritDoc} */
    bool isBridgeSourceRequired() const { return false; }

    /** {@inheritDoc} */
    bool onStoreMessage( dxl::broker::core::CoreMessageContext* context,
        struct cert_hashes *certHashes ) const;
};

} /* namespace handler */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* TENANTLIMITRESETEVENTHANDLER_H_ */
