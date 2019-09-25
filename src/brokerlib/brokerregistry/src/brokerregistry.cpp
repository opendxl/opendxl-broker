/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/BrokerHelpers.h"
#include "include/BrokerSettings.h"
#include "include/SimpleLog.h"
#include "brokerregistry/include/brokerregistry.h"
#include "core/include/CoreUtil.h"

using dxl::broker::BrokerSettings;
using dxl::broker::registry::registry_t;
using dxl::broker::registry::connection_t;
using dxl::broker::registry::traversal_t;
using dxl::broker::registry::visit_t;

using namespace dxl::broker::core;
using namespace dxl::broker::topiccache;
using namespace std;

namespace dxl {
namespace broker {

/**
 * Helper class when attempting to find the path to a specified broker. This class is essentially
 * a callback that is invoked as the "connections" for a given broker are iterated. The alternative
 * would be to return a copy of the connections from the broker (broker state). While more complex,
 * this is more efficient.
 */
class FindPathToBrokerHelper
{
public:
    /**
     * Constructs the helper
     *
     * @param   registry The broker registry
     * @param   start The start broker identifier
     * @param   finish The finish (end) broker identifier
     * @param   visited State containing brokers that have been visited
     * @param   found Information about the current path (from start to finish)
     */
    FindPathToBrokerHelper( 
        const BrokerRegistry* registry, const std::string &start, const std::string &finish,
        registry::visit_t &visited, traversal_t& found ) : 
        m_registry( registry ), m_start( start ), m_finish( finish ),
        m_visited( visited ), m_found( found ) {}

    /**
     * Visits the specified connection
     *
     * @param   connectionId The connection being visited
     * @return  True if iteration should continue
     */
    bool visitConnection( const std::string& connectionId )
    {
        if( !m_found.first && !m_visited[connectionId] )
        {
            m_found = m_registry->findPathToBroker( connectionId, m_finish, m_visited );
            if( m_found.first )
            {
                m_found.second.push( m_start );
            }
        }                    

        return true;
    }

private:
    /** The broker registry */
    const BrokerRegistry* m_registry;
    /* The start broker identifier */
    const std::string& m_start;
    /* The finish (end) broker identifier */
    const std::string& m_finish;
    /* State containing brokers that have been visited */
    registry::visit_t& m_visited;
    /* Information about the current path (from start to finish) */
    traversal_t& m_found;
};

/**
 * Finder used to find a subscriber for a topic
 */
class FindSubscriberVisitor : public BrokerRegistry::FabricVisitor
{
public:
    /**
     * Constructs the finder
     * 
     * @param   broker The broker we are performing the find on behalf of
     * @param   topic The topic to find a subscriber for
     */
    FindSubscriberVisitor( const std::string& broker, const std::string& topic ) : 
        m_broker( broker ), m_topic( topic ), m_found( false ) 
    {
        if( SL_LOG.isDebugEnabled() )
        {
            SL_START << "FindSubscriber: " << broker << ", " << topic << SL_DEBUG_END;
        }
    }

    /** {@inheritDoc} */
    bool allowVisit( const BrokerRegistry& /*registry*/, const std::string& to ) const
    {
        return to != m_broker;
    }

    /** {@inheritDoc} */
    bool visit( const BrokerRegistry& registry, const std::string& to )
    {
        if( SL_LOG.isDebugEnabled() )
        {
            SL_START << "  visit: " << to << SL_DEBUG_END;
        }

        if( registry.isSubscriberInBroker( to, m_topic ) )
        {
            m_found = true;
            return false;
        }

        return true;
    }

    /**
     * Returns whether we found a subscriber
     *
     * @return  Whether we found a subscriber
     */
    bool isFound() { return m_found; }

private:
    /** The broker (reference due to the fact that this is performed on stack) */
    const std::string& m_broker;

    /** The topic (reference due to the fact that this is performed on stack) */
    const std::string& m_topic;

    /** Whether we found a subscriber */
    bool m_found;
};

/**
 * Helper class when descending the broker hierarchy. This class is essentially
 * a callback that is invoked as the "connections" for a given broker are iterated. The alternative
 * would be to return a copy of the connections from the broker (broker state). While more complex,
 * this is more efficient.
 */
class DepthFirstTraversalHelper
{
public:
    /**
     * Constructs the helper
     *
     * @param   registry The broker registry
     * @param   visitor The hierarchy visitor callback
     * @param   visited State containing brokers that have been visited
     */
    DepthFirstTraversalHelper( 
        const BrokerRegistry* registry, BrokerRegistry::FabricVisitor& visitor, 
        registry::visit_t &visited ) : 
        m_registry( registry ), m_visitor( visitor ), m_visited( visited ) {}

    /**
     * Visits the specified connection
     *
     * @param   connectionId The connection being visited
     * @return  True if iteration should continue
     */
    bool visitConnection( const std::string& connectionId )
    {
        if( !m_visited[ connectionId ] )
        {
            if( m_visitor.allowVisit( *m_registry, connectionId  ) )
            {
                if( !m_registry->_depthFirstTraversal( connectionId, m_visited, m_visitor ) )
                {
                    return false;
                }
            }
        }

        return true;
    }

private:
    /** The broker registry */
    const BrokerRegistry* m_registry;
    /** The fabric visitor callback */
    BrokerRegistry::FabricVisitor& m_visitor;
    /* State containing brokers that have been visited */
    registry::visit_t& m_visited;
};

/** {@inheritDoc} */
BrokerRegistry& BrokerRegistry::getInstance()
{
    static BrokerRegistry instance;
    return instance;
}

/** {@inheritDoc} */
BrokerRegistry::BrokerRegistry() : m_ttlCheckTime( 0 ), m_localBrokerPort( 0 ),
    m_localBrokerWebSocketPort( 0 ), m_localBrokerConnectionLimit( 0 )
{
    m_topicCacheService.m_brokerRegistry = this;
}

/** {@inheritDoc} */
void BrokerRegistry::clearAllCaches()
{
    // Invalidate routing cache
    m_cache.invalidate();
    // Invalidate topic cache (topic-based routing)
    m_topicCacheService.clearCacheWithDelay();
}

/** {@inheritDoc} */
bool BrokerRegistry::addBroker(
    const std::string &brokerId, const std::string &hostname, uint32_t port, uint32_t ttl, 
    uint32_t startTime, uint32_t webSocketPort, const std::string& policyHostname,
    const std::string& policyIpAddress,const std::string& policyHubName, uint32_t policyPort,
    const std::string& brokerVersion,
    uint32_t connectionLimit, bool topicRoutingEnabled )
{
    bool retVal( false );

    if( !brokerId.empty() && !hostname.empty() )
    {        
        auto iter = m_registry.find( brokerId );

        // The broker exists and its start time has not changed
        if( ( iter != m_registry.end() ) &&
            ( iter->second.getBrokerInternal().getStartTime() == startTime ) )
        {
            // Updating existing broker state
            Broker& broker = iter->second.getBrokerInternal();

            // Topic routing enabled has been changed. 
            // Clear the topic-based routing cache
            if( broker.isTopicRoutingEnabled() != topicRoutingEnabled )
            {
                // Invalidate topic cache (topic-based routing)
                m_topicCacheService.clearCacheWithDelay();
            }

            broker.setHostname( hostname );
            broker.setPort( port );
            broker.updateTtl( ttl );
            broker.setStartTime( startTime );
            broker.setPolicyHostName( policyHostname );
            broker.setPolicyIpAddress( policyIpAddress );
            broker.setPolicyHubName( policyHubName );
            broker.setPolicyPort( policyPort );
            broker.setWebSocketPort( webSocketPort );
            broker.setBrokerVersion( brokerVersion );
            broker.setConnectionLimit( connectionLimit );
            broker.setTopicRoutingEnabled( topicRoutingEnabled );
        }
        else
        {
            // Adding new broker state
            m_registry[brokerId] =
                BrokerState(
                    Broker( brokerId, hostname, port, ttl, startTime, policyHostname,
                        policyIpAddress, policyHubName, policyPort, webSocketPort, brokerVersion,
                        connectionLimit, topicRoutingEnabled ) );

            // Invalidate routing and topic caches
            clearAllCaches();
        }

        retVal = exists( brokerId );        
    }

    return retVal;
}

/** {@inheritDoc} */
bool BrokerRegistry::removeBroker( const std::string &brokerId )
{
    bool retVal( false );

    if( exists( brokerId ) )
    {
        // Remove all connections to the broker
        for( auto i = m_registry.begin(); i != m_registry.end(); ++i )
        {
            (*i).second.removeConnection( brokerId );
            //cache_.invalidate((*i).first, brokerId);
        }

        retVal = ( m_registry.erase( brokerId ) == 1 );

        // Invalidate routing and topic caches
        clearAllCaches();
    }

    return retVal;
}

/** {@inheritDoc} */
bool BrokerRegistry::updateTtl( const std::string &brokerId, uint32_t ttl )
{
    auto it = m_registry.find( brokerId );
    if( it != m_registry.end() )
    {
        (*it).second.updateTtl( ttl );
        return true;
    }

    return false;
}

/** {@inheritDoc} */
bool BrokerRegistry::getBroker( const std::string &brokerId, Broker& broker ) const
{
    auto it = m_registry.find( brokerId );
    if( it != m_registry.end() )
    {
        broker = (*it).second.getBroker();
        return true;
    }
    
    return false;
}

/** {@inheritDoc} */
bool BrokerRegistry::getBrokerState( const std::string &brokerId, BrokerState& state ) const
{
    auto it = m_registry.find( brokerId );
    if( it != m_registry.end() )
    {
        state = (*it).second;
        return true;
    }

    return false;
}

/** {@inheritDoc} */
const BrokerState* BrokerRegistry::getBrokerStatePtr( const std::string &brokerId ) const
{
    auto it = m_registry.find( brokerId );
    if( it != m_registry.end() )
    {
        return &((*it).second);
    }

    return NULL;
}

/** {@inheritDoc} */
std::vector<BrokerState> BrokerRegistry::getAllBrokerStates() const
{
    std::vector<BrokerState> retVal;
    for( auto it = m_registry.begin(); it != m_registry.end(); ++it )
    {
        retVal.push_back( it->second );
    }
    return retVal;
}

/** {@inheritDoc} */
std::vector<const BrokerState*> BrokerRegistry::getAllBrokerStatePtrs() const
{
    std::vector<const BrokerState*> retVal;
    for( auto it = m_registry.begin(); it != m_registry.end(); ++it )
    {
        retVal.push_back( &(it->second) );
    }
    return retVal;
}

/** {@inheritDoc} */
bool BrokerRegistry::addConnection( 
    const std::string &brokerId, const std::string &connectionId, bool isChild )
{
    if( !connectionId.empty() )
    {
        // Make a connection from brokerId to connectionId
        auto it = m_registry.find( brokerId );
        if( it != m_registry.end() )
        {
            // TODO: Check if change prior to clearing cache
            (*it).second.addConnection( connectionId, isChild );

            // Invalidate routing and topic caches
            clearAllCaches();

            return true;
        }
    }

    return false;
}

/** {@inheritDoc} */
bool BrokerRegistry::removeConnection(
    const std::string &brokerId, const std::string &connectionId )
{
    if( !connectionId.empty() )
    {
        // Remove the connection from brokerId to connectionId
        auto it = m_registry.find( brokerId );
        if( it != m_registry.end() )
        {
            // TODO: Check if change prior to clearing cache
            (*it).second.removeConnection( connectionId );
            //cache_.invalidate((*it).first, connectionId);

            // Invalidate routing and topic caches
            clearAllCaches();

            return true;
        }
    }

    return false;
}

/** {@inheritDoc} */
bool BrokerRegistry::hasConnection( 
    const std::string &brokerId, const std::string &connectionId ) const
{
    if( !brokerId.empty() && !connectionId.empty() )
    {
        auto iter = m_registry.find( brokerId );
        if( iter != m_registry.end() )
        {
            return iter->second.hasConnection( connectionId );
        }
    }
    return false;
}

/** {@inheritDoc} */
bool BrokerRegistry::setConnections(
    const std::string &brokerId, 
    const registry::connection_t &connectionIds,
    const registry::childConnections_t &childConnectionIds )
{
    if( !connectionIds.empty() )
    {
        // Find the broker whose connections we're going to modify
        auto regItr = m_registry.find( brokerId );
        if( regItr != m_registry.end() )
        {    
            // Replace the list of current connections
            if( (*regItr).second.setConnections( connectionIds, childConnectionIds ) )
            {            
                // Invalidate routing and topic caches
                clearAllCaches();
            }

            return true;
        }
    }

    return false;
}

/** {@inheritDoc} */
bool BrokerRegistry::updateRegistrationTime( const std::string &brokerId )
{
    auto regItr = m_registry.find( brokerId );
    if( regItr != m_registry.end() )
    {
        regItr->second.updateRegistrationTime();
        return true;
    }    
    return false;
}

/** {@inheritDoc} */
bool BrokerRegistry::exists( const std::string &brokerId ) const
{
    return( !brokerId.empty() && m_registry.count( brokerId ) != 0 );
}

/** {@inheritDoc} */
std::string BrokerRegistry::getNextBroker( const std::string &from, const std::string &to ) const
{
    std::string retVal;
    if( !from.empty() && !to.empty() && exists( from ) && exists( to ) )
    {
        if( from.compare( to ) != 0 )
        {
            // Use the route in the cache if there is one
            retVal = m_cache.get( from, to );
            if( retVal.empty() )
            {
                visit_t visited;
                traversal_t ret = findPathToBroker( from, to, visited );
                if( ret.first )
                {
                    // Pop the first element.  It should be the
                    // current broker.
                    ret.second.pop();

                    // This is the next broker
                    retVal = ret.second.top();

                    // Update the cache
                    m_cache.add( from, to, retVal );
                }
            }
        }
        else
        {
            // Return the value of "to" if "from" and "to" are the same
            retVal = to;
        }
    }
    return retVal;
}

/** {@inheritDoc} */
traversal_t BrokerRegistry::findPathToBroker( 
    const std::string &start, const std::string &finish, registry::visit_t &visited ) const
{
    traversal_t found;

    // We've found a path from start to finish
    if( start == finish )
    {
        found.first = true;
        found.second.push( finish );
    }
    else
    {
        visited[start] = true;
        auto it = m_registry.find( start );
        if( it != m_registry.end() )
        {
            FindPathToBrokerHelper helper( this, start, finish, visited, found );
            (*it).second.forEachConnection(
                std::bind( &FindPathToBrokerHelper::visitConnection, &helper, std::placeholders::_1 ) );
        }
    }

    return found;
}

/** {@inheritDoc} */
void BrokerRegistry::depthFirstTraversal( const std::string &start, FabricVisitor& visitor ) const
{
    visit_t visited;
    if( visitor.allowVisit( *this, start ) )
    {
        _depthFirstTraversal( start, visited, visitor );
    }
}

/** {@inheritDoc} */
bool BrokerRegistry::_depthFirstTraversal( 
    const std::string &start, registry::visit_t &visited, FabricVisitor& visitor ) const
{    
    visited[start] = true;

    if( !visitor.visit( *this, start ) )
    {
        return false;
    }                

    auto it = m_registry.find( start );
    if( it != m_registry.end() )
    {
        DepthFirstTraversalHelper helper( this, visitor, visited );
        if( !( (*it).second.forEachConnection(
            std::bind( &DepthFirstTraversalHelper::visitConnection, &helper, std::placeholders::_1 ) ) ) )
        {
            return false;
        }
    }

    return true;
}

/** {@inheritDoc} */
void BrokerRegistry::onCoreMaintenance( time_t time )
{
    if( ( time - m_ttlCheckTime ) >=
        ( BrokerSettings::getTtlCheckIntervalMins() * 60 ) )
    {
        if( SL_LOG.isDebugEnabled() )
            SL_START << "Checking Broker TTLs: time=" << time << SL_DEBUG_END;

        // Update last check time
        m_ttlCheckTime = time;

        // Check TTLs for services
        for( auto i = m_registry.begin(); i != m_registry.end(); )
        {                        
            if( i->second.isExpired() )
            {
                // The broker that has expired
                std::string currBrokerGuid = i->first;

                // Next (Necessary so the iterator does not get corrupted when broker is removed)
                i++;

                if( SL_LOG.isInfoEnabled() )
                {
                    SL_START << "Broker TTL expired, removing: " << currBrokerGuid << SL_INFO_END;
                }

                // Remove broker
                removeBroker( currBrokerGuid );
            }
            else
            {
                // Next 
                i++;
            }
        }
    }
}

/** {@inheritDoc} */
uint32_t BrokerRegistry::getTopicCount( const std::string &brokerId ) const
{
    auto it = m_registry.find( brokerId );
    if( it != m_registry.end() )
    {
        return (*it).second.getTopicCount();
    }                

    return 0;
}

/** {@inheritDoc} */
bool BrokerRegistry::hasTopic( const std::string &brokerId, const std::string& topic ) const
{
    auto it = m_registry.find( brokerId );
    if( it != m_registry.end() )
    {
        return (*it).second.hasTopic( topic );
    }                

    return false;
}

/** {@inheritDoc} */
bool BrokerRegistry::hasTopics(
    const std::string &brokerId, const registry::subscriptions_t& topics ) const
{
    auto it = m_registry.find( brokerId );
    if( it != m_registry.end() )
    {
        return (*it).second.hasTopics( topics );
    }

    return false;
}

/** {@inheritDoc} */
bool BrokerRegistry::addTopic( const std::string &brokerId, const std::string &topic )
{
    auto it = m_registry.find( brokerId );
    if( it != m_registry.end() )
    {
        if( (*it).second.addTopic( topic ) )
        {
            // Update the topic cache service
            m_topicCacheService.addTopic( brokerId, topic );
            return true;
        }
    }

    return false;
}

/** {@inheritDoc} */
bool BrokerRegistry::removeTopic( const std::string &brokerId, const std::string &topic )
{
    auto it = m_registry.find( brokerId );
    if( it != m_registry.end() )
    {
        if( (*it).second.removeTopic( topic ) )
        {
            // Update the topic cache
            m_topicCacheService.removeTopic( brokerId, topic );
            return true;
        }
    }

    return false;
}

/** {@inheritDoc} */
uint32_t BrokerRegistry::getTopicWildcardCount( const std::string &brokerId ) const
{
    auto it = m_registry.find( brokerId );
    if( it != m_registry.end() )
    {
        return (*it).second.getTopicWildcardCount();
    }

    return 0;
}

/** {@inheritDoc} */
bool BrokerRegistry::batchTopics( const std::string& brokerId, const int charCount, 
    const BrokerState::TopicsCallback& callback ) const
{
    auto it = m_registry.find( brokerId );
    if( it != m_registry.end() )
    {
        (*it).second.batchTopics( charCount, callback );
        return true;
    }

    return false;
}

/** {@inheritDoc} */
bool BrokerRegistry::clearPendingTopics( const std::string& brokerId )
{
    auto it = m_registry.find( brokerId );
    if( it != m_registry.end() )
    {
        (*it).second.clearPendingTopics();
        return true;
    }

    return false;
}

/** {@inheritDoc} */
bool BrokerRegistry::addPendingTopics( 
    const std::string& brokerId, const registry::subscriptions_t& subs, uint32_t wildcardCount )
{
    auto it = m_registry.find( brokerId );
    if( it != m_registry.end() )
    {
        (*it).second.addPendingTopics( subs, wildcardCount );
        return true;
    }

    return false;
}

/** {@inheritDoc} */
bool BrokerRegistry::swapPendingTopics( const std::string& brokerId ) 
{
    auto it = m_registry.find( brokerId );
    if( it != m_registry.end() )
    {
        (*it).second.swapPendingTopics();

        // TODO: Optimize this so it only clears if state has changed
        // Invalidate topic cache (topic-based routing)
        m_topicCacheService.clearCacheWithDelay();

        return true;
    }

    return false;
}

/** {@inheritDoc} */
bool BrokerRegistry::setTopicsChangeCount( const std::string& brokerId, uint32_t changeCount )
{
    auto it = m_registry.find( brokerId );
    if( it != m_registry.end() )
    {
        it->second.setTopicsChangeCount( changeCount );
        return true;
    }

    return false;
}

/** {@inheritDoc} */
bool BrokerRegistry::isSubscriberInBroker( const std::string &brokerId, const std::string &topic ) const
{
    auto it = m_registry.find( brokerId );
    if( it != m_registry.end() )
    {
        const BrokerState& state = it->second;

        // If topic routing is disabled or the broker has the topic
        if( !state.isTopicRoutingEnabled() || state.hasTopic( topic ) )
        {
            return true;
        }

        // Check for wildcards (if applicable)
        if( state.getTopicWildcardCount() > 0 )
        {
            char* wcTopic = CoreUtil::iterateWildcardBegin( topic.c_str() );

            bool found = false;
            while( CoreUtil::iterateWildcardNext( wcTopic ) )
            {
                if( state.hasTopic( wcTopic ) )
                {
                    found = true;
                    break;
                }
            }
            CoreUtil::iterateWildcardEnd( wcTopic );

            if( found )
            {
                return true;
            }
        }
    }

    return false;
}

/** {@inheritDoc} */
bool BrokerRegistry::isSubscriberInHierarchy( 
    const std::string &brokerId, const std::string &connection, const std::string &topic ) const
{
    // TODO: Should we check to ensure the connection exists?

    bool found = false;
    if( !m_topicCacheService.isSubscriber( brokerId, connection, topic, &found ) )
    {
        FindSubscriberVisitor finder( brokerId, topic );
        depthFirstTraversal( connection, finder );
        found = finder.isFound();
    }

    return found;
}

/** {@inheritDoc} */
void BrokerRegistry::setLocalBrokerProperties( 
    const string &hostName, const string& ipAddress, const string &hub, uint32_t port, uint32_t webSocketPort )
{
    // Lock mutex
    lock_guard<mutex> lock( m_localBrokerPropsMutex );

    m_localBrokerHostName = hostName;
    m_localBrokerIpAddress = ipAddress;
    m_localBrokerHub = hub;
    m_localBrokerPort = port;
    m_localBrokerWebSocketPort = webSocketPort;
}

/** {@inheritDoc} */
void BrokerRegistry::setLocalManagingEpoName( const string &managingEpoName )
{
    // Lock mutex
    lock_guard<mutex> lock( m_localBrokerPropsMutex );

    m_localManagingEpoName = managingEpoName;
}

/** {@inheritDoc} */
string BrokerRegistry::getLocalManagingEpoName()
{
    // Lock mutex
    lock_guard<mutex> lock( m_localBrokerPropsMutex );

    return m_localManagingEpoName;
}

/** {@inheritDoc} */
string BrokerRegistry::getLocalBrokerHostname()
{
    // Lock mutex
    lock_guard<mutex> lock( m_localBrokerPropsMutex );

    return m_localBrokerHostName;
}

/** {@inheritDoc} */
string BrokerRegistry::getLocalBrokerIpAddress()
{
    // Lock mutex
    lock_guard<mutex> lock( m_localBrokerPropsMutex );

    return m_localBrokerIpAddress;
}

/** {@inheritDoc} */
string BrokerRegistry::getLocalBrokerHub()
{
    // Lock mutex
    lock_guard<mutex> lock( m_localBrokerPropsMutex );

    return m_localBrokerHub;
}

/** {@inheritDoc} */
string BrokerRegistry::getLocalBrokerVersion()
{
    return BrokerHelpers::PROPERTY_VALUE_PRODUCT_VERSION;
}

/** {@inheritDoc} */
uint32_t BrokerRegistry::getLocalBrokerPort()
{
    // Lock mutex
    lock_guard<mutex> lock( m_localBrokerPropsMutex );

    return m_localBrokerPort;
}

/** {@inheritDoc} */
uint32_t BrokerRegistry::getLocalBrokerWebSocketPort()
{
    // Lock mutex
    lock_guard<mutex> lock( m_localBrokerPropsMutex );

    return m_localBrokerWebSocketPort;
}

/** {@inheritDoc} */
void BrokerRegistry::setLocalBrokerConnectionLimit( int limit )
{
    // Lock mutex
    lock_guard<mutex> lock( m_localBrokerPropsMutex );

    m_localBrokerConnectionLimit = limit;
}

/** {@inheritDoc} */
uint32_t BrokerRegistry::getLocalBrokerConnectionLimit()
{
    // Lock mutex
    lock_guard<mutex> lock( m_localBrokerPropsMutex );

    return m_localBrokerConnectionLimit;
}

/** {@inheritDoc} */
bool BrokerRegistry::isLocalBrokerTopicRoutingEnabled() const
{
    return BrokerSettings::isTopicRoutingEnabled();
}

/** {@inheritDoc} */
std::ostream & operator <<( std::ostream &out, const BrokerRegistry &brokerRegistry )
{
    for(auto i = brokerRegistry.m_registry.begin(); i != brokerRegistry.m_registry.end(); ++i)
    {
        out << "Key:  " << (*i).first << std::endl;
        out << (*i).second << std::endl;
    }
                
    out << "Cache:  " << std::endl;
    out << brokerRegistry.m_cache;
    return out;
}

}
}
