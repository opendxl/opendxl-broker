/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/BrokerSettings.h"
#include "message/include/DxlMessageConstants.h"
#include "message/payload/include/ServiceRegistryRegisterEventPayload.h"

using namespace std;
using namespace dxl::broker;
using namespace dxl::broker::message;
using namespace dxl::broker::message::payload;
using namespace dxl::broker::service;
using namespace Json;

/** {@inheritDoc} */
bool ServiceRegistryRegisterEventPayload::operator==( 
    const ServiceRegistryRegisterEventPayload& rhs ) const
{    
    return m_serviceRegistration == rhs.m_serviceRegistration;
}

/** {@inheritDoc} */
void ServiceRegistryRegisterEventPayload::write( Json::Value& out ) const
{
    ServiceRegistryRegisterEventPayload::write( out, false );
}

/** {@inheritDoc} */
void ServiceRegistryRegisterEventPayload::write( Json::Value& out, bool isServiceQuery ) const
{
    out[ DxlMessageConstants::PROP_SERVICE_TYPE ] = getServiceType();
    out[ DxlMessageConstants::PROP_SERVICE_GUID ] = getServiceGuid();
    out[ DxlMessageConstants::PROP_CLIENT_GUID ] = getClientGuid();
    out[ DxlMessageConstants::PROP_CLIENT_INSTANCE_GUID ] = getClientInstanceGuid();
    out[ DxlMessageConstants::PROP_BROKER_GUID ] = getBrokerGuid();    
    out[ DxlMessageConstants::PROP_TTL_MINS ] = getTtlMins();    
    out[ DxlMessageConstants::PROP_REGISTRATION_TIME ] = (UInt64)getRegistrationTime();
    Value channels( arrayValue );
    unordered_set<string> requestChannels = getRequestChannels();
    for( auto it = requestChannels.begin(); it != requestChannels.end(); ++it )
    {
        channels.append( *it );
    }
    out[ DxlMessageConstants::PROP_REQUEST_CHANNELS ] = channels;
    Value metaData( objectValue );
    unordered_map<string, string> metaDataVals = getMetaData();
    for( auto mapIt = metaDataVals.begin(); mapIt != metaDataVals.end(); mapIt++ ) 
    {
        metaData[ mapIt->first ] = mapIt->second;
    }
    out[ DxlMessageConstants::PROP_METADATA ] = metaData;     
    Value certificates( arrayValue );
    unordered_set<string> clientCertificates = getCertificates();
    for( auto it = clientCertificates.begin(); it != clientCertificates.end(); ++it )
    {
        certificates.append( *it );
    }
    out[ DxlMessageConstants::PROP_CERTIFICATES ] = certificates; 
    out[ DxlMessageConstants::PROP_MANAGED ] = isManagedClient();

    if( BrokerSettings::isMultiTenantModeEnabled() && !isServiceQuery )
    {
        Value targetTenantGuids( arrayValue );
        unordered_set<string> tenantGuids = getTargetTenantGuids();
        for( auto it = tenantGuids.begin(); it != tenantGuids.end(); ++it )
        {
            targetTenantGuids.append( *it );
        }
        out[ DxlMessageConstants::PROP_TARGET_TENANT_GUIDS ] = targetTenantGuids; 
        out[ DxlMessageConstants::PROP_CLIENT_TENANT_GUID ] = getClientTenantGuid();        
    }
}

/** {@inheritDoc} */
void ServiceRegistryRegisterEventPayload::read( const Json::Value& in )
{
    m_serviceRegistration.setServiceType( 
        in[ DxlMessageConstants::PROP_SERVICE_TYPE ].asString() );
    m_serviceRegistration.setServiceGuid( 
        in[ DxlMessageConstants::PROP_SERVICE_GUID ].asString() );
    m_serviceRegistration.setClientGuid( 
        in[ DxlMessageConstants::PROP_CLIENT_GUID ].asString() );
    m_serviceRegistration.setClientInstanceGuid( 
        in[ DxlMessageConstants::PROP_CLIENT_INSTANCE_GUID ].asString() );
    m_serviceRegistration.setBrokerGuid( 
        in[ DxlMessageConstants::PROP_BROKER_GUID ].asString() );
    m_serviceRegistration.setTtlMins(
        in[ DxlMessageConstants::PROP_TTL_MINS ].asUInt() );
    Json::Value reqChannels = in[ DxlMessageConstants::PROP_REQUEST_CHANNELS ];
    unordered_set<string> requestChannels;
    for( Value::iterator itr = reqChannels.begin(); itr != reqChannels.end(); itr++ )
    {
        requestChannels.insert( (*itr).asString() );
    }
    m_serviceRegistration.setRequestChannels( requestChannels );
    Json::Value metaData = in[ DxlMessageConstants::PROP_METADATA ];
    unordered_map<string, string> metaDataVals;
    for( Value::iterator itr = metaData.begin(); itr != metaData.end(); itr++ )
    {
        metaDataVals[ itr.key().asString() ] = (*itr).asString();
    }
    m_serviceRegistration.setMetaData( metaDataVals );

    // Need to check for nulls as these fields were added in 3.0.1
    unordered_set<string> clientCerts;
    Json::Value certs = in[ DxlMessageConstants::PROP_CERTIFICATES ];
    if( !certs.isNull() )
    {
        for( Value::iterator itr = certs.begin(); itr != certs.end(); itr++ )
        {
            clientCerts.insert( (*itr).asString() );
        }
    }
    m_serviceRegistration.setCertificates( clientCerts );

    Json::Value isManaged = in[ DxlMessageConstants::PROP_MANAGED ];
    // Default managed to true if from a broker prior to 3.0.1
    m_serviceRegistration.setManagedClient(
        isManaged.isNull() ? true : isManaged.asBool() );

    if( BrokerSettings::isMultiTenantModeEnabled() )
    {
        // Need to check for nulls as these fields were added in 3.1.0
        Json::Value targetTenantGuids = in[ DxlMessageConstants::PROP_TARGET_TENANT_GUIDS ];
        if ( !targetTenantGuids.isNull() )
        {        
            unordered_set<string> tenantGuids;
            for( Value::iterator itr = targetTenantGuids.begin(); itr != targetTenantGuids.end(); itr++ )
            {
                tenantGuids.insert( (*itr).asString() );
            }
            m_serviceRegistration.setTargetTenantGuids( tenantGuids );
        }

        Json::Value clientTenantGuid = in[ DxlMessageConstants::PROP_CLIENT_TENANT_GUID ];
        if( !clientTenantGuid.isNull() )
        {
            m_serviceRegistration.setClientTenantGuid( clientTenantGuid.asString() );
        }
    }
}
