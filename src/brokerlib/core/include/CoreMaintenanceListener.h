/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef COREMAINTENANCELISTENER_H_
#define COREMAINTENANCELISTENER_H_

#include <ctime>

namespace dxl {
namespace broker {
namespace core {

/** 
 * Listener that is notified when core maintenance is performed
 */
class CoreMaintenanceListener
{
public:
    /** Destructor */
    virtual ~CoreMaintenanceListener() {}

    /**
     * Invoked when the core does its maintenance
     *
     * @param   time The time of the core maintenance (in seconds)
     */
    virtual void onCoreMaintenance( time_t time ) = 0;
};

} /* namespace core */
} /* namespace broker */
} /* namespace dxl */

#endif  // COREMAINTENANCELISTENER_H_
