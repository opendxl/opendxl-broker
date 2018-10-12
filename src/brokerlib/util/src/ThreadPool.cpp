/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "util/include/ThreadPool.h"
#include "include/SimpleLog.h"
#include <sstream>

using namespace std;

namespace dxl {
namespace broker {
namespace util {

/** Constructor */
ThreadPool::ThreadPool(unsigned int numThreads) 
    : m_numThreads( numThreads )
{
    init();
}

/** Destructor */
ThreadPool::~ThreadPool() 
{
    shutdown();
}

/** {@inheritDoc} */
void ThreadPool::run() 
{
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "Starting thread pool runner." << SL_DEBUG_END;
    }

    while( true ) 
    {
        try
        {
            shared_ptr<Runnable> ptr = getWork();
            if( ptr.get() )
            {
                ptr->run();
            }
            else
            {
                /** A null runnable means to end the thread */
                break;
            }
        }
        catch( ... )
        {
            SL_START << "Catching unknown exception" << SL_DEBUG_END;
        }
    }
    
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "Ending thread pool runner." << SL_DEBUG_END;
    }    
}

/** {@inheritDoc} */
bool ThreadPool::addWork( std::shared_ptr<Runnable> runnable )
{
    return addWork( false, runnable );
}

/** {@inheritDoc} */
bool ThreadPool::addWork( bool allowNull, std::shared_ptr<Runnable> runnable )
{
    if ( !allowNull && !runnable.get() )
    {
        return false;
    }

    lock_guard<mutex> lock( m_mutex );

    m_queue.push( runnable );

    m_event.notify_all();

    return true;
}

/** {@inheritDoc} */
std::shared_ptr<ThreadPool::Runnable> ThreadPool::getWork()
{
    unique_lock<mutex> lock( m_mutex );

    while( m_queue.empty() )
    {
        m_event.wait( lock );
    }

    shared_ptr<Runnable> ptr = m_queue.front();
    
    m_queue.pop();
    
    return ptr;
}

/** {@inheritDoc} */
void ThreadPool::init()
{
    for( unsigned int i = 0; i < m_numThreads; ++i )
    {
        m_threadPool.push_back( thread( &ThreadPool::run, this ) );
    }
}

/** {@inheritDoc} */
void ThreadPool::shutdown()
{
    for( unsigned int i = 0; i < m_numThreads; ++i )
    {
        addWork( true, shared_ptr<Runnable>() );
    }

    for( unsigned int i = 0; i < m_numThreads; ++i )
    {
        if( m_threadPool[i].joinable() )
        {
            m_threadPool[i].join();
        }
    }
}

} /* namespace util */
} /* namespace broker */
} /* namespace dxl */
