/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/include/DxlMessageConstants.h"
#include "message/payload/include/BrokerTopicQueryRequestPayload.h"

using namespace std;
using namespace dxl::broker;
using namespace dxl::broker::message;
using namespace dxl::broker::message::payload;
using namespace Json;

/** {@inheritDoc} */
void BrokerTopicQueryRequestPayload::read( const Json::Value& in )
{
    m_brokerGuid = in[ DxlMessageConstants::PROP_BROKER_GUID ].asString();

    Json::Value queryTopicsJson = in[ DxlMessageConstants::PROP_TOPICS ];
    unordered_set<string> queryTopics;
    for( Value::iterator itr = queryTopicsJson.begin(); itr != queryTopicsJson.end(); itr++ )
    {
        queryTopics.insert( (*itr).asString() );
    }
    m_queryTopics = queryTopics;
}

