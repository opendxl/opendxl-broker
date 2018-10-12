/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef REVOCATIONSERVICE_H_
#define REVOCATIONSERVICE_H_

#include "include/unordered_set.h"
#include "core/include/CoreMaintenanceListener.h"
#include "cert_hashes.h"
#include <string>

namespace dxl {
namespace broker {
namespace cert {

/**
 * Service that tracks certificates that have been revoked
 */
class RevocationService : public dxl::broker::core::CoreMaintenanceListener
{
public:
    /** Destructor */
    virtual ~RevocationService() {}

    /**
     * Returns the single service instance
     * 
     * @return  The single service instance
     */
    static RevocationService& getInstance();

    /**
     * Returns whether the specified certificate is revoked
     *
     * @param   cert The certificate
     * @return  Whether the specified certificate is revoked
     */     
    bool isRevoked( const char* cert ) const;

    /**
     * Adds the specified certificate (thumbprint) to the set of revoked certificates
     *
     * @param   cert The certificate (thumbprint) to add to the revoked certificates
     */
    void addCertificate( const char* cert );

    /**
     * Reads the revoked certificates from the specified file
     *
     * @param   filename The file to read the revoked certificates to
     */
    bool readFromFile( const std::string& filename );

    /** {@inheritDoc} */
    void onCoreMaintenance( time_t time );

private:
    /** Constructor */
    RevocationService();

    /**
     * Writes the revoked certificates to the specified file
     *
     * @param   filename The file to write the revoked certificates to
     */
    bool writeToFile( const std::string& filename ) const;

    /** The set of revoked certificates */
    unordered_set<std::string> m_certs;

    /** The set of new certificates */
    cert_hashes *m_newCerts;
};

}
}
}

#endif /* REVOCATIONSERVICE_H_ */
