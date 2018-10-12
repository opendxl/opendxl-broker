/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef MQTTWORKQUEUE_H_
#define MQTTWORKQUEUE_H_

#include <queue>
#include <memory>
#include <mutex>

namespace dxl {
namespace broker {
namespace core {

/**
 * This queue allows for operations to be queued and then executed on the main MQTT
 * work thread (Mosquitto is single threaded). For example, the  queue can be used to 
 * make bridging changes, etc. and since the operation is executing in the single 
 * Mosquitto thread, you don't have to deal with multi-threading issues.
 */
class MqttWorkQueue
{    
public:

    /** Forward class reference to the runnable interface */
    class Runnable;

    /** 
     * Returns the single queue instance 
     *
     * @return  The single queue instance
     */
    static MqttWorkQueue& getInstance();

    /** Destructor */
    virtual ~MqttWorkQueue() {};

    /**
     * Adds a runnable to be executed by the main Mosquitto thread
     *
     * @param   runnable The runnable to be executed by the main Mosquitto thread
     */
    void add( const std::shared_ptr<Runnable> runnable );

    /**
     * Executes the queue
     */
    void runQueue();

private:
    /** Constructor */
    MqttWorkQueue() {};

    /** The queue of runnables */
    std::queue<std::shared_ptr<Runnable>> m_queue;

    /** The queue mutex */
    std::mutex m_queueMutex;

public:

    /**
     * Interface to be implemented by "runnables" that are to be executed on the
     * main Mosquitto thread.
     */
    class Runnable
    {
    public:

        /** Destructor */
        virtual ~Runnable() {}    

        /**
         * Executes the runnable 
         */
        virtual void run() = 0;
    };    
};

} /* namespace core */
} /* namespace broker */
} /* namespace dxl */

#endif /* MQTTWORKQUEUE_H_ */
