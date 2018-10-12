/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/BrokerSettings.h"
#include "include/Configuration.h"
#include "include/SimpleLog.h"
#include "json/include/JsonService.h"
#include <sstream>
#include <stdexcept>
#include <boost/format.hpp>

using namespace std;
using namespace Json;
using namespace dxl::broker;
using namespace dxl::broker::json;

/** {@inheritDoc} */
JsonService& JsonService::getInstance()
{
    static JsonService instance;
    return instance;
}

/** {@inheritDoc} */
string JsonService::toJson( const JsonWriter& writer, bool log ) const
{
    Value jsonObject( Json::objectValue );
    writer.write( jsonObject );
    string out = 
        BrokerSettings::isJsonPrettyPrintEnabled() ?
            StyledWriter().write( jsonObject ) :
            FastWriter().write( jsonObject );

    if( log && SL_LOG.isDebugEnabled() ) 
    {
        SL_START << "JsonService::toJson: " << out << SL_DEBUG_END;
    }

    return out;
}

/** {@inheritDoc} */
void JsonService::fromJson( const std::string& jsonString, JsonReader& reader ) const
{
    if( SL_LOG.isDebugEnabled() ) 
    {
        SL_START << "JsonService::fromJson: " << jsonString << SL_DEBUG_END;
    }

    Value jsonObject;
    Reader jsonReader;
    if( !jsonReader.parse( jsonString, jsonObject ) )
    {
        stringstream errMsg;
        errMsg << "Error parsing JSON: " << jsonReader.getFormattedErrorMessages();
        throw runtime_error( errMsg.str() );                
    }
    reader.read( jsonObject );
}


/** {@inheritDoc} */
string JsonService::parseString( const Value& in, const char* propName, bool required )
{
    string value = in[ propName ].asString();
    if( required && value.empty() )
    {
        throw runtime_error(
            ( boost::format(
                "Property '%1%' is required." ) % propName ).str() );
    }
    return value;
}
