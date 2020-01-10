/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "msgpack.h"
#include "message/include/dx_message.h"

#include "dxlcommon.h"

#include <uuid/uuid.h>


/* All the messages are going to be promoted to this DXL message version. */
#define DXL_MSG_VERSION 4

struct dxl_message_context_t
{
    log_callback_t logger_;
    void *cb_arg_;
};


/* Forward defines */
dxl_message_error_t generateMessageId(const char** messageId);
dxl_message_error_t unpackUInt32(msgpack_unpacker *unpack, uint32_t *value);
dxl_message_error_t unpackInt(msgpack_unpacker *unpack, int* value);
dxl_message_error_t unpackRaw(msgpack_unpacker *unpack,
    unsigned char** bufferOut, size_t* bufferSize, bool addNullTerminator);
static dxl_message_error_t unpackStringArray(msgpack_unpacker *unpack, char* (*arrayOut[]),
    size_t* arraySize);
static void freeStringArray( char* strArray[], size_t strArraySize );
static dxl_message_error_t copyStringArray(
    const char* sourceArray[], size_t sourceArraySize,
    char* (*destArray[]), size_t* destArraySize );

dxl_message_error_t createDxlMessageContext(log_callback_t logcb, void *cb_log,struct dxl_message_context_t **context)
{
    if (!context)
        return DXLMP_INVALID_PARAMS;
    *context = (struct dxl_message_context_t *)calloc(1, sizeof(struct dxl_message_context_t ));
    if(*context)
    {
        (*context)->logger_ = logcb;
        (*context)->cb_arg_ = cb_log;
    }
    else
        return DXLMP_NO_MEMORY;
    return DXLMP_OK;
}

dxl_message_error_t releaseDxlMessageContext(struct dxl_message_context_t *context)
{
    free(context);
    return DXLMP_OK;
}

dxl_message_error_t dxlMessageAssignNewMessageId(struct dxl_message_context_t */*context*/,
    dxl_message_t* message, const char* prefix)
{
    const char* oldMessageId = message->messageId;
    const char* id = NULL;
    dxl_message_error_t res = generateMessageId(&id);
    if (res == DXLMP_OK)
    {
        if (!prefix) 
        {
            message->messageId = id;
        }
        else
        {
            int prefixLen = strlen(prefix);
            int idLen = strlen(id);
            int len = prefixLen + idLen + 2;
            char* newId = (char*)malloc(len * sizeof(char));
            strncpy(newId, prefix, prefixLen);
            newId[prefixLen] = ':';
            strncpy(newId + prefixLen + 1, id, idLen);            
            newId[len-1] = '\0';
            free( (void*)id );
            message->messageId = newId;
        }        

        if( oldMessageId )
        {
            free( (void*)oldMessageId );
        }
    }    

    return res;
}

/******************************************************************************/
dxl_message_error_t initBaseMessage( struct dxl_message_context_t* /*context*/,
    dxl_message_t* base,
    dxl_message_type_t messageType,
    const char* clientId, const char* clientInstanceId, const char* messageId)
/******************************************************************************/
{
    base->messageType = messageType;
    base->sourceClientId = strdup(clientId);
    base->sourceClientInstanceId = strdup(clientInstanceId ? clientInstanceId : "");

    if ( messageId )
        base->messageId = strdup(messageId);
    else
        generateMessageId(&base->messageId);

    if ( !base->messageId )
        return DXLMP_NO_MEMORY;

    base->sourceBrokerGuid = strdup( "" );
    if( !base->sourceBrokerGuid )
        return DXLMP_NO_MEMORY;

    ////////////////////////////////////////////////////////////////////////////
    // Version history
    //
    // Version 0:
    //   1.0 to version 1.1:
    //     Standard fields.
    // Version 1:
    //   2.0+:
    //     Added "other" fields (string array)
    // Version 2:
    //   3.1+:
    //     Added tenant data:
    //     * Source tenant GUID (string)
    //     * Destination tenant GUIDs (string array)
    // Version 3:
    //   5.0+:
    //     Added "sourceClientInstanceId" to support multiple 
    //     connections per client (string).
    // Version 4:
    //   TBD:
    //     Multi-service request message
    ////////////////////////////////////////////////////////////////////////////
    base->version = DXL_MSG_VERSION;

    return DXLMP_OK;
}

/******************************************************************************/
dxl_message_error_t createDxlMessage(struct  dxl_message_context_t *context,
    dxl_message_type_t messageType,
    const char* clientId, const char* clientInstanceId, const char* messageId, dxl_message_t** pNewMessge)
/******************************************************************************/
{
    dxl_message_error_t ret;

    dxl_message_t* pMsg = (dxl_message_t*)calloc(1, sizeof(dxl_message_t));
    if ( !pMsg )
        return DXLMP_NO_MEMORY;

    /* Init base stuff */
    ret = initBaseMessage(context, pMsg, messageType, clientId, clientInstanceId, messageId);
    if ( ret != DXLMP_OK )
    {
        free(pMsg); /* message is invalid, free with free and not the normal mechanism */
        return ret;
    }

    if ( messageType == DXLMP_REQUEST )
    {
        pMsg->dxl_message_specificData.requestData =
            (dxl_message_request_t*)calloc(1, sizeof(dxl_message_request_t));
        pMsg->dxl_message_specificData.requestData->replyToTopic = strdup("");
        pMsg->dxl_message_specificData.requestData->serviceInstanceId = strdup("");
        pMsg->dxl_message_specificData.requestData->isMultiServiceRequest = false;
    }
    else if ( messageType == DXLMP_RESPONSE )
    {
        pMsg->dxl_message_specificData.responseData =
            (dxl_message_response_t*)calloc(1, sizeof(dxl_message_response_t));
        pMsg->dxl_message_specificData.responseData->requestMessageId = strdup("");
        pMsg->dxl_message_specificData.responseData->serviceInstanceId = strdup("");
    }
    else if ( messageType == DXLMP_RESPONSE_ERROR )
    {
        pMsg->dxl_message_specificData.responseErrorData =
            (dxl_message_response_error_t*)calloc(1, sizeof(dxl_message_response_error_t));
        pMsg->dxl_message_specificData.responseErrorData->errorMessage = strdup("");
        pMsg->dxl_message_specificData.responseErrorData->requestMessageId = strdup("");
        pMsg->dxl_message_specificData.responseErrorData->serviceInstanceId = strdup("");

    }
    else if ( messageType == DXLMP_EVENT )
    {
        /* Nothing to do here... */
    }
    else
    {
        freeDxlMessage(context,pMsg);
        return DXLMP_WRONG_MESSAGE_TYPE;
    }

    // Default source tenant GUID initialization.
    pMsg->sourceTenantGuid = strdup( "" );
    if( !pMsg->sourceTenantGuid )
        return DXLMP_NO_MEMORY;

    // CHECK :  the next line could be REMOVED, because the type was already set
    //          in initBaseMessage() method call.
    pMsg->messageType = messageType;

    *pNewMessge = pMsg;

    return ret;
}

/******************************************************************************/
dxl_message_error_t copyDxlMessage(struct  dxl_message_context_t *context,
    const dxl_message_t* source,
    dxl_message_t** pNewMessge)
/******************************************************************************/
{
    dxl_message_t* dest = NULL;
    dxl_message_error_t ret = DXLMP_BAD_DATA;

    /* To think about in the future: Pay attention to message version? */

    if ( !source )
        return DXLMP_INVALID_PARAMS;


    ret = createDxlMessage(context,source->messageType,
        source->sourceClientId, source->sourceClientInstanceId, source->messageId, &dest);

    if ( ret != DXLMP_OK )
        return ret;

    ////////////////////////////////////////////////////////////////////////////
    // Version 0
    ////////////////////////////////////////////////////////////////////////////

    /* Generic stuff */
    if ( source->destinationTopic )
        dest->destinationTopic = strdup(source->destinationTopic);

    /* Maybe rethink this... Dangerous for client */
    dest->userData = source->userData;

    /* source broker GUID */
    if( source->sourceBrokerGuid )
    {
        free( (void*)dest->sourceBrokerGuid );
        dest->sourceBrokerGuid = strdup(source->sourceBrokerGuid);
    }

    /* Broker guids */
    copyStringArray(
        source->brokerGuids, source->brokerGuidCount,
        (char***)&(dest->brokerGuids), &(dest->brokerGuidCount) );

    /* Client guids */
    copyStringArray(
        source->clientGuids, source->clientGuidCount,
        (char***)&(dest->clientGuids), &(dest->clientGuidCount) );

    if ( source->payloadSize )
    {
        dest->payloadSize = source->payloadSize;
        dest->payload = (unsigned char*)malloc(dest->payloadSize);
        memcpy((void*)dest->payload, source->payload, dest->payloadSize);
    }

    ret = DXLMP_OK;
    if ( dest->messageType == DXLMP_REQUEST )
    {
        if ( source->dxl_message_specificData.requestData->replyToTopic )
        {
            free((void *)dest->dxl_message_specificData.requestData->replyToTopic);
            dest->dxl_message_specificData.requestData->replyToTopic =
              strdup(source->dxl_message_specificData.requestData->replyToTopic);
        }
        if ( source->dxl_message_specificData.requestData->serviceInstanceId )
        {
            free((void *)dest->dxl_message_specificData.requestData->serviceInstanceId);
            dest->dxl_message_specificData.requestData->serviceInstanceId =
              strdup(source->dxl_message_specificData.requestData->serviceInstanceId);
        }
    }
    else if ( dest->messageType == DXLMP_RESPONSE )
    {
        if ( source->dxl_message_specificData.responseData->requestMessageId )
        {
            free((void *)dest->dxl_message_specificData.responseData->requestMessageId);
            dest->dxl_message_specificData.responseData->requestMessageId =
              strdup(source->dxl_message_specificData.responseData->requestMessageId);
        }
        if ( source->dxl_message_specificData.responseData->serviceInstanceId )
        {
            free((void *)dest->dxl_message_specificData.responseData->serviceInstanceId);
            dest->dxl_message_specificData.responseData->serviceInstanceId =
              strdup(source->dxl_message_specificData.responseData->serviceInstanceId);
        }
    }
    else if ( dest->messageType == DXLMP_EVENT )
    {
        /* Nothing to do here. */
    }
    else if ( dest->messageType == DXLMP_RESPONSE_ERROR )
    {
        /* Request message ID */
        if ( source->dxl_message_specificData.responseErrorData->requestMessageId )
        {
            free((void *)dest->dxl_message_specificData.responseErrorData->requestMessageId);
            dest->dxl_message_specificData.responseErrorData->requestMessageId =
              strdup(source->dxl_message_specificData.responseErrorData->requestMessageId);
        }
        /* Service instance ID */
        if ( source->dxl_message_specificData.responseErrorData->serviceInstanceId )
        {
            free((void *)dest->dxl_message_specificData.responseErrorData->serviceInstanceId);
            dest->dxl_message_specificData.responseErrorData->serviceInstanceId =
              strdup(source->dxl_message_specificData.responseErrorData->serviceInstanceId);
        }
        /* Error code */
        dest->dxl_message_specificData.responseErrorData->code =
            source->dxl_message_specificData.responseErrorData->code;

        /* Error message */
        if ( source->dxl_message_specificData.responseErrorData->errorMessage )
        {
            free((void *)dest->dxl_message_specificData.responseErrorData->errorMessage);
            dest->dxl_message_specificData.responseErrorData->errorMessage =
              strdup(source->dxl_message_specificData.responseErrorData->errorMessage);
        }
    }
    else
    {
        ret = DXLMP_BAD_DATA;
        freeDxlMessage(context,dest);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Version 1
    ////////////////////////////////////////////////////////////////////////////

    if( ret == DXLMP_OK && source->version > 0 )
    {
        /* Other fields */
        copyStringArray(
            source->otherFields, source->otherFieldsCount,
            (char***)&(dest->otherFields), &(dest->otherFieldsCount) );
    }

    ////////////////////////////////////////////////////////////////////////////
    // Version 2
    ////////////////////////////////////////////////////////////////////////////

    if( ret == DXLMP_OK && source->version > 1 )
    {
        /* Source tenant GUID */
        if( source->sourceTenantGuid )
        {
            free( (void*)dest->sourceTenantGuid );
            dest->sourceTenantGuid = strdup(source->sourceTenantGuid);
        }

        /* Destination tenant GUIDs */
        copyStringArray(
            source->tenantGuids, source->tenantGuidCount,
            (char***)&(dest->tenantGuids), &(dest->tenantGuidCount) );
    }

    ////////////////////////////////////////////////////////////////////////////
    // Version 3
    ////////////////////////////////////////////////////////////////////////////

    if( ret == DXLMP_OK && source->version > 2 )
    {
        /* Source tenant GUID */
        if( source->sourceClientInstanceId )
        {
            free( (void*)dest->sourceClientInstanceId );
            dest->sourceClientInstanceId = strdup(source->sourceClientInstanceId);
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // Version 4
    ////////////////////////////////////////////////////////////////////////////

    if( ret == DXLMP_OK && source->version > 3 )
    {
        if ( dest->messageType == DXLMP_REQUEST )
        {
            /* Multi-service request message */
            dest->dxl_message_specificData.requestData->isMultiServiceRequest =
                source->dxl_message_specificData.requestData->isMultiServiceRequest;
        }
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // Common final steps
    ////////////////////////////////////////////////////////////////////////////

    if ( ret == DXLMP_OK )
        *pNewMessge = dest;

    return ret;
}

/******************************************************************************/
dxl_message_error_t setDxlMessageSourceBrokerGuid(
    struct dxl_message_context_t *context, dxl_message_t* message,
    const char* sourceBrokerGuid)
/******************************************************************************/
{
    if (context && context->logger_)
        context->logger_(context->cb_arg_,"Setting message source broker GUID");

    if( message->sourceBrokerGuid )
    {
        free( (void *)message->sourceBrokerGuid );
    }

    message->sourceBrokerGuid =
        strdup(sourceBrokerGuid ? sourceBrokerGuid : "");

    return message->sourceBrokerGuid ? DXLMP_OK : DXLMP_NO_MEMORY;
}

/******************************************************************************/
dxl_message_error_t setDxlMessageSourceClientId(
    struct dxl_message_context_t *context, dxl_message_t* message,
    const char* sourceClientId)
/******************************************************************************/
{
    if (context && context->logger_)
        context->logger_(context->cb_arg_,"Setting message source client id");

    if( message->sourceClientId )
    {
        free( (void *)message->sourceClientId );
    }

    message->sourceClientId =
        strdup(sourceClientId ? sourceClientId : "");

    return message->sourceClientId ? DXLMP_OK : DXLMP_NO_MEMORY;
}

/******************************************************************************/
dxl_message_error_t setDxlMessageSourceClientInstanceId(
    struct dxl_message_context_t *context, dxl_message_t* message,
    const char* sourceClientInstanceId)
/******************************************************************************/
{
    if (context && context->logger_)
        context->logger_(context->cb_arg_,"Setting message source client instance id");

    if( message->sourceClientInstanceId )
    {
        free( (void *)message->sourceClientInstanceId );
    }

    message->sourceClientInstanceId =
        strdup(sourceClientInstanceId ? sourceClientInstanceId : "");

    return message->sourceClientInstanceId ? DXLMP_OK : DXLMP_NO_MEMORY;
}

/******************************************************************************/
void setDxlMessageDestinationTopic(struct dxl_message_context_t* /*context*/,
    dxl_message_t* message, const char* topic)
/******************************************************************************/
{
    message->destinationTopic = strdup(topic);
}

/******************************************************************************/
dxl_message_error_t setDxlMessageBrokerGuids(
    struct dxl_message_context_t *context, dxl_message_t* message,
    const char* brokerGuids[], size_t brokerGuidCount)
/******************************************************************************/
{
    if (context && context->logger_)
        context->logger_(context->cb_arg_,"Setting message broker guids");

    freeStringArray( (char**)message->brokerGuids, message->brokerGuidCount );

    return copyStringArray(
        brokerGuids, brokerGuidCount,
        (char ***)&(message->brokerGuids), &(message->brokerGuidCount) );
}

/******************************************************************************/
dxl_message_error_t setDxlMessageClientGuids(
    struct dxl_message_context_t *context, dxl_message_t* message,
    const char* clientGuids[], size_t clientGuidCount)
/******************************************************************************/
{
    if (context && context->logger_)
        context->logger_(context->cb_arg_,"Setting message client guids");

    freeStringArray( (char**)message->clientGuids, message->clientGuidCount );

    return copyStringArray(
        clientGuids, clientGuidCount,
        (char ***)&(message->clientGuids), &(message->clientGuidCount) );
}

/******************************************************************************/
dxl_message_error_t setDxlMessageOtherFields(
    struct dxl_message_context_t *context, dxl_message_t* message,
    const char* otherFields[], size_t otherFieldsCount)
/******************************************************************************/
{
    if (context && context->logger_)
        context->logger_(context->cb_arg_,"Setting message other fields");

    freeStringArray( (char**)message->otherFields, message->otherFieldsCount );

    return copyStringArray(
        otherFields, otherFieldsCount,
        (char ***)&(message->otherFields), &(message->otherFieldsCount) );
}

/******************************************************************************/
dxl_message_error_t createDxlMessageFromBytes(struct  dxl_message_context_t *context,
    const unsigned char* bytes,
    size_t size, dxl_message_t** message)
/******************************************************************************/
{
    dxl_message_t* dest = NULL;
    const char* messageId = NULL, *clientId = NULL, *sourceBrokerGuid = NULL, 
        *sourceTenantGuid = NULL;
    const size_t unpacker_buffer_size = 2048;
    msgpack_unpacker pac;
    size_t temp = 0;
    uint32_t version = 0;
    int type = 0;
    unsigned char* buffer;
    char** stringArray = NULL;
    size_t stringArraySize = 0;
    dxl_message_error_t result;

    if ( !bytes )
        return DXLMP_INVALID_PARAMS;

    if ( !msgpack_unpacker_init(&pac, unpacker_buffer_size) )
        return DXLMP_INIT_FAILED;

    if ( !msgpack_unpacker_reserve_buffer(&pac, size) )
    {
        msgpack_unpacker_destroy(&pac);
        return DXLMP_NO_MEMORY;
    }

    memcpy(msgpack_unpacker_buffer(&pac), bytes, size);
    msgpack_unpacker_buffer_consumed(&pac, size);

    ////////////////////////////////////////////////////////////////////////////
    // Version 0
    ////////////////////////////////////////////////////////////////////////////

    /* Start reading (version) */
    if ( DXLMP_OK != unpackUInt32(&pac,&version) )
    {
        if (context && context->logger_)
            context->logger_(context->cb_arg_,"Failed to read message version ");

        msgpack_unpacker_destroy(&pac);
        return DXLMP_BAD_DATA;
    }

    /* Next item is message type */
    if ( DXLMP_OK != unpackInt(&pac, (int*)&type) )
    {
        if (context && context->logger_)
            context->logger_(context->cb_arg_,"Failed to get message type");
        msgpack_unpacker_destroy(&pac);
        return DXLMP_BAD_DATA;
    }

    if ( type != DXLMP_REQUEST && type != DXLMP_RESPONSE && type != DXLMP_EVENT && type != DXLMP_RESPONSE_ERROR )
    {
        if (context && context->logger_)
            context->logger_(context->cb_arg_,"Unknown message type");
        msgpack_unpacker_destroy(&pac);
        return DXLMP_BAD_DATA;
    }

    /* Message id */
    if ( DXLMP_OK != unpackRaw(&pac, (unsigned char**)&messageId, &temp, true) )
    {
        if (context && context->logger_)
            context->logger_(context->cb_arg_,"Failed to get message id");
        msgpack_unpacker_destroy(&pac);
        return DXLMP_BAD_DATA;
    }

    /* source client id */
    if ( DXLMP_OK != unpackRaw(&pac, (unsigned char**)&clientId, &temp, true) )
    {
        if (context && context->logger_)
            context->logger_(context->cb_arg_,"Failed to get client id");

        if ( messageId )
            free((void*)messageId);

        msgpack_unpacker_destroy(&pac);
        return DXLMP_BAD_DATA;
    }

    result = createDxlMessage(context,(dxl_message_type_t)type, clientId, NULL, messageId, &dest);

    if ( messageId )
    {
        free((void*)messageId);
        messageId = NULL;
    }

    if ( clientId )
    {
        free((void*)clientId);
        clientId = NULL;
    }

    if ( DXLMP_OK != result )
    {
        if (context && context->logger_)
            context->logger_(context->cb_arg_,"Failed to deserialize message ");
        msgpack_unpacker_destroy(&pac);
        return result;
    }

    /* Source broker GUID */
    if ( DXLMP_OK != unpackRaw(&pac, (unsigned char**)&sourceBrokerGuid, &temp, true) )
    {
        if (context && context->logger_)
            context->logger_(context->cb_arg_,"Failed to get source broker guid");
        freeDxlMessage(context,dest);
        msgpack_unpacker_destroy(&pac);
        return DXLMP_BAD_DATA;
    }
    free( (void*)dest->sourceBrokerGuid );
    dest->sourceBrokerGuid = sourceBrokerGuid;

    /* broker guids */
    if ( DXLMP_OK != unpackStringArray(&pac, &stringArray, &stringArraySize ) )
    {
        freeDxlMessage(context,dest);
        msgpack_unpacker_destroy(&pac);
        return DXLMP_BAD_DATA;
    }
    dest->brokerGuids = (const char**)stringArray;
    dest->brokerGuidCount = stringArraySize;

    /* client guids */
    if ( DXLMP_OK != unpackStringArray(&pac, &stringArray, &stringArraySize ) )
    {
        freeDxlMessage(context,dest);
        msgpack_unpacker_destroy(&pac);
        return DXLMP_BAD_DATA;
    }
    dest->clientGuids = (const char**)stringArray;
    dest->clientGuidCount = stringArraySize;

    /* payload */
    if ( DXLMP_OK != unpackRaw(&pac, &buffer, &temp, false) )
    {
        freeDxlMessage(context,dest);
        msgpack_unpacker_destroy(&pac);
        return DXLMP_BAD_DATA;
    }

    dest->payload = buffer;
    dest->payloadSize = temp;

    if ( dest->messageType == DXLMP_REQUEST || dest->messageType == DXLMP_RESPONSE
            || dest->messageType == DXLMP_RESPONSE_ERROR )
    {
        /* First thing for all these is a string */
        if ( DXLMP_OK != unpackRaw(&pac, &buffer, &temp, true) )
        {
            if (context && context->logger_)
                context->logger_(context->cb_arg_,"Failed to extracting message payload");
            msgpack_unpacker_destroy(&pac);
            freeDxlMessage(context,dest);
            return DXLMP_BAD_DATA;
        }

        switch ( dest->messageType )
        {
        case DXLMP_REQUEST:
            free((void *)dest->dxl_message_specificData.requestData->replyToTopic);
            dest->dxl_message_specificData.requestData->replyToTopic = (char*)buffer;
            break;
        case DXLMP_RESPONSE:
            free((void *)dest->dxl_message_specificData.responseData->requestMessageId);
            dest->dxl_message_specificData.responseData->requestMessageId = (char*)buffer;
            break;
        case DXLMP_RESPONSE_ERROR:
            free((void *)dest->dxl_message_specificData.responseErrorData->requestMessageId);
            dest->dxl_message_specificData.responseErrorData->requestMessageId = (char*)buffer;
            break;
        default:
            break;
        }

        if ( type == DXLMP_REQUEST)
        {
            /* service instance id */
            if ( DXLMP_OK != unpackRaw(&pac, &buffer, &temp, true) )
            {
                freeDxlMessage(context,dest);
                msgpack_unpacker_destroy(&pac);
                return DXLMP_BAD_DATA;
            }
            free((void *)dest->dxl_message_specificData.requestData->serviceInstanceId);
            dest->dxl_message_specificData.requestData->serviceInstanceId = (char*)buffer;
        }
        else if ( type == DXLMP_RESPONSE )
        {

            /* service instance id */
            if ( DXLMP_OK != unpackRaw(&pac, &buffer, &temp, true) )
            {
                freeDxlMessage(context,dest);
                msgpack_unpacker_destroy(&pac);
                return DXLMP_BAD_DATA;
            }
            free((void *)dest->dxl_message_specificData.responseData->serviceInstanceId);
            dest->dxl_message_specificData.responseData->serviceInstanceId = (char*)buffer;
        }
        else if ( type == DXLMP_RESPONSE_ERROR )
        {
            int code = 0;
            /* service instance id */
            if ( DXLMP_OK != unpackRaw(&pac, &buffer, &temp, true) )
            {
                freeDxlMessage(context,dest);
                msgpack_unpacker_destroy(&pac);
                return DXLMP_BAD_DATA;
            }
            free((void *)dest->dxl_message_specificData.responseErrorData->serviceInstanceId);
            dest->dxl_message_specificData.responseErrorData->serviceInstanceId = (char*)buffer;

            /* error code */
            if ( DXLMP_OK != unpackInt(&pac, &code) )
            {
                freeDxlMessage(context,dest);
                msgpack_unpacker_destroy(&pac);
                return DXLMP_BAD_DATA;
            }
            dest->dxl_message_specificData.responseErrorData->code = code;

            /* error message */
            if ( DXLMP_OK != unpackRaw(&pac, &buffer, &temp, true) )
            {
                freeDxlMessage(context,dest);
                msgpack_unpacker_destroy(&pac);
                return DXLMP_BAD_DATA;
            }
            free((void *)dest->dxl_message_specificData.responseErrorData->errorMessage);
            dest->dxl_message_specificData.responseErrorData->errorMessage = (char*)buffer;
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // Version 1
    ////////////////////////////////////////////////////////////////////////////

    if( version > 0 )
    {
        /* other fields */
        if ( DXLMP_OK != unpackStringArray(&pac, &stringArray, &stringArraySize ) )
        {
            freeDxlMessage(context,dest);
            msgpack_unpacker_destroy(&pac);
            return DXLMP_BAD_DATA;
        }

        dest->otherFields = (const char**)stringArray;
        dest->otherFieldsCount = stringArraySize;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Version 2
    ////////////////////////////////////////////////////////////////////////////

    if ( version > 1 )
    {
        /* Source tenant GUID */
        if ( DXLMP_OK != unpackRaw(&pac, (unsigned char**)&sourceTenantGuid, &temp, true) )
        {
            if (context && context->logger_)
                context->logger_(context->cb_arg_,"Failed to get source tenant guid");
            freeDxlMessage(context,dest);
            msgpack_unpacker_destroy(&pac);
            return DXLMP_BAD_DATA;
        }
        free( (void*)dest->sourceTenantGuid );
        dest->sourceTenantGuid = sourceTenantGuid;

        /* Destination tenant GUIDs. */
        if ( DXLMP_OK != unpackStringArray(&pac, &stringArray, &stringArraySize ) )
        {
            freeDxlMessage(context, dest);
            msgpack_unpacker_destroy(&pac);
            return DXLMP_BAD_DATA;
        }
        dest->tenantGuids = (const char**)stringArray;
        dest->tenantGuidCount = stringArraySize;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Version 3
    ////////////////////////////////////////////////////////////////////////////

    if ( version > 2 )
    {
        const char* sourceClientInstanceId;
        /* Source client instance identifier */
        if ( DXLMP_OK != unpackRaw(&pac, (unsigned char**)&sourceClientInstanceId, &temp, true) )
        {
            if (context && context->logger_)
                context->logger_(context->cb_arg_,"Failed to get client instance identifier");
            freeDxlMessage(context,dest);
            msgpack_unpacker_destroy(&pac);
            return DXLMP_BAD_DATA;
        }
        free( (void*)dest->sourceClientInstanceId );
        dest->sourceClientInstanceId = sourceClientInstanceId;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Version 4
    ////////////////////////////////////////////////////////////////////////////

    if ( version > 3 )
    {
        if ( type == DXLMP_REQUEST)
        {
            if ( DXLMP_OK != unpackInt(&pac, 
                    (int*)&(dest->dxl_message_specificData.requestData->isMultiServiceRequest)) )
            {
                if (context && context->logger_)
                    context->logger_(context->cb_arg_,
                        "Failed to get multi-service request flag");
                msgpack_unpacker_destroy(&pac);
                return DXLMP_BAD_DATA;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // Common final steps
    ////////////////////////////////////////////////////////////////////////////

    msgpack_unpacker_destroy(&pac);
    *message = dest;

    return DXLMP_OK;
}

/******************************************************************************/
dxl_message_error_t dxlMessageToBytes( struct  dxl_message_context_t *context,
    unsigned char** bytes,
    size_t* size, dxl_message_t* message, int stripClientGuids)
/******************************************************************************/
{
    size_t i;
    size_t temp;

    /* msgpack::sbuffer is a simple buffer implementation. */
    msgpack_sbuffer simple_buffer;
    msgpack_sbuffer *buffer = &simple_buffer;
    msgpack_sbuffer_init(buffer);

    /* serialize values into the buffer using msgpack_sbuffer_write callback function. */
    msgpack_packer packer;
    msgpack_packer *pk = &packer;
    msgpack_packer_init(pk, buffer, msgpack_sbuffer_write);    

    ////////////////////////////////////////////////////////////////////////////
    // Version 0
    ////////////////////////////////////////////////////////////////////////////

    /* Pack version and type */
    msgpack_pack_int32(pk,message->version);
    msgpack_pack_int8(pk, (unsigned char)message->messageType);

    /* Next comes message id */
    temp = strlen(message->messageId);
    msgpack_pack_v4raw(pk,temp);
    msgpack_pack_v4raw_body(pk,temp ? message->messageId : NULL, temp);

    /* client ID */
    temp = strlen(message->sourceClientId);
    msgpack_pack_v4raw(pk,temp);
    msgpack_pack_v4raw_body(pk,temp ? message->sourceClientId : NULL, temp);

    /* source broker GUID */
    temp = strlen(message->sourceBrokerGuid);
    msgpack_pack_v4raw(pk,temp);
    msgpack_pack_v4raw_body(pk,temp ? message->sourceBrokerGuid : NULL, temp);

    /* Broker GUIDs */
    msgpack_pack_array(pk, message->brokerGuidCount);
    for( i = 0; i < message->brokerGuidCount; i++ )
    {
        temp = strlen(message->brokerGuids[i]);
        msgpack_pack_v4raw(pk, temp);
        msgpack_pack_v4raw_body(pk, message->brokerGuids[i], temp);
    }

    /* Client GUIDs */
    if (context && context->logger_)
        context->logger_(context->cb_arg_,"Packing client guids");
    if( stripClientGuids )
    {
        msgpack_pack_array(pk, 0);
    }
    else
    {
        msgpack_pack_array(pk, message->clientGuidCount);
        for( i = 0; i < message->clientGuidCount; i++ )
        {
            temp = strlen(message->clientGuids[i]);
            msgpack_pack_v4raw(pk, temp);
            msgpack_pack_v4raw_body(pk, message->clientGuids[i], temp);
        }
    }

    /* payload */
    msgpack_pack_v4raw(pk,message->payloadSize);
    msgpack_pack_v4raw_body(pk,message->payloadSize ? message->payload : NULL, message->payloadSize);

    /* And now message type specific stuff */
    if ( message->messageType == DXLMP_REQUEST )
    {
        temp = strlen(message->dxl_message_specificData.requestData->replyToTopic);
        msgpack_pack_v4raw(pk,temp);
        msgpack_pack_v4raw_body(pk,temp ? message->dxl_message_specificData.requestData->replyToTopic : NULL, temp);

        temp = strlen(message->dxl_message_specificData.requestData->serviceInstanceId);
        msgpack_pack_v4raw(pk,temp);
        msgpack_pack_v4raw_body(pk,temp ? message->dxl_message_specificData.requestData->serviceInstanceId : NULL, temp);
    }
    else if ( message->messageType == DXLMP_RESPONSE )
    {
        /* Request message ID */
        temp = strlen(message->dxl_message_specificData.responseData->requestMessageId);
        msgpack_pack_v4raw(pk,temp);
        msgpack_pack_v4raw_body(pk,temp ? message->dxl_message_specificData.responseData->requestMessageId : NULL, temp);

        /* Service instance ID */
        temp = strlen(message->dxl_message_specificData.responseData->serviceInstanceId);
        msgpack_pack_v4raw(pk,temp);
        msgpack_pack_v4raw_body(pk,temp ? message->dxl_message_specificData.responseData->serviceInstanceId : NULL, temp);
    }
    else if ( message->messageType == DXLMP_RESPONSE_ERROR )
    {
        /* Request message ID */
        temp = strlen(message->dxl_message_specificData.responseErrorData->requestMessageId);
        msgpack_pack_v4raw(pk,temp);
        msgpack_pack_v4raw_body(pk,temp ? message->dxl_message_specificData.responseErrorData->requestMessageId : NULL, temp);

        /* Service instance ID */
        temp = strlen(message->dxl_message_specificData.responseErrorData->serviceInstanceId);
        msgpack_pack_v4raw(pk,temp);
        msgpack_pack_v4raw_body(pk,temp ? message->dxl_message_specificData.responseErrorData->serviceInstanceId : NULL, temp);

        /* Error code */
        msgpack_pack_int32(pk,message->dxl_message_specificData.responseErrorData->code);

        /* Error text */
        temp = strlen(message->dxl_message_specificData.responseErrorData->errorMessage);
        msgpack_pack_v4raw(pk,temp);
        msgpack_pack_v4raw_body(pk,temp ? message->dxl_message_specificData.responseErrorData->errorMessage : NULL, temp);
    }

    /* Nothing to do for event */

    ////////////////////////////////////////////////////////////////////////////
    // Version 1
    ////////////////////////////////////////////////////////////////////////////

    if ( message->version > 0 )
    {
        /* Other Fields */
        if (context && context->logger_)
            context->logger_(context->cb_arg_,"Packing other fields");

        msgpack_pack_array(pk, message->otherFieldsCount);
        for( i = 0; i < message->otherFieldsCount; i++ )
        {
            temp = strlen(message->otherFields[i]);
            msgpack_pack_v4raw(pk, temp);
            msgpack_pack_v4raw_body(pk, message->otherFields[i], temp);
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // Version 2
    ////////////////////////////////////////////////////////////////////////////

    if ( message->version > 1 )
    {
        /* Source tenant GUID */
        temp = strlen(message->sourceTenantGuid);
        msgpack_pack_v4raw(pk,temp);
        msgpack_pack_v4raw_body(pk,temp ? message->sourceTenantGuid : NULL, temp);

        /* Destination tenant GUIDs */
        msgpack_pack_array(pk, message->tenantGuidCount);
        for( i = 0; i < message->tenantGuidCount; i++ )
        {
            temp = strlen(message->tenantGuids[i]);
            msgpack_pack_v4raw(pk, temp);
            msgpack_pack_v4raw_body(pk, message->tenantGuids[i], temp);
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // Version 3
    ////////////////////////////////////////////////////////////////////////////

    if ( message->version > 2 )
    {
        /* Source client instance identifier */
        temp = strlen(message->sourceClientInstanceId);
        msgpack_pack_v4raw(pk,temp);
        msgpack_pack_v4raw_body(pk,temp ? message->sourceClientInstanceId : NULL, temp);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Version 4
    ////////////////////////////////////////////////////////////////////////////

    if ( message->version > 3 )
    {
        if ( message->messageType == DXLMP_REQUEST )
        {
            /* Multi-service request message */
            msgpack_pack_int8(pk, (unsigned char)
                message->dxl_message_specificData.requestData->isMultiServiceRequest);
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // Common final steps
    ////////////////////////////////////////////////////////////////////////////

    /* Copy data */
    *bytes = (unsigned char*)malloc(buffer->size);
    memcpy((void*)*bytes, buffer->data, buffer->size);

    *size = buffer->size;
    msgpack_sbuffer_destroy(buffer);
    if (context && context->logger_)
        context->logger_(context->cb_arg_,"Message serialized ");

    return DXLMP_OK;
}

/*******************************************************************************/
dxl_message_error_t generateMessageId(const char** messageId)
/******************************************************************************/
{
    const unsigned int messageSize = 40u;
    *messageId = (char *)malloc(messageSize * sizeof(char));

    if (!*messageId)
        return DXLMP_NO_MEMORY;

    // Create a guid using OS libuuid mechanism (v2.17.2).
    uuid_t newGuid;
    uuid_generate_random(newGuid);
    uuid_unparse_lower(newGuid, (char*)*messageId);

    // If there was an error creating messageId it will be freed in "freeDxlMessage" method.
    return DXLMP_OK;
}

/******************************************************************************/
dxl_message_error_t unpackUInt32(msgpack_unpacker *unpack, uint32_t * value)
/******************************************************************************/
{
    dxl_message_error_t ret = DXLMP_BAD_DATA;

    msgpack_unpacked result;
    msgpack_unpacked_init(&result);

    /* Unpack */
    if ( !msgpack_unpacker_next(unpack,&result) )
    {
        msgpack_unpacked_destroy(&result);
        return DXLMP_INIT_FAILED;
    }

    if ( result.data.type != MSGPACK_OBJECT_POSITIVE_INTEGER )
        return DXLMP_BAD_DATA;

    *value = (uint32_t)result.data.via.u64;

    ret = DXLMP_OK;

    msgpack_unpacked_destroy(&result);

    return ret;
}

/******************************************************************************/
dxl_message_error_t unpackInt( msgpack_unpacker *unpack, int* value)
/******************************************************************************/
{
    dxl_message_error_t ret = DXLMP_BAD_DATA;

    msgpack_unpacked result;
    msgpack_unpacked_init(&result);

    /* Unpack */
    if ( !msgpack_unpacker_next(unpack,&result) )
    {
        msgpack_unpacked_destroy(&result);
        return DXLMP_INIT_FAILED;
    }

    if ( result.data.type != MSGPACK_OBJECT_POSITIVE_INTEGER &&
        result.data.type != MSGPACK_OBJECT_NEGATIVE_INTEGER )
    {
        msgpack_unpacked_destroy(&result);
        return DXLMP_BAD_DATA;
    }


    *value = (int)result.data.via.i64;

    ret = DXLMP_OK;

    msgpack_unpacked_destroy(&result);

    return ret;
}

/******************************************************************************/
dxl_message_error_t unpackRaw(msgpack_unpacker *unpack,
    unsigned char** bufferOut, size_t* bufferSize, bool addNullTerminator)
/******************************************************************************/
{
    msgpack_unpacked result;
    msgpack_unpacked_init(&result);

    /* Unpack */
    if ( !msgpack_unpacker_next(unpack,&result) )
    {
        msgpack_unpacked_destroy(&result);
        return DXLMP_INIT_FAILED;
    }

    if ( result.data.type != MSGPACK_OBJECT_STR /*RAW in previous versions*/ )
    {
        msgpack_unpacked_destroy(&result);
        return DXLMP_BAD_DATA;
    }

    *bufferOut = (unsigned char*)calloc(result.data.via.bin.size + (addNullTerminator ? 1 : 0), 1);
    if ( !*bufferOut )
    {
        msgpack_unpacked_destroy(&result);
        return DXLMP_NO_MEMORY;
    }

    memcpy(*bufferOut, result.data.via.bin.ptr, result.data.via.bin.size);
    *bufferSize = result.data.via.bin.size + (addNullTerminator ? 1 : 0);

    msgpack_unpacked_destroy(&result);

    return DXLMP_OK;
}

/******************************************************************************/
static dxl_message_error_t unpackStringArray(msgpack_unpacker *unpack,
    char* (*stringArray[]), size_t* stringArraySize)
/******************************************************************************/
{
    size_t i;
    size_t mpArraySize;
    msgpack_unpacked result;
    msgpack_unpacked_init(&result);

    *stringArray = NULL;
    *stringArraySize = 0;

    /* Unpack */
    if ( !msgpack_unpacker_next(unpack,&result) )
    {
        msgpack_unpacked_destroy(&result);
        return DXLMP_INIT_FAILED;
    }

    if ( result.data.type != MSGPACK_OBJECT_ARRAY )
    {
        msgpack_unpacked_destroy(&result);
        return DXLMP_BAD_DATA;
    }

    mpArraySize = result.data.via.array.size;
    if( mpArraySize > 0 )
    {
        *stringArray = (char**)calloc(mpArraySize, sizeof(char*));
        if( !(*stringArray) )
        {
            msgpack_unpacked_destroy( &result );
            return DXLMP_NO_MEMORY;
        }

        for( i = 0; i < mpArraySize; i++ )
        {
            (*stringArray)[i] = (char*)calloc(result.data.via.array.ptr[i].via.bin.size+1,sizeof(char));
            if( !((*stringArray)[i]) )
            {
                freeStringArray( *stringArray, i );
                msgpack_unpacked_destroy( &result );
                *stringArray = NULL;
                *stringArraySize = 0;
                return DXLMP_NO_MEMORY;
            }
            memcpy((*stringArray)[i],
                result.data.via.array.ptr[i].via.bin.ptr,
                result.data.via.array.ptr[i].via.bin.size);
        }
        *stringArraySize = mpArraySize;
    }

    msgpack_unpacked_destroy(&result);

    return DXLMP_OK;
}

/******************************************************************************/
static void freeStringArray( char* strArray[], size_t strArraySize )
/******************************************************************************/
{
    size_t i;
    if( strArray && strArraySize > 0 )
    {
        for( i = 0; i < strArraySize; i++ )
        {
            free( strArray[i] );
        }
        free( strArray );
    }
}

/******************************************************************************/
static dxl_message_error_t copyStringArray(
    const char* sourceArray[], size_t sourceArraySize,
    char *(*destArray[]), size_t* destArraySize )
/******************************************************************************/
{
    size_t i;

    *destArray = NULL;
    *destArraySize = 0;
    if( sourceArray && sourceArraySize > 0 )
    {
        *destArray = (char**)calloc(sourceArraySize, sizeof(char*));
        *destArraySize = sourceArraySize;
        if( !(*destArray ) )
        {
            return DXLMP_NO_MEMORY;
        }
        for( i = 0; i < sourceArraySize; i++ )
        {
            (*destArray)[i] = strdup( sourceArray[i] );
            if( !(*destArray)[i] )
            {
                freeStringArray( *destArray, i );
                *destArray = NULL;
                *destArraySize = 0;
                return DXLMP_NO_MEMORY;
            }
        }
    }
    return DXLMP_OK;
}


/******************************************************************************/
void freeDxlMessageAllocatedData(struct  dxl_message_context_t* /*context*/,
    void* pData)
/******************************************************************************/
{
    if ( pData )
        free(pData);
}

/******************************************************************************/
void freeDxlMessage(struct  dxl_message_context_t* /*context*/,
    dxl_message_t* message)
/******************************************************************************/
{
    if ( message )
    {

    ////////////////////////////////////////////////////////////////////////////
    // Version 0
    ////////////////////////////////////////////////////////////////////////////

        if ( message->messageType == DXLMP_REQUEST )
        {
            if ( message->dxl_message_specificData.requestData->replyToTopic )
                free((void*)message->dxl_message_specificData.requestData->replyToTopic);

            if ( message->dxl_message_specificData.requestData->serviceInstanceId )
                free((void*)message->dxl_message_specificData.requestData->serviceInstanceId);

            free(message->dxl_message_specificData.requestData);
        }
        else if ( message->messageType == DXLMP_RESPONSE )
        {
            if ( message->dxl_message_specificData.responseData->requestMessageId )
                free((void*)message->dxl_message_specificData.responseData->requestMessageId);

            if ( message->dxl_message_specificData.responseData->serviceInstanceId )
                free((void*)message->dxl_message_specificData.responseData->serviceInstanceId);

            free(message->dxl_message_specificData.responseData);
        }
        else if ( message->messageType == DXLMP_RESPONSE_ERROR )
        {
            if ( message->dxl_message_specificData.responseErrorData->serviceInstanceId )
                free((void*)message->dxl_message_specificData.responseErrorData->serviceInstanceId);

            if ( message->dxl_message_specificData.responseErrorData->errorMessage )
                free((void*)message->dxl_message_specificData.responseErrorData->errorMessage);

            if ( message->dxl_message_specificData.responseErrorData->requestMessageId )
                free((void*)message->dxl_message_specificData.responseErrorData->requestMessageId);

            free(message->dxl_message_specificData.responseErrorData);
        }

        if ( message->destinationTopic )
            free((void*)message->destinationTopic);

        if ( message->messageId )
            free((void*)message->messageId);

        if ( message->payload )
            free((void*)message->payload);

        if( message->sourceClientId )
            free((void*)message->sourceClientId);
        if( message->sourceBrokerGuid )
            free((void*)message->sourceBrokerGuid);

        freeStringArray( (char **)message->brokerGuids, message->brokerGuidCount );
        freeStringArray( (char **)message->clientGuids, message->clientGuidCount );

    ////////////////////////////////////////////////////////////////////////////
    // Version 1
    ////////////////////////////////////////////////////////////////////////////

        freeStringArray( (char **)message->otherFields, message->otherFieldsCount );

    ////////////////////////////////////////////////////////////////////////////
    // Version 2
    ////////////////////////////////////////////////////////////////////////////

        if( message->sourceTenantGuid )
            free((void*)message->sourceTenantGuid);

        freeStringArray( (char **)message->tenantGuids, message->tenantGuidCount );

    ////////////////////////////////////////////////////////////////////////////
    // Version 3
    ////////////////////////////////////////////////////////////////////////////

        if( message->sourceClientInstanceId )
            free((void*)message->sourceClientInstanceId);

    ////////////////////////////////////////////////////////////////////////////
    // Common final step
    ////////////////////////////////////////////////////////////////////////////

        free(message);
    }
}

/******************************************************************************/
dxl_message_error_t setDxlMessagePayload(struct  dxl_message_context_t* /*context*/,
    dxl_message_t* message, const unsigned char* buffer,
    size_t size)
/******************************************************************************/
{
    if ( message->payload )
        free((void*)message->payload);

    if ( size )
    {
        message->payload = (unsigned char*)malloc(size);
        message->payloadSize = size;
        memcpy((void*)message->payload, buffer, size);
    }

    return DXLMP_OK;
}

/******************************************************************************/
dxl_message_error_t setDxlRequestMessageAttributes(struct  dxl_message_context_t* /*context*/,
    dxl_message_t* message,
    enum dxl_request_attribute_t type,
    const char* value)
/******************************************************************************/
{
    if ( message->messageType != DXLMP_REQUEST )
        return DXLMP_WRONG_MESSAGE_TYPE;

    switch(type)
    {
    case reply_to_topic:
        if ( message->dxl_message_specificData.requestData->replyToTopic )
            free((void*)message->dxl_message_specificData.requestData->replyToTopic);

        message->dxl_message_specificData.requestData->replyToTopic = strdup(value);
        break;
    case service_instance_id:
        if ( message->dxl_message_specificData.requestData->serviceInstanceId )
            free((void*)message->dxl_message_specificData.requestData->serviceInstanceId);

        message->dxl_message_specificData.requestData->serviceInstanceId = strdup(value);
        break;
    }
    return DXLMP_OK;
}

/******************************************************************************/
dxl_message_error_t setDxlRequestMultiService(struct dxl_message_context_t* /*context*/,
    dxl_message_t* message, bool isMultiService)
/******************************************************************************/
{
    if ( message->messageType != DXLMP_REQUEST )
        return DXLMP_WRONG_MESSAGE_TYPE;

    message->dxl_message_specificData.requestData->isMultiServiceRequest = isMultiService;

    return DXLMP_OK;
}


/******************************************************************************/
dxl_message_error_t setDxlResponseMessageAttributes(struct  dxl_message_context_t* /*context*/,
    dxl_message_t* message,
    const char* requestMessageId)
/******************************************************************************/
{
    if ( message->messageType != DXLMP_RESPONSE &&
        message->messageType != DXLMP_RESPONSE_ERROR )
    {
        return DXLMP_WRONG_MESSAGE_TYPE;
    }

    if ( message->messageType == DXLMP_RESPONSE )
    {
        if ( message->dxl_message_specificData.responseData->requestMessageId )
            free((void*)message->dxl_message_specificData.responseData->requestMessageId);

        message->dxl_message_specificData.responseData->requestMessageId = strdup(requestMessageId);
    }

    if ( message->messageType == DXLMP_RESPONSE_ERROR )
    {
        if ( message->dxl_message_specificData.responseErrorData->requestMessageId )
            free((void*)message->dxl_message_specificData.responseErrorData->requestMessageId);

        message->dxl_message_specificData.responseErrorData->requestMessageId = strdup(requestMessageId);
    }

    return DXLMP_OK;
}

/******************************************************************************/
dxl_message_error_t setDxlResponseMessageServiceId(struct dxl_message_context_t* /*context*/,
    dxl_message_t* message,
    const char* serviceId)
/******************************************************************************/
{
    if ( message->messageType != DXLMP_RESPONSE &&
        message->messageType != DXLMP_RESPONSE_ERROR )
    {
        return DXLMP_WRONG_MESSAGE_TYPE;
    }

    if ( message->messageType == DXLMP_RESPONSE )
    {
        if ( message->dxl_message_specificData.responseData->serviceInstanceId )
            free((void*)message->dxl_message_specificData.responseData->serviceInstanceId);

        message->dxl_message_specificData.responseData->serviceInstanceId = strdup(serviceId);
    }

    if ( message->messageType == DXLMP_RESPONSE_ERROR )
    {
        if ( message->dxl_message_specificData.responseErrorData->serviceInstanceId )
            free((void*)message->dxl_message_specificData.responseErrorData->serviceInstanceId);

        message->dxl_message_specificData.responseErrorData->serviceInstanceId = strdup(serviceId);
    }

    return DXLMP_OK;
}

/******************************************************************************/
dxl_message_error_t setDxlErrorResponseMessageAttributes(struct  dxl_message_context_t* /*context*/,
    dxl_message_t* message,
    int errorCode, const char* errorMessage)
/******************************************************************************/
{
    if ( message->messageType != DXLMP_RESPONSE_ERROR )
        return DXLMP_WRONG_MESSAGE_TYPE;

    message->dxl_message_specificData.responseErrorData->code = errorCode;

    if ( message->dxl_message_specificData.responseErrorData->errorMessage )
        free((void*)message->dxl_message_specificData.responseErrorData->errorMessage);

    message->dxl_message_specificData.responseErrorData->errorMessage = errorMessage ? strdup(errorMessage) : strdup("");

    return DXLMP_OK;
}

/* Provide api to release bytes, to prevent memory operations from crossing dll boundaries
The buffer was allocated by dxlMessageToBytes
*/
/******************************************************************************/
dxl_message_error_t dxlMessageReleaseSerializedBytes( struct  dxl_message_context_t* /*context*/,
    unsigned char* bytes)
/******************************************************************************/
{
    free(bytes);
    return DXLMP_OK;
}

/******************************************************************************/
dxl_message_error_t setDxlMessageSourceTenantGuid(
    struct dxl_message_context_t *context, dxl_message_t* message,
    const char* sourceTenantGuid)
/******************************************************************************/
{
    if ( context && context->logger_ )
        context->logger_(context->cb_arg_,"Setting message source tenant GUID");

    if ( message->sourceTenantGuid )
    {
        free((void *)message->sourceTenantGuid);
    }

    message->sourceTenantGuid =
        strdup(sourceTenantGuid ? sourceTenantGuid : "");

    return message->sourceTenantGuid ? DXLMP_OK : DXLMP_NO_MEMORY;
}

/******************************************************************************/
dxl_message_error_t setDxlMessageTenantGuids(
    struct dxl_message_context_t *context, dxl_message_t* message,
    const char* tenantGuids[], size_t tenantGuidCount)
/******************************************************************************/
{
    if ( context && context->logger_ )
        context->logger_(context->cb_arg_,"Setting message tenant guids");

    freeStringArray((char**)message->tenantGuids, message->tenantGuidCount);

    return copyStringArray(
        tenantGuids, tenantGuidCount,
        (char ***)&(message->tenantGuids), &(message->tenantGuidCount));
}
