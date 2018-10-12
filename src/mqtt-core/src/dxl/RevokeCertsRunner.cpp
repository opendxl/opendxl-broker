/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "RevokeCertsRunner.h"
#include "logging_mosq.h"
#include "mosquitto_broker.h"
#include "DxlFlags.h"

using namespace dxl::broker::core;

/** {@inheritDoc} */
void RevokeCertsRunner::run()
{
    if( IS_DEBUG_ENABLED )
        _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG, "RevokeCertsRunner::run" );    

    if( m_revokedCerts )
    {        
        // Mosquitto DB
        struct mosquitto_db* db = _mosquitto_get_db();
        struct cert_hashes *current, *tmp;

        // Walk the contexts
        for( int i = 0; i < db->context_count; i++ )
        {
            struct mosquitto* context = db->contexts[i];
            if( context &&
                context->cert_hashes )
            {
                struct cert_hashes* s = NULL;                
                HASH_ITER( hh, context->cert_hashes, current, tmp )
                {
                    HASH_FIND_STR( m_revokedCerts, current->cert_sha1, s );
                    if( s )
                    {
                        // Force disconnect
                        if( IS_DEBUG_ENABLED )
                            _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG, "Force disconnect: %s", current->cert_sha1 );                                    
                        mqtt3_context_disconnect( db, db->contexts[i] );                        
                    }
                }                
            }
        }

        // Free revoked certs
        HASH_ITER( hh, m_revokedCerts, current, tmp ) {
            HASH_DEL( m_revokedCerts, current );
            free( (void*)current->cert_sha1 );
            free( current );
        }
    }
}
