/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef ZONESERVICES_H_
#define ZONESERVICES_H_

#include "brokerregistry/include/brokerregistry.h"
#include "serviceregistry/include/ServiceRegistration.h"
#include "topicauthorization/include/topicauthorizationservice.h"
#include <set>
#include <string>

namespace dxl {
namespace broker {
namespace service {

/**
 * Contains information regarding the services available in a service zone
 */
class ZoneServices
{
public:
    /**
      * Constructs the service zone
     *
     * @param   zone The name of the zone
     * @param   topic The name of the topic associated with the zone
     */
    ZoneServices( const std::string& zone, const std::string& topic );

    /** Destructor */
    virtual ~ZoneServices() {}

    /**
      * Returns the name of the zone
     *
     * @return  The zone
     */
    std::string getZone() const { return m_zone; }

    /**
      * Returns the name of the topic
     *
     * @return  The topic
     */
    std::string getTopic() const { return m_topic; }

    /**
     * Adds the specified service to the set of services that are in this zone
     * 
     * @param   reg The service registration to add
     */
    void addService( serviceRegistrationPtr_t reg );

    /**
     * Removes the specified service from the set of services that support the zone
     * 
     * @param   reg The service registration to remove
     */
    void removeService( serviceRegistrationPtr_t reg );

    /**
     * Returns the services associated with the zone
     *
     * @return  The services associated with the zone
     */
    std::set<serviceRegistrationPtr_t> getServices() const { return m_services; }

    /**
     * Returns the count of services
     *
     * @return  The count of services
     */
    uint32_t getServicesSize() const { return (uint32_t)m_services.size(); }

    /**
     * Returns the next service for processing a request
     *
     * @param   targetServiceTenantGuid The tenant GUID to find the service for
     * @param   serviceType The type of service to lookup (optional)
     * @return  The next service for processing a request
     */
    serviceRegistrationPtr_t getNextService( const char* targetServiceTenantGuid = "",
        const char* serviceType = NULL );

private:
    /** The zone */
    std::string m_zone;

    /** The topic */
    std::string m_topic;

    /** The services in this zone */
    std::set<serviceRegistrationPtr_t> m_services;

    /** Iterator to the next service for request processing */
    std::set<serviceRegistrationPtr_t>::iterator m_nextServiceIter;

    /** The local broker GUID */
    std::string m_localBrokerGuid;

    /** The broker registry */
    dxl::broker::BrokerRegistry& m_brokerRegistry;

    /** The topic authorization service */
    std::shared_ptr<TopicAuthorizationService> m_authService;
};

/** The zone services pointer type */
typedef std::shared_ptr<ZoneServices> zoneServicesPtr_t;

} /* namespace service */
} /* namespace broker */
} /* namespace dxl */

#endif /* ZONESERVICES_H_ */
