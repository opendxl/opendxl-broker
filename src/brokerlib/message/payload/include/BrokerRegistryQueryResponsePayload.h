/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERREGISTRYQUERYRESPONSEPAYLOAD_H_
#define BROKERREGISTRYQUERYRESPONSEPAYLOAD_H_

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
class BrokerRegistryQueryResponsePayload : 
    public dxl::broker::json::JsonWriter
{
public:
    /** 
     * Constructor
     */
    explicit BrokerRegistryQueryResponsePayload( 
        const std::vector<const BrokerState*> states ) : m_states( states ) {}

    /** Destructor */
    virtual ~BrokerRegistryQueryResponsePayload() {}

    /** {@inheritDoc} */
    void write( Json::Value& out ) const;    

private:
    /** The broker states */
    const std::vector<const BrokerState*> m_states;    
};

} /* namespace payload */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* BROKERREGISTRYQUERYRESPONSEPAYLOAD_H_ */
