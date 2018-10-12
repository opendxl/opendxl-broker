/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef _BROKER_STATE_H_
#define _BROKER_STATE_H_

#include "include/unordered_map.h"
#include "include/unordered_set.h"
#include <ctime>
#include <string>
#include <functional>
#include "broker.h"

namespace dxl {
namespace broker {

namespace registry 
{
    /** The type used to store broker connections. */
    typedef unordered_set<std::string> connection_t;

    /** The type used to store child connections. */
    typedef unordered_set<std::string> childConnections_t;

    /** Type used to store counted connections */
    typedef unordered_map<std::string,uint32_t> countedConnections_t;

    /** Type used to store subscriptions (topics) */
    typedef unordered_set<std::string> subscriptions_t;
}

/**
 * State information related to a particular broker
 */
class BrokerState
{
    /** Allow registry to access private members */
    friend class BrokerRegistry;

public:
    /**
     * Callback invoked for each batch of topics
     */
    class TopicsCallback
    {
    public:
        /**
         * Callback for each batch
         *
         * @param   brokerState The broker state that is invoking the callback
         * @param   topics The topics in the batch
         * @param   index The index of the batch (0-based)
         * @param   isFirst If it is the first batch
         * @param   isLast If it is the last batch
         */
        virtual void handleBatch( 
            const BrokerState* brokerState,
            const registry::subscriptions_t& topics, 
            int32_t index,
            bool isFirst, 
            bool isLast ) const = 0;
    };

public:
    /**
     * Constructor for broker state
     *
     * @param   broker The broker associated with the state
     * @param   connections The broker's connections
     * @param   childConnections The broker's child connections
     */
    explicit BrokerState(
        const Broker& broker = Broker(), 
        const registry::connection_t& connections = unordered_set<std::string>(),
        const registry::childConnections_t& childConnections = unordered_set<std::string>()
    );

    /**
     * Add a connection to the broker's connection list 
     *
     * @param   connectionId Broker id of the connection to be added.
     * @param   isChild Whether the broker is a child in the connection (connected to parent)
     */
    void addConnection( const std::string &connectionId, bool isChild );

    /**
     * Remove a connection to the broker's connection list 
     *
     * @param   connectionId Broker id of the connection to be removed.
     */
    void removeConnection( const std::string &connectionId );

    /**
     * Returns true if the specified connection exists
     *
     * @param   connectionId Broker id of the connection
     */
    bool hasConnection( const std::string &connectionId ) const;

    /**
     * Set the connections in the connection list
     * 
     * @param   connectionIds The connection identifiers
     * @param   childConnectionIds The child connection identifiers
     * @return  Whether the connections were updated (they were different)
     */
    bool setConnections(
        const registry::connection_t &connectionIds,
        const registry::childConnections_t &childConnectionIds );

    /**
     * Returns a copy of the connections in the connection list
     *
     * @return  The copy of the connections in the connection list
     */
    registry::connection_t getConnections() const;

    /**
     * Invokes the callback function for each connection identifier
     * 
     * @param   fn callback function to invoke
     */
    bool forEachConnection( std::function<bool( const std::string& )> fn ) const;

    /**
     * Returns a copy of the child connections in the connection list
     *
     * @return  A copy of the child connections in the connection list
     */                
    registry::childConnections_t getChildConnections() const;

    /**
     * Returns the count of topics for the broker
     * 
     * @return  The topic count
     */                
    uint32_t getTopicCount() const { return (uint32_t)m_subscriptions.size(); }

    /**
     * Invokes the callback function for each topic
     * 
     * @param   fn callback function to invoke
     */
    bool forEachTopic( std::function<bool( const std::string& )> fn ) const;

    /**
     * Returns true if all topics are in the subscription list
     *
     * @return  True if all topics are in the subscription list
     */                
    bool hasTopics( const registry::subscriptions_t& topics ) const;

    /** 
     * Add a topic to the broker's subscription list
     *
     * @param   topic The topic that is subscribed to
     * @return  True if the topic was added
     */
    bool addTopic( const std::string &topic );

    /** Removes a topic to the broker's subscription list
     *
     * @param   topic The topic that is no longer subscribed to
     * @return  True if the topic was removed
     */
    bool removeTopic( const std::string &topic );

    /**
     * Returns the change count related to topics
     *
     * @return  The change count related to topics
     */
    uint32_t getTopicsChangeCount() const { return m_subscriptionsChangeCount; }

    /**
     * Sets the change count related to topics
     *
     * @param   changeCount The change count related to topics
     */
    void setTopicsChangeCount( uint32_t changeCount ) { m_subscriptionsChangeCount = changeCount; }

    /**
     * Returns whether the topic exists for the broker
     *
     * @param   topic The broker topic
     * @return  Whether the topic exists for the broker
     */
    bool hasTopic( const std::string& topic ) const { return m_subscriptions.find( topic ) != m_subscriptions.end(); }

    /**
     * Returns the count of topics that have a wildcard
     *
     * @return  The count of topics that have a wildcard
     */
    uint32_t getTopicWildcardCount() const { return m_subscriptionsWildcardCount; }

    /**     
     * Returns the current set of topics in batches
     *
     * @param   charCount The count of characters (across multiple topics that make up a
     *          batch).
     * @param   callback The callback to invoke
     */
    void batchTopics( const int charCount, const TopicsCallback& callback ) const;

    /**
     * Clears the topics for the broker that are pending. This set of topics will be
     * swapped when swapPendingTopics() is invoked.
     */                
    void clearPendingTopics();

    /**
     * Adds the specified topics to the set of pending topics for the broker
     *
     * @param   subs The topics to add
     * @param   wildcardCount The wildcard count for the specified subscriptions
     */                
    void addPendingTopics( const registry::subscriptions_t& subs, uint32_t wildcardCount );

    /**
     * Swaps the pending topics with the current set of topics for the broker
     */                
    void swapPendingTopics();

    /** 
     * Sets the time to live value
     *
     * @param   ttl The new time to live value.
     */                    
    inline void updateTtl( uint32_t ttl ) { return m_broker.updateTtl( ttl ); }

    /**
     * Whether the broker state object has expired (based on TTL)
     *
     * @return  Whether the broker state object has expired (based on TTL)
     */
    bool isExpired() const;

    /**
     * Returns the broker information associated with the state
     *
     * @return  The broker information associated with the state
     */
    inline Broker getBroker() const { return m_broker; }

    /**
     * Returns the broker start time
     *
     * @return  The broker start time
     */
    uint32_t getBrokerStartTime() const { return m_broker.getStartTime(); }

    /**
     * Returns whether topic routing is enabled
     *
     * @return  Whether topic routing is enabled
     */    
    bool isTopicRoutingEnabled() const { return m_broker.isTopicRoutingEnabled(); }

    /**
     * Updates the broker registration time (used to determine if it has expired via TTL)
     */
    void updateRegistrationTime() { time( &m_regTime ); }

    /** operator== */
    inline bool operator==( const BrokerState &rhs ) const { 
        return ( ( m_broker == rhs.m_broker ) && ( getConnections() == rhs.getConnections() ) ); }
    /** operator!= */
    inline bool operator!=( const BrokerState &rhs ) const { return !( *this == rhs ); }
    /** Print */
    friend std::ostream & operator <<( std::ostream &out, const BrokerState &brokerState );

private:

    /**
     * Returns the broker. This method should not be exposed publicly as it returns
     * a reference.
     *
     * @return  The broker
     */
    Broker& getBrokerInternal() { return m_broker; }

    /** The broker information associated with the state */
    Broker m_broker;
    /** Connections to the broker (with counts) */
    registry::countedConnections_t m_countedConnections;
    /** Child connections to the broker */
    registry::childConnections_t m_childConnections;
    /** Current topics subscribed to on the broker */
    registry::subscriptions_t m_subscriptions;
    /** The count of topics containing a wildcard */
    uint32_t m_subscriptionsWildcardCount;
    /** Subscriptions that are pending (currently being received via broker state messages) */
    registry::subscriptions_t m_pendingSubscriptions;
    /** The count of pending subscriptions that contain wildcards */
    uint32_t m_pendingSubscriptionsWildcardCount;
    /** The subscriptions change count */
    uint32_t m_subscriptionsChangeCount;
    /** The time the broker state was registered */
    time_t m_regTime;
};

} /* namespace broker */ 
} /* namespace dxl */ 

#endif
