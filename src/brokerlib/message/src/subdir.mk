###############################################################################
# Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
###############################################################################

OBJS += \
	message/src/messageinterface.o \
	message/src/dxl_error_message.o \
	message/src/messageImpl.o \
	message/src/DxlErrorResponse.o \
	message/src/DxlEvent.o \
	message/src/DxlMessage.o \
	message/src/DxlMessageConstants.o \
	message/src/DxlMessageService.o \
	message/src/DxlRequest.o \
	message/src/DxlResponse.o \
	message/builder/src/BrokerStateEventBuilder.o \
	message/builder/src/FabricChangeEventBuilder.o \
	message/handler/src/AuthorizationHandler.o \
	message/handler/src/BrokerDisableTestModeRequestHandler.o \
	message/handler/src/BrokerEnableTestModeRequestHandler.o \
	message/handler/src/BrokerHealthRequestHandler.o \
	message/handler/src/BrokerRegistryQueryRequestHandler.o \
	message/handler/src/BrokerStateEventHandler.o \
	message/handler/src/BrokerStateTopicsEventHandler.o \
	message/handler/src/BrokerSubsRequestHandler.o \
	message/handler/src/BrokerTopicEventHandler.o \
	message/handler/src/BrokerTopicQueryRequestHandler.o \
	message/handler/src/ClientRegistryConnectEventHandler.o \
	message/handler/src/ClientRegistryQueryRequestHandler.o \
	message/handler/src/DumpBrokerStateEventHandler.o \
	message/handler/src/DumpServiceStateEventHandler.o \
	message/handler/src/DxlMessageHandlers.o \
	message/handler/src/FabricChangeEventHandler.o \
	message/handler/src/MessageRoutingHandler.o \
	message/handler/src/NoEventDestinationHandler.o \
	message/handler/src/NoRequestDestinationHandler.o \
	message/handler/src/RevocationListEventHandler.o \
	message/handler/src/ServiceLookupHandler.o \
	message/handler/src/ServiceRegistryQueryRequestHandler.o \
	message/handler/src/ServiceRegistryRegisterEventHandler.o \
	message/handler/src/ServiceRegistryRegisterRequestHandler.o \
	message/handler/src/ServiceRegistryUnregisterEventHandler.o \
	message/handler/src/ServiceRegistryUnregisterRequestHandler.o \
	message/handler/src/TenantExceedsLimitEventHandler.o \
	message/handler/src/TenantLimitResetEventHandler.o \
	message/payload/src/AbstractBrokerTopicEventPayload.o \
	message/payload/src/BrokerHealthRequestPayload.o \
	message/payload/src/BrokerHealthResponsePayload.o \
	message/payload/src/BrokerRegistryQueryRequestPayload.o \
	message/payload/src/BrokerRegistryQueryResponsePayload.o \
	message/payload/src/BrokerStateEventPayload.o \
	message/payload/src/BrokerStateTopicsEventPayload.o \
	message/payload/src/BrokerSubsRequestPayload.o \
	message/payload/src/BrokerSubsResponsePayload.o \
	message/payload/src/BrokerTopicEventPayload.o \
	message/payload/src/BrokerTopicQueryRequestPayload.o \
	message/payload/src/BrokerTopicQueryResponsePayload.o \
	message/payload/src/ClientRegistryConnectEventPayload.o \
	message/payload/src/ClientRegistryQueryRequestPayload.o \
	message/payload/src/ClientRegistryQueryResponsePayload.o \
	message/payload/src/EventSubscriberNotFoundEventPayload.o \
	message/payload/src/FabricChangeEventPayload.o \
	message/payload/src/ServiceRegistryRegisterEventPayload.o \
	message/payload/src/ServiceRegistryUnregisterEventPayload.o \
	message/payload/src/ServiceRegistryQueryRequestPayload.o \
	message/payload/src/ServiceRegistryQueryResponsePayload.o \
	message/payload/src/ServiceRegistryQueryResponseServicePayload.o \
	message/payload/src/TenantExceedsLimitEventPayload.o