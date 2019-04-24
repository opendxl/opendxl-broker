/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef _BROKER_REGISTRY_H_
#define _BROKER_REGISTRY_H_

#include <ctime>
#include <list>
#include <mutex>
#include <stack>
#include <string>
#include <vector>

#include "include/unordered_map.h"
#include "include/unordered_set.h"
#include "brokerregistry/include/brokerstate.h"
#include "brokerregistry/include/broker.h"
#include "brokerregistry/include/cache.h"
#include "brokerregistry/topiccache/include/TopicCacheService.h"
#include "core/include/CoreMaintenanceListener.h"

namespace dxl {
namespace broker {

/** Namespace for broker registry-related declarations */
namespace registry {
/**
 * The type used to store information about broker's and their connections.
 * It is an adjacency list stored in a map. If the broker's are connected
 * like so, the adjacency list looks like:
 *
 *     a: b, d, c                A
 *     b: a, e                  /|\
 *     c: a                    B D C
 *     d: a                   /
 *     e: b                  E
 *
 * The letter is a key.  It points to a BrokerState object.
 */
typedef unordered_map<std::string, BrokerState> registry_t;
const char DEFAULTHOSTNAME[] = "UNKNOWN";
const char DEFAULTIPADDRESS[] = "UNKNOWN";
typedef unordered_map<std::string, bool> visit_t;
typedef std::stack<std::string> path_t;
typedef std::pair<bool, path_t> traversal_t;
}

/**
 * Broker registry contains the state of the brokers throughout the current fabric hierarchy
 */
class BrokerRegistry : public dxl::broker::core::CoreMaintenanceListener
{
    /** Helper for the find path method */
    friend class FindPathToBrokerHelper;
    /** Helper for the depth first search method */
    friend class DepthFirstTraversalHelper;
    /** Visitor for finding subscribers */
    friend class FindSubscriberVisitor;

public:
    /**
     * Interface that can be used to visit (iterate) the broker hierarchy
     */
    class FabricVisitor
    {
    public:
        /**
         * Returns whether the specified broker should be visited
         *
         * @param   registry The broker registry
         * @param   to The guid of the broker
         * @return  True if the broker should be visited
         */
        virtual bool allowVisit( 
            const BrokerRegistry& registry, const std::string& to ) const = 0;

        /**
          * Invoked when a broker is visited
         *
         * @param   registry The broker registry
         * @param   to The guid of the broker being visited
         * @return  True if visiting should continue
         */
        virtual bool visit( 
            const BrokerRegistry& registry, const std::string& to ) = 0;
    };

public:
    /**
     * Returns the single service instance
     *
      * @return The single service instance
     */
    static BrokerRegistry& getInstance();

    /**
     * Adds a broker to the registry. If the broker already exists, its values will be updated with
     * those specified.
     *
     * @param   brokerId The broker identifier
     * @param   hostname The broker host name
     * @param   port The broker port
     * @param   ttl The broker time to live
     * @param   startTime The broker startup time
     * @param   webSocketPort The WebSocket port
     * @param   policyHostname The policy host name
     * @param   policyIpAddress The policy IP address
     * @param   policyHubName The policy hub name
     * @param   policyPort The policy port
     * @param   brokerVersion The broker version      
     * @param   connectionLimit The connection limit
     * @param   topicRoutingEnabled Whether topic-based routing is enabled
     * @return  true if the broker is added to the registry.  Otherwise, return false.
    */
    bool addBroker( const std::string &brokerId, const std::string &hostname = registry::DEFAULTHOSTNAME,
        uint32_t port = DEFAULTPORT, uint32_t ttl = registry::DEFAULTTTL, uint32_t startTime = 0,
        uint32_t webSocketPort = registry::DEFAULTWEBSOCKETPORT,
        const std::string& policyHostname ="", const std::string& policyIpAddress ="",
        const std::string& policyHubName ="", uint32_t policyPort = DEFAULTPORT,
        const std::string& brokerVersion = "",
        uint32_t connectionLimit = DEFAULTCONNLIMIT,
        bool topicRoutingEnabled = false );

    /**
     * Removes a broker from the registry
     * 
     * @param   brokerId The broker identifier
     * @return  True if the broker and all it's connections were removed from the registry.
     */
    bool removeBroker( const std::string &brokerId );

    /**
     * Updates the specified broker's TTL
     *
     * @param   brokerId The broker identifier
     * @param   ttl The time to live.
     * @return  true if the broker's time to live was updated.
     */
    bool updateTtl( const std::string &brokerId, uint32_t ttl );

    /**
     * Return true if the broker is in the registry.
     *
     * @param   brokerId The broker identifier
     * @return  True if the broker is in the registry.
     */
    bool exists( const std::string &brokerId ) const;

    /**
     * Copies information for the specified broker into the broker parameter
     *
     * @param   brokerId The broker identifier
     * @param   broker The broker to copy the information into
     * @return  True if the copy was successful
     */
    bool getBroker( const std::string &brokerId, Broker& broker ) const;

    /**
     * Copies information for the specified broker's state into the state parameter
     *
     * @param   brokerId The broker identifier
     * @param   state The state to copy the information into
     * @return  True if the copy was successful
     */
    bool getBrokerState( const std::string &brokerId, BrokerState& state ) const;

    /**
     * Returns a pointer to the broker state. This method exists simply to minimize the
     * overhead of copying broker state and should be used with caution.
     *
     * @param   brokerId The broker identifier
     * @return  Pointer to the broker state or null if it doesn't exist
     */
    const BrokerState* getBrokerStatePtr( const std::string &brokerId ) const;

    /**
     * Returns the state for all brokers
     *
     * @return  The state for all brokers
     */
    std::vector<BrokerState> getAllBrokerStates() const;

    /**
     * Returns the state for all brokers as pointers. This method exists simply to minimize the
     * overhead of copying broker state and should be used with caution.
     *
     * @return  The state for all brokers as pointers
     */
    std::vector<const BrokerState*> getAllBrokerStatePtrs() const;

    /**
     * Updates the broker's connections with the specified connection identifier
     *
     * @param   brokerId The broker identifier
     * @param   connectionId The connection identifier
     * @param   isChild Whether it is a child connection
     * @return  True if the connection was added successfully
     */
    bool addConnection( const std::string &brokerId, const std::string &connectionId, bool isChild );

    /**
     * Removes the specified connection identifier from the broker's connections
     *
     * @param   brokerId The broker identifier
     * @param   connectionId The connection identifier
     * @return  True if the connection was removed successfully
     */
    bool removeConnection( const std::string &brokerId, const std::string &connectionId );

    /**
     * Returns true if the specified connection exists
     *
     * @param   brokerId The broker identifier
     * @param   connectionId The connection identifier
     */
    bool hasConnection( const std::string &brokerId, const std::string &connectionId ) const;

    /**
     * Sets the connections for the specified broker
     *
     * @param   brokerId The broker identifier
     * @param   connectionIds The connection identifiers
     * @param   childConnectionIds The identifiers that are child connections
     * @return  Whether setting connections succeeded
     */
    bool setConnections(const std::string &brokerId, 
        const registry::connection_t &connectionIds, const registry::childConnections_t &childConnectionIds );

    /**
     * Updates the broker registration time (used to determine if it has expired via TTL)
     *
     * @param   brokerId The broker to update the registration time for
     * @return  Whether the registration time was updated
     */
    bool updateRegistrationTime( const std::string &brokerId );

    /**
     * Returns the next broker in the path (the one after the "from") starting at the specified "from" 
     * location and walking to the specified "to" location.
     *
     * @param   from The start broker
     * @param   to The end broker
     * @return  The broker after the start broker to reach the end broker. Empty string is returned if
     *          no path is found.
     */
    std::string getNextBroker( const std::string &from, const std::string &to ) const;                    

    /**
     * Returns the count of topics for the broker
     *
     * @param   brokerId The broker identifier
     * @return  The topic count
     */                
    uint32_t getTopicCount( const std::string &brokerId ) const;

    /**
     * Returns whether the topic exists for the specified broker
     *
     * @param   brokerId The broker identifier
     * @param   topic The broker topic
     * @return  Whether the topic exists for the specified broker
     */
    bool hasTopic( const std::string &brokerId, const std::string& topic ) const;

    /**
     * Returns true if the specified broker has all of the specified topics
     *
     * @param   brokerId The broker identifier
     * @param   topics The topics to find
     * @return  True if all the topics are found
     */                
    bool hasTopics( const std::string &brokerId, const registry::subscriptions_t& topics ) const;

    /**
     * Returns whether the topic was added to the specified broker state
     *
     * @param   brokerId The broker identifier
     * @param   topic The topic
     * @return  True if the topic was added
     */                
    bool addTopic( const std::string &brokerId, const std::string &topic );

    /**
     * Returns whether the topic was removed from the specified broker state
     *
     * @param   brokerId The broker identifier
     * @param   topic The topic
     * @return  True if the topic was removed
     */                
    bool removeTopic( const std::string &brokerId, const std::string &topic );

    /**
     * Returns the current set of topics in batches
     *
     * @param   brokerId The broker identifier
     * @param   charCount The count of characters (across multiple topics that make up a batch).
     * @param   callback The callback to invoke
     * @return  True if broker was found
     */
    bool batchTopics(const std::string& brokerId, const int charCount, const 
        BrokerState::TopicsCallback& callback) const;

    /**
     * Returns the count of topics that have a wildcard
     *
     * @param   brokerId The broker identifier
     * @return  The count of topics that have a wildcard
     */
    uint32_t getTopicWildcardCount(const std::string &brokerId) const;

    /**
     * Clears the topics for the broker that are pending. This set of topics will be
     * swapped when swapPendingTopics() is invoked.
     *
     * @param   brokerId The broker identifier
     * @return  True if broker was found
     */                
    bool clearPendingTopics( const std::string& brokerId );

    /**
     * Adds the specified topics to the set of pending topics for the broker
     *
     * @param   brokerId The broker identifier
     * @param   subs The topics to add
     * @param   wildcardCount The wildcard count for the specified subscriptions
     * @return  True if broker was found
     */                
    bool addPendingTopics( 
        const std::string& brokerId, 
        const registry::subscriptions_t& subs, 
        uint32_t wildcardCount );

    /**
     * Swaps the pending topics with the current set of topics for the broker
     *
     * @param   brokerId The broker identifier
     * @return  True if broker was found
     */                
    bool swapPendingTopics( const std::string& brokerId );

    /**
     * Sets the change count related to topics
     *
     * @param   brokerId The broker identifier
     * @param   changeCount The change count related to topics
     * @return  Whether the count was able to be set
     */
    bool setTopicsChangeCount( const std::string& brokerId, uint32_t changeCount );

    /**
     * Returns true if a subscriber exists for the topic via the specified connection 
     * from the given broker. This includes recursing the hierarchy and taking into
     * consideration wildcards. This will also return true if any of the brokers in the
     * hierarchy being evaluated do not support topic-based routing.
     *
     * @param   brokerId The broker from which to determine if there is a subscriber
     * @param   connection The connection from the broker to search hierarchically for a
     *          subscriber
     * @param   topic The topic
     * @return  True if a subscriber exists for the topic via the specified connection
     *          from the given broker. This includes recursing the hierarchy and taking into
     *          consideration wildcards.
     */
    bool isSubscriberInHierarchy( 
        const std::string &brokerId, const std::string &connection, const std::string &topic ) const;                    

    /**
     * Sets properties regarding the local broker (as received via policy)
     *
     * @param   hostName The broker host name
     * @param   ipAddress The broker IP address
     * @param   hub The broker hub name
     * @param   port The broker port
     * @param   webSocketPort The broker WebSocket port
     */
    void setLocalBrokerProperties( const std::string &hostName, const std::string& ipAddress,
        const std::string &hub, uint32_t port, uint32_t webSocketPort );

    /**
     * Sets the hostname of the managing ePO for the local broker
     *
     * @param   managingEpoName The hostname of the managing ePO server
     */
    void setLocalManagingEpoName( const std::string &managingEpoName );

    /**
     * Returns the local managing ePO name
     *
     * @return  The local managing ePO name
     */
    std::string getLocalManagingEpoName();

    /**
     * Returns the local broker host name 
     *
     * @return  The local broker host name
     */
    std::string getLocalBrokerHostname();

    /**
     * Returns the local broker IP address
     *
     * @return  The local broker IP address
     */
    std::string getLocalBrokerIpAddress();

    /**
     * Returns the local broker hub
     *
     * @return  The local broker hub
     */
    std::string getLocalBrokerHub();

    /**
     * Returns the local broker version
     *
     * @return  The local broker version
     */
    std::string getLocalBrokerVersion();

    /**
     * Returns the local broker port
     *
     * @return  The local broker port
     */
    uint32_t getLocalBrokerPort();

    /**
     * Returns the local broker WebSocket port
     *
     * @return  The local broker WebSocket port
     */
    uint32_t getLocalBrokerWebSocketPort();

    /**
     * Sets the local broker connection limit
     *
     * @param   limit The local broker connection limit
     */
    void setLocalBrokerConnectionLimit( int limit );

    /**
     * Returns the local broker connection limit
     *
     * @return  The local broker connection limit
     */
    uint32_t getLocalBrokerConnectionLimit();

    /**
     * Returns whether topic-based routing is enabled for the local broker
     *
     * @return  Whether topic-based routing is enabled for the local broker
     */
    bool isLocalBrokerTopicRoutingEnabled() const;

    /**
     * Performs a depth first traversal of the broker hierarchy
     *
     * @param   start The broker GUID to start at
     * @param   visitor The visitor that is invoked as the brokers are visited
     */
    void depthFirstTraversal( const std::string &start, FabricVisitor& visitor ) const;

    /**
     * Invoked when core maintenance occurs
     *
     * @param   time The time of the maintenance
     */
    void onCoreMaintenance( time_t time );

    /** Printing to output stream */
    friend std::ostream & operator <<( std::ostream &out, const BrokerRegistry &brokerRegistry );

private:
    /** Constructor */
    BrokerRegistry();

    /**
     * Returns true if a subscriber exists for the topic on the specified broker.
     *
     * @param   broker The broker from which to determine if there is a subscriber
     * @param   topic The topic
     * @return  True if a subscriber exists for the topic on the specified broker.
     */
    bool isSubscriberInBroker( const std::string &brokerId, const std::string &topic ) const;

    /**
     * Depth first traversal to determine if a route exists between the specified brokers
     * 
     * @param   start The "start" broker guid
     * @param   finish The "finish" broker guid
     * @param   visited The brokers that have been visited
     * @return  Information about the traversal
     */
    registry::traversal_t findPathToBroker(
        const std::string &start, const std::string &finish, registry::visit_t &visited ) const;

    /**
     * Worker method for depth first traversal of the broker hierarchy
     *
     * @param   start The broker GUID to start at
     * @param   visited The brokers that have been visited
     * @param   visitor The visitor that is invoked as the brokers are visited
     * @return  True if the recursion should continue
     */
    bool _depthFirstTraversal( 
        const std::string &start, registry::visit_t &visited, FabricVisitor& vistor ) const;

    /**
     * Returns the topic cache service
     *
     * @return  The topic cache service
     */
    dxl::broker::topiccache::TopicCacheService& getTopicCacheService() const
        { return m_topicCacheService; }

    /**
     * Clears all of the caches associated with the registry (routing and topic-based).
     */
    void clearAllCaches();

    /** The broker registry */
    registry::registry_t m_registry;
    /** The last TTL check time */
    time_t m_ttlCheckTime;
    /** Broker registry cache */
    mutable Cache m_cache;
    /** The topic cache */
    mutable dxl::broker::topiccache::TopicCacheService m_topicCacheService;

    //
    // State for the local broker
    //

    /** The local broker host name (as reported in policy) */
    std::string m_localBrokerHostName;
    /** The local broker IP address (as reported in policy) */
    std::string m_localBrokerIpAddress;
    /** The local broker hub (as reported in policy) */
    std::string m_localBrokerHub;
    /** The local broker port (as reported in policy) */
    uint32_t m_localBrokerPort;
    /** The local broker WebSocket port */
    uint32_t m_localBrokerWebSocketPort;
    /** The local broker version (as reported in BrokerHelpers) */
    std::string m_localBrokerVersion;
    /** The local managing ePO (as reported in policy) */
    std::string m_localManagingEpoName;
    /** The local broker connection limit */
    uint32_t m_localBrokerConnectionLimit;

    /** Local broker state mutex */
    std::mutex m_localBrokerPropsMutex;
};

} /* namespace broker */ 
} /* namespace dxl */ 

#endif
