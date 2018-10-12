/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERSTATETOPICSEVENTPAYLOAD_H_
#define BROKERSTATETOPICSEVENTPAYLOAD_H_

#include <cstdint>
#include <string>
#include "include/unordered_set.h"

#include "message/payload/include/AbstractBrokerTopicEventPayload.h"

namespace dxl {
namespace broker {
namespace message {
namespace payload {

/**
 * Payload containing the topics associated with a broker state
 */
class BrokerStateTopicsEventPayload : public AbstractBrokerTopicEventPayload
{
public:

    // States for the transfer of topics
    static const uint8_t STATE_NONE     = 0;
    static const uint8_t STATE_START    = 1 << 0;
    static const uint8_t STATE_END      = 1 << 1;

    /** Topic type def */
    typedef unordered_set<std::string> topics_t;

    /** 
     * Constructor
     *
     * @param   state The state of the topic transfer
     * @param   topics The topics associated with the payload
     * @param   index The index in the batch (0-based)
     */
    explicit BrokerStateTopicsEventPayload(
        uint8_t state = STATE_NONE, 
        topics_t topics = topics_t(),
        int32_t index = 0 ) :
        m_state( state ),
        m_topics( topics ),
        m_readWildcardCount( 0 ),
        m_index( index ) {}
    
    /** Destructor */
    virtual ~BrokerStateTopicsEventPayload() {}

    /**
     * Returns the state of the topics 
     *
     * @return  The state of the topics
     */
    uint8_t getState() const { return m_state; }

    /**
     * Returns the topics 
     *
     * @return  The topics
     */
    topics_t getTopics() const { return m_topics; }

    /**
     * The count of wildcard topics that were read (JsonReader)
     *
     * @return  The count of wildcard topics that were read (JsonReader)
     */
    uint32_t getReadWildcardCount() const { return m_readWildcardCount; }

    /**
     * Returns the index in the batch (0-based)
     *
     * @return  The index in the batch (0-based)
     */
    int32_t getBatchIndex() const { return m_index; }

    /** {@inheritDoc} */
    void read( const Json::Value& in );

    /** {@inheritDoc} */
    void write( Json::Value& out ) const;

    /** Equals operator */
    bool operator==( const BrokerStateTopicsEventPayload& rhs ) const;

    /** Not equals operator */
    bool operator!= (const BrokerStateTopicsEventPayload& rhs ) const {
        return !( *this == rhs );
    }

private:
    /** State of the batch transfer */
    uint8_t m_state;

    /** The topics */
    topics_t m_topics;

    /** The count of wildcards that were read (JsonReader) */
    uint32_t m_readWildcardCount;

    /** The index in the batch (0-based) */
    int32_t m_index;
};

} /* namespace payload */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* BROKERSTATETOPICSEVENTPAYLOAD_H_ */
