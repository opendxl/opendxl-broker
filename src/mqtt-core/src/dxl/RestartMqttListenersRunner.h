/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef RESTARTMQTTLISTENERSRUNNER_H_
#define RESTARTMQTTLISTENERSRUNNER_H_

#include "MqttWorkQueue.h"
#include "cert_hashes.h"

namespace dxl {
namespace broker {
namespace core {

/**
 * Runner restarts the MQTT listeners.
 */
class RestartMqttListenersRunner : public MqttWorkQueue::Runnable
{    
public:

    /**
     * Constructs the runner
     *
     * @param   managedHashes The mosquitto listeners are typcally restarted due to certificate changes.
     *          If this parameter is non-null it will update Mosquitto's set of "managed hashes"
     */
    RestartMqttListenersRunner( struct cert_hashes* managedHashes ) :
        m_managedHashes( managedHashes ) {};

    /** Destructor */
    virtual ~RestartMqttListenersRunner() {};
    
    /** {@inheritDoc} */
    void run();

protected:
    /** The managed certificate hashes */
    struct cert_hashes* m_managedHashes;
};

} /* namespace core */
} /* namespace broker */
} /* namespace dxl */

#endif /* RESTARTMQTTLISTENERSRUNNER_H_ */
