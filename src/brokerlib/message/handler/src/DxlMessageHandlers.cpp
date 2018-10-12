/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/BrokerSettings.h"
#include "include/SimpleLog.h"
#include "message/handler/include/DxlMessageHandlers.h"
#include "message/handler/include/AuthorizationHandler.h"
#include "message/handler/include/BrokerDisableTestModeRequestHandler.h"
#include "message/handler/include/BrokerEnableTestModeRequestHandler.h"
#include "message/handler/include/BrokerHealthRequestHandler.h"
#include "message/handler/include/BrokerRegistryQueryRequestHandler.h"
#include "message/handler/include/BrokerStateEventHandler.h"
#include "message/handler/include/BrokerStateTopicsEventHandler.h"
#include "message/handler/include/BrokerSubsRequestHandler.h"
#include "message/handler/include/BrokerTopicEventHandler.h"
#include "message/handler/include/BrokerTopicQueryRequestHandler.h"
#include "message/handler/include/ClientRegistryQueryRequestHandler.h"
#include "message/handler/include/ClientRegistryConnectEventHandler.h"
#include "message/handler/include/DumpBrokerStateEventHandler.h"
#include "message/handler/include/DumpServiceStateEventHandler.h"
#include "message/handler/include/FabricChangeEventHandler.h"
#include "message/handler/include/MessageRoutingHandler.h"
#include "message/handler/include/NoEventDestinationHandler.h"
#include "message/handler/include/NoRequestDestinationHandler.h"
#include "message/handler/include/RevocationListEventHandler.h"
#include "message/handler/include/ServiceLookupHandler.h"
#include "message/handler/include/ServiceRegistryQueryRequestHandler.h"
#include "message/handler/include/ServiceRegistryRegisterEventHandler.h"
#include "message/handler/include/ServiceRegistryRegisterRequestHandler.h"
#include "message/handler/include/ServiceRegistryUnregisterEventHandler.h"
#include "message/handler/include/ServiceRegistryUnregisterRequestHandler.h"
#include "message/handler/include/TenantExceedsByteLimitEventHandler.h"
#include "message/handler/include/TenantLimitResetEventHandler.h"
#include "message/include/DxlMessageConstants.h"
#include "core/include/CoreMessageHandlerService.h"

using namespace dxl::broker::core;
using namespace dxl::broker::message::handler;

/** The Authorization handler */
static AuthorizationHandler s_authHandler;
/** "Broker disable test mode" request handler */
static BrokerDisableTestModeRequestHandler s_brokerDisableTestModeRequestHandler;
/** "Broker enable test mode" request handler */
static BrokerEnableTestModeRequestHandler s_brokerEnableTestModeRequestHandler;
/** "Broker health" request handler */
static BrokerHealthRequestHandler s_brokerHealthRequestHandler;
/** "Broker registry: query" event handler */
static BrokerRegistryQueryRequestHandler s_brokerRegistryQueryRequestHandler;
/** "Broker state" event handler */
static BrokerStateEventHandler s_brokerStateEventHandler;
/** "Broker state topics" event handler */
static BrokerStateTopicsEventHandler s_brokerStateTopicsEventHandler;
/** "Broker subs" request handler */
static BrokerSubsRequestHandler s_brokerSubsRequestHandler;
/** "Broker topic" event handler */
static BrokerTopicEventHandler s_brokerTopicEventHandler;
/** "Broker topic: query" request handler */
static BrokerTopicQueryRequestHandler s_brokerTopicQueryRequestHandler;
/** "Client registry: connect" event handler */
static ClientRegistryConnectEventHandler s_clientRegistryConnectEventHandler;
/** "Client registry: query" event handler */
static ClientRegistryQueryRequestHandler s_clientRegistryQueryRequestHandler;
/** "Dump broker state" event handler */
static DumpBrokerStateEventHandler s_dumpBrokerStateEventHandler;
/** "Dump service state" event handler */
static DumpServiceStateEventHandler s_dumpServiceStateEventHandler;
/** "Fabric change" event handler */
static FabricChangeEventHandler s_fabricChangeEventHandler;
/** "Message routing" handler */
static MessageRoutingHandler s_messageRoutingHandler;
/** "No event destination" handler */
static NoEventDestinationHandler s_noEventDestHandler;
/** "No request destination" handler */
static NoRequestDestinationHandler s_noRequestDestHandler;
/** "Revocation list" handler */
static RevocationListEventHandler s_revocationListEventHandler;
/** "Service lookup" request handler */
static ServiceLookupHandler s_serviceLookupHandler;
/** "Service registry: query" request handler */
static ServiceRegistryQueryRequestHandler s_serviceRegistryQueryRequestHandler;
/** "Service registry: register" event handler */
static ServiceRegistryRegisterEventHandler s_serviceRegistryRegisterEventHandler;
/** "Service registry: register" request handler */
static ServiceRegistryRegisterRequestHandler s_serviceRegistryRegisterRequestHandler;
/** "Service registry: unregister" event handler */
static ServiceRegistryUnregisterEventHandler s_serviceRegistryUnregisterEventHandler;
/** "Service registry: unregister" request handler */
static ServiceRegistryUnregisterRequestHandler s_serviceRegistryUnregisterRequestHandler;
/** "Tenant exceeds byte limit" event handler */
static TenantExceedsByteLimitEventHandler s_tenantExceedsByteLimitEventHandler;
/** "Tenant limit reset" event handler */
static TenantLimitResetEventHandler s_tenantLimitResetEventHandler;

/** {@inheritDoc} */
void DxlMessageHandlers::registerHandlers()
{
    CoreMessageHandlerService& handlerService =
        CoreMessageHandlerService::getInstance();

    //
    // On publish handlers
    //

    handlerService.registerPublishHandler( &s_authHandler );
    
    //
    // On store handlers
    //

    // Global
    handlerService.registerStoreHandler( &s_serviceLookupHandler );

    // Topic-based
    handlerService.registerStoreHandler( 
        DxlMessageConstants::CHANNEL_DXL_BROKER_STATE_EVENT,         
        &s_brokerStateEventHandler );    

    handlerService.registerStoreHandler( 
        DxlMessageConstants::CHANNEL_DXL_BROKER_STATE_TOPICS_EVENT,         
        &s_brokerStateTopicsEventHandler );    

    handlerService.registerStoreHandler( 
        DxlMessageConstants::CHANNEL_DXL_BROKER_TOPIC_ADDED_EVENT,
        &s_brokerTopicEventHandler );    

    handlerService.registerStoreHandler( 
        DxlMessageConstants::CHANNEL_DXL_BROKER_TOPIC_REMOVED_EVENT,
        &s_brokerTopicEventHandler );    

    handlerService.registerStoreHandler( 
        DxlMessageConstants::CHANNEL_DXL_CLIENTREGISTRY_QUERY_REQUEST,
        &s_clientRegistryQueryRequestHandler );

    handlerService.registerStoreHandler( 
        DxlMessageConstants::CHANNEL_DXL_CLIENTREGISTRY_CONNECT_EVENT,
        &s_clientRegistryConnectEventHandler );

    handlerService.registerStoreHandler( 
        DxlMessageConstants::CHANNEL_DXL_FABRIC_CHANGE_EVENT, 
        &s_fabricChangeEventHandler );    

    handlerService.registerStoreHandler( 
        DxlMessageConstants::CHANNEL_DXL_SVCREGISTRY_REGISTER_EVENT, 
        &s_serviceRegistryRegisterEventHandler );

    handlerService.registerStoreHandler( 
        DxlMessageConstants::CHANNEL_DXL_SVCREGISTRY_REGISTER_REQUEST, 
        &s_serviceRegistryRegisterRequestHandler );

    handlerService.registerStoreHandler( 
        DxlMessageConstants::CHANNEL_DXL_SVCREGISTRY_UNREGISTER_EVENT, 
        &s_serviceRegistryUnregisterEventHandler );

    handlerService.registerStoreHandler( 
        DxlMessageConstants::CHANNEL_DXL_SVCREGISTRY_UNREGISTER_REQUEST, 
        &s_serviceRegistryUnregisterRequestHandler );

    handlerService.registerStoreHandler( 
        DxlMessageConstants::CHANNEL_DXL_BROKERREGISTRY_QUERY_REQUEST, 
        &s_brokerRegistryQueryRequestHandler );

    handlerService.registerStoreHandler( 
        DxlMessageConstants::CHANNEL_DXL_SVCREGISTRY_QUERY_REQUEST, 
        &s_serviceRegistryQueryRequestHandler );    

    handlerService.registerStoreHandler(
        DxlMessageConstants::CHANNEL_DXL_BROKER_HEALTH_REQUEST,
        &s_brokerHealthRequestHandler );

    handlerService.registerStoreHandler(
        DxlMessageConstants::CHANNEL_DXL_BROKER_SUBS_REQUEST,
        &s_brokerSubsRequestHandler );

    handlerService.registerStoreHandler(
        DxlMessageConstants::CHANNEL_DXL_BROKERREGISTRY_TOPICQUERY_REQUEST,
        &s_brokerTopicQueryRequestHandler );

    handlerService.registerStoreHandler(
        DxlMessageConstants::CHANNEL_DXL_TENANT_LIMIT_EXCEEDED_EVENT,
        &s_tenantExceedsByteLimitEventHandler );

    handlerService.registerStoreHandler(
        DxlMessageConstants::CHANNEL_DXL_TENANT_LIMIT_RESET_EVENT,
        &s_tenantLimitResetEventHandler );

    handlerService.registerStoreHandler(
        DxlMessageConstants::CHANNEL_DXL_REVOCATION_LIST_EVENT,
        &s_revocationListEventHandler );

    //
    // On insert handlers
    //

    // Global
    handlerService.registerInsertHandler( &s_messageRoutingHandler );
    handlerService.registerInsertHandler( &s_authHandler );    

    //
    // On finalize handlers
    //

    // Global
    handlerService.registerFinalizeHandler( &s_noRequestDestHandler );


    // Register test mode handlers
    if( BrokerSettings::isTestModeEnabled() )
    {
        registerTestHandlers();
    }
}


/** {@inheritDoc} */
void DxlMessageHandlers::registerTestHandlers()
{
    if( SL_LOG.isInfoEnabled() )
    {
        SL_START << "Registering DXL message 'test' handlers" << SL_INFO_END;
    }

    CoreMessageHandlerService& handlerService =
        CoreMessageHandlerService::getInstance();

    //
    // On publish handlers
    //

    handlerService.registerPublishHandler( 
        DxlMessageConstants::CHANNEL_DXL_DUMPBROKER_STATE_EVENT, 
        &s_dumpBrokerStateEventHandler );    
    
    handlerService.registerPublishHandler( 
        DxlMessageConstants::CHANNEL_DXL_DUMPSERVICE_STATE_EVENT, 
        &s_dumpServiceStateEventHandler );        

    //
    //    On store handler
    // 

    handlerService.registerStoreHandler(
        DxlMessageConstants::CHANNEL_DXL_BROKER_DISABLE_TEST_MODE,
        &s_brokerDisableTestModeRequestHandler );

    handlerService.registerStoreHandler(
        DxlMessageConstants::CHANNEL_DXL_BROKER_ENABLE_TEST_MODE,
        &s_brokerEnableTestModeRequestHandler );

    //
    // On finalize handlers
    //

    // Global
    handlerService.registerFinalizeHandler( &s_noEventDestHandler );
}

