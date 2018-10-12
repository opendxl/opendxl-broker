/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/BrokerSettings.h"
#include "include/SimpleLog.h"
#include "brokerregistry/include/brokerregistry.h"
#include "brokerregistry/topiccache/include/TopicCacheService.h"
#include "util/include/TimeUtil.h"

using namespace std;
using namespace dxl::broker::topiccache;
using namespace dxl::broker::util;

/* {@inheritDoc} */
TopicCacheService::TopicCacheService() : 
    m_brokerRegistry( NULL ), m_wasEnabled( false ), m_enabledTime( 0 )
{
    m_wasEnabled = checkEnabled();
}

/** {@inheritDoc} */
brokerTopicCachePtr_t TopicCacheService::getBrokerCache( const string& brokerId )
{
    auto iter = m_brokerCacheById.find( brokerId );
    if( iter != m_brokerCacheById.end() )
    {
        return iter->second;
    }

    // Ensure the broker exists 
    if( m_brokerRegistry->exists( brokerId ) )
    {
        brokerTopicCachePtr_t ptr = brokerTopicCachePtr_t( new BrokerTopicCache( this, brokerId ) );
        m_brokerCacheById.insert( std::make_pair( brokerId, ptr ) );
        return ptr;
    }
    else
    {
        return brokerTopicCachePtr_t();
    }
}

/** {@inheritDoc} */
void TopicCacheService::addTopic( const string& brokerId, const string& topic )
{
    if( checkEnabled() )
    {
        for( auto iter = m_brokerCacheById.begin(); iter != m_brokerCacheById.end(); iter++ )
        {
            iter->second->addTopic( brokerId, topic );
        }
    }
}

/** {@inheritDoc} */
void TopicCacheService::removeTopic( const string& brokerId, const string& topic )
{
    if( checkEnabled() )
    {
        for( auto iter = m_brokerCacheById.begin(); iter != m_brokerCacheById.end(); iter++ )
        {
            iter->second->removeTopic( brokerId, topic );
        }
    }
}

/** {@inheritDoc} */
bool TopicCacheService::isSubscriber( 
    const string& brokerId, const string& bridgeId, const string& topic, bool* result )
{
    if( checkEnabled() )
    {
        *result = false;
        const brokerTopicCachePtr_t& brokerPtr = getBrokerCache( brokerId );
        if( brokerPtr.get() )
        {
            const brokerBridgeTopicCachePtr_t& bridgePtr = brokerPtr->getBridgeCache( bridgeId );
            if( bridgePtr.get() )
            {
                return bridgePtr->isSubscriber( topic, result );
            }        
        }
    }
    return false;
}

/** {@inheritDoc} */
void TopicCacheService::clearCacheWithDelay( const uint32_t delay )
{
    uint32_t d = delay != 0 ? delay : BrokerSettings::getTopicRoutingCacheClearDelay();

    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "TopicCacheService::clearCacheWithDelay()" << SL_DEBUG_END;
    }

    // Clear cache
    clearCache();

    // Add delay (if applicable)
    m_enabledTime = ( d > 0 ) ? ( TimeUtil::getCurrentTimeSeconds() + d ) : 0;
}

/** {@inheritDoc} */
void TopicCacheService::clearCache()
{
    if( checkEnabled( true ) )
    {
        // Clear the cache
        m_brokerCacheById.clear();    

        if( SL_LOG.isDebugEnabled() )
        {
            SL_START << "TopicCacheService::clearCache()" << SL_DEBUG_END;
        }
    }
}

/** {@inheritDoc} */
bool TopicCacheService::checkEnabled( bool isClear ) 
{
    // Enabled: Topic routing is enabled and we aren't in the middle of a delay
    // prior to becoming enabled.
    bool enabled = 
        BrokerSettings::isTopicRoutingCacheEnabled() &&
        ( ( m_enabledTime == 0 ) || ( TimeUtil::getCurrentTimeSeconds() > m_enabledTime ) );

    if( enabled && m_enabledTime != 0 )
    {
        // Reset enabled time if applicable
        m_enabledTime = 0;
    }

    // If we transitioned from disabled to enabled, clear the cache
    if( !m_wasEnabled && enabled && !isClear )
    {
        clearCache();
    }
    m_wasEnabled = enabled;

    return enabled;
}
