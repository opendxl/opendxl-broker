/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef SERVICEREGISTRY_H_
#define SERVICEREGISTRY_H_

#include "include/unordered_map.h"
#include "brokerconfiguration/include/BrokerConfigurationServiceListener.h"
#include "core/include/CoreMaintenanceListener.h"
#include "serviceregistry/include/ServiceRegistration.h"
#include "serviceregistry/include/TopicServices.h"
#include <iostream>
#include <map>
#include <memory>
#include <mutex>

namespace dxl {
namespace broker {
/** Namespace for DXL service-related declarations */
namespace service {

/** The service zones by node (broker or hub) */
typedef unordered_map<std::string,ServiceZoneList> serviceZonesByNode_t;

/** The service zones by node pointer type */
typedef std::shared_ptr<serviceZonesByNode_t> serviceZonesByNodePtr_t;

/** The event to request topic mappings */
typedef unordered_map<std::string,std::string> eventToRequestPrefix_t;

/** The name of the event to request topic meta-data property */
const char EVENT_TO_REQUEST_TOPIC_PROP[] = "eventToRequestTopic";

/** The name of the event to request prefix meta-data property */
const char EVENT_TO_REQUEST_PREFIX_PROP[] = "eventToRequestPrefix";

/**
 * Registry containing the different services that are available to service requests.
 * Each service has a set of topics (or channels) that they support. This registry will
 * be used to select the appropriate service to handle a given request (point-to-point).
 */
class ServiceRegistry : 
    public dxl::broker::BrokerConfigurationServiceListener,
    public dxl::broker::core::CoreMaintenanceListener
{
    friend class TopicServices;

public:
    /** Destructor */
    virtual ~ServiceRegistry() {}

    /**
     * Returns the single service instance
     * 
     * @return  The single service instance
     */
    static ServiceRegistry& getInstance();

    /**
     * Registers the specified service with the registry
     *
     * @param   reg The service registration
     */
    void registerService( serviceRegistrationPtr_t reg );

    /**
     * Unregisters the service with the specified GUID. 
     *
     * @param   serviceGuid The GUID of the service to unregister
     * @param   fireEvent Whether to fire an event informing other brokers of the unregistration
     */
    void unregisterService( const std::string& serviceGuid, bool fireEvent = true );

    /**
     * Unregisters the service with the specified GUID. The service will only be unregistered
     * if the client guid and client tenant guid match the guids associated with the service that
     * is attempting to be unregistered
     *
     * @param   serviceGuid The GUID of the service to unregister
     * @param   clientGuid The GUID of the client attempting to unregister
     * @param   clientTenantGuid The tenant GUID of the client attempting to unregister
     * @param   fireEvent Whether to fire an event informing other brokers of the unregistration
     */
    void unregisterService( 
        const std::string& serviceGuid, const std::string& clientGuid, const std::string& clientTenantGuid, 
        bool fireEvent = true );

    /**
     * Unregisters the specified service
     *
     * @param   reg The service to unregister
     * @param   fireEvent Whether to fire an event informing other brokers of the unregistration
     */
    void doUnregisterService( serviceRegistrationPtr_t reg, bool fireEvent );

    /**
     * Finds the service with the specified GUID and the specified tenant GUID
     *
     * @param   serviceGuid The GUID of the service
     * @param   targetServiceTenantGuid The tenant GUID to find the service for
     * @return  The service or an empty pointer
     */
    serviceRegistrationPtr_t findService( const std::string& serviceGuid, const char* targetServiceTenantGuid ) const;

    /**
     * Returns the next service for processing a request on the specified topic and, optionally, the
     * specified tenant GUID
     *
     * @param   topic The topic
     * @param   targetServiceTenantGuid The tenant GUID to find the service for
     * @return  The next service for processing a request on the specified topic
     */
    serviceRegistrationPtr_t getNextService( const std::string& topic, const char* targetServiceTenantGuid = "" );

    /**
     * Returns the services that are the specified service type and, optionally, the specified tenant GUID
     *
     * @param   serviceType The service type
     * @param   tenantGuid The tenant GUID to find the service for
     * @return  The list of services of the specified type
     */
    const std::vector<serviceRegistrationPtr_t> findServicesByType( 
        const std::string& serviceType, const char* tenantGuid = "" ) const;
    
    /**
     * Returns the request prefix for the specified event topic
     *
     * @return  The request prefix for the specified event topic
     */
    std::string getRequestPrefixForEvent( const std::string& eventTopic ) const;

    /**
     * Returns whether services are registered that will accept requests via transforming
     * events.
     *
     * @return  Whether services are registered that will accept requests via transforming
     *          events.
     */
    bool isEventToRequestEnabled() const { return m_eventToRequestPrefix.size() > 0; }
    
    /**
     * Returns all services and, optionally, the specified tenant GUID
     *
     * @param   tenantGuid The tenant GUID to find the service for
     * @return  The list of all services
     */
    const std::vector<serviceRegistrationPtr_t> getAllServices( const char* tenantGuid = ""  ) const;

    /**
     * Sends service registration events for all local services that are registered
     */
    void sendServiceRegistrationEvents() const;

    /**
     * Checks the registered services TTL values. If they have expired, they are removed
     * from the service registry.
     */
    void checkServiceTtls();

    /**
     * Dump state of the registry
     */
    friend std::ostream& operator<<( std::ostream &out, const ServiceRegistry &reg );

    /** {@inheritDoc} */
    void onConfigurationUpdated( std::shared_ptr<const dxl::broker::BrokerConfiguration> brokerConfig );

    /** {@inheritDoc} */
    void onCoreMaintenance( time_t time );

    /**
     * Get the number of Local Services
     *
     * @return  Then number of local services
     */
    unsigned int getLocalSvcCounter() const;

private:
    /** Constructor */
    ServiceRegistry();

    /**
     * Sends a service registration event to other brokers informing of local service 
     * registrations.
     *
     * @param   reg The service registration
      */
    void sendServiceRegistrationEvent( serviceRegistrationPtr_t reg ) const;

    /**
     * Sends a service unregistration event to other brokers informing of local service 
     * unregistrations.
     *
     * @param   reg The service registration
     */
    void sendServiceUnregistrationEvent( serviceRegistrationPtr_t reg ) const;

    /**
     * Sets the service zones by node (hub or broker)
     *
     * @param   zonesByNode The service zones by node (hub or broker)
     */
    void setServiceZonesByNode( serviceZonesByNodePtr_t zonesByNode );

    /**
     * Returns the service zones by node (hub or broker)
     *
     * @return  The service zones by node (hub or broker)
     */
    serviceZonesByNodePtr_t getServiceZonesByNode() const;

    /**
     * Clears all service zones. They will be recalculated on the next request.
     */
    void clearServiceZones();

    /**
     * Increment the number of Local Services Register
     */
    void incLocalSvc( serviceRegistrationPtr_t reg );

    /**
     * Decrement the number of locals Services Registers
     */
    void decLocalSvc( serviceRegistrationPtr_t reg );

    /**
     * Rebuilds the mapping of event topics to request prefixes
     */
    void rebuildEventToRequestPrefixMap();

    /**
     * Adds event to request mapping for the specified service
     *
     * @param   regPtr The registration
     */
    void addEventToRequestPrefix( const serviceRegistrationPtr_t& regPtr );
    
    /**
     * Adds the specified service to the registry 
     *
     * @param   reg The service registration
     */
    void addService( const serviceRegistrationPtr_t reg );

    /**
     * Updates an existing service within the registry
     *
     * @param   existingReg The existing service registration
     * @param   reg The new service registration
     */
    void updateService( serviceRegistrationPtr_t existingReg, const serviceRegistrationPtr_t reg );

    /**
     * Finds the service with the specified GUID
     *
     * @param   serviceGuid The GUID of the service
     * @return  The service or an empty pointer
     */
    serviceRegistrationPtr_t findService( const std::string& serviceGuid ) const;

    /** Service registrations by service identifier */
    unordered_map<std::string, serviceRegistrationPtr_t> m_servicesById;
    /** Service GUIDs by service type */
    std::multimap<std::string, serviceRegistrationPtr_t> m_servicesByType;
    /** Services mapped by topic */
    unordered_map<std::string, topicServicesPtr_t> m_servicesByTopic;
    /** Mutex used when updating service state */
    mutable std::mutex m_mutex;
    /** service zones by node (hub or broker) */
    serviceZonesByNodePtr_t m_zonesByNode;
    /** The last TTL check time */
    time_t m_ttlCheckTime;
    /** Whether the broker configuration has changed */
    bool m_brokerConfigChanged;
    /** number of local services register*/
    unsigned int m_localSvcCounter;
    /** The event to request prefix map */
    eventToRequestPrefix_t m_eventToRequestPrefix;
};

} /* namespace service */
} /* namespace broker */
} /* namespace dxl */

#endif /* SERVICEREGISTRY_H_ */
