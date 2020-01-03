/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#pragma once

/* DXL Message packer structs and enums. Skim through dxlMessagePacker.h, then come here */

#if defined __cplusplus
extern "C"
{
#endif

/* Error IDs returned by the dxl message packer */
typedef enum dxl_message_error_t
{
    DXLMP_OK                           = 0,
    DXLMP_INIT_FAILED                  = 1,
    DXLMP_INVALID_OP_FOR_MESSAGE_TYPE  = 2,
    DXLMP_NOT_IMPLEMENTED              = 3,
    DXLMP_BAD_DATA                     = 4,
    DXLMP_WRONG_MESSAGE_TYPE           = 5,
    DXLMP_NO_MEMORY                    = 6,
    DXLMP_INVALID_PARAMS               = 7
} dxl_message_error_t; 

/* Message Types */
typedef enum dxl_message_type_t
{
    DXLMP_REQUEST           = 0,
    DXLMP_RESPONSE          = 1,
    DXLMP_EVENT             = 2,
    DXLMP_RESPONSE_ERROR    = 3
} dxl_message_type_t; 

/*
 * NOTE: These message specific structs should be use through
 * the dxl_message_t struct (via dxl_message_specificData member)
 */

/*
 * Request specific message data. Access this through dxl_message_t. Use 'set' method to set
 * data. Data is copied.
 */
typedef struct dxl_message_request_t
{
    const char* replyToTopic;
    const char *serviceInstanceId;
    bool isMultiServiceRequest;
} dxl_message_request_t; 

/*
 * Response specific message data. Access this through dxl_message_t. Use 'set' method to set
 * data. Data is copied.
 */
typedef struct dxl_message_response_t
{
    const char* requestMessageId;
    const char* serviceInstanceId;
} dxl_message_response_t; 

/*
 * Response error specific message data. Access this through dxl_message_t. Use 'set' method to set
 * data. Data is copied.
 */
typedef struct dxl_message_response_error_t
{
    const char* requestMessageId;
    const char* serviceInstanceId;
    const char* errorMessage;
    int code;
} dxl_message_response_error_t; 

/*
 * This is the main message structure. Use appropriate create/set methods for populating and
 * use freeDxlMessage to free this message. Data is copied in 'set' functions, and is freed
 * in freeDxlMessage
 */
typedef struct dxl_message_t
{
    ////////////////////////////////////////////////////////////////////////////
    // Version 0
    ////////////////////////////////////////////////////////////////////////////
    dxl_message_type_t messageType;     /* The type of this message */
    unsigned int version;               /* Version */
    const char* messageId;              /* Unique message ID */
    const char* sourceClientId;         /* ID of the source client */
    const char* sourceBrokerGuid;       /* GUID of the source broker */
    const char** brokerGuids;           /* The GUIDs of brokers to deliver the message to */
    size_t brokerGuidCount;             /* The count of broker GUIDs */
    const char** clientGuids;           /* The GUIDs of clients to deliver the message to */
    size_t clientGuidCount;             /* The count of client GUIDs */
    const unsigned char* payload;       /* payload data */
    size_t payloadSize;                 /* payload size */
    const char* destinationTopic;       /* destination topic */
    void* userData;                     /* Any user data you want to store */

    /* union of message specific data  */
    union 
    {
        dxl_message_request_t* requestData;                 /* Request specific stuff  */
        dxl_message_response_t* responseData;               /* Response specific stuff */
        dxl_message_response_error_t* responseErrorData;    /* response error stuff  */
    } dxl_message_specificData; 

    ////////////////////////////////////////////////////////////////////////////
    // Version 1
    ////////////////////////////////////////////////////////////////////////////
    size_t otherFieldsCount;                    /* The count of other message fields */
    const char** otherFields;                   /* Future use - way to add fields to message types */

    ////////////////////////////////////////////////////////////////////////////
    // Version 2
    ////////////////////////////////////////////////////////////////////////////
    const char* sourceTenantGuid;               /* GUID of the source tenant */
    size_t tenantGuidCount;                     /* The count of tenant GUIDs */
    const char** tenantGuids;                   /* The GUIDs of tenants to deliver the message to */

    ////////////////////////////////////////////////////////////////////////////
    // Version 3
    ////////////////////////////////////////////////////////////////////////////
    const char* sourceClientInstanceId;         /* Instance ID of the source client */

} dxl_message_t; /* struct dxl_message_t */

#if defined __cplusplus
}
#endif
