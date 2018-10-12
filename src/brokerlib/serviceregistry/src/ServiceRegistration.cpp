/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/BrokerSettings.h"
#include "cert/include/ManagedCerts.h"
#include "serviceregistry/include/ServiceRegistration.h"

using namespace std;
using namespace dxl::broker::cert;

namespace dxl {
namespace broker {
namespace service {

/** {@inheritDoc}*/
ServiceRegistration::ServiceRegistration( 
    const std::string& serviceType, 
    const std::string& serviceGuid,
    const std::string& clientGuid,
    const std::string& clientInstanceGuid,
    const std::string& brokerGuid,
    uint32_t ttlMins, 
    const unordered_set<std::string> requestChannels,
    const unordered_map<std::string, std::string> metaData,
    const std::string& clientTenantGuid,
    const unordered_set<std::string> targetTenantGuids,
    const unordered_set<std::string> certificates,
    bool managedClient) :
    m_serviceType( serviceType ), m_serviceGuid( serviceGuid ),  
    m_ttlMins( ttlMins ), m_requestChannels( requestChannels ),
    m_metaData( metaData ),
    m_brokerService( false ),
    m_clientTenantGuid( clientTenantGuid ),
    m_targetTenantGuids( targetTenantGuids ),    
    m_managedClient( managedClient ),
    m_certsUthash( NULL )
{
    // Set client GUID
    setClientGuid( clientGuid );

    // Set client instance GUID
    setClientInstanceGuid( clientInstanceGuid );

    // Ensure isLocal is updated appropriately
    setBrokerGuid( brokerGuid );

    // Ensure is Ops is set correctly
    setClientTenantGuid( clientTenantGuid );

    // Set the registration time
    time( &m_regTime );

    // Sets the certificates (along with the UT hash form)
    setCertificates( certificates );
}

/** {@inheritDoc} */
ServiceRegistration::ServiceRegistration( const ServiceRegistration& reg ) : 
    m_certsUthash( NULL )
{
    operator=( reg );
}

/** {@inheritDoc} */
ServiceRegistration& ServiceRegistration::operator=(const ServiceRegistration& other )
{    
    if( this != &other )
    {
        setCertificates( other.getCertificates() );
        
        m_serviceType = other.m_serviceType;
        m_serviceGuid = other.m_serviceGuid;
        m_clientGuid = other.m_clientGuid;    
        m_clientInstanceGuid = other.m_clientInstanceGuid;
        m_brokerGuid = other.m_brokerGuid;
        m_ttlMins = other.m_ttlMins;
        m_requestChannels = other.m_requestChannels;
        m_metaData = other.m_metaData;
        m_isLocal = other.m_isLocal;
        m_regTime = other.m_regTime;
        m_brokerService = other.m_brokerService;
        m_managedClient = other.m_managedClient;
        m_clientTenantGuid = other.m_clientTenantGuid;
        m_targetTenantGuids = other.m_targetTenantGuids;
        m_serviceIsOps = other.m_serviceIsOps;
    }

    return *this;
}

/** {@inheritDoc} */
ServiceRegistration::~ServiceRegistration()
{
    // Free the UT hash certificates
    freeCertificatesUtHash();
}

/** {@inheritDoc} */
void ServiceRegistration::freeCertificatesUtHash()
{
    // Free UT hash form of certificates
    struct cert_hashes *current, *tmp;
    HASH_ITER( hh, m_certsUthash, current, tmp ) {
        HASH_DEL( m_certsUthash, current );
        free( (void*)current->cert_sha1 );
        free( current );
    }
    m_certsUthash = NULL;
}

/** {@inheritDoc} */
void ServiceRegistration::setCertificates( const unordered_set<std::string>& certs )
{
    // Set the certificates
    m_certificates = certs;

    // Free the UT hash certificates
    freeCertificatesUtHash();

    // Create UT hash form of certificates
    ManagedCerts::writeToUthash( m_certificates, &m_certsUthash );
}

/** {@inheritDoc} */
void ServiceRegistration::setBrokerGuid( const std::string& guid )
{ 
    m_brokerGuid = guid;
    m_isLocal = ( guid == BrokerSettings::getGuid() );
}

/** {@inheritDoc} */
bool ServiceRegistration::operator==( 
    const ServiceRegistration& rhs ) const
{
    return
        m_serviceType == rhs.m_serviceType &&
        m_serviceGuid == rhs.m_serviceGuid &&
        m_clientGuid == rhs.m_clientGuid &&
        m_clientInstanceGuid == rhs.m_clientInstanceGuid &&
        m_brokerGuid == rhs.m_brokerGuid &&
        m_ttlMins == rhs.m_ttlMins &&
        m_requestChannels == rhs.m_requestChannels &&
        m_metaData == rhs.m_metaData &&
        m_clientTenantGuid == rhs.m_clientTenantGuid &&
        m_targetTenantGuids == rhs.m_targetTenantGuids &&
        m_serviceIsOps == rhs.m_serviceIsOps &&
        m_certificates == rhs.m_certificates &&
        m_managedClient == rhs.m_managedClient;
}

/** {@inheritDoc} */
bool ServiceRegistration::isExpired() const
{
    time_t current;
    time( &current );
    return difftime( current, m_regTime ) > 
        ( ( m_ttlMins + BrokerSettings::getTtlGracePeriodMins() ) * 60 );
}

/** {@inheritDoc} */
uint32_t ServiceRegistration::getAdjustedTtlMins() const
{
    time_t current;
    time( &current );
    return getAdjustedTtlMins( current );
}

/**
 * Returns the adjusted TTL (in minutes). The TTL based on the specified
 * time.
 *
 * @param    time The time to adjust based on
 * @return    The adjusted TTL (in minutes). The TTL based on the specified
 *             time.
 */
uint32_t ServiceRegistration::getAdjustedTtlMins( time_t time ) const
{
    double diff = difftime( time, m_regTime );
    int32_t minDiff = (int32_t)( ( diff + 30 ) / 60 );
    int32_t ttlDiff = m_ttlMins - minDiff;

    return ttlDiff > 0 ? ttlDiff : 0;
}


/** {@inheritDoc} */
void ServiceRegistration::resetRegistrationTime()
{
    time_t current;
    time( &current );

    m_ttlMins = getAdjustedTtlMins( current );
    m_regTime = current;
}

/** {@inheritDoc} */
bool ServiceRegistration::isServiceAvailable( const char* tenantGuid ) const
{
    if( !BrokerSettings::isMultiTenantModeEnabled() )
    {
        return true;
    }

    if( isOps() )
    {
        // Case 1: empty guids indicates the service is available for all tenants
        // Case 2: The tenant is listed for this service
        return m_targetTenantGuids.empty() || 
            m_targetTenantGuids.find( tenantGuid ) != m_targetTenantGuids.end();
    }
    else
    {
        // Case 3: clientTenantGuid (that registered service) is the same of the requested one
        return  m_clientTenantGuid == tenantGuid;
    }
}

/** {@inheritDoc} */
void ServiceRegistration::setClientTenantGuid( const std::string& tenantGuid ) 
{ 
    m_clientTenantGuid = tenantGuid;
    m_serviceIsOps = 
        ( BrokerSettings::isMultiTenantModeEnabled() ? tenantGuid == BrokerSettings::getTenantGuid() : false );
}

/** {@inheritDoc} */
std::ostream& operator<<( std::ostream &out, const ServiceRegistration &reg )
{
    out << "\t" << "Registration: " << reg.getServiceGuid() << endl;
    out << "\t\t" << "Service type: " << reg.getServiceType() << endl;
    out << "\t\t" << "brokerGuid: " << reg.getBrokerGuid() << endl;
    out << "\t\t" << "clientGuid: " << reg.getClientGuid() << endl;    
    out << "\t\t" << "clientInstanceGuid: " << reg.getClientInstanceGuid() << endl;    
    out << "\t\t" << "TTL (mins): " << reg.getTtlMins() << endl;
    out << "\t\t" << "isLocal: " << reg.isLocal() << endl;
    out << "\t\t" << "clientTenantGuid: " << reg.getClientTenantGuid() << endl;
    out << "\t\t" << "isOps: " << reg.isOps() << endl;
    out << "\t\t" << "isManagedClient: " << reg.isManagedClient() << endl;
    out << "\t\t" << "Channels: " << endl;

    if( !reg.isManagedClient() )
    {
        out << "\t\t" << "Certificates: " << endl;
        const unordered_set<string> certs = reg.getCertificates();
        for( auto iter = certs.begin(); iter != certs.end(); iter++ )
        {
            out << "\t\t\t" << *iter << endl;
        }
    }

    out << "\t\t" << "Channels: " << endl;
    const unordered_set<string> channels = reg.getRequestChannels();
    for( auto iter = channels.begin(); iter != channels.end(); iter++ )
    {
        out << "\t\t\t" << *iter << endl;
    }

    out << "\t\t" << "Meta-data: " << endl;
    const unordered_map<string,string> metaData = reg.getMetaData();
    for( auto iter = metaData.begin(); iter != metaData.end(); iter++ )
    {
        out << "\t\t\t" << iter->first << ":" << iter->second << endl;
    }

    out << "\t\t" << "targetTenantGuids: " << endl;
    const unordered_set<string> tenantGuids = reg.getTargetTenantGuids();
    for( auto iter = tenantGuids.begin(); iter != tenantGuids.end(); iter++ )
    {
        out << "\t\t\t" << *iter << endl;
    }

    return out;
}

}
}
}
