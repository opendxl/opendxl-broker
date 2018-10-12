/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/BrokerSettings.h"
#include "include/SimpleLog.h"
#include "include/brokerlib.h"
#include "json/include/JsonService.h"
#include "message/handler/include/ClientRegistryConnectEventHandler.h"
#include "message/payload/include/ClientRegistryConnectEventPayload.h"
#include "message/include/DxlMessageConstants.h"
#include "message/include/DxlMessageService.h"

using namespace std;
using namespace dxl::broker::json;
using namespace dxl::broker::message;
using namespace dxl::broker::message::payload;
using namespace dxl::broker::message::handler;
using namespace dxl::broker::core;

/** {@inheritDoc} */
bool ClientRegistryConnectEventHandler::onStoreMessage(
    CoreMessageContext* context, struct cert_hashes* /*certHashes*/ ) const
{
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "ClientRegistryConnectEventHandler::onStoreMessage" << SL_DEBUG_END;
    }

    if( BrokerSettings::isUniqueClientIdPerFabricEnabled() && !context->isLocalBrokerSource() )
    {
        // Get the DXL event
        DxlEvent* evt = context->getDxlEvent();

        // Parse the payload
        ClientRegistryConnectEventPayload payload;
        JsonService::getInstance().fromJson( evt->getPayloadStr(), payload );        

        // The client identifier
        string clientId = payload.getClientId();

        // Check to see if client is connected
        if( getCoreInterface()->isClientConnected( clientId ) )
        {
            if( SL_LOG.isInfoEnabled() )
            {
                SL_START << "ClientRegistryConnectEventHandler::onStoreMessage: Client '"
                    << payload.getClientId() << "' has connected to broker '" 
                    << evt->getSourceBrokerGuid()  << "' forcing disconnect."
                    << SL_INFO_END;
            }

            getCoreInterface()->disconnectClient( clientId );
        }
    }

    // propagate event
    return true;
}
