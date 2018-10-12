/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef COREINTERFACEEVENTHANDLER_H_
#define COREINTERFACEEVENTHANDLER_H_

#include "core/include/CoreInterface.h"
#include "brokerconfiguration/include/BrokerConfigurationServiceListener.h"

namespace dxl {
namespace broker {
namespace core {

/** 
 * This class is responsible for registering and listening for events that are forwarded to the
 * CoreInterface. This handler is separated from the CoreInterface due to the fact that
 * we don't want to introduce additional dependencies that would have to be included in the core
 * messaging implementation.
 */
class CoreInterfaceEventHandler :
    public dxl::broker::BrokerConfigurationServiceListener
{

public:

    /**
     * Starts the event handler
     *
     * @param   coreInterface The core interface to associate with the handler
     */
    static void start( CoreInterface* coreInterface );

    /** Destructor */
    virtual ~CoreInterfaceEventHandler() {}

    /** {@inheritDoc} */
    void onConfigurationUpdated( std::shared_ptr<const BrokerConfiguration> brokerConfig );

private:

    /** 
     * Constructor
     *
     * @param   coreInterface The core interface to handle events for
     */
    CoreInterfaceEventHandler( CoreInterface *coreInterface )
        : m_coreInterface( coreInterface ) {}

    /** The core interface */
    CoreInterface* m_coreInterface;
};

} /* namespace core */
} /* namespace broker */
} /* namespace dxl */

#endif /* COREINTERFACEEVENTHANDLER_H_ */
