/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "core/include/CoreBridgeConfigurationFactory.h"
#include "include/SimpleLog.h"
#include "brokerconfiguration/include/ConfigBroker.h"
#include <iostream>

using namespace std;
using namespace dxl::broker;
using namespace dxl::broker::core;

/** {@inheritDoc} */
CoreBridgeConfiguration CoreBridgeConfigurationFactory::createEmptyConfiguration()
{
    // Empty configuration
    return CoreBridgeConfiguration( "EMPTY" );
}

/** {@inheritDoc} */
CoreBridgeConfiguration CoreBridgeConfigurationFactory::createConfiguration(
    const string& guid,
    shared_ptr<const BrokerConfiguration> brokerConfig)
{    
    // The configuration
    CoreBridgeConfiguration config( guid );

    // Get the local broker
    shared_ptr<const ConfigNode> localNode = brokerConfig->getConfigNode( guid );

    if( localNode )
    {
        if( !localNode->isHub() )
        {
            // Cast to broker type
            const shared_ptr<const ConfigBroker> localBroker = 
                static_pointer_cast<const ConfigBroker>( localNode );

            // Sets the properties for the broker
            config.setBrokerProperties(
                localBroker->getHostname(),
                localBroker->getIpAddress(),
                localBroker->getPort() );

            // Add the primary hub to list of bridged brokers (if applicable)
            addPrimaryHub( config, brokerConfig, localBroker );

            // Add parents
            addParents( config, brokerConfig, localBroker );
        }
        else
        {
            // The local broker can't be a hub
            SL_START << "Local broker is a hub!: " << guid << SL_ERROR_END;
        }
    }
    else
    {
        // We didn't find the local broker in the configuration
        if( SL_LOG.isWarnEnabled() )
            SL_START << "Unable to find local broker in configuration: " << guid << SL_WARN_END;
    }

    return config;
}

/** {@inheritDoc} */
void CoreBridgeConfigurationFactory::addPrimaryHub(
    CoreBridgeConfiguration& config,
    shared_ptr<const BrokerConfiguration> brokerConfig,
    shared_ptr<const ConfigBroker> localBroker )
{
    const string localBrokerId = localBroker->getId();

    // Get the containing hub    
    shared_ptr<const Hub> containingHub = brokerConfig->getContainingHub( localBrokerId );

    // Is the local broker a member of a hub?
    if( containingHub.get() )
    {
        if( SL_LOG.isDebugEnabled() )
            SL_START << "Local broker member of hub: "
                << containingHub->getId() << " : " << localBrokerId << SL_DEBUG_END;

        // Set the hub name
        config.setHubName( containingHub->getName() );

        // Get the other hub member
        const string otherBrokerId = 
            ( localBrokerId.compare( containingHub->getPrimaryBrokerId() ) == 0 ) ?
                containingHub->getSecondaryBrokerId() : containingHub->getPrimaryBrokerId();

        if( !otherBrokerId.empty() )
        {
            if( SL_LOG.isDebugEnabled() )
                SL_START << "Other hub member: " << otherBrokerId << SL_DEBUG_END;

            // The guids of the members of the hub will be sorted lexicographically
            // The first guid will be the primary member of the hub (always connects to parent)
            // The second guid will be the non-primary child (attempts to connect to primary, 
            // falls back to parents)
            if( localBrokerId.compare( otherBrokerId ) > 0 )
            {
                if( SL_LOG.isDebugEnabled() )
                    SL_START << "Local broker is non-primary hub member" << SL_DEBUG_END;

                // Add the primary hub broker to the list of bridged brokers
                addBroker( config, brokerConfig, otherBrokerId );

                // Since we just added the primary "brokers" (a broker can be added for
                // hostname and ip address, etc.), set the primary broker count to the
                // current broker count.
                config.setPrimaryBrokerCount( (uint32_t)config.getBrokerCount() );
            }
        }
    }
}

/** {@inheritDoc} */
void CoreBridgeConfigurationFactory::addParents(
    CoreBridgeConfiguration& config,
    shared_ptr<const BrokerConfiguration> configService,
    shared_ptr<const ConfigBroker> localBroker )
{    
    // Get the parent identifier
    string parentId;
    const shared_ptr<const Hub> containingHub = configService->getContainingHub( localBroker->getId() );
    if( containingHub.get() )
    {
        // Local broker is hub member, use the containing hub's parent
        parentId = containingHub->getParentId();
    }
    else
    {
        // Use the broker's parent
        parentId = localBroker->getParentId();
    }

    if( parentId.empty() )
    {
        // No parents
        return;
    }

    if( SL_LOG.isDebugEnabled() )
        SL_START << "Parent broker: " << parentId << SL_DEBUG_END;

    // Get the parent node
    const shared_ptr<const ConfigNode> parentNode = configService->getConfigNode( parentId );
    if( !parentNode.get() )
    {
        SL_START << "Unable to find parent node: " << parentId << SL_ERROR_END;
        return;
    }

    if( !parentNode->isHub() )
    {
        // Add the parent to the list of bridged brokers
        addBroker( config, configService, parentId );
    }
    else
    {
        // Cast to hub type
        const shared_ptr<const Hub> parentHub = static_pointer_cast<const Hub>( parentNode );

        // The parent guids will be ordered lexicographically
        if( parentHub->getPrimaryBrokerId().compare( parentHub->getSecondaryBrokerId() ) < 0 )
        {
            addBroker( config, configService, parentHub->getPrimaryBrokerId() );
            addBroker( config, configService, parentHub->getSecondaryBrokerId() );
        }
        else
        {
            addBroker( config, configService, parentHub->getSecondaryBrokerId() );
            addBroker( config, configService, parentHub->getPrimaryBrokerId() );
        }
    }
}

/** {@inheritDoc} */
void CoreBridgeConfigurationFactory::addBroker(
    CoreBridgeConfiguration& config,
    shared_ptr<const BrokerConfiguration> brokerConfig,
    const string& brokerId )
{
    if( brokerId.empty() )
    {
        // Broker identifier is empty
        return;
    }

    if( SL_LOG.isDebugEnabled() )
        SL_START << "Adding broker: " << brokerId << SL_DEBUG_END;

    // Get the node
    shared_ptr<const ConfigNode> node = brokerConfig->getConfigNode( brokerId );
    if( !node.get() )
    {
        SL_START << "Unable to find broker: " << brokerId << SL_ERROR_END;
        return;
    }

    // Ensure is a broker (not a hub)
    if( node->isHub() )
    {
        SL_START << "Broker is a hub!: " << brokerId << SL_ERROR_END;
        return;
    }

    // Cast to broker type
    const shared_ptr<const ConfigBroker> broker = 
        static_pointer_cast<const ConfigBroker>( node );

    // Add the broker
    config.addBroker( 
        CoreBridgeConfiguration::Broker(
            broker->getId(), broker->getHostname(), broker->getPort() ) );

    // Add the broker w/ IP address (if available)
    if( !broker->getIpAddress().empty() )
    {
        config.addBroker( 
            CoreBridgeConfiguration::Broker(
                broker->getId(), broker->getIpAddress(), broker->getPort() ) );
    }
}
    
