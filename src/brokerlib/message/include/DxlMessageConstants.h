/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef DXLMESSAGECONSTANTS_H_
#define DXLMESSAGECONSTANTS_H_

/** The prefix for DXL events */
#define DXL_BROKER_EVENT_PREFIX "/mcafee/event/dxl/"
#define DXL_BROKER_EVENT_PREFIX_LEN 18

/** The prefix for DXL request */
#define DXL_BROKER_REQUEST_PREFIX "/mcafee/service/dxl/"
#define DXL_BROKER_REQUEST_PREFIX_LEN 20

/** The prefix for DXL clients */
#define DXL_CLIENT_PREFIX "/mcafee/client/"
#define DXL_CLIENT_PREFIX_LEN 15
#define DXL_CLIENT_PREFIX_BRACKET DXL_CLIENT_PREFIX "{"
#define DXL_CLIENT_PREFIX_BRACKET_LEN 16

namespace dxl {
namespace broker {
namespace message {

/**
 * DXL Message related constants
 */
struct DxlMessageConstants 
{
    /** Channel for the "Broker state event" */
    static const char* CHANNEL_DXL_BROKER_STATE_EVENT;
    /** Channel for the "Broker state topics event" */
    static const char* CHANNEL_DXL_BROKER_STATE_TOPICS_EVENT;
    /** Channel for the "Broker topic added event" */
    static const char* CHANNEL_DXL_BROKER_TOPIC_ADDED_EVENT;
    /** Channel for the "Broker topic removed event" */
    static const char* CHANNEL_DXL_BROKER_TOPIC_REMOVED_EVENT;
    /** Channel for the "Client registry: connect event" */
    static const char* CHANNEL_DXL_CLIENTREGISTRY_CONNECT_EVENT;
    /** Channel for the "Client registry: disconnect event" */
    static const char* CHANNEL_DXL_CLIENTREGISTRY_DISCONNECT_EVENT;
    /** Channel for dumping broker state */
    static const char* CHANNEL_DXL_DUMPBROKER_STATE_EVENT;
    /** Channel for dumping service state */
    static const char* CHANNEL_DXL_DUMPSERVICE_STATE_EVENT;
    /** Channel for the "Event subscriber is not found event" */
    static const char* CHANNEL_DXL_EVENT_SUBSCRIBER_NOT_FOUND_EVENT;
    /** Channel for the "Fabric change event" */
    static const char* CHANNEL_DXL_FABRIC_CHANGE_EVENT;
    /** Channel for the "Revocation list event" */
    static const char* CHANNEL_DXL_REVOCATION_LIST_EVENT;
    /** Channel for the "Service registry: register event" */
    static const char* CHANNEL_DXL_SVCREGISTRY_REGISTER_EVENT;
    /** Channel for the "Service registry: unregister event" */
    static const char* CHANNEL_DXL_SVCREGISTRY_UNREGISTER_EVENT;
    /** Channel for the "Tenant limit exceeded event" */
    static const char* CHANNEL_DXL_TENANT_LIMIT_EXCEEDED_EVENT;
    /** Channel for the "Tenant limit reset event" */
    static const char* CHANNEL_DXL_TENANT_LIMIT_RESET_EVENT;    

    /** Channel for the "Disable test mode" request */
    static const char* CHANNEL_DXL_BROKER_DISABLE_TEST_MODE;
    /** Channel for the "Enable test mode" request */
    static const char* CHANNEL_DXL_BROKER_ENABLE_TEST_MODE;
    /** Channel for the "Broker health" request */
    static const char* CHANNEL_DXL_BROKER_HEALTH_REQUEST;
    /** Channel for the "Broker subscriptions" request */
    static const char* CHANNEL_DXL_BROKER_SUBS_REQUEST;
    /** Channel for a "Broker registry: query" request */
    static const char* CHANNEL_DXL_BROKERREGISTRY_QUERY_REQUEST;
    /** Channel for a "Broker topic: query" request */
    static const char* CHANNEL_DXL_BROKERREGISTRY_TOPICQUERY_REQUEST;
    /** Channel for a "Client registry: query" request */
    static const char* CHANNEL_DXL_CLIENTREGISTRY_QUERY_REQUEST;
    /** Channel for a "Service registry: query" request */
    static const char* CHANNEL_DXL_SVCREGISTRY_QUERY_REQUEST;
    /** Channel for the "Service registry register request" */
    static const char* CHANNEL_DXL_SVCREGISTRY_REGISTER_REQUEST;
    /** Channel for the "Service registry unregister request" */
    static const char* CHANNEL_DXL_SVCREGISTRY_UNREGISTER_REQUEST;

    /** Bridges property */
    static const char* PROP_BRIDGES;
    /** Bridges that are children */
    static const char* PROP_BRIDGE_CHILDREN;
    /** The broker GUID property */
    static const char* PROP_BROKER_GUID;
    /** Broker Version */
    static const char* PROP_BROKER_VERSION;
    /** The brokers property */
    static const char* PROP_BROKERS;
    /** The certificates property */
    static const char* PROP_CERTIFICATES;
    /** The change count property */
    static const char* PROP_CHANGE_COUNT;
    /** The client GUID property */
    static const char* PROP_CLIENT_GUID;
    /** The client instance GUID property */
    static const char* PROP_CLIENT_INSTANCE_GUID;
    /** The client tenant GUID */
    static const char* PROP_CLIENT_TENANT_GUID;
    /** Connected clients property */
    static const char* PROP_CONNECTED_CLIENTS;
    /** The connection limit property */
    static const char* PROP_CONNECTION_LIMIT;
    /** The count property */
    static const char* PROP_COUNT;
    /** Display name property */
    static const char* PROP_DISPLAY_NAME;
    /** The exists property */
    static const char* PROP_EXISTS;
    /** A fabrics property */
    static const char* PROP_FABRICS;
    /** A GUID property */
    static const char* PROP_GUID;
    /** A hostname property */
    static const char* PROP_HOSTNAME;
    /** Incoming Messages Property */
    static const char* PROP_INCOMING_MSGS;
    /** Index Property */
    static const char* PROP_INDEX;
    /** Whether the service is local property */
    static const char* PROP_LOCAL;
    /** Outgoing Messages Property  */
    static const char* PROP_LOCAL_SVC_COUNTER;
    /** A managing ePO name */
    static const char* PROP_MANAGING_EPO_NAME;
    /** Managed property name */
    static const char* PROP_MANAGED;
    /** Metadata (name/value pairs) property */
    static const char* PROP_METADATA;
    /** Incoming Messages Property  */
    static const char* PROP_OUTGOING_MSGS;
    /** A patterns property */
    static const char* PROP_PATTERNS;
    /** A plugins property */
    static const char* PROP_PLUGINS;
    /** A policy host name property */
    static const char* PROP_POLICY_HOSTNAME;
    /** A policy hub property */
    static const char* PROP_POLICY_HUB;
    /** A policy IP address property */
    static const char* PROP_POLICY_IP_ADDRESS;
    /** A policy port property */
    static const char* PROP_POLICY_PORT;
    /** A port property */
    static const char* PROP_PORT;
    /** A WebSocket port property */
    static const char* PROP_WEBSOCKET_PORT;
    /** A properties property */
    static const char* PROP_PROPERTIES;
    /** Set of request channels property */
    static const char* PROP_REQUEST_CHANNELS;
    /** A requests property */
    static const char* PROP_REQUESTS;
    /** The registration time */
    static const char* PROP_REGISTRATION_TIME;
    /** Services property */
    static const char* PROP_SERVICES;
    /** A service GUID property */
    static const char* PROP_SERVICE_GUID;
    /** The service type property */
    static const char* PROP_SERVICE_TYPE;
    /** The broker start time */
    static const char* PROP_START_TIME;
    /** The state property */
    static const char* PROP_STATE;
    /** The string property */
    static const char* PROP_STRING;
    /** The target tenant GUIDs property */
    static const char* PROP_TARGET_TENANT_GUIDS;
    /** The type of limit a tenant has exceeded */
    static const char* PROP_TENANT_LIMIT_TYPE;
    /** Topic routing property */
    static const char* PROP_TOPIC_ROUTING;
    /** Topic property */
    static const char* PROP_TOPIC;
    /** Topics property */
    static const char* PROP_TOPICS;
    /** TTL (minutes) property */
    static const char* PROP_TTL_MINS;
    /** Type property */
    static const char* PROP_TYPE;
    /** Unauthorized channels property */
    static const char* PROP_UNAUTHORIZED_CHANNELS;
    /** Value property */
    static const char* PROP_VALUE;
};

} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* DXLMESSAGECONSTANTS_H_ */
