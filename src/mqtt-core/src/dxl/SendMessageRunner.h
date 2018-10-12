/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef SENDMESSAGERUNNER_H_
#define SENDMESSAGERUNNER_H_

#include "MqttWorkQueue.h"
#include "logging_mosq.h"
#include "memory_mosq.h"
#include <stdint.h>

namespace dxl {
namespace broker {
namespace core {

/**
 * Runner that sends an MQTT message.
 */
class SendMessageRunner : public MqttWorkQueue::Runnable
{    
public:

    /**
     * Constructs the runner
     *
     * @param   topic The topic to send the message to
     * @param   payloadLen The length of the paylaod
     * @param   payload The payload
     */
    SendMessageRunner( const char* topic, uint32_t payloadLen, const void* payload );

    /** Destructor */
    virtual ~SendMessageRunner();
    
    /** {@inheritDoc} */
    void run();

private:
    char* m_topic;
    uint32_t m_payloadLen;
    void* m_payload;
};

} /* namespace core */
} /* namespace broker */
} /* namespace dxl */

#endif /* SENDMESSAGERUNNER_H_ */
