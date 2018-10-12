/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef TOPICAUTHORIZATIONSTATE_H_
#define TOPICAUTHORIZATIONSTATE_H_

#include <memory>
#include <string>
#include "include/unordered_set.h"
#include "include/unordered_map.h"

namespace dxl {
namespace broker {

/**
 * State used to determine if clients are able to publish and subscribe to topics
 */
class TopicAuthorizationState
{
public:
    // TopicAuthorizationState is a map of topic names to a set of keys
    // If a topic does not exist in the map, anyone can access the topic
    // If a topic exists, a client may use the topic if it's key is in the set
    typedef unordered_map<std::string,unordered_set<std::string>> TopicAuthorizationStateData;

    /**
     * Returns the authorization state as read from the specified JSON file
     *
     * @param   infile The file to read from
     * @return  the authorization state as read from the specified JSON file
     */
    static std::shared_ptr<TopicAuthorizationState> CreateFromJsonFile( const std::string& infile );

    /**
     * Constructor
     *
     * @param   publishers Information on clients that can publish
     * @param   subscribers Information on clients that can subscribe
     */
    explicit TopicAuthorizationState(
        const TopicAuthorizationStateData& publishers, const TopicAuthorizationStateData& subscribers );

    /** Destructor */
    ~TopicAuthorizationState() {};

    /**
     * Returns whether the specified key (client id or certificate) is able to publish to
     * the specified topic.
     *
     * @param   key The key (client id or certificate)
     * @param   topic The topic
     * @param   hit Whether a corresponding entry was found (out)
     * @return  Whether publish is allowed
     */
    bool isAuthorizedToPublish( const std::string& key, const std::string& topic, bool* hit = NULL ) const;

    /**
     * Returns whether the specified key (client id or certificate) is able to subscribe to
     * the specified topic.
     *
     * @param   key The key (client id or certificate)
     * @param   topic The topic
     * @param   hit Whether a corresponding entry was found (out)
     * @return  Whether subscribe is allowed
     */
    bool isAuthorizedToSubscribe( const std::string& key, const std::string& topic, bool* hit = NULL ) const;

    /**
     * Whether topic wildcarding is enabled
     *
     * @return  Whether topic wildcarding is enabled
     */
    bool isWildcardingEnabled() const { return m_isWildcardingEnabled; }

    /** Equality operator */
    bool operator==( const TopicAuthorizationState& rhs ) const;
    /** Inequality operator */
    bool operator!=( const TopicAuthorizationState& rhs ) const;

private:

    /**
     * Returns whether the specified key (client id or certificate) is authorized for
     * the specified topic.
     *
     * @param   key The key (client id or certificate)
     * @param   topic The topic
     * @param   hit Whether a corresponding entry was found (out)
     * @return  Whether the key is authorized
     */
    bool isAuthorized(
        const TopicAuthorizationStateData& state, const std::string& key,
        const std::string& topic, bool* hit = NULL ) const;

    /** Information on clients that can publish */
    TopicAuthorizationStateData m_publishers;
    /** Information on clients that can subscribe */
    TopicAuthorizationStateData m_subscribers;
    /** Whether topic wildcarding is enabled */
    bool m_isWildcardingEnabled;
};

}
}

#endif /* TOPICAUTHORIZATIONSTATE_H_ */
