/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef CLIENTREGISTRYQUERYREQUESTPAYLOAD_H_
#define CLIENTREGISTRYQUERYREQUESTPAYLOAD_H_

#include <string>
#include "json/include/JsonReader.h"

namespace dxl {
namespace broker {
namespace message {
namespace payload {

/**
 * Payload for a "ClientRegistry : query" request message
 */
class ClientRegistryQueryRequestPayload : 
    public dxl::broker::json::JsonReader
{
public:
    /** 
     * Constructor
     */
    explicit ClientRegistryQueryRequestPayload() {}

    /** Destructor */
    virtual ~ClientRegistryQueryRequestPayload() {}

    /** {@inheritDoc} */
    void read( const Json::Value& in );    

    /**
     * Returns the client GUID
     *
     * @return  The client GUID
     */
    std::string getClientGuid() const { return m_clientGuid; }

private:
    /** The client guid */
    std::string m_clientGuid;
};

} /* namespace payload */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* CLIENTREGISTRYQUERYREQUESTPAYLOAD_H_ */
