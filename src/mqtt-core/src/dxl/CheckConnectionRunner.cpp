/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "CheckConnectionRunner.h"
#include <thread>
#include <chrono>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include "logging_mosq.h"
#include "net_mosq.h"

using std::thread;

namespace dxl {
namespace broker {
namespace core {

/** {@inheritDoc} */
CheckConnectionRunner::CheckConnectionRunner() : m_exiting( false )
{
}

/** {@inheritDoc} */
CheckConnectionRunner::~CheckConnectionRunner() 
{
    if( m_runner.joinable() )
    {
        {
            std::unique_lock<std::mutex> lck( m_cvmutex );
            m_exiting = true;
            m_cv.notify_all();
        }

        m_runner.join();
    }
}

/** {@inheritDoc} */
void CheckConnectionRunner::run() 
{
    while( true ) 
    {
        CheckConnection::address addr;
        {
            // Wait until we have work or are exiting
            std::unique_lock<std::mutex> lck( m_cvmutex );
            if( !hasWork() && !m_exiting )
            {            
                m_cv.wait( lck );
            }

            if( m_exiting )
            {
                return;
            }

            addr = getWork();
        }
        
        CheckConnection::result ccres;

        if( IS_INFO_ENABLED )
            _mosquitto_log_printf( NULL, MOSQ_LOG_INFO,
                "Checking connection to bridge: %s:%d", addr.host.c_str(), addr.port );
        int bridge_sock = INVALID_SOCKET;

        ccres.res = _mosquitto_try_connect( addr.host.c_str(), addr.port, &bridge_sock, NULL, true );
        ccres.host = addr.host;
        ccres.port = addr.port;

        if( ccres.res == MOSQ_ERR_EAI )
        {
            ccres.EAIresult = errno;
            _mosquitto_log_printf( NULL, MOSQ_LOG_ERR, 
                "Error checking connection to bridge: %s", gai_strerror( ccres.EAIresult ) );
        }
        else if( ccres.res != MOSQ_ERR_SUCCESS ) 
        {
            _mosquitto_log_printf( NULL, MOSQ_LOG_ERR,
                "Error checking connection to bridge: %d", ccres.res );
        }

        if( bridge_sock != INVALID_SOCKET ) 
        {
            COMPAT_CLOSE( bridge_sock );
        }

        if( IS_INFO_ENABLED )
            _mosquitto_log_printf(
                NULL, MOSQ_LOG_INFO,
                "Checking connection to bridge: %s finished. Result = %d", addr.host.c_str(), ccres.res );

        addResult( ccres );
        // Mark that the work has been done
        workDone();
    }
}

/** {@inheritDoc} */
void CheckConnectionRunner::add( const char* host, uint16_t port ) 
{
    CheckConnection::address addr;
    addr.host = host;
    addr.port = port;
    add( addr );
}

/** {@inheritDoc} */
void CheckConnectionRunner::add( const CheckConnection::address& addr )
{
    start();

    std::lock_guard<std::mutex> grd( m_inqueue_mutex );
    m_inqueue.push( addr );
    m_cv.notify_all();
}

/** {@inheritDoc} */
bool CheckConnectionRunner::hasWork() 
{
    std::lock_guard<std::mutex> grd( m_inqueue_mutex );
    return !m_inqueue.empty();
}

/** {@inheritDoc} */
CheckConnection::address CheckConnectionRunner::getWork()
{
    std::lock_guard<std::mutex> grd( m_inqueue_mutex );
    CheckConnection::address addr = m_inqueue.front();

    return addr;
}

/** {@inheritDoc} */
void CheckConnectionRunner::workDone()
{
    std::lock_guard<std::mutex> grd( m_inqueue_mutex );
    m_inqueue.pop();
}

/** {@inheritDoc} */
void CheckConnectionRunner::addResult( const CheckConnection::result ccres ) 
{
    std::lock_guard<std::mutex> grd( m_outqueue_mutex );
    m_outqueue.push( ccres );
}

/** {@inheritDoc} */
bool CheckConnectionRunner::hasResult() 
{
    std::lock_guard<std::mutex> grd( m_outqueue_mutex );
    return !m_outqueue.empty();
}

/** {@inheritDoc} */
bool CheckConnectionRunner::getResult( CheckConnection::result& result ) 
{
    std::lock_guard<std::mutex> grd( m_outqueue_mutex );

    if( m_outqueue.empty() )
    {
        if( IS_DEBUG_ENABLED )
            _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG, "Result queue is empty" );
        return false;
    }
    result = m_outqueue.front();
    m_outqueue.pop();

    if( IS_DEBUG_ENABLED )
        _mosquitto_log_printf( NULL, MOSQ_LOG_DEBUG, "Getting result to bridge: %s", result.host.c_str() );

    return true;
}

/** {@inheritDoc} */
void CheckConnectionRunner::start() 
{
    if( m_runner.joinable() )
    {
        return;
    }

    m_runner = thread( &CheckConnectionRunner::run, this );
}


/** {@inheritDoc} */
CheckConnection::WorkStatus CheckConnectionRunner::getStatus() 
{
    if( hasResult() ) 
    {
        return CheckConnection::WorkStatus::results_available;
    }
    else if( hasWork() ) 
    {
        return CheckConnection::WorkStatus::working;
    }

    return CheckConnection::WorkStatus::nothing;
}

} /* namespace core */
} /* namespace broker */
} /* namespace dxl */
