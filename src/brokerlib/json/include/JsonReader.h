/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef JSONREADER_H_
#define JSONREADER_H_

#include "json/json.h"

namespace dxl {
namespace broker {
namespace json {

/**
 * Implemented by objects capable of reading their state from a JSON representation
 */
class JsonReader
{    
public:
    /** Destructor */
    virtual ~JsonReader() {}

    /**
     * Implemented by objects capable of reading their state from a JSON representation. 
     *
     * @param   in The value object containing the JSON representation of the object.
     */
    virtual void read( const Json::Value& in ) = 0;        
};

} /* namespace json */
} /* namespace broker */
} /* namespace dxl */

#endif /* JSONREADER_H_ */
