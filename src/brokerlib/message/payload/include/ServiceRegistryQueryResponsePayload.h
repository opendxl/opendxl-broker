/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef SERVICEREGISTRYQUERYRESPONSEPAYLOAD_H_
#define SERVICEREGISTRYQUERYRESPONSEPAYLOAD_H_

#include <list>
#include "json/include/JsonWriter.h"
#include "serviceregistry/include/ServiceRegistration.h"

namespace dxl {
namespace broker {
namespace message {
namespace payload {

/**
 * Payload for a "ServiceRegistry : query" response message
 */
class ServiceRegistryQueryResponsePayload : 
    public dxl::broker::json::JsonWriter
{
public:
    /** 
     * Constructor
     *
     * @param   registrations The registrations to return
     */
    explicit ServiceRegistryQueryResponsePayload( 
        const std::vector<dxl::broker::service::serviceRegistrationPtr_t> registrations ) :
        m_registrations( registrations ) {}

    /** Destructor */
    virtual ~ServiceRegistryQueryResponsePayload() {}

    /** {@inheritDoc} */
    void write( Json::Value& out ) const;    

private:
    /** The registrations */
    const std::vector<dxl::broker::service::serviceRegistrationPtr_t> m_registrations;
};

} /* namespace payload */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* SERVICEREGISTRYQUERYRESPONSEPAYLOAD_H_ */
