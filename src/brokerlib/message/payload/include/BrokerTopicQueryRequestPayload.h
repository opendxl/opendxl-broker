/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERTOPICQUERYREQUESTPAYLOAD_H_
#define BROKERTOPICQUERYREQUESTPAYLOAD_H_

#include "include/unordered_set.h"
#include "json/include/JsonReader.h"

namespace dxl {
namespace broker {
namespace message {
namespace payload {

/**
 * Payload for a "Broker Topic : query" request message
 */
class BrokerTopicQueryRequestPayload : 
    public dxl::broker::json::JsonReader
{
public:
    /** 
     * Constructor
     */
    explicit BrokerTopicQueryRequestPayload() {}

    /** Destructor */
    virtual ~BrokerTopicQueryRequestPayload() {}

    /** {@inheritDoc} */
    void read( const Json::Value& in );    

    /**
     * Returns the broker GUID
     *
     * @return  The service GUID
     */
    std::string getBrokerGuid() const { return m_brokerGuid; }

    /**
     * Returns the query topics
     *
     * @return  The query topics
     */
    unordered_set<std::string> getQueryTopics() const { return m_queryTopics; }

private:
    /** The broker guid */
    std::string m_brokerGuid;

    /** The queried topics */
    unordered_set<std::string> m_queryTopics;
};

} /* namespace payload */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /** BROKERTOPICQUERYREQUESTPAYLOAD_H_ */