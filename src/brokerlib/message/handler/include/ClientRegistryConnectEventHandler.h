/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef CLIENTREGISTRYCONNECTEVENTHANDLER_H_
#define CLIENTREGISTRYCONNECTEVENTHANDLER_H_

#include "core/include/CoreOnStoreMessageHandler.h"

namespace dxl {
namespace broker {
namespace message {
namespace handler {

/**
 * Handler for "Connect event" messages
 */
class ClientRegistryConnectEventHandler : public dxl::broker::core::CoreOnStoreMessageHandler
{
public:
    /** Constructor */
    ClientRegistryConnectEventHandler() {}

    /** Destructor */
    virtual ~ClientRegistryConnectEventHandler() {}

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

#endif /* CLIENTREGISTRYCONNECTEVENTHANDLER_H_ */
