/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/


#include "topicauthorization/include/topicauthorizationstate.h"
#include <memory>
#include <string>
#include "include/unordered_map.h"
#include "include/unordered_set.h"
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include "include/SimpleLog.h"
#include "core/include/CoreUtil.h"
#include "json/include/JsonReader.h"
#include "json/include/JsonService.h"
#include "util/include/FileUtil.h"

using namespace std;
using namespace Json;
using namespace dxl::broker::core;
using namespace dxl::broker::json;
using namespace dxl::broker::util;

namespace dxl {
namespace broker {

/**
 * A set of clients that are associated with the topic.
 * Used for parsing the policy in JSON format.
 */
class TopicClientsJson : public dxl::broker::json::JsonReader
{
public:
    static const char* TOPIC_PROP;
    static const char* CLIENTS_PROP;

    explicit TopicClientsJson() {}
    virtual ~TopicClientsJson() {}

    string getTopic() const
    {
        return m_topic;
    }

    unordered_set<string> getClients() const
    {
        return m_clients;
    }

    virtual void read( const Value& in )
    {
        m_topic = JsonService::parseString( in, TOPIC_PROP, true );
        Value clients = in[ CLIENTS_PROP ];
        for( Value::iterator itr = clients.begin(); itr != clients.end(); itr++ )
        {
            m_clients.insert( (*itr).asString() );
        }
    }
protected:
    string m_topic;
    unordered_set<string> m_clients;
};

const char* TopicClientsJson::TOPIC_PROP = "topic";
const char* TopicClientsJson::CLIENTS_PROP = "clients";

/**
 * The authorization policy.
 * Used for parsing the policy in JSON format.
 */
class AuthConfigJson : public dxl::broker::json::JsonReader
{
public:
    static const char* PUBLISHERS_PROP;
    static const char* SUBSCRIBERS_PROP;

    explicit AuthConfigJson() {}
    virtual ~AuthConfigJson() {}
    void read( const Value& in )
    {
        Value publishers = in[ PUBLISHERS_PROP ];
        for( Value::iterator itr = publishers.begin(); itr != publishers.end(); itr++ )
        {
            TopicClientsJson t;
            t.read( *itr );
            m_publishers[t.getTopic()] = t.getClients();
        }

        Value subscribers = in[ SUBSCRIBERS_PROP ];
        for( Value::iterator itr = subscribers.begin(); itr != subscribers.end(); itr++ )
        {
            TopicClientsJson t;
            t.read( *itr );
            m_subscribers[t.getTopic()] = t.getClients();
        }
    }
    shared_ptr<TopicAuthorizationState> createConfig() const
    {
        return shared_ptr<TopicAuthorizationState>(
            new TopicAuthorizationState( m_publishers, m_subscribers ) );
    }
private:
    TopicAuthorizationState::TopicAuthorizationStateData m_publishers;
    TopicAuthorizationState::TopicAuthorizationStateData m_subscribers;
};

const char* AuthConfigJson::PUBLISHERS_PROP = "send";
const char* AuthConfigJson::SUBSCRIBERS_PROP = "receive";

/**
 * Returns whether wildcard topics are being used in the state
 *
 * @param   stateData The state data
 * @return  Whether wildcard topics are being used in the state
 */
static bool hasWildcards( const TopicAuthorizationState::TopicAuthorizationStateData& stateData )
{
    for( auto it = stateData.begin(); it != stateData.end(); ++it )
    {
        string name = it->first;
        if( CoreUtil::isWildcard( name.c_str() ) )
        {
            return true;
        }
    }

    return false;
}

/** {@inheritDoc} */
shared_ptr<TopicAuthorizationState> TopicAuthorizationState::CreateFromJsonFile(
    const string& configFile )
{
    if( !FileUtil::fileExists( configFile ) )
    {
        // Return empty configuration if file does not exist
        TopicAuthorizationState::TopicAuthorizationStateData publishers;
        TopicAuthorizationState::TopicAuthorizationStateData subscribers;
        return shared_ptr<TopicAuthorizationState>(
            new TopicAuthorizationState( publishers, subscribers ) );
    }

    AuthConfigJson config;
    try
    {
        JsonService::getInstance().fromJson(
            FileUtil::getFileAsString( configFile ), config );
    }
    catch( runtime_error& e )
    {
        throw runtime_error(
            ( boost::format(
                "Error occurred parsing authorization policy file: %1%, %2%" )
                % configFile % e.what() ).str() );
    }

    return config.createConfig();
}

/** {@inheritDoc} */
TopicAuthorizationState::TopicAuthorizationState(
    const TopicAuthorizationState::TopicAuthorizationStateData& publishers,
    const TopicAuthorizationState::TopicAuthorizationStateData& subscribers ) :
    m_publishers( publishers ), m_subscribers( subscribers ), m_isWildcardingEnabled( false )
{
    // Look for wildcards
    m_isWildcardingEnabled = hasWildcards( publishers ) || hasWildcards( subscribers );

    if( SL_LOG.isDebugEnabled() )
        SL_START<< "Authorization wildcarding enabled: " << m_isWildcardingEnabled << SL_DEBUG_END;
}

/** {@inheritDoc} */
bool TopicAuthorizationState::isAuthorized(
    const TopicAuthorizationStateData& state, const string& key, const string& topic, bool* hit ) const
{
    if( hit )
    {
        *hit = false;
    }

    auto topicitr = state.find( topic );
    if( topicitr == state.end() )
    {
        return true;
    }

    if( hit )
    {
        *hit = true;
    }

    auto agentitr = topicitr->second.find( key );
    return agentitr != topicitr->second.end();
}

/** {@inheritDoc} */
bool TopicAuthorizationState::isAuthorizedToPublish(
    const string& key, const string& topic, bool* hit ) const
{
    return isAuthorized( m_publishers, key, topic, hit );
}

/** {@inheritDoc} */
bool TopicAuthorizationState::isAuthorizedToSubscribe(
    const string& key, const string& topic, bool* hit ) const
{
    return isAuthorized( m_subscribers, key, topic, hit );
}

/** {@inheritDoc} */
bool TopicAuthorizationState::operator!=( const TopicAuthorizationState& rhs ) const
{
    return !( *this == rhs );
}

/** {@inheritDoc} */
bool TopicAuthorizationState::operator==( const TopicAuthorizationState& rhs ) const
{
    return m_publishers.size() == rhs.m_publishers.size() &&
           m_subscribers.size() == rhs.m_subscribers.size() &&
           m_publishers == rhs.m_publishers &&
           m_subscribers == rhs.m_subscribers;
}

}
}
