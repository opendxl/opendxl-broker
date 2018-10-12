/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef CLIENTREGISTRYQUERYREQUESTHANDLER_H_
#define CLIENTREGISTRYQUERYREQUESTHANDLER_H_

#include "core/include/CoreOnStoreMessageHandler.h"

namespace dxl {
namespace broker {
namespace message {
namespace handler {

/**
 * Handler for "ClientRegistry : Query" request messages
 */
class ClientRegistryQueryRequestHandler : public dxl::broker::core::CoreOnStoreMessageHandler
{
public:
    /** Constructor */
    ClientRegistryQueryRequestHandler() {}

    /** Destructor */
    virtual ~ClientRegistryQueryRequestHandler() {}

    /** {@inheritDoc} */
    bool onStoreMessage( dxl::broker::core::CoreMessageContext* context,
        struct cert_hashes *certHashes ) const;
};

} /* namespace handler */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* CLIENTREGISTRYQUERYREQUESTHANDLER_H_ */
