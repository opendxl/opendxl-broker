/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include <algorithm>
#include "brokerregistry/include/brokerstate.h"
#include "include/BrokerSettings.h"
#include "core/include/CoreUtil.h"

using dxl::broker::registry::connection_t;
using namespace dxl::broker::core;

namespace dxl {
namespace broker {

/** {@inheritDoc} */
BrokerState::BrokerState(
    const Broker &broker, 
    const registry::connection_t &connections,
    const registry::childConnections_t &childConnections ) : 
    m_broker( broker ), 
    m_subscriptionsWildcardCount( 0 ), 
    m_pendingSubscriptionsWildcardCount( 0 ),
    m_subscriptionsChangeCount( 0 )
{
    // Set connections
    setConnections( connections, childConnections );    
    
    // Set the registration time
    updateRegistrationTime();
}

/** {@inheritDoc} */
void BrokerState::addConnection( const std::string &connectionId, bool isChild )
{
    auto connection = m_countedConnections.find( connectionId );
    if( connection != m_countedConnections.end() )
    {
        connection->second++;
    }
    else
    {
        m_countedConnections.insert( 
            std::pair<std::string, uint32_t>( connectionId, 1 ) );
    }

    if( isChild )
    {
        m_childConnections.insert( connectionId );
    }
}

/** {@inheritDoc} */
void BrokerState::removeConnection(const std::string &connectionId)
{
    auto connection = m_countedConnections.find( connectionId );
    if( connection != m_countedConnections.end() )
    {
        connection->second--;
        if( connection->second == 0 )
        {
            m_countedConnections.erase( connectionId );
            m_childConnections.erase( connectionId );
        }
    }
}

/** {@inheritDoc} */
bool BrokerState::hasConnection( const std::string &connectionId ) const
{
    return m_countedConnections.find( connectionId ) != m_countedConnections.end();
}

/** {@inheritDoc} */
bool BrokerState::setConnections(
    const registry::connection_t &connectionIds,
    const registry::childConnections_t &childConnectionIds )
{
    if( ( connectionIds != getConnections() ) ||
        ( childConnectionIds != m_childConnections ) )
    {
        m_countedConnections.clear();
        m_childConnections.clear();

        for( auto iter = connectionIds.begin(); iter != connectionIds.end(); iter++ )
        {
            addConnection( *iter, ( childConnectionIds.find( *iter ) != childConnectionIds.end() ) );
        }

        return true;
    }

    return false;
}

/** {@inheritDoc} */
registry::connection_t BrokerState::getConnections() const
{
    registry::connection_t connectionIds;
    for( auto iter = m_countedConnections.begin(); iter != m_countedConnections.end(); iter++ )
    {
        connectionIds.insert( iter->first );
    }
    return connectionIds;
}

/** {@inheritDoc} */
bool BrokerState::forEachConnection( std::function<bool( const std::string& )> fn ) const
{
    for( auto iter = m_countedConnections.begin(); iter != m_countedConnections.end(); iter++ )
    {
        if( !fn( iter->first ) )
        {
            return false;
        }
    }
    return true;
}

/** {@inheritDoc} */
registry::childConnections_t BrokerState::getChildConnections() const
{
    return m_childConnections;
}

/** {@inheritDoc} */
bool BrokerState::hasTopics( const registry::subscriptions_t& topics ) const
{
    for( auto itr = topics.begin(); itr != topics.end(); ++itr ) 
    {
        if( m_subscriptions.find( *itr ) == m_subscriptions.end() )
        {
            return false;
        }
    }

    return true;
}

/** {@inheritDoc} */
bool BrokerState::addTopic( const std::string &topic )
{
    auto res = m_subscriptions.insert( topic );
    if( res.second )
    {
        if( getBrokerInternal().isLocalBroker() )
        {
            m_subscriptionsChangeCount++;
        }

        if( CoreUtil::isWildcard( topic.c_str() ) )
        {
            m_subscriptionsWildcardCount++;
        }
        return true;
    }
    return false;
}

/** {@inheritDoc} */
bool BrokerState::removeTopic( const std::string &topic )
{
    if( m_subscriptions.erase( topic ) )
    {
        if( getBrokerInternal().isLocalBroker() )
        {
            m_subscriptionsChangeCount++;
        }

        if( CoreUtil::isWildcard( topic.c_str() ) )
        {
            m_subscriptionsWildcardCount--;
        }
        return true;
    }
    return false;
}

/** {@inheritDoc} */
bool BrokerState::isExpired() const
{
    if( m_broker.getId() == BrokerSettings::getGuid() )
    {
        return false;
    }

    time_t current;
    time( &current );
    return difftime( current, m_regTime ) >
        ( ( m_broker.getTtl() + BrokerSettings::getTtlGracePeriodMins() ) * 60 );
}

/** {@inheritDoc} */
void BrokerState::batchTopics( int charCount, const TopicsCallback& callback ) const
{
    registry::subscriptions_t batch;

    int curCharCount = 0;                                
    int count = 0;
    int size = (int)m_subscriptions.size();
    int32_t index = 0;

    if( size == 0 )
    {
        callback.handleBatch( this, batch, index++, true, true );
    }
    else
    {
        bool isFirst = true;
        for( auto itr = m_subscriptions.begin(); itr != m_subscriptions.end(); ++itr )
        {
            batch.insert( (*itr) );
            curCharCount += (int)((*itr).length());

            bool isLast = ( count == ( size - 1 ) );
            if( ( curCharCount >= charCount ) || isLast )
            {
                callback.handleBatch( this, batch, index++, isFirst, isLast );

                batch.clear();
                curCharCount = 0;
                isFirst = false;
            }

            count++;
        }
    }
}

/** {@inheritDoc} */
bool BrokerState::forEachTopic( std::function<bool( const std::string& )> fn ) const
{
    for( auto iter = m_subscriptions.begin(); iter != m_subscriptions.end(); iter++ )
    {
        if( !fn( *iter ) )
        {
            return false;
        }
    }
    return true;
}

/** {@inheritDoc} */
void BrokerState::addPendingTopics( const registry::subscriptions_t& subs, uint32_t wildcardCount )
{
    m_pendingSubscriptions.insert( subs.begin(), subs.end() );
    m_pendingSubscriptionsWildcardCount += wildcardCount;
}

/** {@inheritDoc} */
void BrokerState::swapPendingTopics()
{
    m_subscriptions = m_pendingSubscriptions;
    m_subscriptionsWildcardCount = m_pendingSubscriptionsWildcardCount;
    clearPendingTopics();
}

/** {@inheritDoc} */
void BrokerState::clearPendingTopics()
{
    m_pendingSubscriptions.clear();
    m_pendingSubscriptionsWildcardCount = 0;
}

/** {@inheritDoc} */
std::ostream & operator <<( std::ostream &out, const BrokerState &brokerState )
{
    out << "\t" << "Broker:  " << brokerState.m_broker << std::endl;
    out << "\t" << "Connections:  ";
    for(auto i = brokerState.m_countedConnections.begin(); i != brokerState.m_countedConnections.end(); i++)
    {
        if(i != brokerState.m_countedConnections.begin())
        {
            out << ", ";
        }

        out << i->first << " (" << i->second << ")";
    }
    out << std::endl;
    out << "\t" << "Child connections:  ";
    for(auto i = brokerState.m_childConnections.begin(); i != brokerState.m_childConnections.end(); i++)
    {
        if(i != brokerState.m_childConnections.begin())
        {
            out << ", ";
        }

        out << (*i);
    }
    out << std::endl;
    out << "\t" << "Expired: " << brokerState.isExpired() << std::endl;                

    return out;
}

}
}
