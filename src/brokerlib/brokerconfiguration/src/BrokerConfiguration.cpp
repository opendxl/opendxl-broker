/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include "brokerconfiguration/include/BrokerConfiguration.h"
#include "brokerconfiguration/include/ConfigBroker.h"
#include "brokerconfiguration/include/Hub.h"
#include "include/SimpleLog.h"
#include "json/include/JsonReader.h"
#include "json/include/JsonService.h"
#include "util/include/FileUtil.h"

using namespace dxl::broker::json;
using namespace dxl::broker::util;
using namespace std;
using namespace Json;

namespace dxl {
namespace broker {

/**
 * Base class for broker and hub types.
 * Used for parsing config file in JSON format.
 */
class AbstractTypeJson : public dxl::broker::json::JsonReader
{
public:
    static const char* ID_PROP;
    static const char* PARENT_ID_PROP;
    static const char* SERVICE_ZONE_PROP;

    explicit AbstractTypeJson() {}
    virtual ~AbstractTypeJson() {}
    virtual void read( const Value& in )
    {
        m_id = JsonService::parseString( in, ID_PROP, true );
        m_serviceZone = JsonService::parseString( in, SERVICE_ZONE_PROP );
        m_parentId = JsonService::parseString( in, PARENT_ID_PROP );
    }
protected:
    string m_id;
    string m_serviceZone;
    string m_parentId;
};

const char* AbstractTypeJson::ID_PROP = "id";
const char* AbstractTypeJson::SERVICE_ZONE_PROP = "serviceZone";
const char* AbstractTypeJson::PARENT_ID_PROP = "parentId";

/**
 * Broker type.
 * Used for parsing config file in JSON format.
 */
class BrokerTypeJson : public AbstractTypeJson
{
public:
    static const char* HOSTNAME_PROP;
    static const char* ALT_HOSTNAME_PROP;
    static const char* PORT_PROP;

    explicit BrokerTypeJson() {}
    virtual ~BrokerTypeJson() {}
    void read( const Value& in )
    {
        AbstractTypeJson::read( in );
        m_hostname = JsonService::parseString( in, HOSTNAME_PROP, true );
        m_port = JsonService::parseString( in, PORT_PROP, true );
        m_altHostname = JsonService::parseString( in, ALT_HOSTNAME_PROP, false );
        if( m_altHostname.empty() )
        {
            m_altHostname = m_hostname;
        }
    }
    shared_ptr<ConfigBroker> createBroker() const
    {
        unsigned int port;
        try
        {
            port = boost::lexical_cast<unsigned int>( m_port );
        }
        catch( boost::bad_lexical_cast& )
        {
            SL_START << "Unable to parse port: " << m_port << SL_ERROR_END;
            throw invalid_argument( "the broker port is invalid." );
        }
        
        return shared_ptr<ConfigBroker>(
            new ConfigBroker( m_id, m_hostname, port, m_serviceZone, m_parentId, m_altHostname ) );
    }
private:
    string m_hostname;
    string m_altHostname;
    string m_port;
};

const char* BrokerTypeJson::HOSTNAME_PROP = "hostname";
const char* BrokerTypeJson::ALT_HOSTNAME_PROP = "altHostname";
const char* BrokerTypeJson::PORT_PROP = "port";

/**
 * Hub type.
 * Used for parsing config file in JSON format.
 */
class HubTypeJson : public AbstractTypeJson
{
public:
    static const char* PRIMARY_PROP;
    static const char* SECONDARY_PROP;
    static const char* NAME_PROP;

    explicit HubTypeJson() {}
    virtual ~HubTypeJson() {}
    void read( const Value& in )
    {
        AbstractTypeJson::read( in );
        m_primaryBrokerId = JsonService::parseString( in, PRIMARY_PROP );
        m_secondaryBrokerId = JsonService::parseString( in, SECONDARY_PROP );
        m_name = JsonService::parseString( in, NAME_PROP, true );
    }
    shared_ptr<Hub> createHub() const
    {
        return shared_ptr<Hub>(
            new Hub( m_id, m_primaryBrokerId, m_secondaryBrokerId, m_parentId, m_serviceZone, m_name ) );
    }
private:
    string m_name;
    string m_primaryBrokerId;
    string m_secondaryBrokerId;
};

const char* HubTypeJson::PRIMARY_PROP = "primaryBroker";
const char* HubTypeJson::SECONDARY_PROP = "secondaryBroker";
const char* HubTypeJson::NAME_PROP = "name";

/**
 * Used for parsing config file in JSON format.
 */
class BrokerConfigJson : public dxl::broker::json::JsonReader
{
public:
    static const char* BROKERS_PROP;
    static const char* HUBS_PROP;

    explicit BrokerConfigJson() {}
    virtual ~BrokerConfigJson() {}
    void read( const Value& in )
    {
        Value brokers = in[ BROKERS_PROP ];
        for( Value::iterator itr = brokers.begin(); itr != brokers.end(); itr++ )
        {
            BrokerTypeJson b;
            b.read( *itr );
            m_brokers.push_back( b.createBroker() );
        }

        Value hubs = in[ HUBS_PROP ];
        for( Value::iterator itr = hubs.begin(); itr != hubs.end(); itr++ )
        {        
            HubTypeJson h;
            h.read( *itr );
            m_hubs.push_back( h.createHub() );
        }
    }
    shared_ptr<BrokerConfiguration> createConfig() const
    {
        return shared_ptr<BrokerConfiguration>( new BrokerConfiguration( m_hubs, m_brokers ) );
    }
private:
    vector<shared_ptr<Hub>> m_hubs;
    vector<shared_ptr<ConfigBroker>> m_brokers;
};

const char* BrokerConfigJson::BROKERS_PROP = "brokers";
const char* BrokerConfigJson::HUBS_PROP = "hubs";

/** {@inheritDoc} */
shared_ptr<BrokerConfiguration> BrokerConfiguration::CreateFromJsonConfigFile(
    const string& configFile )
{
    if( !FileUtil::fileExists( configFile ) )
    {
        // Return empty configuration if file does not exist
        vector<shared_ptr<Hub>> hubs;
        vector<shared_ptr<ConfigBroker>> brokers;
        return shared_ptr<BrokerConfiguration>( new BrokerConfiguration( hubs, brokers ) );
    }

    BrokerConfigJson config;
    try
    {
        JsonService::getInstance().fromJson(
            FileUtil::getFileAsString( configFile ), config );
    }
    catch( runtime_error& e )
    {
        throw runtime_error(
            ( boost::format(
                "Error occurred parsing broker state policy file: %1%, %2%" )
                % configFile % e.what() ).str() );
    }

    return config.createConfig();
}

/** {@inheritDoc} */
BrokerConfiguration::BrokerConfiguration(
    const vector<shared_ptr<Hub>>& hubs,
    const vector<std::shared_ptr<ConfigBroker>>& brokers ) 
{
    m_hubs = hubs;
    m_brokers = brokers;

    for( auto hubItr = m_hubs.begin(); hubItr != m_hubs.end(); ++hubItr ) 
    {
        m_nodeMap.insert( make_pair( (*hubItr)->getId(), *(hubItr) ) );
    }

    for( auto brokerItr = m_brokers.begin(); brokerItr != m_brokers.end(); ++brokerItr) 
    {
        m_nodeMap.insert( make_pair( (*brokerItr)->getId(), *(brokerItr) ) );
    }

    for( auto nodeItr = m_nodeMap.begin(); nodeItr != m_nodeMap.end(); ++nodeItr) 
    {
        nodeItr->second->generateServiceZoneList( *this );
    }
}

/** {@inheritDoc} */
BrokerConfiguration::~BrokerConfiguration() 
{
}

/** {@inheritDoc} */
shared_ptr<Hub> BrokerConfiguration::getContainingHub( const string& brokerId ) const 
{
    auto it = find_if( m_hubs.begin(), m_hubs.end(), bind2nd( Hub::PtrHubContainsId(), brokerId ) );

    if( it == m_hubs.end() ) 
    {
        shared_ptr<Hub> empty;
        return empty;
    }

    return *it;
}

/** {@inheritDoc} */
shared_ptr<ConfigNode> BrokerConfiguration::getConfigNode( const string& id ) const
{
    auto it = m_nodeMap.find( id );

    if( it == m_nodeMap.end() ) 
    {
        shared_ptr<ConfigNode> empty;
        return empty;
    }

    return it->second;
}

/** {@inheritDoc} */
bool BrokerConfiguration::operator==( const BrokerConfiguration& rhs ) const 
{
    if (m_hubs.size() != rhs.m_hubs.size() || m_brokers.size() != rhs.m_brokers.size()) 
    {
        return false;
    }

    for( auto it = m_hubs.begin(); it != m_hubs.end(); ++it ) 
    {
        auto hub = std::dynamic_pointer_cast<Hub>( rhs.getConfigNode((*it)->getId() ) );
        
        if( !hub ) 
        {
            return false;
        }
        if( (*(*it)) != (*hub) ) 
        {
            return false;
        }
    }

    for( auto it = m_brokers.begin(); it != m_brokers.end(); ++it ) 
    {
        auto broker = 
            std::dynamic_pointer_cast<ConfigBroker>( rhs.getConfigNode( (*it)->getId() ) );

        if( !broker ) 
        {
            return false;
        }
        if( (*(*it)) != ( *broker ) ) 
        {
            return false;
        }
    }

    return true;
}

/** {@inheritDoc} */
bool BrokerConfiguration::operator!=( const BrokerConfiguration& rhs ) const 
{
    return !( *this == rhs );
}

}
}
