/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "brokerregistry/include/brokerregistry.h"
#include "brokerregistry/topiccache/include/BrokerTopicCache.h"
#include "brokerregistry/topiccache/include/TopicCacheService.h"

using namespace std;
using namespace dxl::broker::topiccache;

/* {@inheritDoc} */
BrokerTopicCache::BrokerTopicCache( TopicCacheService* cacheService, const string& brokerId ) : 
    m_brokerId( brokerId ),
    m_topicCacheService( cacheService )    
{
}

/** {@inheritDoc} */
brokerBridgeTopicCachePtr_t BrokerTopicCache::getBridgeCache( const string& bridgeId )
{
    auto iter = m_brokerBridgeTopicCacheById.find( bridgeId );
    if( iter != m_brokerBridgeTopicCacheById.end() )
    {
        return iter->second;
    }

    // Ensure the broker exists 
    if( getBrokerRegistry()->hasConnection( m_brokerId, bridgeId ) )
    {
        brokerBridgeTopicCachePtr_t ptr = 
            brokerBridgeTopicCachePtr_t( new BrokerBridgeTopicCache( this, bridgeId ) );
        m_brokerBridgeTopicCacheById.insert( std::make_pair( bridgeId, ptr ) );
        return ptr;
    }
    else
    {
        return brokerBridgeTopicCachePtr_t();
    }
}

/** {@inheritDoc} */
void BrokerTopicCache::addTopic( const string& brokerId, const string& topic ) const
{
    for( auto iter = m_brokerBridgeTopicCacheById.begin(); 
        iter != m_brokerBridgeTopicCacheById.end(); iter++ )
    {
        if( iter->second->addTopic( brokerId, topic ) )
        {
            // We attempted to add the topic to the correct bridge cache, exit
            break;
        }
    }
}

/** {@inheritDoc} */
void BrokerTopicCache::removeTopic( const string& brokerId, const string& topic ) const
{
    for( auto iter = m_brokerBridgeTopicCacheById.begin(); 
        iter != m_brokerBridgeTopicCacheById.end(); iter++ )
    {
        if( iter->second->removeTopic( brokerId, topic ) )
        {
            // We attempted to remove the topic from the correct bridge cache, exit
            break;
        }
    }
}

/** {@inheritDoc} */
dxl::broker::BrokerRegistry* BrokerTopicCache::getBrokerRegistry() const
{ 
    return m_topicCacheService->getBrokerRegistry(); 
}



