/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERLIBTHREADPOOL_H_
#define BROKERLIBTHREADPOOL_H_

#include "util/include/ThreadPool.h"
#include <memory>

namespace dxl {
namespace broker {
namespace util {

/**
 * Thread pool singleton used by the broker library
 */
class BrokerLibThreadPool
{
public:
    /** Returns the singleton instance */
    static BrokerLibThreadPool& getInstance();

    /** 
      * Adds a runnable to be executed by a thread
      *
      * @param  runnable The runnable to be executed by a thread
     */
    bool addWork( std::shared_ptr<ThreadPool::Runnable> runnable );

    /** Stops all threads and waits for them to end */    
    void shutdown();

    /** Destructor */
    ~BrokerLibThreadPool();

protected:
    /** Constructor */
    BrokerLibThreadPool();

    /** The thread pool */
    ThreadPool m_threadPool;
};

} /* namespace util */
} /* namespace broker */
} /* namespace dxl */

#endif /* BROKERLIBTHREADPOOL_H_ */
