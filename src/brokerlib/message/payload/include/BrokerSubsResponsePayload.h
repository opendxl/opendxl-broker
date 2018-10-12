/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERSUBSRESPONSEPAYLOAD_H_
#define BROKERSUBSRESPONSEPAYLOAD_H_

#include <cstdint>
#include "json/include/JsonWriter.h"

namespace dxl {
namespace broker {
namespace message {
namespace payload {

/**
 * Payload for a Broker subscriptions response message
 */
class BrokerSubsResponsePayload : 
    public dxl::broker::json::JsonWriter
{
public:
    /** 
     * Constructor
     *
     * @param   subCount The subscription count
     */
    explicit BrokerSubsResponsePayload( uint32_t subCount ) :
        m_subCount( subCount ) {}

    /** Destructor */
    virtual ~BrokerSubsResponsePayload() {}

    /** {@inheritDoc} */
    void write( Json::Value& out ) const;

private:
    /** The broker subscription count */
    uint32_t m_subCount;
};

} /* namespace payload */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* BROKERSUBSRESPONSEPAYLOAD_H_ */
