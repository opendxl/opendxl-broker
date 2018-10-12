/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERREGISTRYQUERYREQUESTPAYLOAD_H_
#define BROKERREGISTRYQUERYREQUESTPAYLOAD_H_

#include <string>
#include "json/include/JsonReader.h"

namespace dxl {
namespace broker {
namespace message {
namespace payload {

/**
 * Payload for a "BrokerRegistry : query" request message
 */
class BrokerRegistryQueryRequestPayload : 
    public dxl::broker::json::JsonReader
{
public:
    /** 
     * Constructor
     */
    explicit BrokerRegistryQueryRequestPayload() {}

    /** Destructor */
    virtual ~BrokerRegistryQueryRequestPayload() {}

    /** {@inheritDoc} */
    void read( const Json::Value& in );    

    /**
     * Returns the broker GUID
     *
     * @return  The service GUID
     */
    std::string getBrokerGuid() const { return m_brokerGuid; }

private:
    /** The broker guid */
    std::string m_brokerGuid;
};

} /* namespace payload */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* BROKERREGISTRYQUERYREQUESTPAYLOAD_H_ */
