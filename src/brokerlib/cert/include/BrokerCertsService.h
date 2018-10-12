/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERCERTSSERVICE_H_
#define BROKERCERTSSERVICE_H_

#include "include/unordered_set.h"
#include "core/include/CoreMaintenanceListener.h"
#include <mutex>
#include <string>
#include <vector>

namespace dxl {
namespace broker {
/** Namespace for certificate-related declarations */
namespace cert {

/**
 * Methods used to generate the TLS-related files for the core messaging layer
 */
class BrokerCertsService
{    
public:
    /** Destructor */
    virtual ~BrokerCertsService() {}

    /**
     * Returns the single service instance
     * 
     * @return    The single service instance
     */
    static BrokerCertsService& getInstance();


    /** 
     * Returns the ASN1 NID for the Client GUID in certificates 
     *
     * @return  The ASN1 NID for the Client GUID in certificates
     */
    int getClientGuidNid() const { return m_clientGuidNid; }

    /** 
     * Returns the ASN1 NID for the Tenant GUID in certificates 
     *
     * @return  The ASN1 NID for the Tenant GUID in certificates
     */
    int getTenantGuidNid() const { return m_tenantGuidNid; }

    /**
     * Determines the broker tenant GUID by parsing the broker certificate and pulling
     * the tenant identifier from the X509 extension
     *
     * @return  The broker tenant GUID (or empty string if it could not be determined)
     */
    std::string determineBrokerTenantGuid() const;

    /**
     * Determines the broker GUID by parsing the broker certificate and pulling
     * the identifier from the X509 extension
     *
     * @return  The broker GUID (or empty string if it could not be determined)
     */
    std::string determineBrokerGuid() const;

    /** 
     * Whether the keys and certificates exist for the broker
     *
     * @return  Whether keys and certificates exist for the broker
     */
    bool getFilesExist();


private:
    /** Constructor */
    BrokerCertsService();

    /** 
     * Checks to see if the keys and certificates exist for the broker
     *
     * @return    whether keys and certificates exist for the broker
     */
    bool checkFilesExist() const;


    /** The ASN1 NID for the Client GUID in certificates  */
    int m_clientGuidNid;
    /** The ASN1 NID for the Tenant GUID in certificates  */
    int m_tenantGuidNid;
};

} /* namespace cert */
} /* namespace broker */
} /* namespace dxl */

#endif /* BROKERCERTSSERVICE_H_ */
