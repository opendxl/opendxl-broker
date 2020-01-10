/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/BrokerSettings.h"
#include "include/SimpleLog.h"
#include "serviceregistry/include/ServiceRegistry.h"

using namespace SimpleLogger;
using namespace std;
using namespace dxl::broker;
using namespace dxl::broker::service;

/** {@inheritDoc} */
TopicServices::TopicServices(  const string& topic ) : m_topic( topic )
{
}

/** {@inheritDoc} */
void TopicServices::clearServiceZones()
{
    // Clear the services by zone
    m_servicesByZone.reset();

    if( SL_LOG.isDebugEnabled() )
        SL_START << "Cleared the services by zone for topic: " 
            << m_topic << SL_DEBUG_END;
}

/** {@inheritDoc} */
void TopicServices::clearServiceTypes()
{
    // Clear the types
    m_serviceTypes.clear();

    if( SL_LOG.isDebugEnabled() )
        SL_START << "Cleared the service types for topic: " 
            << m_topic << SL_DEBUG_END;
}

/** {@inheritDoc} */
void TopicServices::calculateServiceZones()
{
    // Create services by zone
    m_servicesByZone.reset( new zoneServicesVector_t() );

    // Create a copy of the set of services
    std::set<serviceRegistrationPtr_t> services = getServices();
    if( services.size() > 0 )
    {

        // Add local broker services (IPE, etc.)
        zoneServicesPtr_t localServicesPtr( new ZoneServices( "(local)", m_topic ) );

        // Walk the services
         for( auto servicesIter = services.begin(); servicesIter != services.end(); )
         {
            // The current service
             const serviceRegistrationPtr_t& servicePtr = *servicesIter;
            if( servicePtr->isBrokerService() )
            {                
                // Add the service
                localServicesPtr->addService( servicePtr );

                // Remove the service from the remaining services
                services.erase( servicesIter++ );
            }
            else
            {
                servicesIter++;
            }
        }
        if( localServicesPtr->getServicesSize() > 0 )
        {
            m_servicesByZone->push_back( localServicesPtr );
        }
        // Get the service zones by node
        const serviceZonesByNodePtr_t servicesZonesByNode = 
            ServiceRegistry::getInstance().getServiceZonesByNode();

        // Find the service zones for the local broker
        auto localZonesFind = servicesZonesByNode->find( BrokerSettings::getGuid() );
        if( localZonesFind != servicesZonesByNode->end() )
        {
            // The local broker's zones
            const ServiceZoneList& localBrokerZones = localZonesFind->second;
            // Walk each of the local broker's zones
            for( auto localBrokerZoneIter = localBrokerZones.begin(); 
                localBrokerZoneIter != localBrokerZones.end(); localBrokerZoneIter++ )
            {
                // The name of the current service zone
                const std::string& currentZone = *localBrokerZoneIter;

                // Create zone services pointer
                zoneServicesPtr_t zoneServicesPtr;

                // Walk each of the remaining services
                for( auto servicesIter = services.begin(); servicesIter != services.end(); )
                {
                    // The current service
                    const serviceRegistrationPtr_t& servicePtr = *servicesIter;

                    // Find the service zones for the service's broker
                    auto serviceZonesFind = servicesZonesByNode->find( servicePtr->getBrokerGuid() );
                    if( serviceZonesFind != servicesZonesByNode->end() )
                    {
                        // See if the service is available
                        const ServiceZoneList& serviceBrokerZones = serviceZonesFind->second;
                        // TODO: Optimize this find?
                        if( find( serviceBrokerZones.begin(), serviceBrokerZones.end(), currentZone ) 
                                != serviceBrokerZones.end() )
                        {
                            // Create the zone services object if necessary
                            if( !zoneServicesPtr.get() )
                            {
                                zoneServicesPtr.reset( new ZoneServices( currentZone, m_topic ) );
                            }

                            // Adding the service
                            zoneServicesPtr->addService( servicePtr );

                            // Remove the service from the remaining services
                            services.erase( servicesIter++ );
                        }
                        else
                        {
                            servicesIter++;
                        }
                    }
                    else
                    {
                        servicesIter++;
                    }
                }

                // If we found some services in the zone, add them to the services by zone
                // vector.
                if( zoneServicesPtr.get() )
                {
                    m_servicesByZone->push_back( zoneServicesPtr );
                }
            }                
        }

        // Add any remaining services (if applicable)
        if( services.size() > 0 )
        {
            zoneServicesPtr_t zoneServicesPtr( new ZoneServices( "", m_topic ) );
            for( auto servicesIter = services.begin(); servicesIter != services.end(); 
                servicesIter++ )
            {
                zoneServicesPtr->addService( *servicesIter );
            }
            m_servicesByZone->push_back( zoneServicesPtr );
        }
    }

    // Dump services for topic/service zone if applicable
    if( SL_LOG.isDebugEnabled() )
    {
        for( auto iter = m_servicesByZone->begin(); iter != m_servicesByZone->end(); iter++ )
        {
            const zoneServicesPtr_t zonePtr = (*iter);
            SL_START << "Zone topic: " << zonePtr->getTopic() << SL_DEBUG_END;
            SL_START << "    Service zone: " << 
                ( zonePtr->getZone().size() ? zonePtr->getZone() : "(none)" ) << SL_DEBUG_END;
            const auto services = zonePtr->getServices();
            for( auto serviceIter = services.begin(); serviceIter != services.end(); serviceIter++ )
            {
                SL_START << "        Service: " << (*serviceIter)->getServiceGuid()
                    << ", broker=" << (*serviceIter)->getBrokerGuid()
                    << ", client=" << (*serviceIter)->getClientGuid()
                    << SL_DEBUG_END;
            }
        }
    }
}

/** {@inheritDoc} */
topicServiceTypes_t *TopicServices::getServiceTypes() {
SL_START << "TopicServices::getServiceTypes(): " << SL_INFO_END;
    if( m_serviceTypes.empty() )
    {
        auto it = m_serviceTypes.begin();
        for( auto iter = m_services.begin(); iter != m_services.end(); iter++ )
        {
SL_START << "    Service: " << (*iter)->getServiceGuid() << ", " << (*iter)->getServiceType() << SL_INFO_END;            
            it = m_serviceTypes.insert( it, (*iter)->getServiceType() );
SL_START << "        Type: " << (*iter)->getServiceType() << SL_INFO_END;                        
        }
    }
    return &m_serviceTypes;
}

/** {@inheritDoc} */
void TopicServices::addService( serviceRegistrationPtr_t reg ) 
{     
    m_services.insert( reg );
    clearServiceZones();
    clearServiceTypes();
}

/** {@inheritDoc} */
void TopicServices::removeService( serviceRegistrationPtr_t reg ) 
{
    m_services.erase( reg );
    clearServiceZones();
    clearServiceTypes();
}

/** {@inheritDoc} */
serviceRegistrationPtr_t TopicServices::getNextService( const char* targetServiceTenantGuid, 
    const char* serviceType )
{
    if( !m_servicesByZone.get() )
    {
        // Calculate the service zones
        calculateServiceZones();
    }

    // Search for service by zone
    for( auto iter = m_servicesByZone->begin(); iter != m_servicesByZone->end(); iter++ )
    {
        const serviceRegistrationPtr_t& service = (*iter)->getNextService( 
            targetServiceTenantGuid, serviceType );
        if( service.get() )
        {
            return service;
        }
    }

    return serviceRegistrationPtr_t();
}
