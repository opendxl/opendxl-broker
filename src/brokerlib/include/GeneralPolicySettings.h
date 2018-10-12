/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef GENERALPOLICYSETTINGS_H_
#define GENERALPOLICYSETTINGS_H_

#include "include/Configuration.h"
#include "include/unordered_set.h"
#include <cstdint>
#include <mutex>

namespace dxl {
namespace broker {

/**
 * Settings used by the local broker
 */
class GeneralPolicySettings
{    
protected:
    /**
     * Simple derivation of configuration so we can re-use property get/set
     * and writing to a file
     */
    class ConfigSettings : public Configuration
    {
    public: 
        ConfigSettings() {};
        virtual void setProperty( const std::string& propertyName, const std::string& value );
        virtual ~ConfigSettings() {};
    };

public:
    /**
     * Sets the keep alive interval
     *
     * @param   intervalStr The keep alive interval (as a string)
     */
    static void setKeepAliveInterval( const std::string intervalStr );

    /**
     * Returns the keep alive interval
     *
     * @return  The keep alive interval
     */
    static uint32_t getKeepAliveInterval();

    /**
     * Sets the broker connection limit
     *
     * @param   limit The broker connection limit
     */
    static void setConnectionLimit( const std::string limit );

    /**
     * Returns the broker connection limit
     *
     * @return  The broker connection limit
     */
    static uint32_t getConnectionLimit();

    /**
     * Loads the initial settings from disk
     */
    static void loadSettings();

    /**
     * This method should be invoked after a set of properties have been set.
     * For example, after setting values via policy, etc. This allows for these
     * values to be written to disk, appropriate actions to be taken based on 
     * the changes, set.
     *
     * @param   writeSettings Whether the settings should be written to disk
     */
    static void editingCompleted( bool writeSettings = true );

private:
    /**
     * Sets a property value
     *
     * @param   name The name of the property
     * @param   value The value of the property
     */
    static void setProperty( const std::string name, const std::string value );

    /**
     * Returns the u32 value for the specified property name
     *
     * @param   name The property name
     * @param   defaultValue The default value
     */
    static uint32_t getIntegerProperty( const std::string name, uint32_t defaultValue );

    /**
     * Returns the string value for the specified property name
     *
     * @param   name The property name
     * @param   defaultValue The default value
     */
    static std::string getStringProperty( const std::string name, const std::string defaultValue );

    /**
     * Invoked so that property changes can be handled. This will be invoked after
     * editing of properties has completed.
     * 
     * @param   propNames The names of the properties that have changed
     */
    static void handlePropertyChanges( const unordered_set<std::string>& propNames );

    /** The configuration settings */
    static ConfigSettings sm_settings;

    /** Properties that have changed since the last edit complete event */
    static unordered_set<std::string> sm_changedProperties;

    /** The general properties mutex */
    static std::recursive_mutex m_mutex;
};

} /* namespace broker */
} /* namespace dxl */

#endif /* GENERALPOLICYSETTINGS_H_ */
