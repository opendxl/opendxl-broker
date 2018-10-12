/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include <cstring>
#include <cstdlib> 
#include "core/include/CoreBrokerHealth.h"

namespace dxl {
namespace broker {
namespace core {

/** {@inheritDoc} */
CoreBrokerHealth::CoreBrokerHealth(): m_connectedClients(0),
    m_incomingMsgs(0), 
    m_outgoingMsgs(0), 
    m_startUpTime(0), 
    m_localServices(0)
{
}

/** {@inheritDoc} */
CoreBrokerHealth::~CoreBrokerHealth()
{
}

/** {@inheritDoc} */
void CoreBrokerHealth::setConnectedClients( const int connectedClients )
{
    m_connectedClients = connectedClients;
}

/** {@inheritDoc} */
int CoreBrokerHealth::getConnectedClients() const
{
    return m_connectedClients;
}

/** {@inheritDoc} */
void CoreBrokerHealth::setIncomingMsgs( const float msgs )
{
    m_incomingMsgs = msgs;
}

/** {@inheritDoc} */
float CoreBrokerHealth::getIncomingMsgs() const
{    
    return m_incomingMsgs;
}

/** {@inheritDoc} */
void CoreBrokerHealth::setOutgoingMsgs( const float msgs )
{
    m_outgoingMsgs = msgs;
}

/** {@inheritDoc} */
float CoreBrokerHealth::getOutgoingMsgs() const
{
    return m_outgoingMsgs;
}

/** {@inheritDoc} */
void CoreBrokerHealth::setStartUpTime( const std::time_t& time )
{
    m_startUpTime = time;
}

/** {@inheritDoc} */
std::time_t CoreBrokerHealth::getStartUpTime() const
{
    return m_startUpTime;
}
    
/** {@inheritDoc} */
void CoreBrokerHealth::setLocalServicesCounter ( const std::size_t& svcs )
{
    m_localServices = svcs;
}

/** {@inheritDoc} */
std::size_t CoreBrokerHealth::getLocalServicesCounter() const
{
    return m_localServices;
}

}
}
}
