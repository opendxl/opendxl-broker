/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERHEALTHREQUESTPAYLOAD_H_
#define BROKERHEALTHREQUESTPAYLOAD_H_

#include "json/include/JsonReader.h"

namespace dxl {
namespace broker {
namespace message {
namespace payload {

/**
 * Payload for a Broker Health request message
 */
class BrokerHealthRequestPayload : 
    public dxl::broker::json::JsonReader
{
public:
    /** 
     * Constructor
     */
    explicit BrokerHealthRequestPayload() {}

    /** Destructor */
    virtual ~BrokerHealthRequestPayload() {}

    /** {@inheritDoc} */
    void read( const Json::Value& in );
};

} /* namespace payload */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* BROKERHEALTHREQUESTPAYLOAD_H_ */
