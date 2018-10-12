/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERTOPICEVENTPAYLOAD_H_
#define BROKERTOPICEVENTPAYLOAD_H_

#include <string>

#include "message/payload/include/AbstractBrokerTopicEventPayload.h"

namespace dxl {
namespace broker {
namespace message {
namespace payload {

/**
 * Payload containing a broker topic (that has been added/removed)
 */
class BrokerTopicEventPayload : public AbstractBrokerTopicEventPayload
{
public:
    /** 
     * Constructor
     *
     * @param   topic The topic that has been added/removed
     */
    explicit BrokerTopicEventPayload( const std::string& topic = "" ) :
        m_topic( topic ) {}

    /**
     * Returns the topic
     *
     * @return  The topic
     */
    std::string getTopic() const { return m_topic; }

    /** Destructor */
    virtual ~BrokerTopicEventPayload() {}

    /** {@inheritDoc} */
    void read( const Json::Value& in );

    /** {@inheritDoc} */
    void write( Json::Value& out ) const;

private:
    /** The topic that has been added/removed */
    std::string m_topic;
};

} /* namespace payload */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* BROKERTOPICEVENTPAYLOAD_H_ */
