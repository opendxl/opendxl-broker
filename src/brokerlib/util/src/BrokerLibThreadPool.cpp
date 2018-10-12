/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "util/include/BrokerLibThreadPool.h"
#include "include/BrokerSettings.h"

namespace dxl {
namespace broker {
namespace util {

/** {@inheritDoc} */
BrokerLibThreadPool& BrokerLibThreadPool::getInstance()
{
    static BrokerLibThreadPool instance;
    return instance;
}

/** {@inheritDoc} */
BrokerLibThreadPool::BrokerLibThreadPool()
    : m_threadPool( BrokerSettings::getBrokerLibThreadPoolSize() )
{
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "Started broker lib thread pool." << SL_DEBUG_END;
    }    
}

/** {@inheritDoc} */
BrokerLibThreadPool::~BrokerLibThreadPool()
{
}

/** {@inheritDoc} */
bool BrokerLibThreadPool::addWork( std::shared_ptr<ThreadPool::Runnable> runnable )
{
    return m_threadPool.addWork( runnable );
}

/** {@inheritDoc} */
void BrokerLibThreadPool::shutdown()
{
    m_threadPool.shutdown();

    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "Shut down broker lib thread pool." << SL_DEBUG_END;
    }    
}

} /* namespace util */
} /* namespace broker */
} /* namespace dxl */
