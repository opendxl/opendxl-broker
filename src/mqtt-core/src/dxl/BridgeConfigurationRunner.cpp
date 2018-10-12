/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "BridgeConfigurationRunner.h"
#include "logging_mosq.h"
#include "memory_mosq.h"

#include <iostream>

using namespace std;
using namespace dxl::broker::core;

// External reference
extern void clean_session( struct mosquitto_db *db, uint32_t ctx_idx );

/** {@inheritDoc} */
void BridgeConfigurationRunner::clearCurrentBridges( struct mosquitto_db *db )
{
    for( int i = 0; i < db->context_count; i++ )
    {
        if(db->contexts[i] && db->contexts[i]->bridge )
        {
            // Force the session to be cleaned
            db->contexts[i]->clean_session = true;

            // Disconnect the bridge
            mqtt3_context_disconnect( db, db->contexts[i] );
            clean_session( db, i );
        }        
    }

    // Clear the current bridges from the configuration
    mqtt3_config_clear_bridges( db->config );
}

/** {@inheritDoc} */
void BridgeConfigurationRunner::run()
{
    if( IS_DEBUG_ENABLED )
        _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG, "BridgeConfigurationRunner::run" );    

    struct mosquitto_db *db = _mosquitto_get_db();

    // Clear current bridges
    clearCurrentBridges( db );

    if( m_config.isEnabled() )
    {
        const vector<CoreBridgeConfiguration::Broker> brokers =
            m_config.getBrokers();
        auto brokerCount = brokers.size();

        // Allocate Mosquitto addresses
        struct bridge_address* addresses = (bridge_address*)
            _mosquitto_calloc( brokerCount, sizeof(struct bridge_address) );

        struct _mqtt3_bridge* bridge;

        for( vector<CoreBridgeConfiguration::Broker>::size_type i = 0; i < brokerCount; i++ )
        {
            const CoreBridgeConfiguration::Broker& broker = brokers.at( i );

            // In this particular case the address we are passing will be duped during the mqtt3_config_add_bridge()
            // method that is invoked. The address structure is simply being used to pass the address
            // information which will be copied.
            addresses[i].address = _mosquitto_strdup( broker.getHost().c_str() );
            addresses[i].port = broker.getPort();
        }            

        int rc = mqtt3_config_add_bridge( 
            db->config,
            m_config.getBridgeId().c_str(),
            addresses, 
            (int)brokerCount,
            &bridge );

        // Free the duplicated addresses
        for( vector<CoreBridgeConfiguration::Broker>::size_type i = 0; i < brokerCount; i++ )
        {
            _mosquitto_free( addresses[i].address );
        }

        // Free Mosquitto addresses
        _mosquitto_free( addresses );

        if( rc != MOSQ_ERR_SUCCESS )
        {
            _mosquitto_log_printf( NULL, MOSQ_LOG_ERR, "Error during mqtt3_config_add_bridge." );    
        }

        // Set round robin mode
        bridge->round_robin = m_config.isRoundRobin();

        // Set the initial broker index
        bridge->cur_address = m_config.getInitialBrokerIndex();

        // Set the primary address count
        bridge->primary_address_count = m_config.getPrimaryBrokerCount();

        // Start the bridge
        mqtt3_bridge_new( db, bridge );

        if( IS_DEBUG_ENABLED )
            _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG, 
                "Bridging enabled: %s, roundRobin: %d", bridge->name, bridge->round_robin );
    }        
    else
    {
        if( IS_DEBUG_ENABLED )
            _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG, "Bridging is disabled." );
    }
}
