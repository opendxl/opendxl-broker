/*
Copyright (c) 2010-2014 Roger Light <roger@atchoo.org>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. Neither the name of mosquitto nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.


This product includes software developed by the OpenSSL Project for use in the
OpenSSL Toolkit. (http://www.openssl.org/)
This product includes cryptographic software written by Eric Young
(eay@cryptsoft.com)
This product includes software written by Tim Hudson (tjh@cryptsoft.com)
*/

#ifndef _MOSQUITTO_H_
#define _MOSQUITTO_H_

#ifdef __cplusplus
extern "C" {
#endif

#    ifndef __cplusplus
#        include <stdbool.h>
#    endif

#define LIBMOSQUITTO_MAJOR 1
#define LIBMOSQUITTO_MINOR 3
#define LIBMOSQUITTO_REVISION 5
/* LIBMOSQUITTO_VERSION_NUMBER looks like 1002001 for e.g. version 1.2.1. */
#define LIBMOSQUITTO_VERSION_NUMBER (LIBMOSQUITTO_MAJOR*1000000+LIBMOSQUITTO_MINOR*1000+LIBMOSQUITTO_REVISION)

/* Log types */
#define MOSQ_LOG_NONE 0x00
#define MOSQ_LOG_INFO 0x01
#define MOSQ_LOG_NOTICE 0x02
#define MOSQ_LOG_WARNING 0x04
#define MOSQ_LOG_ERR 0x08
#define MOSQ_LOG_DEBUG 0x10
#define MOSQ_LOG_SUBSCRIBE 0x20
#define MOSQ_LOG_UNSUBSCRIBE 0x40
#define MOSQ_LOG_ALL 0xFFFF

/* Error values */
enum mosq_err_t {
    MOSQ_ERR_CONN_PENDING = -1,
    MOSQ_ERR_SUCCESS = 0,
    MOSQ_ERR_NOMEM = 1,
    MOSQ_ERR_PROTOCOL = 2,
    MOSQ_ERR_INVAL = 3,
    MOSQ_ERR_NO_CONN = 4,
    MOSQ_ERR_CONN_REFUSED = 5,
    MOSQ_ERR_NOT_FOUND = 6,
    MOSQ_ERR_CONN_LOST = 7,
    MOSQ_ERR_TLS = 8,
    MOSQ_ERR_PAYLOAD_SIZE = 9,
    MOSQ_ERR_NOT_SUPPORTED = 10,
    MOSQ_ERR_AUTH = 11,
    MOSQ_ERR_ACL_DENIED = 12,
    MOSQ_ERR_UNKNOWN = 13,
    MOSQ_ERR_ERRNO = 14,
    MOSQ_ERR_EAI = 15
};

/* MQTT specification restricts client ids to a maximum of 23 characters */
#define MOSQ_MQTT_ID_MAX_LENGTH 23

struct mosquitto_message{
    int mid;
    char *topic;
    void *payload;
    int payloadlen;
    // DXL Start
    void *client_payload;
    int client_payloadlen;
    // DXL End
    int qos;
    bool retain;
};

struct mosquitto;

/* =============================================================================
 *
 * Section: Utility functions
 *
 * =============================================================================
 */

/*
 * Function: mosquitto_topic_matches_sub
 *
 * Check whether a topic matches a subscription.
 *
 * For example:
 *
 * foo/bar would match the subscription foo/# or +/bar
 * non/matching would not match the subscription non/+/+
 *
 * Parameters:
 *      sub -       subscription string to check topic against.
 *      topic -     topic to check.
 *      result -    bool pointer to hold result. Will be set to true if the topic
 *                  matches the subscription.
 *
 * Returns:
 *      MOSQ_ERR_SUCCESS -  on success
 *      MOSQ_ERR_INVAL -    if the input parameters were invalid.
 *      MOSQ_ERR_NOMEM -    if an out of memory condition occurred.
 */
int mosquitto_topic_matches_sub(const char *sub, const char *topic, bool *result);

#ifdef __cplusplus
}
#endif

#endif
