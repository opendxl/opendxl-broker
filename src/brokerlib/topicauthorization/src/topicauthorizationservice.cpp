/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/SimpleLog.h"
#include "topicauthorization/include/topicauthorizationservice.h"
#include "core/include/CoreUtil.h"

using dxl::broker::common::MutexLock;
using namespace dxl::broker::core;
using namespace std;

namespace dxl {
namespace broker {

/** {@inheritDoc} */
TopicAuthorizationService::TopicAuthorizationService() {}

/** {@inheritDoc} */
TopicAuthorizationService::~TopicAuthorizationService() {}

/** {@inheritDoc} */
const shared_ptr<TopicAuthorizationService> TopicAuthorizationService::Instance()
{
    static shared_ptr<TopicAuthorizationService> service( new TopicAuthorizationService() );
    return service;
}

/** {@inheritDoc} */
void TopicAuthorizationService::setTopicState( shared_ptr<TopicAuthorizationState> newState )
{
    MutexLock lock( &m_threadingMutex );
    if ( !m_state || ( *newState != *m_state ) )
    {
        m_state = newState;
    }
}

/** {@inheritDoc} */
bool TopicAuthorizationService::_isAuthorizedToSubscribe( shared_ptr<const TopicAuthorizationState>& state,
    const std::string& key, const std::string& topic )
{
    bool hit;
    bool authorized = state->isAuthorizedToSubscribe( key, topic, &hit );

    //
    // Wildcard authorization support
    //
    if( state->isWildcardingEnabled() )
    {
        if( hit && authorized )
        {
            return true;
        }

        char* wcTopic = CoreUtil::iterateWildcardBegin( topic.c_str() );
        while( CoreUtil::iterateWildcardNext( wcTopic ) )
        {
            if( state->isAuthorizedToSubscribe( key, wcTopic, &hit ) )
            {
                if( hit )
                {
                    authorized = true;
                    break;
                }
            }
            else
            {
                authorized = false;
            }
        }
        CoreUtil::iterateWildcardEnd( wcTopic );
    }

    return authorized;
}

/** {@inheritDoc} */
bool TopicAuthorizationService::_isAuthorizedToPublish( shared_ptr<const TopicAuthorizationState>& state,
    const std::string& key, const std::string& topic )
{
    bool hit;
    bool authorized = state->isAuthorizedToPublish( key, topic, &hit );

    //
    // Wildcard authorization support
    //    
    if( state->isWildcardingEnabled() )
    {
        if( hit && authorized )
        {
            return true;
        }

        char* wcTopic = CoreUtil::iterateWildcardBegin( topic.c_str() );
        while( CoreUtil::iterateWildcardNext( wcTopic ) )
        {
            if( state->isAuthorizedToPublish( key, wcTopic, &hit ) )
            {
                if( hit )
                {
                    authorized = true;
                    break;
                }
            }
            else
            {
                authorized = false;
            }
        }
        CoreUtil::iterateWildcardEnd( wcTopic );
    }

    return authorized;
}

/** {@inheritDoc} */
bool TopicAuthorizationService::isAuthorizedToPublish( const std::string& clientId, const std::string& topic )
{
    auto state = getTopicAuthorizationState();
    return _isAuthorizedToPublish( state, clientId, topic );
}

/** {@inheritDoc} */
bool TopicAuthorizationService::isAuthorizedToSubscribe( const std::string& clientId, const std::string& topic )
{
    auto state = getTopicAuthorizationState();
    return _isAuthorizedToSubscribe( state, clientId, topic );
}

/** {@inheritDoc} */
bool TopicAuthorizationService::isAuthorizedToPublish( struct cert_hashes* certHashes, const std::string& topic )
{
    auto state = getTopicAuthorizationState();    
    if( certHashes )
    {
        // Iterate the certificate hashes, if we are authorized, return true (short circuit)
        struct cert_hashes *current, *tmp;
        HASH_ITER( hh, certHashes, current, tmp ) 
        {
            if( _isAuthorizedToPublish( state, current->cert_sha1, topic ) )
            {
                return true;
            }
        }
        return false;
    }

    // Handles case where there are no certs (TLS is disabled?)
    return _isAuthorizedToPublish( state, "", topic );
}

/** {@inheritDoc} */
bool TopicAuthorizationService::isAuthorizedToSubscribe( struct cert_hashes* certHashes, const std::string& topic )
{
    auto state = getTopicAuthorizationState();    
    if( certHashes )
    {
        // Iterate the certificate hashes, if we are authorized, return true (short circuit)
        struct cert_hashes *current, *tmp;
        HASH_ITER( hh, certHashes, current, tmp ) 
        {
            if( _isAuthorizedToSubscribe( state, current->cert_sha1, topic ) )
            {
                return true;
            }
        }
        return false;
    }

    // Handles case where there are no certs (TLS is disabled?)
    return _isAuthorizedToSubscribe( state, "", topic );
}

/** {@inheritDoc} */
const shared_ptr<const TopicAuthorizationState> TopicAuthorizationService::getTopicAuthorizationState()
{
    MutexLock lock(&m_threadingMutex);
    return m_state;
}

}
}

