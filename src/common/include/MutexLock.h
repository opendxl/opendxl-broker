/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef MUTEXLOCK_H_
#define MUTEXLOCK_H_

#include <mutex>

namespace dxl {
namespace broker {
/** Namespace for common components (between broker library and core) */
namespace common {

/**
 * Resource Acquisition Iis Initialization (RAII) pattern for locking and unlocking the specified
 * mutex.
 */
class MutexLock
{    
public:
    /** 
     * Locks the specified mutex
     *
     * @param   mutex The mutex to lock during construction
     */
    MutexLock( std::mutex* mutex ) 
    {  
        m_mutex = mutex;
        m_mutex->lock();
    }

    /**
     * Destructor, unlocks the mutex
     */
    virtual ~MutexLock()
    {
        m_mutex->unlock();
    }

private:
    /** The mutex */
    std::mutex* m_mutex;
};

} /* namespace common */
} /* namespace broker */
} /* namespace dxl */

#endif /* MUTEXLOCK_H_ */
