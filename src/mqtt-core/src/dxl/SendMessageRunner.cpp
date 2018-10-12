/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "SendMessageRunner.h"
#include "mosquitto_broker.h"
#include "logging_mosq.h"
#include "memory_mosq.h"

using namespace std;
using namespace dxl::broker::core;

/** {@inheritDoc} */
SendMessageRunner::SendMessageRunner( const char* topic, uint32_t payloadLen, const void* payload ) : 
    m_topic( NULL ), m_payloadLen( 0 ), m_payload( NULL )
{
    if( IS_DEBUG_ENABLED )
        _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG, "SendMessageRunner::SendMessageRunner(): %s", topic );    
        
    if( topic != NULL )
    {
        m_topic = strdup( topic );
        if( !m_topic )
        {            
            // Unable to allocate memory
            throw bad_alloc();            
        }
    }

    if( payloadLen > 0 && payload )
    {
        m_payloadLen = payloadLen;

        m_payload = _mosquitto_malloc( sizeof(char) * payloadLen );
        if( m_payload )
        {
            memcpy( m_payload, payload, sizeof(char) * payloadLen );
        }
        else
        {
            if( m_topic )
            {
                _mosquitto_free( m_topic );
            }

            // Unable to allocate memory
            throw bad_alloc();
        }
    }
};

/** {@inheritDoc} */
SendMessageRunner::~SendMessageRunner()
{
    if( IS_DEBUG_ENABLED )
        _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG, "SendMessageRunner::~SendMessageRunner()" );    

    if( m_topic )
    {
        _mosquitto_free( m_topic );
    }

    if( m_payload )
    {
        _mosquitto_free( m_payload );
    }
}
    
/** {@inheritDoc} */
void SendMessageRunner::run()
{
    int result = 
        mqtt3_db_messages_easy_queue(
            _mosquitto_get_db(), 
            NULL, /* context id */
            m_topic, 
            0, /* qos */
            m_payloadLen,
            m_payload, 
            0 /* Retain */
        );

    if( result != MOSQ_ERR_SUCCESS )
    {
        _mosquitto_log_printf( NULL, MOSQ_LOG_ERR, 
            "SendMessageRunner: error attempting to queue message for topic: %s", 
            m_topic != NULL ? m_topic : "(null)" );    
    }
}
