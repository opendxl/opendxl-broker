/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef SERVICELOOKUPHANDLER_H_
#define SERVICELOOKUPHANDLER_H_

#include "core/include/CoreOnStoreMessageHandler.h"
#include "serviceregistry/include/ServiceRegistration.h"

namespace dxl {
namespace broker {
namespace message {
namespace handler {

/**
 * Handler for looking up the service to handle an incoming request
 */
class ServiceLookupHandler : public dxl::broker::core::CoreOnStoreMessageHandler
{
public:
    /** Constructor */
    ServiceLookupHandler() {};

    /** Destructor */
    virtual ~ServiceLookupHandler() {}

    /** {@inheritDoc} */
    bool onStoreMessage( dxl::broker::core::CoreMessageContext* context,
        struct cert_hashes *certHashes ) const;

private:

    /**
     * Handles the processing of a DXL request
     *
     * @return  Whether the message should be propagated
     */
    bool handleRequest(
        const dxl::broker::core::CoreMessageContext* context,
        dxl::broker::message::DxlRequest* request ) const;

    /**
     * Handles the processing of a DXL event
     *
     * @return  Whether the message should be propagated
     */
    bool handleEvent(
        dxl::broker::core::CoreMessageContext* context,
        dxl::broker::message::DxlEvent* event ) const;

    /**
     * Handles the processing of a DXL error response
     *
     * @return    Whether the message should be propagated
     */
    bool handleErrorResponse(
        dxl::broker::core::CoreMessageContext* context,
        dxl::broker::message::DxlErrorResponse* errorResponse ) const;

    /**
     * Finds the service for the specified topic
     *
     * @param   topic The topic to find the service for
     * @param   tenantGuid The tenant GUID to find the service for (Optional)
     * @return  The service to use for the specified topic
     */
    dxl::broker::service::serviceRegistrationPtr_t findServiceForTopic(
        const char* topic,
        const char* tenantGuid = "" ) const;
};

} /* namespace handler */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* SERVICELOOKUPHANDLER_H_ */
