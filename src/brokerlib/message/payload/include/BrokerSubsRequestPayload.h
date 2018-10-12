/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERSUBSREQUESTPAYLOAD_H_
#define BROKERSUBSREQUESTPAYLOAD_H_

#include "json/include/JsonReader.h"

namespace dxl {
namespace broker {
namespace message {
namespace payload {

/**
 * Payload for a Broker Subscriptions request message
 */
class BrokerSubsRequestPayload : 
    public dxl::broker::json::JsonReader
{
public:
    /** 
     * Constructor
     */
    explicit BrokerSubsRequestPayload() {}

    /** Destructor */
    virtual ~BrokerSubsRequestPayload() {}

    /** {@inheritDoc} */
    void read( const Json::Value& in );

    /**
     * Returns the subscription topic
     *
     * @return  The subscription topic
     */
    std::string getTopic() const { return m_topic; }

private:
    /** The subscription topic*/
    std::string m_topic;
};

} /* namespace payload */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* BROKERSUBSREQUESTPAYLOAD_H_ */
