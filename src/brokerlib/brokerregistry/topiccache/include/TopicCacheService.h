/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef TOPICCACHESERVICE_H_
#define TOPICCACHESERVICE_H_

#include <cstdint>
#include <string>

#include "include/unordered_map.h"
#include "brokerregistry/topiccache/include/BrokerTopicCache.h"

namespace dxl {
namespace broker {

/** Forward reference */
class BrokerRegistry;

/** Namespace for topic caching-related declarations */
namespace topiccache {

/** Type definition for the broker topic caches by the broker identifier */
typedef unordered_map<std::string, brokerTopicCachePtr_t> brokerTopicCacheById_t;

/**
 * The service used for accessing subscription (topic) caches for brokers
 */
class TopicCacheService
{    
    /** The broker cache is a friend */
    friend class BrokerTopicCache;

    /** The broker registry */
    friend class dxl::broker::BrokerRegistry;

public:
    /** Destructor */
    virtual ~TopicCacheService() {}

    /**
     * Updates the caches in response to the specified topic being added
     * to the specified broker
     *
     * @param   brokerId The broker the topic was added to
     * @param   topic The topic that was added
     */
    void addTopic( const std::string& brokerId, const std::string& topic );

    /**
     * Updates the caches in response to the specified topic being removed
     * from the specified broker
     *
     * @param   brokerId The broker the topic was removed from
     * @param   topic The topic that was remove
     */
    void removeTopic( const std::string& brokerId, const std::string& topic );

    /**
     * Returns true if a subscriber exists for the topic via the specified bridge
     * for the specified broker. 
     *
     * NOTE: The result is only valid if the return value from this method is true.
     *       The issue is that the cache can become enabled/disabled at various times.
     *       Therefore, the return value must be checked to ensure the result is correct.
     *       If the return value is not valid, a non-cached form of subscriber lookup must
     *       be utilized.
     *
     * @param   brokerId The broker from which to determine if there is a subscriber
     * @param   bridgeId The bridge from the broker
     * @param   topic The topic
     * @param   result The result of the lookup operation
     * @return  True if the invocation was successful (the cache is enabled, and the
     *          result is valid).
     */
    bool isSubscriber( const std::string& brokerId, const std::string& bridgeId, 
        const std::string& topic, bool* result );

    /**
     * Clears the entire cache
     */
    void clearCache();

    /**
     * Clears the entire cache and delays it being enabled. It will be disabled
     * for the period specified. This is important as certain events in the broker
     * (fabric change, etc.) can cause multiple calls to clear in a short period
     * of time. The delay minimizes churn.
     *
     * @param   delay The delay for being enabled (in seconds). A delay of 0 specifies
     *          that the default delay should be used (see BrokerSettings).
     */
    void clearCacheWithDelay( const uint32_t delay = 0 );

private:
    /** Constructor */
    TopicCacheService();

    /**
     * Returns the broker cache for the specified broker identifier
     *
     * @param   brokerId The broker identifier
     * @return  The broker cache for the specified broker identifier
     */
    brokerTopicCachePtr_t getBrokerCache( const std::string& brokerId );

    /**
     * Method used within the cache methods to determine if the cache is enabled.
     * This method tracks that enabled state of the cache and resets it appropriately
     * as the state changes between enabled and disabled.
     *
     * @param   Whether it is the clear method that is checking enabled state
     */
    bool checkEnabled( bool isClear = false );

    /** 
     * Returns the broker registry associated with the service
     *
     * @return  The broker registry associated with the service
     */
    dxl::broker::BrokerRegistry* getBrokerRegistry() const { return m_brokerRegistry; }

    /** Broker topic caches by their identifier */
    brokerTopicCacheById_t m_brokerCacheById;

    /** The broker registry */
    dxl::broker::BrokerRegistry* m_brokerRegistry;

    /** Whether the cache was enabled on the last invocation of "checkEnabled()" */
    bool m_wasEnabled;

    /** The time (in seconds) that the cache will be enabled (0 = not enabled) */
    uint32_t m_enabledTime;
};

} /* namespace topiccache */
} /* namespace broker */
} /* namespace dxl */

#endif /* TOPICCACHESERVICE_H_ */
