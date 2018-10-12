/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/SimpleLog.h"
#include "brokerregistry/include/brokerregistry.h"
#include "brokerregistry/topiccache/include/BrokerBridgeTopicCache.h"
#include "brokerregistry/topiccache/include/BrokerTopicCache.h"
#include "core/include/CoreUtil.h"

using namespace std;
using namespace dxl::broker;
using namespace dxl::broker::core;
using namespace dxl::broker::topiccache;

/**
 * Visitor used to find all of the brokers
 */
class FindBrokersVisitor : public BrokerRegistry::FabricVisitor
{
public:
    /**
     * Constructs the finder
     * 
     * @param   broker The broker we are performing the find on behalf of
     * @param   brokers The brokers that were visited during the recursion (output)
     * @param   topicRoutingEnabled Whether topic routing is enabled (output)
     */
    FindBrokersVisitor( const string& broker, brokers_t* brokers, bool* topicRoutingEnabled ) : 
        m_broker( broker ), m_brokers( brokers ), m_topicRoutingEnabled( topicRoutingEnabled )
    {
        if( SL_LOG.isDebugEnabled() )
        {
            SL_START << "FindBrokersVisitor: " << broker << SL_DEBUG_END;
        }
    }

    /** {@inheritDoc} */
    bool allowVisit( const BrokerRegistry& /*registry*/, const string& to ) const
    {
        return to != m_broker;
    }

    /** {@inheritDoc} */
    bool visit( const BrokerRegistry& registry, const string& to )
    {
        if( SL_LOG.isDebugEnabled() )
        {
            SL_START << "  visit: " << to << SL_DEBUG_END;
        }

        m_brokers->insert( to );
        const BrokerState* state = registry.getBrokerStatePtr( to );

        if( state )
        {
            if( !state->isTopicRoutingEnabled() )
            {
                // Topic routing is disabled
                *m_topicRoutingEnabled = false;

                // Stop recursing
                return false;
            }
        }
        else
        {
            SL_START << "Error, unable to find broker state for: " << to << SL_ERROR_END;
        }

        return true;
    }

private:
    /** The broker */
    const string m_broker;
    /** The brokers visited */
    brokers_t* m_brokers;
    /** Whether topic routing is enabled */
    bool* m_topicRoutingEnabled;
};

/* {@inheritDoc} */
BrokerBridgeTopicCache::BrokerBridgeTopicCache( 
    BrokerTopicCache* brokerTopicCache, const string& bridgeId ) : 
    m_brokerTopicCache( brokerTopicCache ), m_bridgeId( bridgeId ), 
    m_topics( /*(std::size_t)100000*/ ), m_wildcardCount( 0 ), 
    m_topicRoutingEnabled( true ), 
    m_state( Start )
{    
}

/** {@inheritDoc} */
bool BrokerBridgeTopicCache::_addTopic( const string& topic )
{
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "    topic: " << topic << SL_DEBUG_END;
    }

    auto res = m_topics.insert( topic );
    if( res.second && CoreUtil::isWildcard( topic.c_str() ) )
    {
        m_wildcardCount++;
    }

    return true;
}

/** {@inheritDoc} */
void BrokerBridgeTopicCache::buildCache()
{
    while( m_state != Completed )
    {
        buildCacheIncrement();
    }
}

/** {@inheritDoc} */
bool BrokerBridgeTopicCache::buildCacheIncrement()
{
    if( m_state == Completed )
    {
        return true;
    }

    BrokerRegistry* registry = getBrokerRegistry();

    switch( m_state )
    {
        case Start:
            {                    
                // Find all of the brokers associated with this cache
                FindBrokersVisitor findBrokers( 
                    m_brokerTopicCache->getBrokerId(), &m_brokers, &m_topicRoutingEnabled );
                registry->depthFirstTraversal( m_bridgeId, findBrokers );

                // Update the brokers iterator
                m_brokersIter = m_brokers.begin();

                // Update the state, Building = routing enabled and we have brokers to examine
                m_state = 
                    ( m_topicRoutingEnabled && m_brokersIter != m_brokers.end() ) ? 
                        Building : Completed;
            }
            break;
        case Building:
            {
                // Add topics for the next broker
                const BrokerState* state = registry->getBrokerStatePtr( *m_brokersIter );
                if( state )
                {
                    state->forEachTopic(
                        std::bind( &BrokerBridgeTopicCache::_addTopic, 
                            this, std::placeholders::_1 ) );

                    // Record that we have cached this particular broker
                    m_cachedBrokers.insert( *m_brokersIter );
                }
                else
                {
                    SL_START << "Error, unable to find broker state for: " <<  
                        *m_brokersIter << SL_ERROR_END;
                }

                // Move to the next broker
                m_brokersIter++;

                if( m_brokersIter == m_brokers.end() )
                {
                    // No more brokers, the cache is complete
                    m_state = Completed;
                }
            }
            break;                    
        case Completed:
            break;
    }

    // Return whether the cache is complete
    return m_state == Completed;
}

/* {@inheritDoc} */
bool BrokerBridgeTopicCache::containsBroker( const string& brokerId ) const
{
    return m_brokers.find( brokerId ) != m_brokers.end();
}

/* {@inheritDoc} */
bool BrokerBridgeTopicCache::isBrokerCached( const string& brokerId ) const
{
    return isComplete() ? 
        containsBroker( brokerId ) :
        m_cachedBrokers.find( brokerId ) != m_cachedBrokers.end();
}

/** {@inheritDoc} */
bool BrokerBridgeTopicCache::addTopic( const string& brokerId, const string& topic )
{
    // Ensure this is the appropriate cache
    if( containsBroker( brokerId ) )
    {
        // Ensure topic routing is enabled, the broker has been cached,
        // and the topic is not in the cache 
        if( m_topicRoutingEnabled && 
            isBrokerCached( brokerId ) && 
            ( m_topics.find( topic ) == m_topics.end() ) )
        {
            // Ensure the broker actually has the topic
            if( getBrokerRegistry()->hasTopic( brokerId, topic ) )                 
            {
                _addTopic( topic );
            }
            else
            {
                SL_START << "Error during add topic to cache, broker: '" << brokerId << 
                    "' does not contain topic '" << topic << "'" << SL_ERROR_END;            
            }
        }

        // True, this is the correct cache for the broker
        return true;
    }

    // This cache does not contain the specified broker
    return false;
}

/** {@inheritDoc} */
bool BrokerBridgeTopicCache::removeTopic( const string& brokerId, const string& topic )
{
    // Ensure this is the appropriate cache
    if( containsBroker( brokerId ) )
    {        
        // Ensure topic routing is enabled, the broker has been cached, 
        // and the topic is in the cache 
        if( m_topicRoutingEnabled && 
            isBrokerCached( brokerId ) && 
            ( m_topics.find( topic ) != m_topics.end() ) )
        {
            // Ensure the broker does not have the topic
            if( !getBrokerRegistry()->hasTopic( brokerId, topic ) ) 
            {
                bool foundTopic = false;
                BrokerRegistry* registry = getBrokerRegistry();
                for( auto iter = m_cachedBrokers.begin(); iter != m_cachedBrokers.end(); iter++ )
                {
                    if( registry->hasTopic( *iter, topic ) )
                    {
                        foundTopic = true;
                        break;
                    }
                }

                // We didn't find the topic, erase it from the cache
                if( !foundTopic )            
                {
                    if( m_topics.erase( topic ) && CoreUtil::isWildcard( topic.c_str() ) )
                    {
                        // Update wildcard count
                        m_wildcardCount--;
                    }
                }
            }
            else
            {
                SL_START << "Error during remove topic from cache, broker: '" << brokerId << 
                    "' still contains topic '" << topic << "'" << SL_ERROR_END;
            }
        }

        // True, this is the correct cache for the broker
        return true;
    }

    // This cache does not contain the specified broker
    return false;
}

/** {@inheritDoc} */
bool BrokerBridgeTopicCache::isSubscriber( const string& topic, bool* result )
{    
    *result = false;

    // Ensure the cache is built
    if( !buildCacheIncrement() )
    {
        // Cache is not built, returning false
        return false;
    }

    // If topic routing is disabled or the broker has the topic
    if( !m_topicRoutingEnabled || ( m_topics.find( topic ) != m_topics.end() ) )
    {
        *result = true;
    }
    else if( m_wildcardCount > 0 )  // Check for wildcards (if applicable)
    {
        char* wcTopic = CoreUtil::iterateWildcardBegin( topic.c_str() );
        while( CoreUtil::iterateWildcardNext( wcTopic ) )
        {
            if( m_topics.find( wcTopic ) != m_topics.end() )
            {
                *result = true;
                break;
            }
        }
        CoreUtil::iterateWildcardEnd( wcTopic );
    }

    return true;
}

/** {@inheritDoc} */
dxl::broker::BrokerRegistry* BrokerBridgeTopicCache::getBrokerRegistry() const
{ 
    return m_brokerTopicCache->getBrokerRegistry(); 
}
