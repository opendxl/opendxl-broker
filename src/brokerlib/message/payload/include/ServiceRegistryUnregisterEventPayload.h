/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef SERVICEREGISTRYUNREGISTEREVENTPAYLOAD_H_
#define SERVICEREGISTRYUNREGISTEREVENTPAYLOAD_H_

#include "json/include/JsonReader.h"
#include "json/include/JsonWriter.h"

namespace dxl {
namespace broker {
namespace message {
namespace payload {

/**
 * Payload for a "Service registry: unregister event" message
 */
class ServiceRegistryUnregisterEventPayload :     
    public dxl::broker::json::JsonReader,
    public dxl::broker::json::JsonWriter
{
public:
    /** 
     * Constructor
     *
     * @param   serviceGuid The service GUID to unregister
     */
    explicit ServiceRegistryUnregisterEventPayload( 
        const std::string& serviceGuid = "" )
    {
        m_serviceGuid = serviceGuid;
    }

    /** Destructor */
    virtual ~ServiceRegistryUnregisterEventPayload() {}

    /**
     * Returns the GUID of the service to unregister
     *
     * @return  The GUID of the service to unregister
     */
    std::string getServiceGuid() const { return m_serviceGuid; }

    /** {@inheritDoc} */
    void read( const Json::Value& in );

    /** {@inheritDoc} */
    void write( Json::Value& out ) const;    

    /** Equals operator */
    bool operator==( const ServiceRegistryUnregisterEventPayload& rhs ) const;

    /** Not equals operator */
    bool operator!= (const ServiceRegistryUnregisterEventPayload& rhs ) const {
        return !( *this == rhs );
    }

private:
    /** The service GUID */
    std::string m_serviceGuid;
};

} /* namespace payload */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* SERVICEREGISTRYUNREGISTEREVENTPAYLOAD_H_ */
