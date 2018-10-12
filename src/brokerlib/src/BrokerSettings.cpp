/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/BrokerHelpers.h"
#include "include/BrokerSettings.h"
#include "brokerregistry/include/brokerregistry.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace dxl::broker;

// GUID
string BrokerSettings::sm_guid;

// Tenant GUID
string BrokerSettings::sm_tenantGuid;

// Listen port
int BrokerSettings::sm_listenPort = 8883;

// Message size limit
int BrokerSettings::sm_messageSizeLimit = 1048576;

// User
string BrokerSettings::sm_user;

// Logging
string BrokerSettings::sm_logFilePath;
int BrokerSettings::sm_logFileMaxSize = 0;
int BrokerSettings::sm_logFileMaxCount = 0;
bool BrokerSettings::sm_logUseStdOut = false;
string BrokerSettings::sm_logLevel;
bool BrokerSettings::sm_noticeLoggingEnabled = false;

// Policy files
string BrokerSettings::sm_policyGeneralStateFile;
string BrokerSettings::sm_policyBrokerStateFile;
string BrokerSettings::sm_policyTopicAuthStateFile;

// The broker TTL
int BrokerSettings::sm_brokerTtl = 60;

// TTL Check interval 
int BrokerSettings::sm_ttlInterval = 10;

// TTL grace period
int BrokerSettings::sm_ttlGracePeriod = 5;

// JSON pretty print
bool BrokerSettings::sm_jsonPrettyPrint = false;

// Certificates
bool BrokerSettings::sm_tlsEnabled = true;
bool BrokerSettings::sm_tlsBridgingInsecure = true;
bool BrokerSettings::sm_fipsEnabled = false;
string BrokerSettings::sm_certKeyStoreDir;
string BrokerSettings::sm_certBrokerCertChainFile;
string BrokerSettings::sm_certClientCertChainFile;
string BrokerSettings::sm_certBrokerCertFile;
string BrokerSettings::sm_certClientCertFile;
string BrokerSettings::sm_certBrokerCsrFile;
string BrokerSettings::sm_certClientCsrFile;
string BrokerSettings::sm_certBrokerPrivateKeyFile;
string BrokerSettings::sm_certClientPrivateKeyFile;
string BrokerSettings::sm_revokedCertsFile;
string BrokerSettings::sm_brokerCertsListFile;
int BrokerSettings::sm_certSignRetryMins = 5;

// Ciphers
string BrokerSettings::sm_ciphers;

// Message sampling
int BrokerSettings::sm_messageSampleSecs = 60;

// Whether to ignore the connection limit as received via policy (for testing)
bool BrokerSettings::sm_ignoreConnectionLimit = false;

// The maximum size of the packet buffer (per context)
uint64_t BrokerSettings::sm_maxPacketBufferSize = 500000;

// The approximate broker state topics batch size (in terms of character count).
int BrokerSettings::sm_brokerStateTopicsCharsBatchSize = 2048;

// Whether topic based routing is enabled 
bool BrokerSettings::sm_topicRoutingEnabled = true;

// Whether the topic based routing cache is enabled 
bool BrokerSettings::sm_topicRoutingCacheEnabled = true;

// The default delay to apply when the topic routing cache is cleared.
uint32_t BrokerSettings::sm_topicRoutingCacheClearDelay = 30;

// Whether to validate certificates against the connection identifier
bool BrokerSettings::sm_certIdentityValidationEnabled = false;

// Whether to to restrict the fabric to a single instance of each unique client identifier
bool BrokerSettings::sm_uniqueClientIdPerFabricEnabled = false;

// Whether test mode is enabled 
bool BrokerSettings::sm_testModeEnabled = false;

// The default brokerLib thread pool size
uint32_t BrokerSettings::sm_brokerLibThreadPoolSize = 1;

// The default multi-tenant mode
bool BrokerSettings::sm_multiTenantModeEnabled = false;

// The tenant byte limit
uint32_t BrokerSettings::sm_tenantByteLimit = 0;

// The tenant connection limit
uint32_t BrokerSettings::sm_tenantConnectionLimit = 0;

// Whether to send events when clients connect/disconnect
bool BrokerSettings::sm_sendConnectEvents = false;


// WHether the broker has been initialized
bool BrokerSettings::sm_brokerInitialized = false;


/**
 * Normalizes slashes for the current platform 
 *
 * @param    str The string to normalize
 */
static void normalizePath( string& str )
{
    boost::replace_all(str, "\\", "/");
}

/** {@inheritDoc} */
const char* BrokerSettings::getGuid( bool throwException )
{
    if( sm_guid.empty() )
    {
        if( throwException )
        {
            throw runtime_error( "Broker identifier has not been set" );
        }
        else
        {
            return NULL;
        }
    }

    return sm_guid.c_str();
}

/** {@inheritDoc} */
const char* BrokerSettings::getTenantGuid( bool throwException )
{
    if( sm_tenantGuid.empty() )
    {
        if( throwException )
        {
            throw runtime_error( "Broker tenant GUID not been set" );
        }
        else
        {
            return NULL;
        }
    }

    return sm_tenantGuid.c_str();
}

/** {@inheritDoc} */
string BrokerSettings::dumpSettings()
{
    stringstream out;    
    out << endl;
    out << "BrokerSettings:" << endl;
    out << "\tbrokerVersion: " << BrokerRegistry::getInstance().getLocalBrokerVersion() << endl;
    out << "\tbrokerId: " << getGuid() << endl;
    out << "\tuser: " << getUser() << endl;
    out << "\tbrokerListenPort: " << getListenPort() << endl;
    out << "\tmessageSizeLimit: " << getMessageSizeLimit() << endl;
    out << "\tlogFilePath: " << getLogFilePath() << endl;    
    out << "\tlogFileMaxSize: " << getLogFileMaxSize() << endl;    
    out << "\tlogFileMaxCount: " << getLogFileMaxCount() << endl;    
    out << "\tlogUseStdOut: " << ( isStdOutLoggingEnabled() ? "true" : "false" ) << endl;    
    out << "\tlogLevel: " << getLogLevel() << endl;    
    out << "\tlogNotices: " << ( isNoticeLoggingEnabled() ? "true" : "false" ) << endl;    
    out << "\tpolicyGeneralStateFile: " << getPolicyGeneralStateFile() << endl;
    out << "\tpolicyBrokerStateFile: " << getPolicyBrokerStateFile() << endl;
    out << "\tpolicyTopicAuthStateFile: " << getPolicyTopicAuthStateFile() << endl;
    out << "\tbrokerTtl (Mins): " << getBrokerTtlMins() << endl;    
    out << "\tttlCheckInterval (Mins): " << getTtlCheckIntervalMins() << endl;    
    out << "\tttlGracePeriod (Mins): " << getTtlGracePeriodMins() << endl;    
    out << "\tjsonPrettyPrint: " << ( isJsonPrettyPrintEnabled() ? "true" : "false" ) << endl;        
    out << "\ttlsEnabled: " << ( isTlsEnabled() ? "true" : "false" ) << endl;
    out << "\ttlsBridgingInsecure: " << ( isTlsBridgingInsecure() ? "true" : "false" ) << endl;
    out << "\tfipsEnabled: " << ( isFipsEnabled() ? "true" : "false" ) << endl;    
    out << "\tcertKeyStoreDir: " << getCertKeystoreDir() << endl;    
    out << "\tbrokerCertChainFile: " << getBrokerCertChainFile() << endl;
    out << "\tbrokerCertFile: " << getBrokerCertFile() << endl;
    out << "\tbrokerCsrFile: " << getBrokerCsrFile() << endl;    
    out << "\tbrokerPrivateKeyFile: " << getBrokerPrivateKeyFile() << endl;
    out << "\tclientCertChainFile: " << getClientCertChainFile() << endl;
    out << "\trevokedCertsFile: " << getRevokedCertsFile() << endl;
    out << "\tbrokerCertsListFile: " << getBrokerCertsListFile() << endl;
    out << "\tciphers: " << getCiphers() << endl;    
    out << "\tmessageSample (Secs): " << getMessageSampleSecs() << endl;
    out << "\tmaximumPacketBufferSize: " << getMaxPacketBufferSize() << endl;    
    out << "\tisConnectionLimitIgnored: " << ( isConnectionLimitIgnored() ? "true" : "false" ) << endl;
    out << "\tbrokerStateTopicsCharsBatchSize: " << getBrokerStateTopicsCharsBatchSize() << endl;
    out << "\ttopicRoutingEnabled: " << ( isTopicRoutingEnabled() ? "true" : "false" ) << endl;
    out << "\ttopicRoutingCacheEnabled: " << ( isTopicRoutingCacheEnabled() ? "true" : "false" ) << endl;
    out << "\ttopicRoutingCacheClearDelay: " << getTopicRoutingCacheClearDelay() << endl;
    out << "\ttestModeEnabled: " << ( isTestModeEnabled() ? "true" : "false" ) << endl;
    out << "\tcertIdentityValidationEnabled: " << ( isCertIdentityValidationEnabled() ? "true" : "false" ) << endl;
    out << "\tuniqueClientIdPerFabricEnabled: " << ( isUniqueClientIdPerFabricEnabled() ? "true" : "false" ) << endl;
    out << "\tbrokerLibThreadPoolSize: " << getBrokerLibThreadPoolSize() << endl;
    out << "\tmultiTenantModeEnabled: " << ( isMultiTenantModeEnabled() ? "true" : "false" ) << endl;
    out << "\tsendConnectEvents: " << ( isSendConnectEventsEnabled() ? "true" : "false" ) << endl;
    if( isMultiTenantModeEnabled() )
    {
        out << "\ttenantByteLimit: " << getTenantByteLimit() << endl; 
        out << "\ttenantConnectionLimit: " << getTenantConnectionLimit() << endl; 
    }
    out << endl;

    return out.str();
}

/** {@inheritDoc} */
void BrokerSettings::setValuesFromConfig( const Configuration& config )
{
    string strValue;

    // Broker identifier
    config.getProperty( "brokerId", sm_guid, "" );

    // Logging
    config.getProperty( "logFile", sm_logFilePath, "./dxlbroker.log" );
    normalizePath( sm_logFilePath );
    config.getProperty( "maxLogSize", strValue, "1000000" );
    sm_logFileMaxSize = atoi( strValue.c_str()) ;
    config.getProperty( "maxLogFiles", strValue, "10" );
    sm_logFileMaxCount = atoi( strValue.c_str() );        
    config.getProperty("stdOutLogger", strValue, "false");
    sm_logUseStdOut = ( strValue == "true" );
    config.getProperty("logLevel", sm_logLevel, "warn");
    config.getProperty("logNotices", strValue, "false");
    sm_noticeLoggingEnabled = ( strValue == "true" );

    // Listener port
    config.getProperty("listenPort", strValue, "8883");
    sm_listenPort = atoi( strValue.c_str() );

    // Message size limit
    config.getProperty("messageSizeLimit", strValue, "1048576");
    sm_messageSizeLimit = atoi( strValue.c_str() );

    // User
    config.getProperty("user", sm_user, "");

    // Policy state files
    config.getProperty( "policyGeneralStateFile", sm_policyGeneralStateFile, POLICYGENERALSTATE_LOC );
    normalizePath( sm_policyGeneralStateFile );
    config.getProperty( "policyBrokerStateFile", sm_policyBrokerStateFile, POLICYBROKERSTATE_LOC );
    normalizePath( sm_policyBrokerStateFile );
    config.getProperty( "policyTopicAuthStateFile", sm_policyTopicAuthStateFile, TOPICAUTHSTATE_LOC );    
    normalizePath( sm_policyTopicAuthStateFile );

    // Broker TTL
    config.getProperty( "brokerTtlMins", strValue, "60" );
    sm_brokerTtl = atoi( strValue.c_str() );

    // TTL check interval
    config.getProperty( "ttlCheckIntervalMins", strValue, "10" );
    sm_ttlInterval = atoi( strValue.c_str() );

    // TTL grace period
    config.getProperty( "ttlGraceMins", strValue, "5" );
    sm_ttlGracePeriod = atoi( strValue.c_str() );

    // JSON pretty print
    config.getProperty( "jsonPrettyPrint", strValue, "false" );
    sm_jsonPrettyPrint = ( strValue == "true" );

    // Certificates
    config.getProperty( "tlsEnabled", strValue, "true" );
    sm_tlsEnabled = ( strValue == "true" );
    config.getProperty( "tlsBridgingInsecure", strValue, "true" );
    sm_tlsBridgingInsecure = ( strValue == "true" );
    config.getProperty( "fipsEnabled", strValue, "false" );
    sm_fipsEnabled = ( strValue == "true" );
    config.getProperty( "certKeystoreDir", sm_certKeyStoreDir, "/var/McAfee/dxlbroker/keystore" );
    normalizePath( sm_certKeyStoreDir );
    sm_certBrokerCertChainFile = sm_certKeyStoreDir + "/ca-broker.crt";
    normalizePath( sm_certBrokerCertChainFile );
    sm_certClientCertChainFile = sm_certKeyStoreDir + "/ca-client.crt";
    normalizePath( sm_certClientCertChainFile );
    sm_certBrokerCertFile = sm_certKeyStoreDir + "/broker.crt";
    normalizePath( sm_certBrokerCertFile );
    sm_certClientCertFile = sm_certKeyStoreDir + "/client.crt";
    normalizePath( sm_certClientCertFile );
    sm_certBrokerCsrFile = sm_certKeyStoreDir + "/broker.csr";
    normalizePath( sm_certBrokerCsrFile );
    sm_certClientCsrFile = sm_certKeyStoreDir + "/client.csr";
    normalizePath( sm_certClientCsrFile );
    sm_certBrokerPrivateKeyFile = sm_certKeyStoreDir + "/broker.key";
    normalizePath( sm_certBrokerPrivateKeyFile );
    sm_certClientPrivateKeyFile = sm_certKeyStoreDir +  "/client.key";
    normalizePath( sm_certClientPrivateKeyFile );
    sm_revokedCertsFile = sm_certKeyStoreDir + "/revokedcerts.lst";
    normalizePath( sm_revokedCertsFile );
    sm_brokerCertsListFile = sm_certKeyStoreDir + "/ca-brokers.lst";
    normalizePath( sm_brokerCertsListFile );

    config.getProperty( "certSigningRetryIntervalMins", strValue, "5" );
    sm_certSignRetryMins = atoi( strValue.c_str() );

    // Ciphers
    config.getProperty( "ciphers", sm_ciphers, "" );

    // Message sample secs
    config.getProperty( "messageSampleSecs", strValue, "60" );
    sm_messageSampleSecs = atoi( strValue.c_str() );

    // Ignore connection limit
    config.getProperty( "ignoreConnectionLimit", strValue, "false" );
    sm_ignoreConnectionLimit = ( strValue == "true" );

    // The maximum size of the packet buffer in (per context)
    config.getProperty( "maximumPacketBufferSize", strValue, "500000" );
    sm_maxPacketBufferSize = std::stoull( strValue.c_str() );

    // The approximate broker state topics batch size (in terms of character count).
    config.getProperty( "brokerStateTopicsCharsBatchSize", strValue, "2048" );
    sm_brokerStateTopicsCharsBatchSize = atoi( strValue.c_str() );

    // Whether topic routing is enabled
    config.getProperty( "topicRoutingEnabled", strValue, "true" );
    sm_topicRoutingEnabled = ( strValue == "true" );    

    // Whether the topic routing cache is enabled
    config.getProperty( "topicRoutingCacheEnabled", strValue, "true" );
    sm_topicRoutingCacheEnabled = ( strValue == "true" );    

    // The topic routing cache clear delay
    config.getProperty( "topicRoutingCacheClearDelay", strValue, "30" );
    sm_topicRoutingCacheClearDelay = atoi( strValue.c_str() );

    // Whether to validate certificates against the connection identifier
    config.getProperty( "certIdentityValidationEnabled", strValue, "false" );
    sm_certIdentityValidationEnabled = ( strValue == "true" );

    // Whether to restrict the fabric to a single instance of each unique client identifier
    config.getProperty( "uniqueClientIdPerFabricEnabled", strValue, "false" );
    sm_uniqueClientIdPerFabricEnabled = ( strValue == "true" );

    // Whether test mode is enabled
    config.getProperty( "testModeEnabled", strValue, "true" );
    sm_testModeEnabled = ( strValue == "true" );    

    // The brokerLib thread pool size
    config.getProperty( "brokerLibThreadPoolSize", strValue, "1" );
    sm_brokerLibThreadPoolSize = atoi( strValue.c_str() );

    // Whether multi-tenant mode is enabled
    config.getProperty( "multiTenantModeEnabled", strValue, "false" );
    sm_multiTenantModeEnabled = ( strValue == "true" );

    // Tenant identifier
    if ( sm_multiTenantModeEnabled )
    {
        config.getProperty( "tenantGuid", sm_tenantGuid, "" );
    }

    // Tenant byte limit
    config.getProperty( "tenantByteLimit", strValue, "0" );
    sm_tenantByteLimit = atol( strValue.c_str() );

    // Tenant connection limit
    config.getProperty( "tenantConnectionLimit", strValue, "0" );
    sm_tenantConnectionLimit = atol( strValue.c_str() );

    // Whether to send events when clients connect/disconnect
    config.getProperty( "sendConnectEvents", strValue, "false" );
    sm_sendConnectEvents = ( strValue == "true" );
}
