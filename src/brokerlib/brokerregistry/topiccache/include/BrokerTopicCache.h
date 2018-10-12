/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERTOPICCACHE_H_
#define BROKERTOPICCACHE_H_

#include <memory>
#include "include/unordered_map.h"
#include "brokerregistry/topiccache/include/BrokerBridgeTopicCache.h"

namespace dxl {
namespace broker {

/** Forward reference */
class BrokerRegistry;

namespace topiccache {

/** Forward reference to the topic cache service */
class TopicCacheService;

/** Type definition for the broker topic caches by the broker identifier */
typedef unordered_map<std::string, brokerBridgeTopicCachePtr_t> brokerBridgeTopicCacheById_t;

/**
 * The cache associated with a specific broker (A cache is created for each bridge for the
 * broker).
 */
class BrokerTopicCache
{    
    /** The bridge cache is a friend */
    friend class BrokerBridgeTopicCache;

public:
    /** 
     * Constructor 
     *
     * @param   topicCacheService The topic cache service
     * @param   brokerId The broker associated with this cache
     */    
    BrokerTopicCache( TopicCacheService* topicCacheService, const std::string& brokerId );

    /** Destructor */
    virtual ~BrokerTopicCache() {}

    /**
     * Returns the cache for the bridge with the specified broker identifier
     *
     * @param   bridgeId The broker bridge identifier
     * @return  The cache for the bridge with the specified broker identifier
     */
    brokerBridgeTopicCachePtr_t getBridgeCache( const std::string& bridgeId );

    /**
     * Updates the caches in response to the specified topic being added
     * to the specified broker
     *
     * @param   brokerId The broker the topic was added to
     * @param   topic The topic that was added
     */
    void addTopic( const std::string& brokerId, const std::string& topic ) const;

    /**
     * Updates the caches in response to the specified topic being removed
     * from the specified broker
     *
     * @param   brokerId The broker the topic was removed from
     * @param   topic The topic that was removed
     */
    void removeTopic( const std::string& brokerId, const std::string& topic ) const;

    /**
     * Returns the broker identifier
     *
     * @return  The broker identifier
     */
    std::string getBrokerId() const { return m_brokerId; }

private:
    /** 
     * Returns the broker registry associated with the service
     *
     * @return  The broker registry associated with the service
     */
    dxl::broker::BrokerRegistry* getBrokerRegistry() const;

    /** Broker topic caches by the bridge */
    brokerBridgeTopicCacheById_t m_brokerBridgeTopicCacheById;

    /** The broker associated with this cache */
    std::string m_brokerId;

    /** The topic cache service */
    TopicCacheService* m_topicCacheService;
};

/** Pointer to a broker topic cache */
typedef std::shared_ptr<BrokerTopicCache> brokerTopicCachePtr_t;

} /* namespace topiccache */
} /* namespace broker */
} /* namespace dxl */

#endif /* BROKERTOPICCACHE_H_ */
