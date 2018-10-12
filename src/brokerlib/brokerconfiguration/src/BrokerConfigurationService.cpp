/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "include/Configuration.h"
#include "include/SimpleLog.h"
#include "brokerconfiguration/include/BrokerConfigurationService.h"
#include "MutexLock.h"

using std::shared_ptr;
using std::string;
using std::vector;
using dxl::broker::common::MutexLock;

namespace dxl {
namespace broker {

/** {@inheritDoc} */
const shared_ptr<BrokerConfigurationService> BrokerConfigurationService::Instance() 
{
    static const shared_ptr<BrokerConfigurationService> singleton( new BrokerConfigurationService );
    return singleton;
}

/** {@inheritDoc} */
BrokerConfigurationService::BrokerConfigurationService() 
{
}

/** {@inheritDoc} */
BrokerConfigurationService::~BrokerConfigurationService() 
{
}

/** {@inheritDoc} */
bool BrokerConfigurationService::setBrokerConfiguration( shared_ptr<BrokerConfiguration> config )
{
    {
        MutexLock lock( &m_writeMutex );
        if( m_config.get() && ( *m_config == *config ) ) 
        {
            // if the configurations match, just return
            if ( SL_LOG.isDebugEnabled() )
            {
                SL_START << "Broker configuration is the same, ignoring." << SL_DEBUG_END;
            }
            return false;
        }
        m_config = config;
    }

    fireConfigurationChanged( config );
    return true;
}

/** {@inheritDoc} */
shared_ptr<const BrokerConfiguration> BrokerConfigurationService::getBrokerConfiguration()
{
    MutexLock lock( &m_writeMutex );
    return m_config;
}

/** {@inheritDoc} */
void BrokerConfigurationService::addListener( BrokerConfigurationServiceListener* listener ) 
{
    if( std::find( m_listeners.begin(), m_listeners.end(), listener ) == m_listeners.end() ) 
    {
        m_listeners.push_back(listener);
    }
}

/** {@inheritDoc} */
void BrokerConfigurationService::removeListener( BrokerConfigurationServiceListener* listener ) 
{
    std::vector<BrokerConfigurationServiceListener*>::iterator itr = std::find(
        m_listeners.begin(), m_listeners.end(), listener );

    if( itr != m_listeners.end() ) 
    {
        m_listeners.erase( itr );
    }
}

/** {@inheritDoc} */
void BrokerConfigurationService::fireConfigurationChanged( shared_ptr<BrokerConfiguration> config )
{
    if ( SL_LOG.isDebugEnabled() )
    {
        SL_START << "Firing configuration changed event..." << SL_DEBUG_END;
    }

    for( size_t i = 0; i < m_listeners.size(); i++ ) 
    {
        m_listeners[i]->onConfigurationUpdated( config );
    }
}

}  // namespace broker
}  // namespace dxl
