/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERSETTINGS_H_
#define BROKERSETTINGS_H_

#include "include/Configuration.h"
#include <string>
#include <cstdint>
#include <string.h>

namespace dxl {
namespace broker {

/**
 * Settings used by the local broker
 */
class BrokerSettings
{    
public:
    /**
     * Sets the broker GUID
     *
     * @param   guid The broker GUID
     */
    static void setGuid( const char* guid ) { sm_guid = guid; }

    /**
     * Returns the broker GUID. If the broker GUID has not been set, an exception will be thrown.
     *
     * @param   throwException Whether to throw an exception if the GUID has not been set
     * @return  The broker GUID. If the broker GUID has not been set, an exception will be thrown.
     */
    static const char* getGuid( bool throwException = true );

    /**
     * Sets the broker tenant GUID
     *
     * @param   guid The broker tenant GUID
     */
    static void setTenantGuid( const char* guid ) { sm_tenantGuid = guid; }

    /**
     * Returns the broker tenant GUID. If the broker tenant GUID has not been set, an exception 
     * will be thrown.
     *
     * @param   throwException Whether to throw an exception if the tenant GUID has not been set
     * @return  The broker tenant GUID. If the broker tenant GUID has not been set, an exception
     *          will be thrown.
     */
    static const char* getTenantGuid( bool throwException = true );

    /**
     * Returns the broker listen port
     *
     * @return  The broker listen port
     */
    static int getListenPort() { return sm_listenPort; }

    /**
     * Returns the broker message size limit
     *
     * @return  The broker message size limit
     */
    static int getMessageSizeLimit() { return sm_messageSizeLimit; }

    /**
     * Returns the user to run the broker as
     *
     * @return  The user to run the broker as
     */
    static std::string getUser() { return sm_user; }

    /**
     * Sets values for the broker settings from the specified configuration
     *
     * @param   config The configuration settings
     */
    static void setValuesFromConfig( const Configuration& config );

    /**
     * Returns the log file path
     *
     * @return  The log file path
     */
    static std::string getLogFilePath() { return sm_logFilePath; }

    /**
     * Returns the maximum log file size
     *
     * @return  The maximum log file size
     */
    static int getLogFileMaxSize() { return sm_logFileMaxSize; }

    /**
     * Returns the maximum log file count
     *
     * @return  The maximum log file count
     */
    static int getLogFileMaxCount() { return sm_logFileMaxCount; }

    /**
     * Returns whether to log notices
     *
     * @return  Whether to log notices
     */
    static bool isNoticeLoggingEnabled() { return sm_noticeLoggingEnabled; }

    /**
     * Returns whether to use the standard out logger
     *
     * @return  Whether to use the standard output logger
     */
    static bool isStdOutLoggingEnabled() { return sm_logUseStdOut; }

    /**
     * Returns the logging level 
     * 
     * @return  The logging level
     */
    static std::string getLogLevel() { return sm_logLevel; }

    /**
     * Returns the policy file for broker state
     *
     * @return  The policy file for broker state
     */
    static std::string getPolicyBrokerStateFile() { return sm_policyBrokerStateFile; }

    /**
     * Returns the policy file for topic authorization state
     *
     * @return  The policy file for topic authorization state
     */
    static std::string getPolicyTopicAuthStateFile() { return sm_policyTopicAuthStateFile; }    

    /**
     * Returns the policy file for general state
     *
     * @return  The policy file for general state
     */
    static std::string getPolicyGeneralStateFile() { return sm_policyGeneralStateFile; }    

    /**
     * Returns the TTL check interval in minutes
     *
     * @return  The TTL check interval in minutes
     */
    static int getTtlCheckIntervalMins() { return sm_ttlInterval; }

    /**
     * Sets the TTL check interval in minutes
     *
     * @param   ttlInterval The TTL check interval in minutes
     */
    static void setTtlCheckIntervalMins( int ttlInterval ) { sm_ttlInterval = ttlInterval; }

    /**
     * Returns the TTL grace period
     *
     * @return  The TTL grace period
     */
    static int getTtlGracePeriodMins() { return sm_ttlGracePeriod; }

    /**
     * Sets the TTL grace period
     *
     * @param   gracePeriod The TTL grace period
     */
    static void setTtlGracePeriodMins( int gracePeriod ) { sm_ttlGracePeriod = gracePeriod; }

    /**
     * Returns the broker TTL
     *
     * @return  The broker TTL
     */
    static int getBrokerTtlMins() { return sm_brokerTtl; }

    /**
     * Sets the broker TTL
     *
     * @param   ttl The broker ttl
     */
    static void setBrokerTtlMins( int ttl ) { sm_brokerTtl = ttl; }

    /**
     * Returns whether JSON pretty printing is enabled
     *
     * @return  Whether JSON pretty printing is enabled
     */
    static bool isJsonPrettyPrintEnabled() { return sm_jsonPrettyPrint; }

    /** 
     * Returns the keystore directory 
     *
     * @return  The keystore directory
     */
    static std::string getCertKeystoreDir() { return sm_certKeyStoreDir; }

    /** 
     * Returns the broker cert chain file
     *
     * @return  The broker cert chain file
     */
    static std::string getBrokerCertChainFile() { return sm_certBrokerCertChainFile; }

    /** 
     * Returns the client cert chain file
     *
     * @return  The client cert chain file
     */
    static std::string getClientCertChainFile() { return sm_certClientCertChainFile; }

    /** 
     * Returns the broker cert file
     *
     * @return  The broker cert file
     */
    static std::string getBrokerCertFile() { return sm_certBrokerCertFile; }

    /** 
     * Returns the client cert file
     *
     * @return  The client cert file
     */
    static std::string getClientCertFile() { return sm_certClientCertFile; }

    /** 
     * Returns the broker CSR file
     *
     * @return  The broker CSR file
     */
    static std::string getBrokerCsrFile() { return sm_certBrokerCsrFile; }

    /** 
     * Returns the client CSR file
     *
     * @return  The client CSR file
     */
    static std::string getClientCsrFile() { return sm_certClientCsrFile; }

    /** 
     * Returns the broker private key file
     *
     * @return  The broker private key file
     */
    static std::string getBrokerPrivateKeyFile() { return sm_certBrokerPrivateKeyFile; }

    /** 
     * Returns the client private key file
     *
     * @return  The client private key file
     */
    static std::string getClientPrivateKeyFile() { return sm_certClientPrivateKeyFile; }

    /** 
     * Returns the revoked certificates file
     *
     * @return  The revoked certificates file
     */
    static std::string getRevokedCertsFile() { return sm_revokedCertsFile; }

    /**
     * Returns the broker certificate hashes (SHA-1) list file
     *
     * @return  The broker certificates hashes (SHA-1) list file
     */
    static std::string getBrokerCertsListFile() { return sm_brokerCertsListFile; }

    /**
     * Returns the ciphers to restrict to
     *
     * @return  The ciphers to restrict to
     */
    static std::string getCiphers() { return sm_ciphers; }

    /**
     * Returns the certificate signing retry interval (in minutes)
     *
     * @return  The certificate signing retry interval (in minutes)
     */
    static int getCertSigningRetryIntervalMins() { return sm_certSignRetryMins; }

    /**
     * Returns whether or not TLS is enabled
     *
     * @return  Whether or not TLS is enabled
     */
    static bool isTlsEnabled() { return sm_tlsEnabled; }

    /**
     * Returns whether or not FIPS is enabled
     *
     * @return  Whether or not FIPS is enabled
     */
    static bool isFipsEnabled() { return sm_fipsEnabled; }

    /**
     * Returns the interval for sampling messages (for messages/sec, etc.)
     *
     * @return  The interval for sampling messages (for messages/sec, etc.)
     */
    static int getMessageSampleSecs() { return sm_messageSampleSecs; }

    /**
     * Returns the maximum size of the packet buffer in core (per context)
     *
     * @return  The maximum size of the packet buffer in core (per context)
     */
    static uint64_t getMaxPacketBufferSize() { return sm_maxPacketBufferSize; }

    /**
     * Returns whether to ignore the connection limit as received via policy     
     * Should only be enabled during testing
     *
     * @return  Whether to ignore the connection limit as received via policy
     */
    static bool isConnectionLimitIgnored() { return sm_ignoreConnectionLimit; }

    /**
     * Returns the approximate broker state topics batch size (in terms of character
     * count).
     *
     * @return  The approximate broker state topics batch size (in terms of character
     *          count).
     */
    static int getBrokerStateTopicsCharsBatchSize() { return sm_brokerStateTopicsCharsBatchSize; }

    /**
     * Returns whether topic based routing is enabled
     *
     * @return  Whether topic based routing is enabled
     */
    static bool isTopicRoutingEnabled() { return sm_topicRoutingEnabled; }

    /**
     * Returns whether the topic based routing cache is enabled
     *
     * @return  Whether the topic based routing cache is enabled
     */
    static bool isTopicRoutingCacheEnabled() { return sm_topicRoutingCacheEnabled; }

    /**
     * Sets whether the topic based routing cache is enabled
     *
     * @param   enable Whether the topic based routing cache is enabled
     */
    static void setTopicRoutingCacheEnabled( bool enable ) { sm_topicRoutingCacheEnabled = enable; }

    /**
     * Sets whether bridging is insecure (host name validation)
     *
     * @param   insecure Whether bridging is insecure (host name validation)
     */
    static void setTlsBridgingInsecure( bool insecure ) { sm_tlsBridgingInsecure = insecure; }

    /**
     * Returns whether bridging is insecure (host name validation)
     *
     * @return  Whether bridging is insecure (host name validation)
     */
    static bool isTlsBridgingInsecure() { return sm_tlsBridgingInsecure; }

    /**
     * Returns the default delay to apply when the topic routing cache is cleared.
     *
     * This is important as certain events in the broker (fabric change, etc.) can 
     * cause multiple calls to clear in a short period of time. The delay minimizes churn.
     *
     * @return  The default delay to apply when the topic routing cache is cleared.
     */
    static uint32_t getTopicRoutingCacheClearDelay() { return sm_topicRoutingCacheClearDelay; }

    /**
     * Sets the default delay to apply when the topic routing cache is cleared.
     *
     * This is important as certain events in the broker (fabric change, etc.) can 
     * cause multiple calls to clear in a short period of time. The delay minimizes churn.
     *
     * @param   delay The default delay to apply when the topic routing cache is cleared.
     */
    static void setTopicRoutingCacheClearDelay( uint32_t delay ) 
        { sm_topicRoutingCacheClearDelay = delay; }

    /*
     * Returns the brokerLib thread pool size
     *
     * @return  The brokerLib thread pool size
     */
    static uint32_t getBrokerLibThreadPoolSize() { return sm_brokerLibThreadPoolSize; }

    /**
     * Returns whether to validate certificates against the connection identifier
     *
     * @return  Whether to validate certificates against the connection identifier
     */
    static bool isCertIdentityValidationEnabled() { return sm_certIdentityValidationEnabled; }

    /**
     * Returns whether to restrict the fabric to a single instance of each unique client identifier
     *
     * @return  Whether to to restrict the fabric to a single instance of each unique client identifier
     */
    static bool isUniqueClientIdPerFabricEnabled() { return sm_uniqueClientIdPerFabricEnabled; }

    /**
     * Returns whether multi-tenant mode is enabled
     *
     * @return  Whether multi-tenant mode is enabled
     */
    static bool isMultiTenantModeEnabled() { return sm_multiTenantModeEnabled; }

    /**
     * Returns the tenant byte limit 
     *
     * @return  The tenant byte limit
     */
    static uint32_t getTenantByteLimit() { return sm_tenantByteLimit; }

    /**
     * Sets the tenant byte limit 
     *
     * @param   limit The tenant byte limit
     */
    static void setTenantByteLimit( uint32_t limit ) { sm_tenantByteLimit = limit; }

    /**
     * Returns the tenant connection limit 
     *
     * @return  The tenant connection limit
     */
    static uint32_t getTenantConnectionLimit() { return sm_tenantConnectionLimit; }

    /**
     * Sets the tenant connection limit 
     *
     * @param   limit The tenant connection limit
     */
    static void setTenantConnectionLimit( uint32_t limit ) { sm_tenantConnectionLimit = limit; }

    /** 
     * Whether to send events when clients connect/disconnect
     *
     * @return  Whether to send events when clients connect/disconnect
     */
    static bool isSendConnectEventsEnabled() { return sm_sendConnectEvents; }

    /**
     * Returns whether the broker is running in test mode
     *
     * @return  Whether the broker is running in test mode
     */
    static bool isTestModeEnabled() { return sm_testModeEnabled; }

    /**
     * Sets whether the broker is running in test mode
     *
     * @param   enabled Whether the broker is running in test mode
     */
    static void setTestModeEnabled( bool enabled ) { sm_testModeEnabled = enabled; }

    /**
     * Dumps the settings into a string
     *
     * @return  The settings dumped into a string
     */
    static std::string dumpSettings();

    /**
     * Returns whether the broker has been initialized
     *
     * @return  Whether  the broker has been initialized
     */
    static bool isBrokerInitialized() { return sm_brokerInitialized; }

    /**
     * Returns whether the broker has been initialized
     *
     * @param   initialized Whether the broker has been initialized
     */
    static void setBrokerInitialized( bool initialized ) { sm_brokerInitialized = initialized; }

private:
    /** The broker GUID */
    static std::string sm_guid;
    /** The broker tenant GUID */
    static std::string sm_tenantGuid;
    /** The broker listen port */
    static int sm_listenPort;
    /** The broker message size limit */
    static int sm_messageSizeLimit;
    /** The user to run the broker as */
    static std::string sm_user;
    /** The log file path */
    static std::string sm_logFilePath;
    /** The maximum log file size */
    static int sm_logFileMaxSize;
    /** The maximum log file count */
    static int sm_logFileMaxCount;
    /** Whether to use the standard output logger */
    static bool sm_logUseStdOut;
    /** The log level */
    static std::string sm_logLevel;
    /** Whether notice logging is enabled */
    static bool sm_noticeLoggingEnabled;

    /** The policy general state file */
    static std::string sm_policyGeneralStateFile;
    /** The policy broker state file */
    static std::string sm_policyBrokerStateFile;
    /** The policy topic authorization state file */
    static std::string sm_policyTopicAuthStateFile;

    /** The broker TTLS */
    static int sm_brokerTtl;

    /** The TTL grace period */
    static int sm_ttlGracePeriod;

    /** The TTL check interval in minutes */
    static int sm_ttlInterval;

    /** JSON pretty print */
    static bool sm_jsonPrettyPrint;

    /** Whether TLS is enabled */
    static bool sm_tlsEnabled;
    /** Whether TLS bridging is insecure */
    static bool sm_tlsBridgingInsecure;
    /** Whether FIPS is enabled */
    static bool sm_fipsEnabled;
    /** Keystore directory */
    static std::string sm_certKeyStoreDir;
    /** Broker cert chain file */
    static std::string sm_certBrokerCertChainFile;
    /** Client cert chain file */
    static std::string sm_certClientCertChainFile;
    /** Broker cert file */
    static std::string sm_certBrokerCertFile;
    /** Client cert file */
    static std::string sm_certClientCertFile;
    /** Broker CSR file */
    static std::string sm_certBrokerCsrFile;
    /** Client CSR file */
    static std::string sm_certClientCsrFile;
    /** Broker private key file */
    static std::string sm_certBrokerPrivateKeyFile;
    /** Client private key file */
    static std::string sm_certClientPrivateKeyFile;
    /** Revoked certificates file */
    static std::string sm_revokedCertsFile;
    /** The broker certificate hashes (SHA-1) list file */
    static std::string sm_brokerCertsListFile;
    /** Ciphers */
    static std::string sm_ciphers;
    /** Certificate signing retry interval */
    static int sm_certSignRetryMins;

    /** The interval for sampling messages (for messages/sec, etc.) */
    static int sm_messageSampleSecs;

    /** Whether to ignore the connection limit as received via policy (for testing) */
    static bool sm_ignoreConnectionLimit;

    /** The maximum size of the packet buffer in core (per context) */
    static uint64_t sm_maxPacketBufferSize;            

    /** The approximate broker state topics batch size (in terms of character count). */
    static int sm_brokerStateTopicsCharsBatchSize;

    /** Whether topic based routing is enabled */
    static bool sm_topicRoutingEnabled;    

    /** Whether the topic based routing cache is enabled */
    static bool sm_topicRoutingCacheEnabled;    

    /** The default delay to apply when the topic routing cache is cleared. */
    static uint32_t sm_topicRoutingCacheClearDelay;

    /** Whether to validate certificates against the connection identifier */
    static bool sm_certIdentityValidationEnabled;

    /** Whether to to restrict the fabric to a single instance of each unique client identifier */
    static bool sm_uniqueClientIdPerFabricEnabled;

    /** Whether test mode is enabled */
    static bool sm_testModeEnabled;

    /** The brokerLib thread pool size */
    static uint32_t sm_brokerLibThreadPoolSize;

    /** Whether multi-tenant mode is enabled */
    static bool sm_multiTenantModeEnabled;

    /** The tenant byte limit */
    static uint32_t sm_tenantByteLimit;

    /** The tenant connection limit */
    static uint32_t sm_tenantConnectionLimit;

    /** Whether to send events when clients connect/disconnect */
    static bool sm_sendConnectEvents;

    /** Whether the broker has been initialized */
    static bool sm_brokerInitialized;
};

} /* namespace broker */
} /* namespace dxl */

#endif /* BROKERSETTINGS_H_ */
