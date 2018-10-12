/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "MqttWorkQueue.h"
#include "MutexLock.h"
#include "logging_mosq.h"

using namespace std;
using namespace dxl::broker::core;
using namespace dxl::broker::common;

/** {@inheritDoc} */
MqttWorkQueue& MqttWorkQueue::getInstance()
{
    // Singleton
    static MqttWorkQueue queue;
    return queue;
}

/** {@inheritDoc} */
void MqttWorkQueue::add( const shared_ptr<MqttWorkQueue::Runnable> runnable )
{
    // Lock mutex
    MutexLock lock( &m_queueMutex );

    if( IS_DEBUG_ENABLED )
        _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG, "MqttWorkQueue::add" );

    // Add specified runnable
    m_queue.push( runnable );
}

/** {@inheritDoc} */
void MqttWorkQueue::runQueue()
{
    shared_ptr<MqttWorkQueue::Runnable> ptr;
    while( true )
    {
        {
            // Lock mutex
            MutexLock lock( &m_queueMutex );
            if( !m_queue.empty() )
            {
                ptr = m_queue.front();
                m_queue.pop();                
            }
            else
            {
                // Queue is empty, return
                return;
            }
        }

        if( ptr.get() )
        {
            // Execute the runner
            ptr->run();
        }
    }
}
