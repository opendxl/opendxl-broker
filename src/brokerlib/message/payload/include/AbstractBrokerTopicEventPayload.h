/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef ABSTRACTBROKERTOPICEVENTPAYLOAD_H_
#define ABSTRACTBROKERTOPICEVENTPAYLOAD_H_

#include <cstdint>
#include "json/include/JsonReader.h"
#include "json/include/JsonWriter.h"

#include "brokerregistry/include/brokerstate.h"
#include "message/include/DxlMessage.h"

namespace dxl {
namespace broker {
namespace message {
namespace payload {

/**
 * Base payload for broker-based topic events
 */
class AbstractBrokerTopicEventPayload : 
    public dxl::broker::json::JsonReader,
    public dxl::broker::json::JsonWriter
{
public:
    /** 
     * Constructor
     */
    explicit AbstractBrokerTopicEventPayload() {}
    
    /**
     * Sets the header values for the specified message 
     * (broker start time and subscriptions change count)
     *
     * @param   message The message
     * @param   brokerState The broker state from which to read the values
     */
    static void setMessageHeaderValues( DxlMessage& message, const BrokerState* brokerState );

    /**
     * Returns the header values from the specified message
     *
     * @param   message The DXL message
     * @param   brokerStartTime The broker star time (out)
     * @param   subsChangeCount The subscriptions change count (out)
     */
    static void getMessageHeaderValues( 
        DxlMessage& message, uint32_t* brokerStartTime, uint32_t* subsChangeCount );

    /** Destructor */
    virtual ~AbstractBrokerTopicEventPayload() {}
};

} /* namespace payload */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* ABSTRACTBROKERTOPICEVENTPAYLOAD_H_ */
