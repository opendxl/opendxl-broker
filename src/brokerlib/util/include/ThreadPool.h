/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <thread>
#include <queue>
#include <condition_variable>
#include <memory>
#include <mutex>

namespace dxl {
namespace broker {
namespace util {

/**
 * Thread pool implementation that allows for the execution of arbitrary
 * "runnable" objects.
 */
class ThreadPool 
{
public:
    /** Forward class reference to the runnable interface */
    class Runnable;

    /** Constructor
     * 
     * @param   numThreads The number of threads (1 by default)
     */
    ThreadPool( unsigned int numThreads = 1 );

    /** Destructor */
    virtual ~ThreadPool();

    /**
     * Adds a runnable to be executed by a thread
     *
     * @param   runnable The runnable to be executed by a thread
     */
    bool addWork( std::shared_ptr<Runnable> runnable );

    /** Stops all threads and waits for them to end */
    void shutdown();

protected:
    /** Entry point and main loop for each thread */
    void run();

    /** Creates all threads and starts them */
    void init();    

    /** 
     * Adds a runnable to the queue
     *
     * @param   allowNull Value for allowing a null runnable to be added to the queue
     * @param   runnable The runnable to be executed by a thread
     */
    bool addWork( bool allowNull, std::shared_ptr<Runnable> runnable );
    
    /** Returns the next runnable in the queue */
    std::shared_ptr<Runnable> getWork();

    /** Vector of threads */
    std::vector<std::thread> m_threadPool;
    
    /** Condition variable to signal the arrival of new work */
    std::condition_variable m_event;
    
    /** Number of Threads */
    unsigned int m_numThreads;
    
     /** The queue of runnables */
    std::queue<std::shared_ptr<Runnable>> m_queue;

     /** The mutex for the queue */
     std::mutex m_mutex;

public:
    /** Interface to be implemented by runnables */
    class Runnable
    {
    public:
        /** Destructor */
        virtual ~Runnable() {}

        /** Executes the runnable */
        virtual void run() = 0;
    };
};

} /* namespace util */
} /* namespace broker */
} /* namespace dxl */

#endif /* THREADPOOL_H_ */
