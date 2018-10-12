/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/include/DxlMessageConstants.h"
#include "message/payload/include/BrokerStateTopicsEventPayload.h"
#include "core/include/CoreUtil.h"

using namespace dxl::broker::message::payload;
using namespace dxl::broker::core;
using namespace Json;

/** {@inheritDoc} */
bool BrokerStateTopicsEventPayload::operator==( const BrokerStateTopicsEventPayload& rhs ) const
{
    return m_state == rhs.m_state && m_topics == rhs.m_topics;
}

/** {@inheritDoc} */
void BrokerStateTopicsEventPayload::write( Json::Value& out ) const
{
    out[ DxlMessageConstants::PROP_STATE ] = m_state;
    out[ DxlMessageConstants::PROP_INDEX ] = m_index;
    Value topics( arrayValue );
    for( auto it = m_topics.begin(); it != m_topics.end(); ++it )
    {                    
        topics.append( *it );
    }
    out[ DxlMessageConstants::PROP_TOPICS ] = topics; 
}

/** {@inheritDoc} */
void BrokerStateTopicsEventPayload::read( const Json::Value& in )
{
    m_readWildcardCount = 0;

    m_state = in[ DxlMessageConstants::PROP_STATE ].asUInt();
    m_index = in[ DxlMessageConstants::PROP_INDEX ].asInt();
    Json::Value topics = in[ DxlMessageConstants::PROP_TOPICS ];
    for( Value::iterator itr = topics.begin(); itr != topics.end(); itr++ )
    {
        const std::string topic = (*itr).asString();
        if( CoreUtil::isWildcard( topic.c_str() ) )
        {
            m_readWildcardCount++;
        }
        m_topics.insert( topic );
    }
}