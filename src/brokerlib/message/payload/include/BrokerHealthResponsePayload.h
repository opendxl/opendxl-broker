/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERHEALTHRESPONSEPAYLOAD_H_
#define BROKERHEALTHRESPONSEPAYLOAD_H_

#include "json/include/JsonWriter.h"
#include "core/include/CoreBrokerHealth.h"

namespace dxl {
namespace broker {
namespace message {
namespace payload {

/**
 * Payload for a BrokerHealth response message
 */
class BrokerHealthResponsePayload : 
    public dxl::broker::json::JsonWriter
{
public:
    /** 
     * Constructor
     *
     * @param   brokerHealth The broker health information
     */
    explicit BrokerHealthResponsePayload(
        const dxl::broker::core::CoreBrokerHealth& brokerHealth ) :
        m_brokerHealth(brokerHealth)
    {}

    /** Destructor */
    virtual ~BrokerHealthResponsePayload()
    {}

    /** {@inheritDoc} */
    void write( Json::Value& out ) const;

private:
    /** The broker health information */
    dxl::broker::core::CoreBrokerHealth m_brokerHealth;
};

} /* namespace payload */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* BROKERHEALTHRESPONSEPAYLOAD_H_ */
