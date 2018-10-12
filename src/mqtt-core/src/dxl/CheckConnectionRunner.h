/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef CHECKCONNECTIONRUNNER_H_
#define CHECKCONNECTIONRUNNER_H_

#include <thread>
#include <queue>
#include <mutex>
#include <string>
#include <condition_variable>

namespace dxl {
namespace broker {
namespace core {

/** Namespace for declarations used to return results of checking a network connection */
namespace CheckConnection 
{
    /**
     * Structure containing an address to check the connection for.
     */
    struct address
    {
        /** The host */
        std::string host;
        /** The port */
        uint16_t port;
    };

    /**
     * The result of checking a connection
     */
    struct result 
    {
        /** The host */
        std::string host;
        /** The port */
        uint16_t port;
        /** The result of the check */
        int res;
        /** Errno information */
        int EAIresult;
    };

    /**
     * The current status of the check
     */
    enum WorkStatus 
    {
        nothing = 0,
        working = 1,
        results_available = 2
    };
};

/**
 * Class used to asynchronously check whether a port on a particular
 * host is working and available
 */
class CheckConnectionRunner 
{
public:
    /** Constructor */
    CheckConnectionRunner();
    /** Destructor */
    virtual ~CheckConnectionRunner();
    /** Adds the host and port to check */
    void add( const char* host, uint16_t port );
    /** Returns the current status of the check operation */
    CheckConnection::WorkStatus getStatus();
    /** Returns the result of the check operation */
    bool getResult( CheckConnection::result& result );

protected:

    /** The run method (main worker method) */
    void run();
    /** Starts the runner */
    void start();

    /**
     * Adds an address to check
     *
     * @param   td The address to check
     */
    void add( const CheckConnection::address& td );

    /**
     * Returns whether any work is available
     *
     * @return  Whether any work is available
     */
    bool hasWork();

    /**
     * Returns work to be processed
     *
     * @return   The address to check
     */
    CheckConnection::address getWork();

    /* Invoked to indicate that work has completed */
    void workDone();

    /**
     * Returns whether a result is available
     *
     * @return  Whether a result is available
     */
    bool hasResult();

    /**
     * Adds the specified result
     *
     * @param   ccres The result to add
     */
    void addResult( const CheckConnection::result ccres );

    /** The thread associated with the runner */
    std::thread m_runner;
    /** Mutex associated with the incoming queue of checks */
    std::mutex m_inqueue_mutex;
    /** The queue of checks to perform */
    std::queue<dxl::broker::core::CheckConnection::address> m_inqueue;
    /** Mutex associated with results */
    std::mutex m_outqueue_mutex;
    /** The queue containing check results */
    std::queue<CheckConnection::result> m_outqueue;
    /** Used to notify when runner is exiting */
    std::condition_variable m_cv;
    /** Mutex when runner is being initialized or destroyed */
    std::mutex m_cvmutex;
    /** Whether the runner is exiting */
    bool m_exiting;
};

} /* namespace core */
} /* namespace broker */
} /* namespace dxl */

#endif /* CHECKCONNECTIONRUNNER_H_ */
