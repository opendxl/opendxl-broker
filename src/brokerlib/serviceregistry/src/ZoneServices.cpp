/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/BrokerSettings.h"
#include "include/SimpleLog.h"
#include "serviceregistry/include/ServiceRegistry.h"
#include "serviceregistry/include/ZoneServices.h"

using namespace SimpleLogger;
using namespace std;
using namespace dxl::broker;
using namespace dxl::broker::service;

/** {@inheritDoc} */
ZoneServices::ZoneServices( const string& zone, const string& topic ) :
    m_zone( zone ),
    m_topic( topic ),
    m_localBrokerGuid( BrokerSettings::getGuid() ),
    m_brokerRegistry( BrokerRegistry::getInstance() ),
    m_authService( TopicAuthorizationService::Instance() )
{
    // Initialize iterator
    m_nextServiceIter = m_services.end();
}

/** {@inheritDoc} */
void ZoneServices::addService( serviceRegistrationPtr_t reg ) 
{     
    m_services.insert( reg );
    // Reset iterator
    m_nextServiceIter = m_services.end();    
}

/** {@inheritDoc} */
void ZoneServices::removeService( serviceRegistrationPtr_t reg ) 
{
    m_services.erase( reg );
    // Reset iterator
    m_nextServiceIter = m_services.end();    
}

/** {@inheritDoc} */
serviceRegistrationPtr_t ZoneServices::getNextService( const char* targetServiceTenantGuid )
{
    const int size = (int)m_services.size();
    for( int i = 0; i < size; i++ )
    {
        if( m_nextServiceIter == m_services.end() )
        {
            m_nextServiceIter = m_services.begin();
        }
                
        const serviceRegistrationPtr_t& ptr = *m_nextServiceIter;
        m_nextServiceIter++;

        // 1.) If it is a service local to this broker, or we can reach it via
        // the current bridge connections
        // 2.) The service has not expired.
        // 3.) The client (hosting the service) is authorized to receive the request
        // 4.) The service is available for the requesting tenant
        if( ( ptr->isLocal() || 
                m_brokerRegistry.getNextBroker( 
                    m_localBrokerGuid, ptr->getBrokerGuid() ).length() > 0 ) &&
            ( !ptr->isExpired() ) &&
            ( ptr->isManagedClient() ?
                m_authService->isAuthorizedToSubscribe( 
                    ptr->getClientGuid(), getTopic() ) :
                m_authService->isAuthorizedToSubscribe( 
                    ptr->getCertificatesUtHash(), getTopic() ) ) &&
            ( ptr->isServiceAvailable( targetServiceTenantGuid ) ) )
        {
            return ptr;
        }
    }

    return serviceRegistrationPtr_t();
}
