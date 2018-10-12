/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef _MESSAGE_INTERFACE_
#define _MESSAGE_INTERFACE_

#include "message/include/dx_message.h"
#include <string>

namespace dxl {
namespace broker {
namespace message {

/** Structure to hold the payload for a DXL message */
typedef struct payload_t
{
    const unsigned char *data;
    size_t size;
} payload_t;


/** Structure to hold information for a DXL error message */
typedef struct error_t
{
    const char *msg;
    int code;
} error_t;

bool createRequestMessage(const char *clientId, const char* clientInstanceId, const char *msgId,
    const payload_t *payload, const char *replyToTopic, dxl_message_t **msg);
bool createResponseMessage(const char *clientId, const char* clientInstanceId, const char *msgId,
    const payload_t *payload, const char *requestId, dxl_message_t **msg);
bool createEventMessage(const char *clientId, const char* clientInstanceId, const char *msgId,
    const payload_t *payload, dxl_message_t **msg);
bool createErrorResponseMessage(const char *clientId, const char* clientInstanceId, const char *msgId,
    const payload_t *payload, const char *requestId, const error_t *error, dxl_message_t **msg);
void freeMessage(dxl_message_t *msg);

}
}
}

#endif
