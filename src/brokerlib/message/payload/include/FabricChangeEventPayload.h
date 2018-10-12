/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef FABRICCHANGEEVENTPAYLOAD_H_
#define FABRICCHANGEEVENTPAYLOAD_H_

#include "json/include/JsonReader.h"
#include "json/include/JsonWriter.h"

namespace dxl {
namespace broker {
namespace message {
namespace payload {

/**
 * Payload for a "FabricChangeEvent" message
 */
class FabricChangeEventPayload : 
    public dxl::broker::json::JsonReader,
    public dxl::broker::json::JsonWriter
{
public:
    /** Constructor */
    FabricChangeEventPayload() {}

    /** Destructor */
    virtual ~FabricChangeEventPayload() {}

    /** {@inheritDoc} */
    void read( const Json::Value& in );

    /** {@inheritDoc} */
    void write( Json::Value& out ) const;    
};

} /* namespace payload */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* FABRICCHANGEEVENTPAYLOAD_H_ */
