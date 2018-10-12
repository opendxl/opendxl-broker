/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/brokerlib.h"
#include <ctime>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include "cert_hashes.h"
#include "include/Configuration.h"
#include "include/SimpleLog.h"
#include "include/BrokerSettings.h"
#include "include/BrokerHelpers.h"
#include "cert/include/ManagedCerts.h"
#include "include/GeneralPolicySettings.h"
#include "brokerconfiguration/include/BrokerConfigurationService.h"
#include "brokerregistry/include/brokerregistry.h"
#include "cert/include/BrokerCertsService.h"
#include "cert/include/RevocationService.h"
#include "core/include/CoreInterfaceEventHandler.h"
#include "core/include/CoreMessageHandlerService.h"
#include "message/include/DxlMessageService.h"
#include "message/handler/include/DxlMessageHandlers.h"
#include "serviceregistry/include/ServiceRegistry.h"
#include "topicauthorization/include/topicauthorizationservice.h"
#include "topicauthorization/include/topicauthorizationstate.h"
#include "util/include/FileUtil.h"
#include "util/include/TimeUtil.h"
#include "util/include/BrokerLibThreadPool.h"

#include "dxlcommon.h"

using namespace std;
using namespace dxl::broker;
using namespace dxl::broker::cert;
using namespace dxl::broker::message;
using namespace dxl::broker::message::handler;
using namespace dxl::broker::core;
using namespace dxl::broker::service;
using namespace dxl::broker::util;

namespace dxl {
namespace broker {


/** The Disk logger */
static SimpleLogger::DiskLogger* s_disklogger = NULL;

/** The Standard Output logger */
static SimpleLogger::StdOutLogger* s_stdoutlogger = NULL;

/** The core interface */
static CoreInterface* s_coreInterface = NULL;

/**
 * Initializes logging for the broker and broker library
 */
static void InitializeLogging() 
{
    // Disk logger
    s_disklogger = 
        new SimpleLogger::DiskLogger(
            BrokerSettings::getLogFilePath().c_str(), 
            BrokerSettings::getLogFileMaxSize(), 
            BrokerSettings::getLogFileMaxCount() );
    SL_LOG.addListener( s_disklogger );

    // Standard output logger
    if( BrokerSettings::isStdOutLoggingEnabled() ) 
    {
        s_stdoutlogger = new SimpleLogger::StdOutLogger();
        SL_LOG.addListener( s_stdoutlogger );
    }

    // Log level
    SL_LOG.setLogLevelFilter( SimpleLogger::warn );  // Default level
    SL_LOG.setLogLevelFilter( BrokerSettings::getLogLevel() );
}

/**
 * Return the location of the broker library configuration file
 *
 * @param   argc The count of command line arguments
 * @param   argv The command line arguments
 * @param   configLocation The configuration location (the string to update)
 */
static void getConfigurationLocation( int argc, char *argv[], string& configLocation )
{
    configLocation = CONFIG_LOC;
    for( int i = 1; i < argc; i++ ) 
    {
        if( !strcmp( argv[i], "--config" ) ) 
        {
            if( i < argc - 1 ) 
            { 
                configLocation = argv[i + 1];
                break;
            } 
        }
    }
}

/**
 * Wait for the broker certificates to be obtained
 *
 * @return  Whether the certificates are available
 */
static bool waitForBrokerCertificates()
{
    BrokerCertsService& certsService = BrokerCertsService::getInstance();

    // Whether the certificate files exist
    bool filesExist = certsService.getFilesExist();

    // If the broker is in multi-tenant mode, we need to determine the
    // OPS tenant identifier (via the certificate)
    bool isMultiTenantMode = BrokerSettings::isMultiTenantModeEnabled();
    // The broker tenant GUID (used if in multi-tenant mode)
    string brokerTenantGuid;

    if( isMultiTenantMode && filesExist )
    {
        brokerTenantGuid = certsService.determineBrokerTenantGuid();
    }


    if( !filesExist ||
        ( isMultiTenantMode && brokerTenantGuid.empty() ) )
    {
            const string errorMsg = "Unable to find file required for TLS: ";
            if( !FileUtil::fileExists( BrokerSettings::getBrokerPrivateKeyFile() ) )
            {
                SL_START << errorMsg << 
                    BrokerSettings::getBrokerPrivateKeyFile() << SL_ERROR_END;
            }
            if( !FileUtil::fileExists( BrokerSettings::getBrokerCertFile() ) )
            {
                SL_START << errorMsg <<
                    BrokerSettings::getBrokerCertFile() << SL_ERROR_END;
            }
            if( !FileUtil::fileExists( BrokerSettings::getBrokerCertChainFile() ) )
            {
                SL_START << errorMsg << 
                    BrokerSettings::getBrokerCertChainFile() << SL_ERROR_END;
            }
            if( !FileUtil::fileExists( BrokerSettings::getClientCertChainFile() ) )
            {
                SL_START << errorMsg << 
                    BrokerSettings::getClientCertChainFile() << SL_ERROR_END;
            }
            if( isMultiTenantMode && brokerTenantGuid.empty() )
            {
                SL_START << "A tenant GUID was not found in the broker's certificate"
                    << SL_ERROR_END;
            }

            return false;
    }

    if ( SL_LOG.isInfoEnabled() )
    {
        SL_START << "Certificate files found." << SL_INFO_END;
    }

    if( isMultiTenantMode )
    {
        // Set the broker tenant GUID
        BrokerSettings::setTenantGuid( brokerTenantGuid.c_str() );
        SL_START << "Broker tenant GUID: " << BrokerSettings::getTenantGuid() << SL_INFO_END;
    }

    return true;
}

/* Core Log types */
#define CORE_LOG_NONE 0x00
#define CORE_LOG_INFO 0x01
#define CORE_LOG_NOTICE 0x02
#define CORE_LOG_WARNING 0x04
#define CORE_LOG_ERR 0x08
#define CORE_LOG_DEBUG 0x10
#define CORE_LOG_SUBSCRIBE 0x20
#define CORE_LOG_UNSUBSCRIBE 0x40
#define CORE_LOG_ALL 0xFFFF

/**
 * Invoked immediately from the core messaging main method
 *
 * @param   argc The argument count
 * @param   argv The arguments
 * @param   tlsEnabled Whether TLS is enabled (out)
 * @param   tlsBridgingInsecure Whether TLS bridging is insecure (out)
 * @param   fipsEnabled Whether FIPS is enabled (out)
 * @param   clientCertChainFile The client certificate chain file (out)
 * @param   brokerCertChainFile The broker certificate chain file (out)
 * @param   brokerKeyFile The broker key file (out)
 * @param   brokerCertFile The broker certificate file (out)
 * @param   ciphers The ciphers to restrict to (out)
 * @param   maxPacketBufferSize The maximum packet buffer size (out)
 * @param   listenPort The listener port (out)
 * @param   coreLogType The core log types (out)
 * @param   messageSizeLimit The core message size limit (out)
 * @param   user The user to run the broker as (out)
 * @param   brokerCertsUtHash List of broker certificate hashes (SHA-1) (out)
 * @return  Whether Messaging core should continue starting
 */
bool brokerlib_main( 
    int argc, char *argv[], 
    bool* tlsEnabled, bool* tlsBridgingInsecure, bool* fipsEnabled,
    const char** clientCertChainFile, const char** brokerCertChainFile,
    const char** brokerKeyFile, const char** brokerCertFile, const char** ciphers,
    uint64_t* maxPacketBufferSize, int* listenPort, int* coreLogType,
    int* messageSizeLimit, char **user,
    struct cert_hashes** brokerCertsUtHash )
{
    bool succeeded = false;
    try 
    {    
        // Read broker configuration
        string configLocation;
        getConfigurationLocation( argc, argv, configLocation );
        Configuration& config = Configuration::Instance();
        config.readConfiguration( configLocation, true );

        // Set the broker settings from the configuration
        BrokerSettings::setValuesFromConfig( config );

        // Set the listen port
        *listenPort = BrokerSettings::getListenPort();

        // Initialize logging
        InitializeLogging();
        if( SL_LOG.isInfoEnabled() ) 
        {
            SL_START << "DXL Broker starting..." << SL_INFO_END;
        }

        // Set the user
        *user = strdup( BrokerSettings::getUser().c_str() );

        // Set message size limit
        *messageSizeLimit = BrokerSettings::getMessageSizeLimit();

        // Set the core logging type
        *coreLogType = 0;
        if( SL_LOG.isDebugEnabled() )
        {
            *coreLogType |= CORE_LOG_DEBUG;
        }
        if( SL_LOG.isInfoEnabled() )
        {
            *coreLogType |= CORE_LOG_INFO;
        }
        if( SL_LOG.isWarnEnabled() )
        {
            *coreLogType |= CORE_LOG_WARNING;
        }
        if( SL_LOG.isErrorEnabled() )
        {
            *coreLogType |= CORE_LOG_ERR;
        }
        if( BrokerSettings::isNoticeLoggingEnabled() )
        {
            *coreLogType |= CORE_LOG_NOTICE;
        }

            const char* guid = BrokerSettings::getGuid( false );
            if( !guid || strlen( guid ) == 0 )
            {
                SL_START << "A broker identifier has not been specified." << SL_ERROR_END;
                return false;
            }

        // Dump broker settings
        if( SL_LOG.isInfoEnabled() ) 
        {
            SL_START << BrokerSettings::dumpSettings() << SL_INFO_END;
        }

        // Set whether TLS is enabled
        *tlsEnabled = BrokerSettings::isTlsEnabled();
        // Set whether TLS bridging is insecure (not validating host name)
        *tlsBridgingInsecure = BrokerSettings::isTlsBridgingInsecure();
        // Set whether FIPS is enabled
        *fipsEnabled = BrokerSettings::isFipsEnabled();

        if( SL_LOG.isInfoEnabled() ) 
        {
            if( *tlsEnabled )
            {
                SL_START << "TLS Enabled." << SL_INFO_END;
            }
            else
            {
                SL_START << "TLS Disabled." << SL_INFO_END;
            }
        }

        // Configure TLS (if applicable)
        if( *tlsEnabled )
        {
            if( waitForBrokerCertificates() )

            {
                *clientCertChainFile = strdup( BrokerSettings::getClientCertChainFile().c_str() );
                *brokerCertChainFile = strdup( BrokerSettings::getBrokerCertChainFile().c_str() );
                *brokerKeyFile = strdup( BrokerSettings::getBrokerPrivateKeyFile().c_str() );
                *brokerCertFile = strdup( BrokerSettings::getBrokerCertFile().c_str() );
                *ciphers = strdup( BrokerSettings::getCiphers().c_str() );

                if( FileUtil::fileExists( BrokerSettings::getBrokerCertsListFile() ) )
                {
                    shared_ptr<const ManagedCerts> brokerCerts;
                    if( !ManagedCerts::CreateFromFile(
                        BrokerSettings::getBrokerCertsListFile().c_str(), brokerCerts ) )
                    {
                        SL_START << "An error occurred attempting to read the broker certs list file."
                            << SL_ERROR_END;
                        return false;
                    }

                    brokerCerts->writeToUthash( brokerCertsUtHash );
                }
            }
            else
            {
                // Unable to find broker-related certificates
                return false;
            }
        }

        // Checks that tenant GUID has been specified when broker is unmanaged and MT is enabled.
        if( BrokerSettings::isMultiTenantModeEnabled() )
        {
            const char* tenantGuid = BrokerSettings::getTenantGuid( false );
            if( !tenantGuid || strlen( tenantGuid ) == 0 )
            {
                SL_START << "A tenant identifier was not found." << SL_ERROR_END;
                return false;
            }
        }

        // Set packet buffer size
        *maxPacketBufferSize = BrokerSettings::getMaxPacketBufferSize();

        succeeded = true;
    } 
    catch( std::runtime_error& e ) 
    {
        SL_START << "A runtime error occurred: " << e.what() << SL_ERROR_END;
    }
    catch( ... ) 
    {
        SL_START << "An unknown error occurred" << SL_ERROR_END;
    }

    return succeeded;
}

/** {@inheritDoc} */
bool init( CoreInterface* coreInterface )
{
    // a simple guard to prevent multiple initializations
    // if we need to allow for multiple initialization, we need to refactor    
    if( BrokerSettings::isBrokerInitialized() ) 
    {
        return true;
    }

    bool succeeded = false;
    try 
    {    
        if( SL_LOG.isInfoEnabled() ) 
        {
            SL_START << "Initializing DXL Broker..." << SL_INFO_END;
        }

        // Store the core interface
        s_coreInterface = coreInterface;


        // Add our local broker to the registry
        BrokerRegistry::getInstance().addBroker( 
            BrokerSettings::getGuid(),  
            coreInterface->getBrokerHostname(),
            coreInterface->getBrokerPort(),
            BrokerSettings::getBrokerTtlMins(),
            TimeUtil::getCurrentTimeSeconds() );
        coreInterface->addMaintenanceListener( &BrokerRegistry::getInstance() );

        // Register message handlers
        coreInterface->addMaintenanceListener( &CoreMessageHandlerService::getInstance() );
        DxlMessageHandlers::registerHandlers();

        // Configure message service
        DxlMessageService::getInstance().setCoreInterface( coreInterface );

        // Start the event handler for the interface
        CoreInterfaceEventHandler::start( coreInterface );

        // Create service registry so it can add appropriate listeners
        // This must be done prior to initializing the broker configuration
        ServiceRegistry::getInstance();

        // Read general policy settings (again)
        // Messaging core has been initialized, events will be fired appropriately
        GeneralPolicySettings::loadSettings();

        // Initialize broker configuration
        BrokerConfigurationService::Instance()->setBrokerConfiguration(
            BrokerConfiguration::CreateFromJsonConfigFile(
                BrokerSettings::getPolicyBrokerStateFile() ) );

        // Load topic authorization state
        auto topicstate =
            TopicAuthorizationState::CreateFromJsonFile(
                BrokerSettings::getPolicyTopicAuthStateFile() );
        TopicAuthorizationService::Instance()->setTopicState( topicstate );

        // Load revoked certificates
        RevocationService::getInstance().readFromFile( BrokerSettings::getRevokedCertsFile() );

        // Create the broker library thread pool
        BrokerLibThreadPool::getInstance();


        BrokerSettings::setBrokerInitialized( true );

        succeeded = true;
    } 
    catch( std::runtime_error& e ) 
    {
        SL_START << "A runtime error occurred: " << e.what() << SL_ERROR_END;
    }
    catch( ... ) 
    {
        SL_START << "An unknown error occurred" << SL_ERROR_END; 
    }

    return succeeded;
}

/** {@inheritDoc} */
void cleanup() 
{
    if( SL_LOG.isInfoEnabled() )
    {
        SL_START << "Cleaning up DXL Broker..." << SL_INFO_END;
    }

    /** Stops all threads and waits for them to end */    
    BrokerLibThreadPool::getInstance().shutdown();


    if( s_disklogger ) 
    {
        s_disklogger->close();
        SL_LOG.removeListener( s_disklogger );
        delete s_disklogger;
    }

    if( s_stdoutlogger ) 
    {
        SL_LOG.removeListener( s_stdoutlogger );
        delete s_stdoutlogger;
    }
}

/** {@inheritDoc} */
CoreInterface* getCoreInterface()
{
    return s_coreInterface;
}


}  // namespace broker
}  // namespace dxl
