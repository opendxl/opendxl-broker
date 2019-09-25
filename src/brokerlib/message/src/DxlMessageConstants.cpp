/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/include/DxlMessageConstants.h"

using namespace dxl::broker::message;

//
// Events
//

const char* DxlMessageConstants::CHANNEL_DXL_BROKER_STATE_EVENT = 
    DXL_BROKER_EVENT_PREFIX "brokerregistry/brokerstate";               
const char* DxlMessageConstants::CHANNEL_DXL_BROKER_STATE_TOPICS_EVENT = 
    DXL_BROKER_EVENT_PREFIX "brokerregistry/brokerstatetopics";                                                           
const char* DxlMessageConstants::CHANNEL_DXL_BROKER_TOPIC_ADDED_EVENT = 
    DXL_BROKER_EVENT_PREFIX "brokerregistry/topicadded";                                                           
const char* DxlMessageConstants::CHANNEL_DXL_BROKER_TOPIC_REMOVED_EVENT = 
    DXL_BROKER_EVENT_PREFIX "brokerregistry/topicremoved";                                                           
const char* DxlMessageConstants::CHANNEL_DXL_CLIENTREGISTRY_CONNECT_EVENT = 
    DXL_BROKER_EVENT_PREFIX "clientregistry/connect";
const char* DxlMessageConstants::CHANNEL_DXL_CLIENTREGISTRY_DISCONNECT_EVENT = 
    DXL_BROKER_EVENT_PREFIX "clientregistry/disconnect";
const char* DxlMessageConstants::CHANNEL_DXL_DUMPBROKER_STATE_EVENT = 
    DXL_BROKER_EVENT_PREFIX "dumpbrokerstate";
const char* DxlMessageConstants::CHANNEL_DXL_DUMPSERVICE_STATE_EVENT =
    DXL_BROKER_EVENT_PREFIX "dumpservicestate";
const char* DxlMessageConstants::CHANNEL_DXL_EVENT_SUBSCRIBER_NOT_FOUND_EVENT = 
    DXL_BROKER_EVENT_PREFIX "eventsubscribernotfound";
const char* DxlMessageConstants::CHANNEL_DXL_FABRIC_CHANGE_EVENT = 
    DXL_BROKER_EVENT_PREFIX "fabricchange";
const char* DxlMessageConstants::CHANNEL_DXL_REVOCATION_LIST_EVENT =
    DXL_BROKER_EVENT_PREFIX "revocation/list";
const char* DxlMessageConstants::CHANNEL_DXL_SVCREGISTRY_REGISTER_EVENT = 
    DXL_BROKER_EVENT_PREFIX "svcregistry/register";
const char* DxlMessageConstants::CHANNEL_DXL_SVCREGISTRY_UNREGISTER_EVENT = 
    DXL_BROKER_EVENT_PREFIX "svcregistry/unregister";
const char* DxlMessageConstants::CHANNEL_DXL_TENANT_LIMIT_EXCEEDED_EVENT = 
    DXL_BROKER_EVENT_PREFIX "tenant/limit/exceeded";
const char* DxlMessageConstants::CHANNEL_DXL_TENANT_LIMIT_RESET_EVENT =
    DXL_BROKER_EVENT_PREFIX "tenant/limit/reset";

//
// Requests
//

const char* DxlMessageConstants::CHANNEL_DXL_BROKER_DISABLE_TEST_MODE =
    DXL_BROKER_REQUEST_PREFIX "broker/testmode/disable";
const char* DxlMessageConstants::CHANNEL_DXL_BROKER_ENABLE_TEST_MODE =
    DXL_BROKER_REQUEST_PREFIX "broker/testmode/enable";
const char* DxlMessageConstants::CHANNEL_DXL_BROKER_HEALTH_REQUEST =
    DXL_BROKER_REQUEST_PREFIX "broker/health";
const char* DxlMessageConstants::CHANNEL_DXL_BROKER_SUBS_REQUEST =
    DXL_BROKER_REQUEST_PREFIX "broker/subs";
const char* DxlMessageConstants::CHANNEL_DXL_BROKERREGISTRY_QUERY_REQUEST = 
    DXL_BROKER_REQUEST_PREFIX "brokerregistry/query";                       
const char* DxlMessageConstants::CHANNEL_DXL_BROKERREGISTRY_TOPICQUERY_REQUEST =
    DXL_BROKER_REQUEST_PREFIX "brokerregistry/topicquery";
const char* DxlMessageConstants::CHANNEL_DXL_CLIENTREGISTRY_QUERY_REQUEST = 
    DXL_BROKER_REQUEST_PREFIX "clientregistry/query";
const char* DxlMessageConstants::CHANNEL_DXL_SVCREGISTRY_QUERY_REQUEST = 
    DXL_BROKER_REQUEST_PREFIX "svcregistry/query";                                                           
const char* DxlMessageConstants::CHANNEL_DXL_SVCREGISTRY_REGISTER_REQUEST = 
    DXL_BROKER_REQUEST_PREFIX "svcregistry/register";
const char* DxlMessageConstants::CHANNEL_DXL_SVCREGISTRY_UNREGISTER_REQUEST = 
    DXL_BROKER_REQUEST_PREFIX "svcregistry/unregister";

//
// Properties
//

const char* DxlMessageConstants::PROP_BRIDGES = "bridges";
const char* DxlMessageConstants::PROP_BRIDGE_CHILDREN = "bridgeChildren";
const char* DxlMessageConstants::PROP_BROKERS = "brokers";
const char* DxlMessageConstants::PROP_BROKER_GUID = "brokerGuid";
const char* DxlMessageConstants::PROP_BROKER_VERSION = "version";
const char* DxlMessageConstants::PROP_CERTIFICATES = "certificates";
const char* DxlMessageConstants::PROP_CHANGE_COUNT = "changeCount";
const char* DxlMessageConstants::PROP_CLIENT_GUID = "clientGuid";
const char* DxlMessageConstants::PROP_CLIENT_INSTANCE_GUID = "clientInstanceGuid";
const char* DxlMessageConstants::PROP_CLIENT_TENANT_GUID = "clientTenantGuid";
const char* DxlMessageConstants::PROP_CONNECTED_CLIENTS = "connectedClients";
const char* DxlMessageConstants::PROP_CONNECTION_LIMIT = "connectionLimit";
const char* DxlMessageConstants::PROP_COUNT = "count";
const char* DxlMessageConstants::PROP_DISPLAY_NAME = "displayName";
const char* DxlMessageConstants::PROP_EXISTS = "exists";
const char* DxlMessageConstants::PROP_FABRICS = "fabrics";
const char* DxlMessageConstants::PROP_GUID = "guid";
const char* DxlMessageConstants::PROP_HOSTNAME = "hostname";
const char* DxlMessageConstants::PROP_INCOMING_MSGS = "incomingMessages";
const char* DxlMessageConstants::PROP_INDEX = "index";
const char* DxlMessageConstants::PROP_LOCAL = "local";
const char* DxlMessageConstants::PROP_LOCAL_SVC_COUNTER = "localServiceCounter";
const char* DxlMessageConstants::PROP_MANAGED = "managed";
const char* DxlMessageConstants::PROP_MANAGING_EPO_NAME = "epoName";
const char* DxlMessageConstants::PROP_METADATA = "metaData";
const char* DxlMessageConstants::PROP_OUTGOING_MSGS = "outgoingMessages";
const char* DxlMessageConstants::PROP_PATTERNS = "patterns";
const char* DxlMessageConstants::PROP_PLUGINS = "plugins";
const char* DxlMessageConstants::PROP_POLICY_HOSTNAME = "policyHostname";
const char* DxlMessageConstants::PROP_POLICY_HUB = "policyHub";
const char* DxlMessageConstants::PROP_POLICY_IP_ADDRESS = "policyIpAddress";
const char* DxlMessageConstants::PROP_POLICY_PORT = "policyPort";
const char* DxlMessageConstants::PROP_PORT = "port";
const char* DxlMessageConstants::PROP_WEBSOCKET_PORT = "webSocketPort";
const char* DxlMessageConstants::PROP_PROPERTIES = "properties";
const char* DxlMessageConstants::PROP_REGISTRATION_TIME = "registrationTime";
const char* DxlMessageConstants::PROP_REQUEST_CHANNELS = "requestChannels";
const char* DxlMessageConstants::PROP_SERVICES = "services";
const char* DxlMessageConstants::PROP_SERVICE_GUID = "serviceGuid";
const char* DxlMessageConstants::PROP_SERVICE_TYPE = "serviceType";
const char* DxlMessageConstants::PROP_START_TIME = "startTime";
const char* DxlMessageConstants::PROP_STATE = "state";
const char* DxlMessageConstants::PROP_STRING = "string";
const char* DxlMessageConstants::PROP_TARGET_TENANT_GUIDS = "targetTenantGuids";
const char* DxlMessageConstants::PROP_TENANT_LIMIT_TYPE = "limitType";
const char* DxlMessageConstants::PROP_TOPIC_ROUTING = "topicRouting";
const char* DxlMessageConstants::PROP_TOPIC = "topic";
const char* DxlMessageConstants::PROP_TOPICS = "topics";
const char* DxlMessageConstants::PROP_TTL_MINS = "ttlMins";
const char* DxlMessageConstants::PROP_TYPE = "type";
const char* DxlMessageConstants::PROP_UNAUTHORIZED_CHANNELS = "unauthorizedChannels";
const char* DxlMessageConstants::PROP_VALUE = "value";

