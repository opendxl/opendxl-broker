/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/include/messageinterface.h"

namespace dxl {
namespace broker{
namespace message {


static bool createMessage(dxl_message_type_t type, const char *clientId,
    const char* clientInstanceId, const char *msgId, const payload_t *payload, dxl_message_t **msg)
{
    bool retVal(false);

    if((clientId && *clientId) && msg)
    {
        if(createDxlMessage(0, type, clientId, clientInstanceId, msgId, msg) == DXLMP_OK)
        {
            bool addedPayload(true);
            if(payload && payload->data && payload->size)
            {
                addedPayload = (setDxlMessagePayload(0, *msg, payload->data, payload->size) == DXLMP_OK);
            }

            retVal = addedPayload;
        }
    }

    return retVal;
}

bool createRequestMessage(const char *clientId,
    const char* clientInstanceId, const char *msgId, const payload_t *payload,
    const char *replyToTopic, dxl_message_t **msg)
{
    bool retVal(false);

    if(createMessage(DXLMP_REQUEST, clientId, clientInstanceId, msgId, payload, msg))
    {
        bool addedReplyToTopic(true);
        if(replyToTopic)
        {
            addedReplyToTopic = (setDxlRequestMessageAttributes(0, *msg, reply_to_topic, replyToTopic) == DXLMP_OK);
        }

        retVal = addedReplyToTopic;
    }

    return retVal;
}

bool createEventMessage(const char *clientId,
    const char* clientInstanceId, const char *msgId, const payload_t *payload, dxl_message_t **msg)
{
    return createMessage(DXLMP_EVENT, clientId, clientInstanceId, msgId, payload, msg);
}

static bool createResponseMessageCommon(dxl_message_type_t type, const char *clientId,
    const char* clientInstanceId, const char *msgId, const payload_t *payload,
    const char *requestId, dxl_message_t **msg)
{
    bool retVal(false);

    if(createMessage(type, clientId, clientInstanceId, msgId, payload, msg))
    {
        bool addedRequestId(true);
        if(requestId)
        {
            addedRequestId = (setDxlResponseMessageAttributes(0, *msg, requestId) == DXLMP_OK);
        }

        retVal = addedRequestId;
    }

    return retVal;
}

bool createResponseMessage(const char *clientId,
    const char* clientInstanceId, const char *msgId, const payload_t *payload,
    const char *requestId, dxl_message_t **msg)
{
    return createResponseMessageCommon(DXLMP_RESPONSE, clientId, clientInstanceId, msgId, payload, requestId, msg);
}

bool createErrorResponseMessage(const char *clientId, const char* clientInstanceId,
    const char *msgId, const payload_t *payload, const char *requestId, const error_t *error, dxl_message_t **msg)
{
    bool retVal(false);

    if(createResponseMessageCommon(DXLMP_RESPONSE_ERROR, clientId, clientInstanceId, msgId, payload, requestId, msg))
    {
        bool addedErrorMsg(true);
        if(error && error->msg)
        {
            addedErrorMsg = (setDxlErrorResponseMessageAttributes(0, *msg, error->code, error->msg) == DXLMP_OK);
        }

        retVal = addedErrorMsg;
    }

    return retVal;
}

void freeMessage(dxl_message_t *msg)
{
    if(msg) freeDxlMessage(0, msg);;
}

}
}
}
