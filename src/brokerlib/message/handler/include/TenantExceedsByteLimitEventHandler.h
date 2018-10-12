/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef TENANTEXCEEDSBYTELIMITEVENTHANDLER_H_
#define TENANTEXCEEDSBYTELIMITEVENTHANDLER_H_

#include "core/include/CoreOnStoreMessageHandler.h"

namespace dxl {
namespace broker {
namespace message {
namespace handler {

/**
 * Handler for "Tenant exceeds byte limit" events
 */
class TenantExceedsByteLimitEventHandler : public dxl::broker::core::CoreOnStoreMessageHandler
{
public:
    /** Constructor */
    TenantExceedsByteLimitEventHandler() {}

    /** Destructor */
    virtual ~TenantExceedsByteLimitEventHandler() {}

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

#endif /* TENANTEXCEEDSBYTELIMITEVENTHANDLER_H_ */
