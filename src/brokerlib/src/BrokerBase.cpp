/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/BrokerBase.h"
#include <string>

namespace dxl {
namespace broker {


/** {@inheritDoc} */
BrokerBase::BrokerBase(
    const std::string& id, const std::string& hostname, uint32_t port /*= DEFAULTPORT */) :
    m_id( id ), m_hostname( hostname ), m_port( port )
    {
}

/** {@inheritDoc} */
BrokerBase::~BrokerBase()
{
}

/** {@inheritDoc} */
bool BrokerBase::operator==( const BrokerBase rhs ) const
{
    return ( m_id == rhs.m_id ) && ( m_hostname == rhs.m_hostname )
        && ( m_port == rhs.m_port );
}

/** {@inheritDoc} */
void BrokerBase::setId( const std::string& id )
{
    m_id = id;
}

/** {@inheritDoc} */
void BrokerBase::setHostname( const std::string& hostname )
{
    m_hostname = hostname;
}

/** {@inheritDoc} */
void BrokerBase::setPort( uint32_t port )
{
    m_port = port;
}

}  // namespace broker
}  // namespace dxl
