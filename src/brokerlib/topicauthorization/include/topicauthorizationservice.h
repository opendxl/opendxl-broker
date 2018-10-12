/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef TOPICAUTHORIZATIONSERVICE_H_
#define TOPICAUTHORIZATIONSERVICE_H_

#include <memory>
#include <mutex>
#include <MutexLock.h>
#include "topicauthorization/include/topicauthorizationstate.h"
#include "cert_hashes.h"

namespace dxl {
namespace broker {

/**
 * Service that is used to determine if clients are able to publish/subscribe
 */
class TopicAuthorizationService
{
public:
    /** Destructor */
    virtual ~TopicAuthorizationService();

    /**
     * Returns the single service instance
     *
     * @return  The single service instance
     */
    static const std::shared_ptr<TopicAuthorizationService> Instance();

    /**
     * Sets the authorization state
     *
     * @param   newState The new authorization state
     */
    void setTopicState(std::shared_ptr<TopicAuthorizationState> newState);

    /**
     * Returns the current authorization state
     *
     * @return  The current authorization state
     */
    const std::shared_ptr<const TopicAuthorizationState> getTopicAuthorizationState();

    /**
     * Returns whether the specified client identifier is authorized to publish to the
     * specified topic.
     *
     * @param   clientId The client identifier
     * @param   topic The topic
     * @return  Whether publish is allowed
     */
    bool isAuthorizedToPublish( const std::string& clientId, const std::string& topic );

    /**
     * Returns whether the specified certificates are authorized to publish to the
     * specified topic.
     *
     * @param   certHashes The certificate hashes
     * @param   topic The topic
     * @return  Whether publish is allowed
     */
    bool isAuthorizedToPublish( struct cert_hashes* certHashes, const std::string& topic );

    /**
     * Returns whether the specified client identifier is authorized to subscribe to the
     * specified topic.
     *
     * @param   clientId The client identifier
     * @param   topic The topic
     * @return  Whether subscribe is allowed
     */
    bool isAuthorizedToSubscribe( const std::string& clientId, const std::string& topic );

    /**
     * Returns whether the specified certificates are authorized to subscribe to the
     * specified topic.
     *
     * @param   certHashes The certificate hashes
     * @param   topic The topic
     * @return  Whether subscribe is allowed
     */
    bool isAuthorizedToSubscribe( struct cert_hashes* certHashes, const std::string& topic );

protected:
    /** Constructor */
    TopicAuthorizationService();

    /**
     * Whether the specified key is allowed to subscribe to the topic for the given state
     *
     * @param   state The authorization state
     * @param   key The key
     * @param   topic The topic
     * @return  Whether subscribe is allowed
     */
    bool _isAuthorizedToSubscribe(
        std::shared_ptr<const TopicAuthorizationState>& state, const std::string& key, const std::string& topic );

    /**
     * Whether the specified key is allowed to publish to the topic for the given state
     *
     * @param   state The authorization state
     * @param   key The key
     * @param   topic The topic
     * @return  Whether publish is allowed
     */
    bool _isAuthorizedToPublish(
        std::shared_ptr<const TopicAuthorizationState>& state, const std::string& key, const std::string& topic );
private:
    /** Authorization state */
    std::shared_ptr<const TopicAuthorizationState> m_state;
    /** Threading mutex */
    std::mutex m_threadingMutex;
};

}
}

#endif // TOPICAUTHORIZATIONSERVICE_H_
