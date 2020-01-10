/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef MULTISERVICEREQUESTRESPONSEPAYLOAD_H_
#define MULTISERVICEREQUESTRESPONSEPAYLOAD_H_

#include <string>
#include "json/include/JsonWriter.h"
#include "serviceregistry/include/ServiceRegistration.h"

namespace dxl {
namespace broker {
namespace message {
namespace payload {

/**
 * Payload containing the response for a multi-service request
 */
class MultiServiceRequestResponsePayload : 
    public dxl::broker::json::JsonWriter
{
public:
    /** 
     * Constructor
     * 
     * @param   servicesByMessageId Map containing the services by request identifier
     */
    explicit MultiServiceRequestResponsePayload(
        const unordered_map<std::string, dxl::broker::service::serviceRegistrationPtr_t> servicesByMessageId ) :
        m_servicesByMessageId( servicesByMessageId ) {}

    /** Destructor */
    virtual ~MultiServiceRequestResponsePayload() {}
    
    /** {@inheritDoc} */
    void write( Json::Value& out ) const;

private:
    /** The services by request identifier */
    const unordered_map<std::string, dxl::broker::service::serviceRegistrationPtr_t>  m_servicesByMessageId;
};

} /* namespace payload */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* MULTISERVICEREQUESTRESPONSEPAYLOAD_H_ */
