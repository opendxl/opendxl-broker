/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#pragma once
#include <stddef.h>
#include "dx_message_types.h"

/*
 * DXL Message library
 * This library is used to create, populate, read, and (de)serialize DXL messages
 *
 * USAGE:
 *
 * Use appropriate methods to create messages and set data.
 * Data may be read directly from the structs.
 * Use freeDxlMessage to destroy a message.
 */

#if defined __cplusplus
extern "C"
{
#endif
enum dxl_request_attribute_t
{
    reply_to_topic,
    service_instance_id
};

/*
 * Use a very simple and rudimentary logger that simply takes a string , no formatting etc.
 */
typedef void (*log_callback_t)(void *cb_arg,const char *log_message);
typedef struct dxl_message_context_t dxl_message_context_t;

dxl_message_error_t createDxlMessageContext(
    log_callback_t callback, void *cb_arg,struct dxl_message_context_t **context);
dxl_message_error_t releaseDxlMessageContext(struct dxl_message_context_t *context);
/*
 * Unable to log messages for serialization, need to create a stateful context to enable logging.
 * Context needed to prevent / use of global / static in implementation
 */


/*
 * Create DXL message
 * NOTE: Passing NULL for message ID will auto-generate an ID
 */
dxl_message_error_t createDxlMessage(struct  dxl_message_context_t *context, dxl_message_type_t messageType,
    const char* clientId, const char* clientInstanceId, const char* messageId, dxl_message_t** pNewMessge);

/* Copy a DXL message from another. */
dxl_message_error_t copyDxlMessage(
    struct dxl_message_context_t *context, const dxl_message_t* source, dxl_message_t** pNewMessge);

/* Create from bytes */
dxl_message_error_t createDxlMessageFromBytes(
    struct dxl_message_context_t *context, const unsigned char* bytes, size_t size, dxl_message_t** message);

/* Free a DXL message */
void freeDxlMessage(struct dxl_message_context_t *context, dxl_message_t* message);

/* Convert a DXL message to bytes */
dxl_message_error_t dxlMessageToBytes(struct dxl_message_context_t *context,
    unsigned char** bytes, size_t* size, dxl_message_t* message, int stripClientGuids);

/* Assigns a new message identifier */
dxl_message_error_t dxlMessageAssignNewMessageId(struct dxl_message_context_t *context,
    dxl_message_t* message, const char* prefix);

/*
 * Provide api to release bytes, to prevent memory operations from crossing dll boundaries
 * The buffer was allocated by dxlMessageToBytes
 */
dxl_message_error_t dxlMessageReleaseSerializedBytes(struct dxl_message_context_t *context, unsigned char* bytes);

////////////////////////////////////////////////////////////////////////////
// Version 0
////////////////////////////////////////////////////////////////////////////

/* Set user data. The data is *NOT* copied, and you are responsible for freeing it  */
void setDxlMessageUserData(struct dxl_message_context_t *context, dxl_message_t* message, void* data);

/* Destination topic. This is here for convenience and is not part of a packed message. This data is copied */
void setDxlMessageDestinationTopic(struct dxl_message_context_t *context, dxl_message_t* message, const char* topic);

/* The message payload. This data is copied */
dxl_message_error_t setDxlMessagePayload(struct dxl_message_context_t *context,
    dxl_message_t* message, const unsigned char* buffer, size_t size);

/* The source broker GUID */
dxl_message_error_t setDxlMessageSourceBrokerGuid(struct dxl_message_context_t *context,
    dxl_message_t* message, const char* sourceBrokerGuid);

/* The source client identifier */
dxl_message_error_t setDxlMessageSourceClientId(struct dxl_message_context_t *context,
    dxl_message_t* message, const char* sourceClientId);

/* The source client instance identifier */
dxl_message_error_t setDxlMessageSourceClientInstanceId(struct dxl_message_context_t *context,
    dxl_message_t* message, const char* sourceClientInstanceId);

/* The destination broker GUIDs */
dxl_message_error_t setDxlMessageBrokerGuids(struct dxl_message_context_t *context,
    dxl_message_t* message, const char* brokerGuids[], size_t brokerGuidCount);

/* The destination client GUIDs */
dxl_message_error_t setDxlMessageClientGuids(struct dxl_message_context_t *context,
    dxl_message_t* message, const char* clientGuids[], size_t clientGuidCount);

/* Request attributes - data is copied */
dxl_message_error_t setDxlRequestMessageAttributes(struct dxl_message_context_t *context,
    dxl_message_t* message, enum dxl_request_attribute_t type, const char* replyToTopic);

/* Response (and response error) attributes - data is copied */
dxl_message_error_t setDxlResponseMessageAttributes(struct dxl_message_context_t *context,
    dxl_message_t* message, const char* requestMessageId);

/* Response (and response error) service identifier - data is copied */
dxl_message_error_t setDxlResponseMessageServiceId(struct dxl_message_context_t *context,
    dxl_message_t* message, const char* serviceId);

/* Response error attributes - data is copied */
/* user setDxlResponseMessageAttributes to set requestMessageId */
dxl_message_error_t setDxlErrorResponseMessageAttributes(struct dxl_message_context_t *context,
    dxl_message_t* message, int errorCode, const char* errorMessage);

////////////////////////////////////////////////////////////////////////////
// Version 1
////////////////////////////////////////////////////////////////////////////

/* Other Fields */
dxl_message_error_t setDxlMessageOtherFields(struct dxl_message_context_t *context,
    dxl_message_t* message, const char* otherFields[], size_t otherFieldsCount);

////////////////////////////////////////////////////////////////////////////
// Version 2
////////////////////////////////////////////////////////////////////////////

/* The source tenant GUID */
dxl_message_error_t setDxlMessageSourceTenantGuid(struct dxl_message_context_t *context,
    dxl_message_t* message, const char* sourceTenantGuid);

/* The destination tenant GUIDs */
dxl_message_error_t setDxlMessageTenantGuids(struct dxl_message_context_t *context,
    dxl_message_t* message, const char* tenantGuids[], size_t tenantGuidCount);

////////////////////////////////////////////////////////////////////////////
// Version 4
////////////////////////////////////////////////////////////////////////////

/* Request multi-service - data is copied */
dxl_message_error_t setDxlRequestMultiService(struct dxl_message_context_t *context,
    dxl_message_t* message, bool isMultiService);


#if defined __cplusplus
}
#endif
