/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef JSONWRITER_H_
#define JSONWRITER_H_

#include "json/json.h"

namespace dxl {
namespace broker {
namespace json {

/**
 * Implemented by objects capable of writing their state to a JSON representation
 */
class JsonWriter
{    
public:
    /** Destructor */
    virtual ~JsonWriter() {}

    /**
     * Implemented by objects capable of writing their state to a JSON representation. 
     *
     * @param   out The value object to write the JSON representation to.
     */
    virtual void write( Json::Value& out ) const = 0;    
};

} /* namespace json */
} /* namespace broker */
} /* namespace dxl */

#endif /* JSONWRITER_H_ */
