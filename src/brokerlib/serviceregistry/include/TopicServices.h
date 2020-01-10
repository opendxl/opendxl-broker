/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef TOPICSERVICES_H_
#define TOPICSERVICES_H_

#include "brokerregistry/include/brokerregistry.h"
#include "serviceregistry/include/ServiceRegistration.h"
#include "serviceregistry/include/ZoneServices.h"
#include "topicauthorization/include/topicauthorizationservice.h"
#include <set>
#include <vector>

namespace dxl {
namespace broker {
namespace service {

/** A vector of services by zone */
typedef std::vector<zoneServicesPtr_t> zoneServicesVector_t;

/** The zones pointer type */
typedef std::shared_ptr<zoneServicesVector_t> zonesPtr_t;

/** The unique types for the topic */
typedef std::set<std::string> topicServiceTypes_t;

/**
 * Contains information regarding the services that are mapped to a particular topic
 */
class TopicServices
{
    friend class ServiceRegistry;

public:
    /**
      * Constructs the object 
     *
     * @param   topic The topic the services are mapped to
     */
    TopicServices( const std::string& topic );

    /** Destructor */
    virtual ~TopicServices() {}

    /**
      * Returns the topic
     *
     * @return  The topic
     */
    std::string getTopic() const { return m_topic; }

    /**
     * Adds the specified service to the set of services that support the topic
     * 
     * @param   reg The service registration to add
     */
    void addService( serviceRegistrationPtr_t reg );

    /**
     * Removes the specified service from the set of services that support the topic
     * 
     * @param   reg The service registration to remove
     */
    void removeService( serviceRegistrationPtr_t reg );

    /**
     * Returns the services associated with the topic
     *
     * @return  The services associated with the topic
     */
    std::set<serviceRegistrationPtr_t> getServices() const { return m_services; }

    /**
     * Returns the unique types for the services
     * <P>
     * NOTE: This is returning a pointer for efficiency. The returned value
     * should not be retained.
     * </P>
     *
     * @return  The unique types for the services
     */
    topicServiceTypes_t *getServiceTypes();

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
     * @param   serviceType The type of service to lookup
     * @return  The next service for processing a request
     */
    serviceRegistrationPtr_t getNextService( const char* targetServiceTenantGuid = "",
        const char* serviceType = NULL );

private:
    /** The topic */
    std::string m_topic;

    /** The set of service GUIDs that support the topic */
    std::set<serviceRegistrationPtr_t> m_services;

    /** The unique types for the services registered */
    topicServiceTypes_t m_serviceTypes;

    /** The list of services by service zone */
    zonesPtr_t m_servicesByZone;

    /**
     * Clears the current service zones. They will be recalculated on the next request.
     */
    void clearServiceZones();

    /**
     * Clears the service types
     */
    void clearServiceTypes();

    /**
     * Calculates and stores the current service zones for the topic
     */
    void calculateServiceZones();
};

/** The topic services pointer type */
typedef std::shared_ptr<TopicServices> topicServicesPtr_t;

} /* namespace service */
} /* namespace broker */
} /* namespace dxl */

#endif /* TOPICSERVICES_H_ */
