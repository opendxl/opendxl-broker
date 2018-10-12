/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef MANAGEDCERTS_H_
#define MANAGEDCERTS_H_

#include "include/unordered_set.h"
#include "cert_hashes.h"
#include <string>

namespace dxl {
namespace broker {
namespace cert {

/**
 * This class contains SHA-1 values for certificates and CAs
 */
class ManagedCerts
{
public:
    /**
     * Constructs ManagedCerts with the SHA-1 values specified
     *
     * @param   managedCerts The set of SHA-1 values for the certificates and CAs
     */
    ManagedCerts( const unordered_set<std::string>& managedCerts );

    /** Destructor */
    virtual ~ManagedCerts();

    /**
     * Persists the contents of this class to a newly allocated UT Hash
     *
     * @param   uthashptr The UT hash pointer to write to (out)
     * @return  Whether the write succeeded
     */
    bool writeToUthash( struct cert_hashes** uthashptr ) const;

    /** Equality */
    bool operator==( const ManagedCerts& rhs ) const;
    /** Inequality */
    bool operator!=( const ManagedCerts& rhs ) const;

    /**
     * Returns an instance of the ManagedCerts class after reading SHA-1 values from the
     * specified file name.
     *
     * @param   filename The file to read the SHA-1 value from
     * @param   certs A pointer to the managed certs object to populate (out)
     * @return  Whether the instance was successfully created.
     */
    static bool CreateFromFile( 
        const std::string& filename, std::shared_ptr<const ManagedCerts>& certs );

    /**
     * Persists the set of cert hashes (as strings) to a newly allocated UT Hash
     *
     * @param   certs The set of cert hashes (as strings)
     * @param   uthashptr The UT hash pointer to write to (out)
     * @return  Whether the write succeeded
     */
    static bool writeToUthash(
        const unordered_set<std::string>& certs, struct cert_hashes** uthashptr );

protected:    
    /** The set of SHA-1 values for the certificates and CAs */
    unordered_set<std::string> m_managedCerts;
};

}
}
}

#endif /* MANAGEDCERTS_H_ */
