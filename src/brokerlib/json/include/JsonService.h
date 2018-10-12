/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef JSONSERVICE_H_
#define JSONSERVICE_H_

#include "json/include/JsonReader.h" 
#include "json/include/JsonWriter.h" 
#include <string>

namespace dxl {
namespace broker {
/** Namespace for JSON-related declarations */
namespace json {

/**
 * Service that facilitates objects reading and writing JSON representations.
 */
class JsonService
{    
public:
    /** Destructor */
    virtual ~JsonService() {}

    /**
     * Returns the single service instance
     *
     * @return  The single service instance
     */
    static JsonService& getInstance();

    /**
     * Returns the JSON representation of the specified object 
     *
     * @param   writer The object to output the JSON representation of
     * @param   log Whether to log the output (if debug is enabled)
     * @return  The JSON representation of the specified object
     */
    std::string toJson( const JsonWriter& writer, bool log = true ) const;

    /**
     * Populates the specified object with the specified JSON representation
     *
     * @param   jsonString the JSON representation
     * @param   reader The object to populate
     */
    void fromJson( const std::string& jsonString, JsonReader& reader ) const;

    /**
     * Parses a string property value from the specified JSON value
     *
     * @param   in The JSON value
     * @param   propName The property name
     * @param   required Whether the property is required
     * @return  The parsed string
     */
    static std::string parseString(
        const Json::Value& in, const char* propName, bool required = false );

private:
    /** Constructor */
    JsonService() {};
};

} /* namespace json */
} /* namespace broker */
} /* namespace dxl */

#endif /* JSONSERVICE_H_ */
