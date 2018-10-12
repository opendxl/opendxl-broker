/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERCONFIGURATIONSERVICELISTENER_H_
#define BROKERCONFIGURATIONSERVICELISTENER_H_

#include <memory>
#include "brokerconfiguration/include/BrokerConfiguration.h"

namespace dxl {
namespace broker {

/** 
 * Listener that is notified when changes are made to the BrokerConfigurationService 
 */
class BrokerConfigurationServiceListener 
{
public:
    /** Destructor */
    virtual ~BrokerConfigurationServiceListener() {}

    /**
     * Invoked when the state of the configuration service is updated.
     *
     * @param   brokerConfig The new broker configuration
     */
    virtual void onConfigurationUpdated(
        std::shared_ptr<const BrokerConfiguration> brokerConfig ) = 0;
};

} /* namespace broker */
} /* namespace dxl */

#endif  // BROKERCONFIGURATIONSERVICELISTENER_H_
