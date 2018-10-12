/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/brokerlib.h"
#include "include/BrokerSettings.h"
#include "brokerregistry/include/brokerregistry.h"
#include "include/GeneralPolicySettings.h"
#include "include/SimpleLog.h"
#include <boost/lexical_cast.hpp>
#include "brokerregistry/include/brokerregistry.h"

using namespace std;
using namespace dxl::broker;
using namespace dxl::broker::core;

/** Constants */
static string KEEP_ALIVE_PROP( "keepAlive" );
static uint32_t KEEP_ALIVE_DEFAULT = 1;
static string CONNECTION_LIMIT_PROP( "connectionLimit" );
static uint32_t CONNECTION_LIMIT_DEFAULT = 0;

/** The configuration settings */
GeneralPolicySettings::ConfigSettings GeneralPolicySettings::sm_settings;

/** Whether the settings are dirty */
unordered_set<string> GeneralPolicySettings::sm_changedProperties;

/** The mutex for the policy settings */
std::recursive_mutex GeneralPolicySettings::m_mutex;

/** {@inheritDoc} */
void GeneralPolicySettings::setProperty( const string name, const string value )
{
    // Lock mutex
    lock_guard<recursive_mutex> lock( m_mutex );

    sm_settings.setProperty( name, value );
}

/** {@inheritDoc} */
uint32_t GeneralPolicySettings::getIntegerProperty( const std::string name, uint32_t defaultValue )
{
    string value = getStringProperty( name, "" );
    return value.empty() ? defaultValue : atoi( value.c_str() );
}

/** {@inheritDoc} */
std::string GeneralPolicySettings::getStringProperty( const std::string name, const std::string defaultValue )
{
    // Lock mutex
    lock_guard<recursive_mutex> lock( m_mutex );

    string value;
    bool found = sm_settings.getProperty( name, value );
    return found ? value.c_str() : defaultValue;
}

/** {@inheritDoc} */
void GeneralPolicySettings::loadSettings()
{    
    // Lock mutex
    lock_guard<recursive_mutex> lock( m_mutex );

    sm_settings.readConfiguration( BrokerSettings::getPolicyGeneralStateFile() );
    editingCompleted( false );
}

/** {@inheritDoc} */
void GeneralPolicySettings::ConfigSettings::setProperty(
    const std::string& propertyName, const std::string& value )
{
    string oldValue;
    bool found = getProperty( propertyName, oldValue );
    
    if( !found || ( oldValue != value ) )
    {
        if( SL_LOG.isDebugEnabled() )
        {
            SL_START << "GeneralPolicySettings::setProperty: " 
                << propertyName << "=" << value << SL_DEBUG_END;
        }
        Configuration::setProperty( propertyName, value );
        sm_changedProperties.insert( propertyName );
    }
}

/** {@inheritDoc} */
void GeneralPolicySettings::setKeepAliveInterval( const string intervalStr )
{
    setProperty( KEEP_ALIVE_PROP, intervalStr  );
}

/** {@inheritDoc} */
uint32_t GeneralPolicySettings::getKeepAliveInterval()
{
    return getIntegerProperty( KEEP_ALIVE_PROP, KEEP_ALIVE_DEFAULT );
}

/** {@inheritDoc} */
void GeneralPolicySettings::setConnectionLimit( const string limit )
{
    if( !BrokerSettings::isConnectionLimitIgnored() )
    {
        setProperty( CONNECTION_LIMIT_PROP, limit  );
    }
}

/** {@inheritDoc} */
uint32_t GeneralPolicySettings::getConnectionLimit()
{
    return !BrokerSettings::isConnectionLimitIgnored() ?
        getIntegerProperty( CONNECTION_LIMIT_PROP, CONNECTION_LIMIT_DEFAULT ) :
        CONNECTION_LIMIT_DEFAULT /* unlimited */;
}

/** {@inheritDoc} */
void GeneralPolicySettings::editingCompleted( bool writeSettings )
{
    // Lock mutex
    lock_guard<recursive_mutex> lock( m_mutex );

    if( sm_changedProperties.empty() )
    {
        // Nothing changed, exit.
        return;
    }

    // Handle the property changes
    handlePropertyChanges( sm_changedProperties );

    if( writeSettings )
    {
        if( SL_LOG.isDebugEnabled() )
        {
            SL_START << "GeneralPolicySettings, dirty, writing general settings."
                << SL_DEBUG_END;
        }

        // Write the settings
        sm_settings.writeConfiguration( BrokerSettings::getPolicyGeneralStateFile() );
    }

    // Reset changed properties
    sm_changedProperties.clear();
}

/** {@inheritDoc} */
void GeneralPolicySettings::handlePropertyChanges( const unordered_set<std::string>& propNames )
{
    // Apply keep alive change
    if( propNames.find( KEEP_ALIVE_PROP ) != propNames.end() )
    {
        CoreInterface *mi = getCoreInterface();
        if( mi )
        {
            mi->setBridgeKeepalive( getKeepAliveInterval() );
        }
    }

    // Apply connection limit change
    if( propNames.find( CONNECTION_LIMIT_PROP ) != propNames.end() )
    {
        const uint32_t limit = getConnectionLimit();
        BrokerRegistry::getInstance().setLocalBrokerConnectionLimit( limit );

        CoreInterface *mi = getCoreInterface();
        if( mi )
        {
            mi->setConnectionLimit( limit );
        }
    }
}
