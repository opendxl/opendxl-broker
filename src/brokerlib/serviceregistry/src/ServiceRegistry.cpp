/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/brokerlib.h"
#include "include/BrokerSettings.h"
#include "include/SimpleLog.h"
#include "include/unordered_map.h"
#include "brokerconfiguration/include/BrokerConfigurationService.h"
#include "message/include/DxlMessageService.h"
#include "message/include/DxlMessageConstants.h"
#include "message/payload/include/ServiceRegistryRegisterEventPayload.h"
#include "message/payload/include/ServiceRegistryUnregisterEventPayload.h"
#include "serviceregistry/include/ServiceRegistry.h"
#include "metrics/include/TenantMetricsService.h"

#include <cstring>

using namespace SimpleLogger;
using namespace std;
using namespace dxl::broker;
using namespace dxl::broker::message;
using namespace dxl::broker::message::payload;
using namespace dxl::broker::metrics;

namespace dxl {
namespace broker {
namespace service {

/** {@inheritDoc} */
ServiceRegistry& ServiceRegistry::getInstance()
{
    static ServiceRegistry instance;
    return instance;
}

/** {@inheritDoc} */
ServiceRegistry::ServiceRegistry() : 
    m_zonesByNode( new serviceZonesByNode_t() ), 
    m_ttlCheckTime( 0 ), 
    m_brokerConfigChanged( false ),
    m_localSvcCounter ( 0 )
{
    // Add listener to broker configuration
    BrokerConfigurationService::Instance()->addListener( this );

    // Add maintenance listener
    getCoreInterface()->addMaintenanceListener( this );
}

/** {@inheritDoc} */
void ServiceRegistry::setServiceZonesByNode( serviceZonesByNodePtr_t zonesByNode )
{
    // Lock when setting service zones (invoked typically via MA threads)
    unique_lock<mutex> lck( m_mutex );

    m_zonesByNode = zonesByNode;

    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "ServiceRegistry::setServiceZonesByNode" << SL_DEBUG_END; 
        for( auto iter = m_zonesByNode->begin(); iter != m_zonesByNode->end(); iter++ )
        {
            SL_START << "    node: " << iter->first << SL_DEBUG_END; 
            for( auto iter2 = iter->second.begin(); iter2 != iter->second.end(); iter2++ )
            {
                SL_START << "        " << *iter2 << SL_DEBUG_END; 
            }
        }
    }
}

/** {@inheritDoc} */
serviceZonesByNodePtr_t ServiceRegistry::getServiceZonesByNode() const
{
    // Lock when getting service zones (setter is called by MA, etc.)
    unique_lock<mutex> lck( m_mutex );

    return m_zonesByNode;
}

/** {@inheritDoc} */
void ServiceRegistry::incLocalSvc( serviceRegistrationPtr_t reg )
{
    if ( reg->isLocal() )
    {
        m_localSvcCounter++;
    }
}

/** {@inheritDoc} */
void ServiceRegistry::decLocalSvc( serviceRegistrationPtr_t reg )
{
    
    if ( reg->isLocal() )
    {
        m_localSvcCounter--;
    }
}

/** {@inheritDoc} */
 unsigned int ServiceRegistry::getLocalSvcCounter(  ) const 
{
    return m_localSvcCounter;
}

/** {@inheritDoc} */
void ServiceRegistry::registerService( serviceRegistrationPtr_t reg )
{    
    serviceRegistrationPtr_t existingReg = findService( reg->getServiceGuid() );
    bool regExists = existingReg.get() != 0;

    if( !regExists ) 
    {
        // Add the new service
        addService( reg );
    }
    else
    {
        // If the service already exists, and the tenant guids match, 
        // attempt to update it.
        if( reg->getClientTenantGuid() == existingReg->getClientTenantGuid() )
        {
            updateService( existingReg, reg );
        }
        else 
        {    
            // The client GUIDs do not match the registered service, ignoring the registration
            if( SL_LOG.isInfoEnabled() )
                SL_START << "The service could not be updated, its client-related GUIDS do not match: "
                    << " serviceGuid=" << reg->getServiceGuid() 
                    << ", clientTenantGuid=" << existingReg->getClientTenantGuid() << ":" << reg->getClientTenantGuid()                    
                    <<  SL_INFO_END;            
            return;
        }
    }

    if( !BrokerSettings::isMultiTenantModeEnabled() || reg->isOps() )
    {
        // Check for event to request related properties
        addEventToRequestPrefix( reg );
    }

    // Send a register event to the other brokers if it is a local service
    sendServiceRegistrationEvent( reg );

    if( BrokerSettings::isMultiTenantModeEnabled() && !reg->isOps() )
    {
        TenantMetricsService::getInstance().updateTenantServiceCount( reg->getClientTenantGuid().c_str(), 1 );
    }
}

/** {@inheritDoc} */
void ServiceRegistry::unregisterService( const std::string& serviceGuid, bool fireEvent )
{
    serviceRegistrationPtr_t reg = findService( serviceGuid );
    if( reg.get() )
    {
        doUnregisterService( reg, fireEvent );
    }

    if( BrokerSettings::isMultiTenantModeEnabled() && !reg->isOps() )
    {
        TenantMetricsService::getInstance().updateTenantServiceCount( reg->getClientTenantGuid().c_str(), -1 );
    }
}

/** {@inheritDoc} */
void ServiceRegistry::unregisterService( 
    const std::string& serviceGuid, const std::string& clientGuid, const std::string& clientTenantGuid,
    bool fireEvent )
{
    // Service is found and the client and client tenant guids match
    serviceRegistrationPtr_t reg = findService( serviceGuid );
    if( reg.get() && 
        ( clientGuid == reg->getClientGuid() ) && 
        ( clientTenantGuid == reg->getClientTenantGuid() ) )
    {
        doUnregisterService( reg, fireEvent );
    }
}

/** {@inheritDoc} */
void ServiceRegistry::doUnregisterService( serviceRegistrationPtr_t reg, bool fireEvent )
{
    // Service is found and the client and client tenant guids match
    if( reg.get() )
    {
        // Remove from by service identifier
        m_servicesById.erase( reg->getServiceGuid() );

        // Remove from by service type
        auto equalResult = m_servicesByType.equal_range( reg->getServiceType() );
        for( auto iter = equalResult.first; iter != equalResult.second; iter++ )
        {
            if( iter->second->getServiceGuid() == reg->getServiceGuid() )
            {
                m_servicesByType.erase( iter );
                break;
            }
        }

        // Remove from service by channels (topics)
        const unordered_set<string> channels = reg->getRequestChannels();
        for( auto channelIter = channels.begin(); channelIter != channels.end(); channelIter++ )
        {
            const string topic = *channelIter;
            auto servicesByTopic = m_servicesByTopic.find( topic );
            if( servicesByTopic != m_servicesByTopic.end() ) 
            {
                topicServicesPtr_t topicServicesPtr = servicesByTopic->second;
                topicServicesPtr->removeService( reg );
                if( topicServicesPtr->getServicesSize() == 0 )
                {
                    m_servicesByTopic.erase( topic );
                }
            }            
        }
        // Decrement # of Local Services Register
        decLocalSvc( reg );

        if( !BrokerSettings::isMultiTenantModeEnabled() || reg->isOps() )
        {
            // Reset event to to request map if applicable
            unordered_map<std::string, std::string> md = reg->getMetaData();
            auto hasEventMapping = md.find( EVENT_TO_REQUEST_PREFIX_PROP );
            if( hasEventMapping != md.end() )
            {
                rebuildEventToRequestPrefixMap();

                if( SL_LOG.isDebugEnabled() )
                    SL_START << "Removed event to request: " << hasEventMapping->second
                        << ", mapSize: " << m_eventToRequestPrefix.size() << SL_DEBUG_END;
            }
        }

        // Send an unregister event to the other brokers if it is a local service
        if( fireEvent )
        {
            sendServiceUnregistrationEvent( reg );
        }
    }
}


/** {@inheritDoc} */
void ServiceRegistry::addService( const serviceRegistrationPtr_t reg ) 
{
    if( SL_LOG.isDebugEnabled() )
            SL_START << "Service registered: " << reg->getServiceGuid() << 
                ", performing full registration." << SL_DEBUG_END;    

    // Register by service identifier
    m_servicesById[ reg->getServiceGuid() ] = reg;        
        
    // Register service identifier by server type 
    m_servicesByType.insert( 
        pair<string,serviceRegistrationPtr_t>( reg->getServiceType(), reg ) );
        
    // Register service by channels (topics)
    const unordered_set<string> channels = reg->getRequestChannels();
    for( auto iter = channels.begin(); iter != channels.end(); iter++ )
    {
        const string topic = *iter;
        topicServicesPtr_t topicServicesPtr;
        auto topicServicesIter = m_servicesByTopic.find( topic );
        if( topicServicesIter == m_servicesByTopic.end() )
        {
            topicServicesPtr = shared_ptr<TopicServices>( new TopicServices( topic ) );
            m_servicesByTopic[ topic ] = topicServicesPtr;
        }
        else
        {
            topicServicesPtr = topicServicesIter->second;
        }
        topicServicesPtr->addService( reg );
    }
    // Increment the number of local Svc register
    incLocalSvc( reg );
}

/** {@inheritDoc} */
void ServiceRegistry::updateService( serviceRegistrationPtr_t existingReg, const serviceRegistrationPtr_t reg )
{
    if( ( existingReg->getBrokerGuid() != reg->getBrokerGuid() ) &&
        ( reg->getTtlMins() < existingReg->getAdjustedTtlMins() ) )
    {
        if( SL_LOG.isDebugEnabled() )
            SL_START << "Ignoring service registration for : " << reg->getServiceGuid() << 
                " TTL: " << reg->getTtlMins() << " is less than local TTL: " <<
                existingReg->getAdjustedTtlMins() << SL_DEBUG_END;

        return;
    }

    // Check to see if the service is already registered and whether it is
    // the same. If it is, update its meta-data, TTL, and registration time.
    if( existingReg->getRequestChannels() == reg->getRequestChannels() &&
        existingReg->getBrokerGuid() == reg->getBrokerGuid() &&
        existingReg->getClientGuid() == reg->getClientGuid() &&
        existingReg->getClientInstanceGuid() == reg->getClientInstanceGuid() &&
        existingReg->getServiceType() == reg->getServiceType() &&
        existingReg->getMetaData() == reg->getMetaData() &&
        existingReg->getTargetTenantGuids() == reg->getTargetTenantGuids() &&
        existingReg->isManagedClient() == reg->isManagedClient() &&
        existingReg->getCertificates() == reg->getCertificates() )
    {
        if( SL_LOG.isDebugEnabled() )
            SL_START << "Service re-registered: " << reg->getServiceGuid() << 
                ", updating TTL." << SL_DEBUG_END;
        // Copy TTL, meta-data, and registration time
        existingReg->setTtlMins( reg->getTtlMins() );
        existingReg->setMetaData( reg->getMetaData() );
        existingReg->setRegistrationTime( reg->getRegistrationTime() );
    }
    else
    {
        // Unregister the service if it is already registered
        // Do not fire an event, as we will be sending another registration
        // event. We do not need to validate the service-related guids since
        // that validation has already occurred prior to this method being called.
        unregisterService( reg->getServiceGuid(), false );
        //Re-register the service
        addService( reg ); 
    }
}

/** {@inheritDoc} */
inline serviceRegistrationPtr_t ServiceRegistry::findService( const std::string& serviceGuid ) const
{
    auto iter = m_servicesById.find( serviceGuid );
    if( iter != m_servicesById.end() )
    {
        return iter->second;
    }
    return serviceRegistrationPtr_t();
}


/** {@inheritDoc} */
serviceRegistrationPtr_t ServiceRegistry::findService( 
    const std::string& serviceGuid, const char* targetServiceTenantGuid ) const
{
    serviceRegistrationPtr_t service = findService( serviceGuid );
    if( service.get() && service->isServiceAvailable( targetServiceTenantGuid ) )
    {
        return service;
    } 
    return serviceRegistrationPtr_t();
}

/** {@inheritDoc} */
const vector<serviceRegistrationPtr_t> ServiceRegistry::findServicesByType(
    const string& serviceType, const char* targetServiceTenantGuid ) const
{
    vector<serviceRegistrationPtr_t> services;
    auto servicesRet = m_servicesByType.equal_range( serviceType );
    for( auto iter = servicesRet.first; iter != servicesRet.second; iter++ )
    {
        if( iter->second->isServiceAvailable( targetServiceTenantGuid ) )
        {
            services.push_back( iter->second );
        }
    }
    return services;
}

/** {@inheritDoc} */
const std::vector<serviceRegistrationPtr_t> ServiceRegistry::getAllServices( const char* targetServiceTenantGuid ) const
{    
    vector<serviceRegistrationPtr_t> services;    
    for( auto iter = m_servicesById.begin(); iter != m_servicesById.end(); iter++ )
    {
        if ( iter->second->isServiceAvailable( targetServiceTenantGuid ) )
        {
            services.push_back( iter->second );
        }
    }
    return services;
}

/** {@inheritDoc} */
serviceRegistrationPtr_t ServiceRegistry::getNextService( const string& topic, const char* targetServiceTenantGuid )
{
    auto servicesByTopic = m_servicesByTopic.find( topic );
    if( servicesByTopic != m_servicesByTopic.end() )
    {
        // Next service for topic
        return servicesByTopic->second->getNextService( targetServiceTenantGuid );
    }

    // Empty pointer
    return serviceRegistrationPtr_t();
}

/** {@inheritDoc} */
void ServiceRegistry::sendServiceRegistrationEvent( serviceRegistrationPtr_t reg ) const
{
    // Send a register event to the other brokers if it is a local service
    if( reg->isLocal() )
    {
        DxlMessageService& messageService = DxlMessageService::getInstance();
        shared_ptr<DxlEvent> registerEvent = messageService.createEvent();
        registerEvent->setPayload( ServiceRegistryRegisterEventPayload( *reg ) );
        // In multi-tenant environments if the service is non-ops only send the service registration
        // event to the tenant owning the service and ops
        if( BrokerSettings::isMultiTenantModeEnabled() && !reg->isOps() )
        {
            const char* tenantGuids[] = { reg->getClientTenantGuid().c_str() };
            registerEvent->setDestinationTenantGuids( tenantGuids, 1 );
        }
        messageService.sendMessage( 
            DxlMessageConstants::CHANNEL_DXL_SVCREGISTRY_REGISTER_EVENT, *registerEvent );
    }
}

/** {@inheritDoc} */
void ServiceRegistry::sendServiceUnregistrationEvent( serviceRegistrationPtr_t reg ) const
{
    if( reg->isLocal() )
    {            
        DxlMessageService& messageService = DxlMessageService::getInstance();
        shared_ptr<DxlEvent> unregisterEvent = messageService.createEvent();
        unregisterEvent->setPayload( ServiceRegistryUnregisterEventPayload( reg->getServiceGuid() ) );
        // In multi-tenant environments if the service is non-ops only send the service unregistration
        // event to the tenant owning the service and ops
        if( BrokerSettings::isMultiTenantModeEnabled() && !reg->isOps() )
        {
            const char* tenantGuids[] = { reg->getClientTenantGuid().c_str() };
            unregisterEvent->setDestinationTenantGuids( tenantGuids, 1 );
        }
        messageService.sendMessage( 
            DxlMessageConstants::CHANNEL_DXL_SVCREGISTRY_UNREGISTER_EVENT, *unregisterEvent );
    }
}

/** {@inheritDoc} */
void ServiceRegistry::sendServiceRegistrationEvents() const
{
    for( auto iter = m_servicesById.begin(); 
            iter != m_servicesById.end(); iter++ )
    {
        if( !iter->second->isExpired() )
        {
            // Create a copy of the registration
            shared_ptr<ServiceRegistration> copy = 
                shared_ptr<ServiceRegistration>( new ServiceRegistration( (*iter->second) ) );

            // Reset the registration time and TTL (based on current time)
            copy->resetRegistrationTime();

            // Send the registration event
            sendServiceRegistrationEvent( copy );
        }
    }
}
 
/** {@inheritDoc} */
void ServiceRegistry::checkServiceTtls()
{
    if( SL_LOG.isDebugEnabled() )
        SL_START << "checkServiceTtls" << SL_DEBUG_END;

    for( auto iter = m_servicesById.begin(); 
        iter != m_servicesById.end(); )
    {
        if( iter->second->isExpired() )
        {
            string serviceGuid = iter->first;
            serviceRegistrationPtr_t reg = iter->second;
            iter++;

            if( SL_LOG.isInfoEnabled() )
                SL_START << "Service TTL expired, unregistering: " 
                    << serviceGuid << SL_INFO_END;

            unregisterService( serviceGuid );
        }
        else
        {
            iter++;
        }
    }
}

/** {@inheritDoc} */
void ServiceRegistry::onConfigurationUpdated( std::shared_ptr<const BrokerConfiguration> brokerConfig )
{    
    // Build service zone map
    serviceZonesByNodePtr_t serviceZonesByNode( new serviceZonesByNode_t() );

    ConfigNodeMap configNodes = brokerConfig->getConfigNodeMap();
    for( auto iter = configNodes.begin(); iter != configNodes.end(); iter++ )
    {
        serviceZonesByNode->insert( make_pair( iter->first, iter->second->getServiceZoneList() ) );
    }

    setServiceZonesByNode( serviceZonesByNode );

    // Lock mutex and mark that the configuration has changed
    unique_lock<mutex> lck( m_mutex );
    m_brokerConfigChanged = true;    
}

/** {@inheritDoc} */
void ServiceRegistry::clearServiceZones()
{
    if( SL_LOG.isDebugEnabled() )
        SL_START << "ServiceRegistry::clearServiceZones" << SL_DEBUG_END;

    for( auto iter = m_servicesByTopic.begin(); iter != m_servicesByTopic.end(); iter++ )
    {
        iter->second->clearServiceZones();
    }
}

/** {@inheritDoc} */
void ServiceRegistry::onCoreMaintenance( time_t time )
{
    if( ( time - m_ttlCheckTime ) >= 
        ( BrokerSettings::getTtlCheckIntervalMins() * 60 ) )
    {
        if( SL_LOG.isDebugEnabled() )
            SL_START << "Checking Service TTLs: time=" << time << SL_DEBUG_END;

        // Update last check time
        m_ttlCheckTime = time;

        // Check TTLs for services
        checkServiceTtls();
    }

    // Lock mutex and check if configuration has changed
    bool configChanged = false;
    {
        unique_lock<mutex> lck( m_mutex );
        if( m_brokerConfigChanged )
        {
            m_brokerConfigChanged = false;
            configChanged = true;
        }
    }

    if( configChanged )
    {
        if( SL_LOG.isDebugEnabled() )
            SL_START << "Broker configuration has changed, clearing service zones." 
                << SL_DEBUG_END;
        clearServiceZones();
    }
}

/** {@inheritDoc} */
std::string ServiceRegistry::getRequestPrefixForEvent( const std::string& eventTopic ) const
{
    auto find = m_eventToRequestPrefix.find( eventTopic );
    if( find != m_eventToRequestPrefix.end() )
    {
        return find->second;
    }

    return "";
}

/** {@inheritDoc} */
void ServiceRegistry::rebuildEventToRequestPrefixMap()
{
    // Clear map
    m_eventToRequestPrefix.clear();

    // Walk currently registered service and add mapping
    for( auto serviceIter = m_servicesById.begin(); 
            serviceIter != m_servicesById.end(); serviceIter++ )
    {
        if( !BrokerSettings::isMultiTenantModeEnabled() || serviceIter->second->isOps() )
        {
            addEventToRequestPrefix( serviceIter->second );
        }
    }
}

/** {@inheritDoc} */
void ServiceRegistry::addEventToRequestPrefix( const serviceRegistrationPtr_t& regPtr )
{
    const unordered_map<std::string, std::string> md = regPtr->getMetaData();

    auto eventToRequestPrefix = md.find( EVENT_TO_REQUEST_PREFIX_PROP );
    if( eventToRequestPrefix != md.end() )
    {
        size_t topicLen = strlen( EVENT_TO_REQUEST_TOPIC_PROP );
        for( auto mdItr = md.begin(); mdItr != md.end(); ++mdItr ) 
        {
            if( strncmp( mdItr->first.c_str(), EVENT_TO_REQUEST_TOPIC_PROP, topicLen ) == 0 )
            {
                // Add event to request prefix
                m_eventToRequestPrefix[ mdItr->second ] = eventToRequestPrefix->second;

                if( SL_LOG.isDebugEnabled() )
                    SL_START << "Added event to request: " <<  mdItr->second <<
                        " = " << eventToRequestPrefix->second <<
                        ", mapSize: " << m_eventToRequestPrefix.size() << SL_DEBUG_END;
            }
        }
    }
}

/** {@inheritDoc} */
ostream& operator<<( ostream &out, const ServiceRegistry &reg )
{
    out << "Service registry: " << endl;
    for( auto iter = reg.m_servicesById.begin(); 
        iter != reg.m_servicesById.end(); iter++ )
    {
        out << *(iter->second) << endl;
    }

    out << "\t" << "By service type: " << endl;
    for( auto iter = reg.m_servicesByType.begin();
        iter != reg.m_servicesByType.end(); iter = 
        reg.m_servicesByType.upper_bound( iter->first ) )
    {
        string type = iter->first;
        out << "\t\t" << type << ":" << endl;
        auto serviceRet = reg.m_servicesByType.equal_range( type );
        for( auto serviceIter = serviceRet.first; serviceIter != serviceRet.second; serviceIter++ )
        {
            out << "\t\t\t" << serviceIter->second->getServiceGuid() << endl;
        }
    }

    out << "\t" << "By channels (topics): " << endl;
    for( auto iter = reg.m_servicesByTopic.begin();
        iter != reg.m_servicesByTopic.end(); iter++ )
    {
        string topic = iter->first;
        out << "\t\t" << topic << ":" << endl;
        const set<serviceRegistrationPtr_t> services = iter->second->getServices();
        for( auto serviceIter = services.begin(); serviceIter != services.end(); serviceIter++ )
        {
            out << "\t\t\t" << (*serviceIter)->getServiceGuid() << endl;
        }
    }
                
    return out;
}

}
}
}
