/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef EVENTSUBSCRIBERNOTFOUNDEVENTPAYLOAD_H_
#define EVENTSUBSCRIBERNOTFOUNDEVENTPAYLOAD_H_

#include <string>
#include "json/include/JsonReader.h"
#include "json/include/JsonWriter.h"

namespace dxl {
namespace broker {
namespace message {
namespace payload {

/**
 * Payload containing the topic that a subscriber was not found for
 */
class EventSubscriberNotFoundEventPayload : 
    public dxl::broker::json::JsonReader,
    public dxl::broker::json::JsonWriter
{
public:
    /** 
     * Constructor
     *
     * @param    topic The topic that was not found
     */
    explicit EventSubscriberNotFoundEventPayload( const std::string& topic = "" ) :
        m_topic( topic ) {}
    
    /**
     * Returns the topic
     *
     * @return  The topic
     */
    std::string getTopic() const { return m_topic; }

    /** Destructor */
    virtual ~EventSubscriberNotFoundEventPayload() {}

    /** {@inheritDoc} */
    void read( const Json::Value& in );

    /** {@inheritDoc} */
    void write( Json::Value& out ) const;

private:
    /** The topic that was not found */
    std::string m_topic;
};

} /* namespace payload */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* EVENTSUBSCRIBERNOTFOUNDEVENTPAYLOAD_H_ */
