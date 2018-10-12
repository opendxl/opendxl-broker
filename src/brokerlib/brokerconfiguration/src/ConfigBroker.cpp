/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/


#include <memory>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include "include/SimpleLog.h"
#include "brokerconfiguration/include/ConfigBroker.h"
#include "brokerconfiguration/include/BrokerConfiguration.h"

using std::string;
using std::shared_ptr;
using std::vector;

namespace dxl {
namespace broker {

/** {@inheritDoc} */
ConfigBroker::ConfigBroker( const std::string &brokerId,
    const std::string& hostname, uint32_t port, const std::string& serviceZone,
    const std::string& parentId, const std::string& ipAddress) :
    BrokerBase( brokerId, hostname, port ), 
    m_calculatedServiceZones( false ),
    m_parentId(parentId), m_serviceZone( serviceZone), m_ipAddress( ipAddress )        
{
}

/** {@inheritDoc} */
ConfigBroker::~ConfigBroker() 
{
}

/** {@inheritDoc} */
const std::string ConfigBroker::getParentId() const 
{
    return m_parentId;
}

/** {@inheritDoc} */
bool ConfigBroker::isHub() const 
{
    return false;
}

/** {@inheritDoc} */
bool ConfigBroker::operator==( const ConfigBroker& rhs ) const 
{
    return BrokerBase::operator==( rhs ) && this->m_parentId == rhs.m_parentId
        && this->m_serviceZone == rhs.m_serviceZone
        && this->m_ipAddress == rhs.m_ipAddress;
}

/** {@inheritDoc} */
bool ConfigBroker::operator!=( const ConfigBroker& rhs ) const 
{
    return !( *this == rhs );
}

/** {@inheritDoc} */
const string ConfigBroker::getServiceZone() const 
{
    return m_serviceZone;
}

/** {@inheritDoc} */
const string ConfigBroker::getIpAddress() const 
{
    return m_ipAddress;
}

/** {@inheritDoc} */
string ConfigBroker::toString() const 
{
    // brokerStr : <brokerGuid>;port;<parentGuid>;<host>;<serviceZone>
    return str(
        boost::format( "%1%;%2%;%3%;%4%;%5%;%6%" ) % getId() % getPort()
        % getParentId() % getHostname() % getServiceZone() % getIpAddress() );
}

/** {@inheritDoc} */
const ServiceZoneList& ConfigBroker::generateServiceZoneList(
    const BrokerConfiguration& brokerConfig )
{
    if( m_calculatedServiceZones )
    {
        return m_serviceZones;
    }

    m_calculatedServiceZones = true;

    ServiceZoneList parentServiceZones;
    
    shared_ptr<Hub> parentHub = brokerConfig.getContainingHub( getId() );
    if( parentHub.get() )
    {
        // If we are a hub member, use the hub's service zone list
        parentServiceZones = parentHub->generateServiceZoneList( brokerConfig );
    }
    else
    {
        if( !m_serviceZone.empty() )
        {
            // add our service zone first.
            m_serviceZones.push_back( m_serviceZone );
        }

        // if we don't have a parent, we're done.
        if( m_parentId.empty() ) 
        {
            return m_serviceZones;
        }

        auto parent = brokerConfig.getConfigNode( getParentId() );
        if( !parent ) 
        {
            if( SL_LOG.isWarnEnabled() )
            {
                SL_START << "Parent configuration node could not be found, but was specified. ParentId : "
                    << getParentId() << SL_WARN_END;
            }

            return m_serviceZones;
        }

        parentServiceZones = parent->generateServiceZoneList( brokerConfig );
    }
    
    if( parentServiceZones.empty() ) 
    {
        return m_serviceZones;
    }

    m_serviceZones.insert( m_serviceZones.end(), parentServiceZones.begin(), parentServiceZones.end() );

    return m_serviceZones;
}

}  // namespace broker
}  // namespace dxl

