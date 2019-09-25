/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef IBROKERLIB_H_
#define IBROKERLIB_H_

//
// This header contains methods for initializing and cleaning up the broker library.
//
// These methods will be invoked by core messaging directly.
//

#include "core/include/CoreInterface.h"
#include "cert_hashes.h"

/** The root DXL namespace */
namespace dxl {
/** Namespace for broker declarations */
namespace broker {

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
 * @param   coreLogCategoryMask The core log category mask (out)
 * @param   messageSizeLimit The core message size limit (out)
 * @param   user The user to run the broker as (out)
 * @param   brokerCertsUtHash List of broker certificate hashes (SHA-1) (out)
 * @param   webSocketsEnabled Whether WebSockets is enabled (out)
 * @param   webSocketsListenPort The broker WebSockets listen port (out)
 * @return  Whether Messaging core should continue starting
 */
bool brokerlib_main( 
    int argc, char *argv[], 
    bool* tlsEnabled, bool* tlsBridgingInsecure, bool* fipsEnabled,
    const char** clientCertChainFile, const char** brokerCertChainFile,
    const char** brokerKeyFile, 
    const char** brokerCertFile, const char** ciphers,
    uint64_t* maxPacketBufferSize, int* listenPort, int* coreLogType,
    unsigned int* coreLogCategoryMask, int* messageSizeLimit, char** user,
    struct cert_hashes** brokerCertsUtHash,
    bool *webSocketsEnabled, int* webSocketsListenPort );

/**
 * Initializes the broker library.
 *
 * @param   coreInterface The concrete implementation of the core interface (delivered by
 *          core implementation).
 *
 * @return  Whether the core should continue starting
 */
bool init( dxl::broker::core::CoreInterface* coreInterface );

/**
 * Allows for cleanup of the broker library
 */
void cleanup();

/**
 * Returns the core interface (if it has been set)
 *
 * @return  The core interface if set, NULL otherwise
 */
dxl::broker::core::CoreInterface* getCoreInterface();

} /* namespace broker */
} /* namespace dxl */

#endif  // BROKERLIB_H_
