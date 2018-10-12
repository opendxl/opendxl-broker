/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef SERVICEREGISTRYUNREGISTERREQUESTHANDLER_H_
#define SERVICEREGISTRYUNREGISTERREQUESTHANDLER_H_

#include "core/include/CoreOnStoreMessageHandler.h"

namespace dxl {
namespace broker {
namespace message {
namespace handler {

/**
 * Handler for "ServiceRegistryUnregister" request messages
 * 
 * Unregistration "requests" are sent from DXL clients that are unregistering services.
 * Once a broker has received a request, it will in turn send a service registry "event"
 * to the other brokers. 
 */
class ServiceRegistryUnregisterRequestHandler : 
    public dxl::broker::core::CoreOnStoreMessageHandler
{
public:
    /** Constructor */
    ServiceRegistryUnregisterRequestHandler() {}

    /** Destructor */
    virtual ~ServiceRegistryUnregisterRequestHandler() {}

    /** {@inheritDoc} */
    bool onStoreMessage( dxl::broker::core::CoreMessageContext* context,
        struct cert_hashes *certHashes ) const;
};

} /* namespace handler */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* SERVICEREGISTRYUNREGISTERREQUESTHANDLER_H_ */
