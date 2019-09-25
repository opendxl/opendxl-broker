/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef TENANTEXCEEDSLIMITEVENTHANDLER_H_
#define TENANTEXCEEDSLIMITEVENTHANDLER_H_

#include "core/include/CoreOnStoreMessageHandler.h"

namespace dxl {
namespace broker {
namespace message {
namespace handler {

/**
 * Handler for "Tenant exceeds limit" events
 */
class TenantExceedsLimitEventHandler : public dxl::broker::core::CoreOnStoreMessageHandler
{
public:
    /** Constructor */
    TenantExceedsLimitEventHandler() {}

    /** Destructor */
    virtual ~TenantExceedsLimitEventHandler() {}

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

#endif /* TENANTEXCEEDSLIMITEVENTHANDLER_H_ */
