/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/


#include <memory>
#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include "include/SimpleLog.h"
#include "brokerconfiguration/include/Hub.h"
#include "brokerconfiguration/include/BrokerConfiguration.h"

using std::string;
using std::shared_ptr;
using std::vector;

namespace dxl {
namespace broker {

/** {@inheritDoc} */
bool Hub::PtrHubContainsId::operator() ( const shared_ptr<Hub> lhs, const string& rhs ) const 
{
    return lhs->getPrimaryBrokerId() == rhs || lhs->getSecondaryBrokerId() == rhs;
}

/** {@inheritDoc} */
Hub::Hub( const string& id, const string& primaryBrokerId,
    const string& secondaryBrokerId, const string& parentId,
    const string& serviceZone, const string& name ) :
    m_calculatedServiceZones( false ),
    m_id( id ), m_primaryBrokerId( primaryBrokerId ),
    m_secondaryBrokerId( secondaryBrokerId ), m_parentId( parentId ),
    m_serviceZone( serviceZone ), m_name( name )
{
}

/** {@inheritDoc} */
Hub::~Hub() 
{
}

// ConfigNode impl
bool Hub::isHub() const 
{
    return true;
}

/** {@inheritDoc} */
const string Hub::getId() const 
{
    return m_id;
}

/** {@inheritDoc} */
const string Hub::getParentId() const 
{
    return m_parentId;
}

/** {@inheritDoc} */
const string Hub::getPrimaryBrokerId() const 
{
    return m_primaryBrokerId;
}

/** {@inheritDoc} */
const string Hub::getSecondaryBrokerId() const 
{
    return m_secondaryBrokerId;
}

/** {@inheritDoc} */
const string Hub::getServiceZone() const 
{
    return m_serviceZone;
}

/** {@inheritDoc} */
const string Hub::getName() const 
{
    return m_name;
}

/** {@inheritDoc} */
bool Hub::operator==( const std::string& rhs ) const
{
    return m_primaryBrokerId == rhs || m_secondaryBrokerId == rhs;
}

/** {@inheritDoc} */
bool Hub::operator==( const Hub& rhs ) const 
{
    return m_id == rhs.m_id && m_primaryBrokerId == rhs.m_primaryBrokerId
        && m_secondaryBrokerId == rhs.m_secondaryBrokerId
        && m_parentId == rhs.m_parentId && m_serviceZone == rhs.m_serviceZone
        && m_name == rhs.m_name;
}

/** {@inheritDoc} */
bool Hub::operator!=(const Hub& rhs) const 
{
    return !( *this == rhs );
}

/** {@inheritDoc} */
string Hub::toString() const 
{
    // <hubGuid>;<broker1Guid>;<broker2Guid>;<parentGuid>;<serviceZone>;<hubname>
    return str(
        boost::format("h%1%;%2%;%3%;%4%;%5%;%6%") % getId() % getPrimaryBrokerId()
        % getSecondaryBrokerId() % getParentId() % getServiceZone()
        % getName() );
}

/** {@inheritDoc} */
const ServiceZoneList& Hub::generateServiceZoneList( const BrokerConfiguration& brokerConfig )
{
    if( m_calculatedServiceZones )
    {
        return m_serviceZones;
    }

    m_calculatedServiceZones = true;

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

    auto parentServiceZones = parent->generateServiceZoneList( brokerConfig );
    if( parentServiceZones.empty() ) 
    {
        return m_serviceZones;
    }

    m_serviceZones.insert( m_serviceZones.end(), parentServiceZones.begin(), parentServiceZones.end() );

    return m_serviceZones;
}

}  // namespace broker
}  // namespace dxl

