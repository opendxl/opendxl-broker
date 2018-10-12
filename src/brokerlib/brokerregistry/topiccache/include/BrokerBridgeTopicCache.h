/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERBRIDGETOPICCACHE_H_
#define BROKERBRIDGETOPICCACHE_H_

#include <memory>
#include "include/unordered_set.h"

namespace dxl {
namespace broker {

/** Forward reference */
class BrokerRegistry;

namespace topiccache {

/** Forward reference */
class BrokerTopicCache;

/** Typedef for a collection of broker identifiers */
typedef unordered_set<std::string> brokers_t;

/** Typedef for the subscriptions */
typedef unordered_set<std::string> cachesubs_t;

/**
 * Contains a cache of the topics that are subscribed to via a particular bridge from
 * a broker.
 */
class BrokerBridgeTopicCache
{    
public:
    /** 
     * Constructor 
     *
     * @param   brokerTopicCache The cache for the broker that this bridge is associated with.
     * @param   bridgeId The bridge that this cache is for
     */
    BrokerBridgeTopicCache( BrokerTopicCache* brokerTopicCache, const std::string& bridgeId );

    /**
     * Updates the caches in response to the specified topic being added
     * to the specified broker
     *
     * @param   brokerId The broker the topic was added to
     * @param   topic The topic that was added
     * @return  Whether this cache contains the specified broker identifier
     */
    bool addTopic( const std::string& brokerId, const std::string& topic );

    /**
     * Updates the caches in response to the specified topic being removed
     * from the specified broker
     *
     * @param   brokerId The broker the topic was removed from
     * @param   topic The topic that was removed
     * @return  Whether this cache contains the specified broker identifier
     */
    bool removeTopic( const std::string& brokerId, const std::string& topic );

    /**
     * Returns true if a subscriber exists for the topic
     *
     * NOTE: The result is only valid if the return value from this method is true.
     *       The issue is that the cache can become enabled/disabled at various times.
     *       Therefore, the return value must be checked to ensure the result is correct.
     *       If the return value is not valid, a non-cached form of subscriber lookup must
     *       be utilized.
     *
     * @param   topic The topic
     * @param   result The result of the lookup operation
     * @return  True if the invocation was successful (the cache is enabled, and the
     *          result is valid).
     */
    bool isSubscriber( const std::string& topic, bool* result );

    /**
     * Whether the broker was used to build the cache
     *
     * @param   brokerId The broker identifier
     * @return  Whether the broker was used to build the cache
     */
    bool containsBroker( const std::string& brokerId ) const;

    /** Destructor */
    virtual ~BrokerBridgeTopicCache() {}

private:
    /** The state of the cache */
    enum CacheState { Start, Building, Completed };

    /** 
     * Returns the broker registry associated with the service
     *
     * @return  The broker registry associated with the service
     */
    dxl::broker::BrokerRegistry* getBrokerRegistry() const;

    /**
     * Returns whether the cache is complete
     *
     * @return  Whether the cache is complete
     */
    bool isComplete() const { return m_state == Completed; }

    /**
     * Whether the broker has had its topics cached
     *
     * @return  Whether the broker has had its topics cached
     */
    bool isBrokerCached( const std::string& brokerId ) const;

    /**
     * Internal method for adding a topic to the cache
     *
     * @param   topic The topic to add
     */
    bool _addTopic( const std::string& topic );
    
    /**
     * Builds the entire cache (not incrementally)
     */
    void buildCache();

    /**
     * Incrementally builds the cache (this invocation builds the next increment)
     *
     * @return  Whether the cache has completed building
     */
    bool buildCacheIncrement();

    /** The broker topic cache */
    BrokerTopicCache* m_brokerTopicCache;

    /** The bridge identifier */
    std::string m_bridgeId;

    /** The brokers in the cache */
    brokers_t m_brokers;

    /** The brokers that have been added to the cache */
    brokers_t m_cachedBrokers;

    /** The topics in the cache */
    cachesubs_t m_topics;

    /** The count of wildcards */
    int m_wildcardCount;

    /** Whether topic based routing is enabled */
    bool m_topicRoutingEnabled;

    /** The state of the cache */
    CacheState m_state;

    /** Iterator, pointing to brokers as the cache is being built */
    brokers_t::iterator m_brokersIter;
};

/** Pointer to a broker topic cache */
typedef std::shared_ptr<BrokerBridgeTopicCache> brokerBridgeTopicCachePtr_t;

} /* namespace topiccache */
} /* namespace broker */
} /* namespace dxl */

#endif /* BROKERBRIDGETOPICCACHE_H_ */
