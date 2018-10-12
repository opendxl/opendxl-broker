/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef SERVICEREGISTRYQUERYREQUESTPAYLOAD_H_
#define SERVICEREGISTRYQUERYREQUESTPAYLOAD_H_

#include <string>
#include "json/include/JsonReader.h"

namespace dxl {
namespace broker {
namespace message {
namespace payload {

/**
 * Payload for a "ServiceRegistry : query" request message
 */
class ServiceRegistryQueryRequestPayload : 
    public dxl::broker::json::JsonReader
{
public:
    /** 
     * Constructor
     */
    explicit ServiceRegistryQueryRequestPayload() {}

    /** Destructor */
    virtual ~ServiceRegistryQueryRequestPayload() {}

    /** {@inheritDoc} */
    void read( const Json::Value& in );    

    /**
     * Returns the service GUID
     *
     * @return  The service GUID
     */
    std::string getServiceGuid() const { return m_serviceGuid; }

    /**
     * Returns the service type
     *
     * @return  The service type
     */
    std::string getServiceType() const { return m_serviceType; }

private:
    /** The service guid */
    std::string m_serviceGuid;
    /** The service type */
    std::string m_serviceType;
};

} /* namespace payload */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* SERVICEREGISTRYQUERYREQUESTPAYLOAD_H_ */
