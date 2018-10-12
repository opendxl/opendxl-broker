/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include "include/unordered_map.h"
#include "include/SimpleLog.h"
#include <string>
#include <vector>
#include <stdexcept>
#include "StringUtil.h"

namespace dxl {
namespace broker {

/**
 * Class used to store a set of configuration settings
 */
class Configuration
{
public:
    /**
     * Returns the singleton
     *
     * @return  The singleton
     */
    static Configuration& Instance()
    {
        static Configuration singleton;
        return singleton;
    }

    /** A map containing named string values */
    typedef unordered_map<std::string, std::string> PropertyMap;

    /**
     * Reads configuration settings from the specified file
     *
     * @param   inFile The file name
     * @param   withDefaults Whether to check for a ".defaults" file
     */
    bool readConfiguration( const std::string& inFile, const bool withDefaults = false )
    {
        m_propValues.clear();

        std::vector<std::string> files;
        if( withDefaults )
        {
            files.push_back( inFile + ".defaults" );
        }
        files.push_back( inFile );

        for( auto filesItr = files.begin(); filesItr != files.end(); ++filesItr )
        {
            // Open the file
            std::ifstream infile;
            infile.open( filesItr->c_str() );
            if( infile.is_open() )
            {
                while ( !infile.eof() )
                {
                    std::string curLine( "eof" );
                    getline( infile, curLine );

                    // This is a hack for aix, where the eof stuff is flaky
                    if( curLine == "eof" )
                        break;

                    parseLine(curLine);
                }
            }
            else
            {
                  if( withDefaults )
                  {
                      if( filesItr == files.begin() )
                      {
                          std::cerr << "Error opening default configuration file: " << *filesItr << std::endl;
                      }
                  }
                  else
                  {
                        std::cerr << "Error opening configuration file: " << *filesItr << std::endl;
                  }
            }
        }

        return m_propValues.size() ? true : false;
    }

    /**
     * Returns the value associates with the specified property name
     *
     * @param   propName The property name
     * @param   value The value for the property (out)
     * @param   defaultValue The default value for the property
     */
    bool getProperty( const std::string& propName, std::string& value,
            const std::string& defaultValue = "" ) const
    {
        bool foundIt = false;
        PropertyMap::const_iterator i = m_propValues.find( propName );
        if( i != m_propValues.end() )
        {
            foundIt = true;
            value = i->second;
        }
        else
        {
            value = defaultValue;
        }

        return foundIt;
    }

    /**
     * Sets the value for the specified property name
     *
     * @param   propertyName The property name
     * @param   value The property value
     */
    virtual void setProperty( const std::string& propertyName, const std::string& value )
    {
        m_propValues[propertyName] = value;
    }

    /**
     * Writes the current settings to the specified file
     *
     * @param   outFile The file to write the settings to
     */
    void writeConfiguration( const std::string& outFile )
    {
        std::ofstream outfile;
        outfile.open( outFile.c_str(), std::ofstream::trunc );
        if( outfile.is_open() )
        {
            for( PropertyMap::iterator it = m_propValues.begin(); it != m_propValues.end(); it++ )
            {
                outfile << it->first << "=" << it->second << "\n";
            }
        }
        outfile.close();
    }

protected:
    /** Constructor */
    Configuration() {}

    /**
     * Parses the specified line for name/value pair
     *
     * @param   line The line to parse
     */
    void parseLine( const std::string& line )
    {
        std::size_t nameEnd = line.find( "=" );
        if( nameEnd == std::string::npos || nameEnd <= 0 )
        {
            // not a key/value pair, skip it
            return;
        }

        // Find value
        size_t valueStart = nameEnd + 1;
        if( valueStart >= line.size() )
            return;

        std::string name = line.substr( 0, nameEnd );
        std::string value = line.substr( valueStart );
        setProperty( name, value );
    }

    /** Map containing the property value */
    PropertyMap m_propValues;
};

}  // namespace broker
}  // namespace dxl

#endif  // CONFIGURATION_H_
