/*
Copyright (c) 2009-2013 Roger Light <roger@atchoo.org>
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
*/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifndef CMAKE
#include <config.h>
#endif

#include <mosquitto_broker.h>
#include <memory_mosq.h>
#include "util_mosq.h"

#include "dxl.h"

extern struct mosquitto_db int_db;

/* Options for logging should be:
 *
 * A combination of:
 * To a file
 * To stdout/stderr
 */

/* Give option of logging timestamp.
 * Logging pid.
 */
static int log_destinations = MQTT3_LOG_STDERR;
// DXL, non static
int log_priorities = MOSQ_LOG_ERR | MOSQ_LOG_WARNING | MOSQ_LOG_NOTICE | MOSQ_LOG_INFO;
unsigned int log_category_mask = 0;

int mqtt3_log_init(int priorities, int destinations, unsigned int category_mask)
{
    int rc = 0;

    log_priorities = priorities;
    log_destinations = destinations;
    log_category_mask = category_mask;

    return rc;
}

int mqtt3_log_close(void)
{
    return MOSQ_ERR_SUCCESS;
}

int _mosquitto_log_printf(struct mosquitto * UNUSED(mosq), int priority, const char *fmt, ...)
{
    va_list va;
    char *s;
    int len;

    if((log_priorities & priority) && log_destinations != MQTT3_LOG_NONE){
        time_t now = time(NULL); // DXL moved into block
        len = (int)(strlen(fmt) + 500);
        s = (char *)_mosquitto_malloc(len*sizeof(char));
        if(!s) return MOSQ_ERR_NOMEM;

        va_start(va, fmt);
        vsnprintf(s, len, fmt, va);
        va_end(va);
        s[len-1] = '\0'; /* Ensure string is null terminated. */

        // DXL Begin
        if(!dxl_is_brokerlib_initialized()){
        // DXL End
            if(log_destinations & MQTT3_LOG_STDOUT){
                if(int_db.config && int_db.config->log_timestamp){
                    fprintf(stdout, "%d: %s\n", (int)now, s);
                }else{
                    fprintf(stdout, "%s\n", s);
                }
                fflush(stdout);
            }
            if(log_destinations & MQTT3_LOG_STDERR){
                if(int_db.config && int_db.config->log_timestamp){
                    fprintf(stderr, "%d: %s\n", (int)now, s);
                }else{
                    fprintf(stderr, "%s\n", s);
                }
                fflush(stderr);
            }
            if(log_destinations & MQTT3_LOG_FILE && int_db.config->log_fptr){
                if(int_db.config && int_db.config->log_timestamp){
                    fprintf(int_db.config->log_fptr, "%d: %s\n", (int)now, s);
                }else{
                    fprintf(int_db.config->log_fptr, "%s\n", s);
                }
            }
        // DXL Begin
        }else{
            dxl_log(priority, s);
        }
        // DXL End

        _mosquitto_free(s);
    }

    return MOSQ_ERR_SUCCESS;
}
