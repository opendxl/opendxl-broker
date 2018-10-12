/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERTOPICQUERYRESPONSEPAYLOAD_H_
#define BROKERTOPICQUERYRESPONSEPAYLOAD_H_

#include <cstdint>
#include <list>
#include "json/include/JsonWriter.h"
#include "brokerregistry/include/brokerstate.h"

namespace dxl {
namespace broker {
namespace message {
namespace payload {

/**
 * Payload for a "BrokerRegistry : query" response message
 */
class BrokerTopicQueryResponsePayload : 
    public dxl::broker::json::JsonWriter
{
public:
    /** 
     * Constructor
     *
     * @param   topicCount The topic count
     * @param   topicsExist Whether the topics exist
     */
    explicit BrokerTopicQueryResponsePayload( uint32_t topicCount = 0,
        bool topicsExist = false ) : m_topicCount( topicCount ), m_topicsExist( topicsExist ) {}

    /** Destructor */
    virtual ~BrokerTopicQueryResponsePayload() {}

    /** {@inheritDoc} */
    void write( Json::Value& out ) const;    

private:
    /** The broker topic count */
    uint32_t m_topicCount;
    
    /** The indicator for if broker has the request topics */
    bool m_topicsExist;
};

} /* namespace payload */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* BROKERTOPICQUERYRESPONSEPAYLOAD_H_ */
