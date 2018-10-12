/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef SERVICEREGISTRYREGISTERREQUESTHANDLER_H_
#define SERVICEREGISTRYREGISTERREQUESTHANDLER_H_

#include "core/include/CoreOnStoreMessageHandler.h"

namespace dxl {
namespace broker {
namespace message {
namespace handler {

/**
 * Handler for "ServiceRegistryRegister" request messages
 *
 * Registration "requests" are sent from DXL clients that are registering services.
 * Once a broker has received a request, it will in turn send a service registry "event"
 * to the other brokers.
 */
class ServiceRegistryRegisterRequestHandler : 
    public dxl::broker::core::CoreOnStoreMessageHandler
{
public:
    /** Constructor */
    ServiceRegistryRegisterRequestHandler() {}

    /** Destructor */
    virtual ~ServiceRegistryRegisterRequestHandler() {}

    /** {@inheritDoc} */
    bool onStoreMessage( dxl::broker::core::CoreMessageContext* context,
        struct cert_hashes *certHashes ) const;
};

} /* namespace handler */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* SERVICEREGISTRYREGISTERREQUESTHANDLER_H_ */
