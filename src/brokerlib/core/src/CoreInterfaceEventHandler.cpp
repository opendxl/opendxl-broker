/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "core/include/CoreInterfaceEventHandler.h"
#include "core/include/CoreBridgeConfigurationFactory.h"
#include "brokerconfiguration/include/BrokerConfigurationService.h"

using namespace std;
using namespace dxl::broker;
using namespace dxl::broker::core;

/** {@inheritDoc} */
void CoreInterfaceEventHandler::start( CoreInterface* coreInterface )
{
    static bool started = false;
    if( started ) 
    {
        return;
    }

    started = true;

    // Singleton
    static CoreInterfaceEventHandler handler( coreInterface );
    // Add broker configuration listener
    BrokerConfigurationService::Instance()->addListener( &handler );
}

/** {@inheritDoc} */
void CoreInterfaceEventHandler::onConfigurationUpdated(
    shared_ptr<const BrokerConfiguration> brokerConfig )
{
    m_coreInterface->setBridgeConfiguration(
        CoreBridgeConfigurationFactory::createConfiguration(
            m_coreInterface->getBrokerGuid(),
            brokerConfig
        ) 
    );
}
