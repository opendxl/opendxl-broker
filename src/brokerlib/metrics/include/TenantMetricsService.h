/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef TENANTMETRICSSERVICE_H_
#define TENANTMETRICSSERVICE_H_

#include <cstdint>
#include "include/unordered_map.h"

namespace dxl {
namespace broker {
/** Namespace for broker metrics */
namespace metrics {

/** The count of sent bytes per tenant */
typedef unordered_map<std::string,uint32_t> sentBytesByTenant_t;
/** The count of connections per tenant */
typedef unordered_map<std::string,int> connectionsByTenant_t;

/**
 * Service that tracks metrics per tenant. These metrics are used to limit the use of
 * the DXL fabric by tenants (bytes, connections, etc.).
 */
class TenantMetricsService 
{    
public:
    /** Destructor */
    virtual ~TenantMetricsService() {}

    /**
     * Updates the count of bytes sent
     *
     * @param   tenantId The tenant identifier
     * @param   byteCount The count of bytes being sent
     * @return  Whether the tenant has exceeded its limit
     */
    bool updateTenantSentByteCount( const char* tenantId, uint32_t byteCount );

    /**
     * Marks that the specified tenant has exceeded its byte count
     *
     * @param   tenantId The tenant identifier
     */
    void markTenantExceedsByteCount( const char* tenantId );

    /**
     * Resets the byte counts for tenants
     */
    void resetTenantByteCounts();

    /**
     * Updates the count of connections
     *
     * @param   tenantId The tenant identifier
     * @param   adjCount The value to add (or subtract) from the current count
     */
    void updateTenantConnectionCount( const char* tenantId, int adjCount );

    /**
     * Whether a connection is allowed for the specified tenant
     *
     * @param   tenantId The tenant identifier
     * @return  Whether a connection is allowed for the specified tenant
     */
    bool isConnectionAllowed( const char* tenantId ) const;

    /**
     * Returns the single service instance
     * 
     * @return  The single service instance
     */
    static TenantMetricsService& getInstance();
    
private:
    /** The count of sent bytes per tenant */
    sentBytesByTenant_t m_sentBytesPerTenant;
    /** The count of connections per tenant */
    connectionsByTenant_t m_connectionsPerTenant;
};

} /* namespace metrics */
} /* namespace broker */
} /* namespace dxl */

#endif /* TENANTMETRICSSERVICE_H_ */
