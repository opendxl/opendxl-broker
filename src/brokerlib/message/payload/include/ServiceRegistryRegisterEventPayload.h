/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef SERVICEREGISTRYREGISTEREVENTPAYLOAD_H_
#define SERVICEREGISTRYREGISTEREVENTPAYLOAD_H_

#include <cstdint>
#include "include/unordered_set.h"
#include "include/unordered_map.h"
#include "json/include/JsonReader.h"
#include "json/include/JsonWriter.h"
#include "serviceregistry/include/ServiceRegistration.h"

namespace dxl {
namespace broker {
namespace message {
namespace payload {

/**
 * Payload for a "Service registry: register event" message
 */
class ServiceRegistryRegisterEventPayload :     
    public dxl::broker::json::JsonReader,
    public dxl::broker::json::JsonWriter
{
public:
    /** 
     * Constructor
     *
     * @param   reg The service registration
     */
    explicit ServiceRegistryRegisterEventPayload( 
        const dxl::broker::service::ServiceRegistration& reg )
    {
        m_serviceRegistration = reg;
    }

    /** 
     * Constructor
     */
    explicit ServiceRegistryRegisterEventPayload() {}

    /** Destructor */
    virtual ~ServiceRegistryRegisterEventPayload() {}

    /**
     * Returns the wrapped service registration 
     *
     * @return  The wrapped service registration
     */
    dxl::broker::service::ServiceRegistration getServiceRegistration() const {
        return m_serviceRegistration;
    }

    /**
     * Returns the service type
     *
     * @return  The service type
     */
    std::string getServiceType() const { 
        return m_serviceRegistration.getServiceType(); 
    }

    /**
     * Returns the service GUID
     *
     * @return  The service GUID
     */
    std::string getServiceGuid() const { 
        return m_serviceRegistration.getServiceGuid(); 
    }

    /**
     * Returns the client GUID
     *
     * @return  The client GUID
     */
    std::string getClientGuid() const { 
        return m_serviceRegistration.getClientGuid(); 
    }

    /**
     * Sets the client GUID
     *
     * @param   guid The client GUID
     */
    void setClientGuid( const std::string& guid ) {
        m_serviceRegistration.setClientGuid( guid );
    }

    /**
     * Returns the client instance GUID
     *
     * @return  The client instance GUID
     */
    std::string getClientInstanceGuid() const { 
        return m_serviceRegistration.getClientInstanceGuid(); 
    }

    /**
     * Sets the client instance GUID
     *
     * @param   guid The client instance GUID
     */
    void setClientInstanceGuid( const std::string& guid ) {
        m_serviceRegistration.setClientInstanceGuid( guid );
    }

    /**
     * Returns the tenant GUID of the client that registered the service
     *
     * @return  The tenant GUID of the client that registered the service
     */
    std::string getClientTenantGuid() const {
        return m_serviceRegistration.getClientTenantGuid();
    }

    /**
     * Sets the tenant GUID of the client that registered the service
     *
     * @param   tenantGuid The tenant GUID of the client that registered the service
     */
    void setClientTenantGuid( const std::string& tenantGuid ) {
        m_serviceRegistration.setClientTenantGuid( tenantGuid );
    }

    /**
     *  Returns the target tenant GUIDs
     *
     *  @return The target tenant GUIDs
     */
    unordered_set<std::string> getTargetTenantGuids() const {
        return m_serviceRegistration.getTargetTenantGuids();
    }

    /**
     * Sets the tenant GUIDs
     * 
     * @param   tenantGuids The collection of tenant GUIDs
     */
    void setTargetTenantGuids( const unordered_set<std::string>& tenantGuids ) {
        m_serviceRegistration.setTargetTenantGuids( tenantGuids );
    }

    /**
     * Returns the broker GUID
     *
     * @return  The broker GUID
     */
    std::string getBrokerGuid() const { 
        return m_serviceRegistration.getBrokerGuid(); 
    }

    /**
     * Sets the broker GUID
     *
     * @param   guid The broker GUID
     */
    void setBrokerGuid( const std::string& guid ) {
        m_serviceRegistration.setBrokerGuid( guid );
    }

    /**
     * Returns the TTL (in minutes)
     *
     * @return  The TTL (in minutes)
     */
    uint32_t getTtlMins() const { 
        return m_serviceRegistration.getTtlMins(); 
    }

    /**
     * Returns the request channels
     *
     * @return  The request channels
     */
    const unordered_set<std::string> getRequestChannels() const { 
        return m_serviceRegistration.getRequestChannels(); 
    }

    /**
     * Returns the meta-data
     *
     * @return  The meta-data
     */
    const unordered_map<std::string, std::string> getMetaData() const { 
        return m_serviceRegistration.getMetaData(); 
    }

    /**
     * Sets whether the service is associated with a managed client
     *
     * @param   isManagedClient Whether the service is associated with a managed client
     */
    void setManagedClient( bool isManagedClient ) { 
        m_serviceRegistration.setManagedClient( isManagedClient );
    }

    /**
     * Whether the service is associated with a managed client
     *
     * @return  Whether the service is associated with a managed client
     */
    bool isManagedClient() const { return m_serviceRegistration.isManagedClient(); } 

    /**
     * Sets the certificates associated with the client that registered the service 
     *
     * @param   certs The certificates associated with the client that registered the service
     */
    void setCertificates( const unordered_set<std::string>& certs ) {
        m_serviceRegistration.setCertificates( certs );
    }

    /**
     * Returns the certificates associated with the client that registered the service 
     *
     * @return  The certificates associated with the client that registered the service
     */
    const unordered_set<std::string> getCertificates() const {
        return m_serviceRegistration.getCertificates();
    }

    /**
     * Returns the registration time
     *
     * @return  The registration time
     */
    uint64_t getRegistrationTime() const 
    { 
        return m_serviceRegistration.getRegistrationTime();
    }
    
    /** {@inheritDoc} */
    void read( const Json::Value& in );

    /** {@inheritDoc} */
    virtual void write( Json::Value& out ) const;    

    /**
     * Implemented by objects capable of writing their state to a JSON representation. 
     *
     * @param   out The value object to write the JSON representation to.
     * @param   isServiceQuery If we are writing out the JSON as a response to a service query
     *          (if so, don't expose tenant GUIDs).
     */
    virtual void write( Json::Value& out, bool isServiceQuery ) const;    

    /** Equals operator */
    bool operator==( const ServiceRegistryRegisterEventPayload& rhs ) const;

    /** Not equals operator */
    bool operator!= (const ServiceRegistryRegisterEventPayload& rhs ) const {
        return !( *this == rhs );
    }

protected:
    /** The service registration */
    dxl::broker::service::ServiceRegistration m_serviceRegistration;
};

} /* namespace payload */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* SERVICEREGISTRYREGISTEREVENTPAYLOAD_H_ */
