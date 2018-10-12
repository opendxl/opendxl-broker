/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef SERVICEREGISTRYQUERYRESPONSESERVICEPAYLOAD_H_
#define SERVICEREGISTRYQUERYRESPONSESERVICEPAYLOAD_H_

#include "message/payload/include/ServiceRegistryRegisterEventPayload.h"

namespace dxl {
namespace broker {
namespace message {
namespace payload {

/**
 * Payload for the "service" in the response to a "ServiceRegistry: Query" message
 */
class ServiceRegistryQueryResponseServicePayload : public ServiceRegistryRegisterEventPayload    
{
public:
    /** 
     * Constructor
     *
     * @param   reg The service registration
     */
    explicit ServiceRegistryQueryResponseServicePayload( 
        const dxl::broker::service::ServiceRegistration& reg )
        : ServiceRegistryRegisterEventPayload( reg ) {}
    
    /** Destructor */
    virtual ~ServiceRegistryQueryResponseServicePayload() {}

    /** {@inheritDoc} */
    void write( Json::Value& out ) const;    
};

} /* namespace payload */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* SERVICEREGISTRYQUERYRESPONSESERVICEPAYLOAD_H_ */
