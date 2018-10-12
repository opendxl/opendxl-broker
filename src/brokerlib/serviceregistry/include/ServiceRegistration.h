/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef SERVICEREGISTRATION_H_
#define SERVICEREGISTRATION_H_

#include "include/unordered_set.h"
#include "include/unordered_map.h"
#include "cert_hashes.h"
#include <cstdint>
#include <ctime>
#include <iostream>
#include <memory>

namespace dxl {
namespace broker {
namespace service {

/**
 * An individual service registration
 */
class ServiceRegistration
{
public:
    /** 
     * Constructor
     *
     * @param   serviceType The service type
     * @param   serviceGuid The service GUID
     * @param   clientGuid The client GUID
     * @param   clientInstanceGuid Instance identifier for the client
     * @param   brokerGuid The broker GUID
     * @param   ttlMins TTL in minutes
     * @param   requestChannels The request channels
     * @param   metaData The meta data
     * @param   clientTenantGuid The client tenant guid
     * @param   targetTenantGuids Tenant guids that the service is available to
     * @param   certificates The certificates associated with the client that registered the service
     * @param   managedClient Whether the service is associated with a managed client
     */
    explicit ServiceRegistration( 
        const std::string& serviceType = "", 
        const std::string& serviceGuid = "",
        const std::string& clientGuid = "",
        const std::string& clientInstanceGuid = "",
        const std::string& brokerGuid = "",
        uint32_t ttlMins = 0, 
        const unordered_set<std::string> requestChannels = 
            unordered_set<std::string>(),
        const unordered_map<std::string, std::string> metaData = 
            unordered_map<std::string, std::string>(),
        const std::string& clientTenantGuid = "", 
        const unordered_set<std::string> targetTenantGuids = unordered_set<std::string>(),
        const unordered_set<std::string> certificates = unordered_set<std::string>(),
        bool managedClient = true );

    /** Copy constructor */
    ServiceRegistration( const ServiceRegistration& reg );

    /** Assignment operator */
    ServiceRegistration& operator=( const ServiceRegistration& other );

    /** Destructor */
    virtual ~ServiceRegistration();

    /**
     * Returns the service type
     *
     * @return  The service type
     */
    std::string getServiceType() const { return m_serviceType; }

    /**
     * Sets the service type
     *
     * @param   serviceType The service type
     */
    void setServiceType( const std::string& serviceType ) {  m_serviceType = serviceType; }

    /**
     * Returns the service GUID
     *
     * @return  The service GUID
     */
    std::string getServiceGuid() const { return m_serviceGuid; }

    /**
     * Sets the service GUID
     *
     * @param   guid The service GUID
     */
    void setServiceGuid( const std::string& guid ) {  m_serviceGuid = guid; }

    /**
     * Returns the client GUID
     *
     * @return  The client GUID
     */
    std::string getClientGuid() const { return m_clientGuid; }

    /**
     * Returns the client instance GUID
     *
     * @return  The client instance GUID
     */
    std::string getClientInstanceGuid() const { 
        return m_clientInstanceGuid.empty() ? getClientGuid() : m_clientInstanceGuid; 
    }

    /**
     * Sets the client GUID
     *
     * @param   guid The client GUID
     */
    void setClientGuid( const std::string& guid ) { m_clientGuid = guid; }

    /**
     * Sets the client instance GUID
     *
     * @param   guid The client instance GUID
     */
    void setClientInstanceGuid( const std::string& guid ) { m_clientInstanceGuid = guid; }

    /**
     * Returns the broker GUID
     *
     * @return  The broker GUID
     */
    std::string getBrokerGuid() const { return m_brokerGuid; }

    /**
     * Sets the broker GUID
     *
     * @param   guid The broker GUID
     */
    void setBrokerGuid( const std::string& guid );

    /**
     * Returns the TTL (in minutes)
     *
     * @return  The TTL (in minutes)
     */
    uint32_t getTtlMins() const { return m_ttlMins; }

    /**
     * Returns the adjusted TTL (in minutes). The TTL based on the current
     * time.
     *
     * @return  The adjusted TTL (in minutes). The TTL based on the current time.
     */
    uint32_t getAdjustedTtlMins() const;

    /**
     * Sets the TTL (in minutes)
     *
     * @param   ttlMins The TTL (in minutes)
     */
    void setTtlMins( uint32_t ttlMins ) { m_ttlMins = ttlMins; }

    /**
     * Returns the request channels
     *
     * @return  The request channels
     */
    const unordered_set<std::string> getRequestChannels() const { return m_requestChannels; }

    /**
     * Sets the request channels
     *
     * @param   channels The request channels
     */
    void setRequestChannels( const unordered_set<std::string>& channels ) { 
        m_requestChannels = channels;
    }

    /**
     * Returns the meta-data
     *
     * @return  The meta-data
     */
    const unordered_map<std::string, std::string> getMetaData() const { return m_metaData; }

    /**
     * Sets the meta-data
     *
     * @param   metaData The meta-data
     */
    void setMetaData( const unordered_map<std::string, std::string>& metaData ) { 
        m_metaData = metaData;
    }

    /**
     * Returns the tenant GUID of the client that registered the service
     *
     * @return  The tenant GUID of the client that registered the service
     */
    std::string getClientTenantGuid() const { return m_clientTenantGuid; }

    /**
     * Sets the tenant GUID of the client that registered the service
     *
     * @param   tenantGuid The tenant GUID of the client that registered the service
     */
    void setClientTenantGuid( const std::string& tenantGuid );
    
    /**
     * Returns whether the service was registerd by an OPS client
     *
     * @return  Whether the service was registerd by an OPS client
     */
    bool isOps() const { return m_serviceIsOps; }

    /**
     * Returns the collection of target tenant GUIDs
     *
     * @return  The collection of target tenant GUIDs
     */
    unordered_set<std::string> getTargetTenantGuids() const { return m_targetTenantGuids; }

    /**
     * Sets the collection of target tenant GUIDs
     *
     * @param   tenantGuids The collection of target tenant GUIDs
     */
    void setTargetTenantGuids( const unordered_set<std::string>& tenantGuids ) { m_targetTenantGuids = tenantGuids;    }

    /**
     * Whether the service is local to this broker
     *
     * @return  Whether the service is local to this broker
     */
    bool isLocal() const { return m_isLocal; }

    /** 
     * Whether the registration has expired (based on TTL)
     *
     * @return  Whether the registration has expired (based on TTL)
     */
    bool isExpired() const;

    /** 
     * Returns the registration time 
     *
     * @return  The registration time
     */
    uint64_t getRegistrationTime() const { return m_regTime; }

    /** 
     * Sets the registration time 
     *
     * @param   regTime The registration time
     */
    void setRegistrationTime( uint64_t regTime ) { m_regTime = regTime; }

    /**
     * Resets the registration time to be the current time. The TTL is updated
     * accordingly.
     */
    void resetRegistrationTime();

    /**
      *
      * Sets whether the service is associated with the broker
      *
      * @param  brokerService Whether the service is associated with the broker
     */
    void setBrokerService( bool brokerService ) {
        m_brokerService = brokerService ;
    }

    /**
     * Whether the service is associated with the broker
     *
     * @return  Whether the service is associated with the broker
     */
    bool isBrokerService() const { return m_brokerService; }

    /**
     * Whether the service is associated with a managed client
     *
     * @return  Whether the service is associated with a managed client
     */
    bool isManagedClient() const { return m_managedClient; } 

    /**
     * Sets whether the service is associated with a managed client
     *
     * @param   isManagedClient Whether the service is associated with a managed client
     */
    void setManagedClient( bool isManagedClient ) { m_managedClient = isManagedClient; } 

    /**
     * Returns the certificates associated with the client that registered the service 
     *
     * @return  The certificates associated with the client that registered the service
     */
    const unordered_set<std::string> getCertificates() const { return m_certificates; }

    /**
     * Sets the certificates associated with the client that registered the service 
     *
     * @param   certs The certificates associated with the client that registered the service
     */
    void setCertificates( const unordered_set<std::string>& certs );

    /**
     * Returns the certificates associated with the client (as a UT hash) that registered 
     * the service 
     *
     * @return  The certificates associated with the client (as a UT hash) that registered
     *          the service
     */
    struct cert_hashes* getCertificatesUtHash() const { return m_certsUthash; }

    /** Equals operator */
    virtual bool operator==( const ServiceRegistration& rhs ) const;

    /** Not equals operator */
    virtual bool operator!= ( const ServiceRegistration& rhs ) const {
        return !( *this == rhs );
    }

    /**
     * Whether the service is available to the specified tenant
     *
     * @param   tenantGuid The guid of a tenant
     * @return  If the service is available to the specified tenant
     */
    bool isServiceAvailable( const char* tenantGuid ) const;

    /**
     * Dump state of the registration
     */
    friend std::ostream& operator<<( std::ostream &out, const ServiceRegistration &reg );

protected:
    /**
     * Returns the adjusted TTL (in minutes). The TTL based on the specified
     * time.
     *
     * @param   time The time to adjust based on
     * @return  The adjusted TTL (in minutes). The TTL based on the specified
     *          time.
     */
    uint32_t getAdjustedTtlMins( time_t time ) const;

    /** 
     * Frees the certificates associated with the client (as a UT hash)
     */
    void freeCertificatesUtHash();

    /** The service type */
    std::string m_serviceType;
    /** The service GUID */
    std::string m_serviceGuid;
    /** The client GUID */
    std::string m_clientGuid;
    /** The client instance GUID */
    std::string m_clientInstanceGuid;
    /** The broker GUID */
    std::string m_brokerGuid;
    /** The registration TTL (minutes) */
    uint32_t m_ttlMins;
    /** The request channels */
    unordered_set<std::string> m_requestChannels;
    /** The meta-data */
    unordered_map<std::string, std::string> m_metaData;
    /** Whether the service is local to the broker */
    bool m_isLocal;
    /** The time the registration was registered */
    time_t m_regTime;
    /** Whether the service is associated with the broker */
    bool m_brokerService;
    /** The client tenant GUID */
    std::string m_clientTenantGuid;
    /** The service tenant GUID is an OPs*/
    bool m_serviceIsOps;
    /** The target tenant GUIDs */
    unordered_set<std::string> m_targetTenantGuids;
    /** The certificates associated with the client that registered the service */
    unordered_set<std::string> m_certificates;
    /** Whether the client that registered the service is managed */
    bool m_managedClient;
    /** The certificates associated with the client (as a UT hash) */
    struct cert_hashes* m_certsUthash;
};

/** The service registration pointer type */
typedef std::shared_ptr<ServiceRegistration> serviceRegistrationPtr_t;

} /* namespace service */
} /* namespace broker */
} /* namespace dxl */

#endif /* SERVICEREGISTRATION_H_ */
