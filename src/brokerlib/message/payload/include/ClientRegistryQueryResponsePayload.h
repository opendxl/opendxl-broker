/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef CLIENTREGISTRYQUERYRESPONSEPAYLOAD_H_
#define CLIENTREGISTRYQUERYRESPONSEPAYLOAD_H_

#include "json/include/JsonWriter.h"

namespace dxl {
namespace broker {
namespace message {
namespace payload {

/**
 * Payload for a "ClientRegistry : query" response message
 */
class ClientRegistryQueryResponsePayload : 
    public dxl::broker::json::JsonWriter
{
public:
    /** 
     * Constructor
     */
    explicit ClientRegistryQueryResponsePayload() {}

    /** Destructor */
    virtual ~ClientRegistryQueryResponsePayload() {}

    /** {@inheritDoc} */
    void write( Json::Value& out ) const;    
};

} /* namespace payload */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* CLIENTREGISTRYQUERYRESPONSEPAYLOAD_H_ */
