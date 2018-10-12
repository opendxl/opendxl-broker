/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef COREBROKERHEALTH_H_
#define COREBROKERHEALTH_H_

#include <ctime>

namespace dxl {
namespace broker {
namespace core {
    
/**
 * This class is used to store information helpful for monitoring purposes
 * such as the number of connected clients or the number of requests per
 * second being handled by the broker. 
 */
class CoreBrokerHealth
{
public:
        
    /** Constructor */
    CoreBrokerHealth();

    /** Destructor */
    virtual ~CoreBrokerHealth();

    /**
     * Set the number of connected clients
     *
     * @param   connectedClients The number of connected clients
     */
    void setConnectedClients(const int connectedClients);

    /**
      * Returns the number of connected clients
      *
      * @return The number of connected clients
      */
    int getConnectedClients() const;

    /**
     * Sets the number of incoming messages
     *
     * @param   msgs The number of incoming messages
     */
    void setIncomingMsgs (const float msgs);

    /**
     * Returns the number of incoming Messages
     *
     * @return  The number of the current incoming messages
     */
    float getIncomingMsgs() const;

    /**
     * Sets the number of outgoing Messages
     *
     * @param   msgs The number of outgoing messages
     */
    void setOutgoingMsgs (const float msgs);

    /**
      * Returns the number of outgoing messages
      *
      * @return     The number of outgoing messages
      */
    float getOutgoingMsgs() const;

    /**
     * Sets the start time of broker
     *
     * @param   time The start up time of the broker
     */
    void setStartUpTime (const std::time_t& time);

    /**
     * Returns the start time of the broker
     *
     * @return  The start time of the broker
     */
    std::time_t getStartUpTime() const;
        
    /**
     * Sets the count of local services registered with the broker
     *
     * @param   svcs The count of local services registered with the broker
     */
    void setLocalServicesCounter (const std::size_t& svcs);

    /**
     * Returns The count of local services registered with the broker
     *
     * @return  The count of local services registered with the broker
     */
    std::size_t getLocalServicesCounter() const;

protected:

    /** The count of connected clients */
    int m_connectedClients;
    /** count of incoming messages to the broker */
    float m_incomingMsgs;
    /** Count of outgoing messages from the broker */
    float m_outgoingMsgs;
    /** Broker start up time */
    std::time_t m_startUpTime;
    /** Count of local services */
    std::size_t m_localServices;
};

} /* namespace core */
} /* namespace broker */
} /* namespace dxl */

#endif /* COREBROKERHEALTH_H_ */
