/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERDISABLETESTMODEREQUESTHANDLER_H_
#define BROKERDISABLETESTMODEREQUESTHANDLER_H_

#include "core/include/CoreOnStoreMessageHandler.h"

namespace dxl {
namespace broker {
namespace message {
namespace handler {

/**
 * Handler used to disable the test mode
 */
class BrokerDisableTestModeRequestHandler : public dxl::broker::core::CoreOnStoreMessageHandler
{
public:
    /** Constructor */
    BrokerDisableTestModeRequestHandler() {}

    /** Destructor */
    virtual ~BrokerDisableTestModeRequestHandler() {}

    /** {@inheritDoc} */
    bool onStoreMessage( dxl::broker::core::CoreMessageContext* context,
        struct cert_hashes *certHashes ) const;
};

} /* namespace handler */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* BROKERDISABLETESTMODEREQUESTHANDLER_H_ */
