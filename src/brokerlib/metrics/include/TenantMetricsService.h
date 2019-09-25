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
/** The count of services per tenant */
typedef unordered_map<std::string,int> servicesByTenant_t;

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
     * Updates the count of services
     *
     * @param   tenantId The tenant identifier
     * @param   adjCount The value to add (or subtract) from the current count
     */
    void updateTenantServiceCount( const char* tenantId, int adjCount );

    /**
     * Whether a service registration is allowed for the specified tenant
     *
     * @param   tenantId The tenant identifier
     * @return  Whether a service registration is allowed for the specified tenant
     */
    bool isServiceRegistrationAllowed( const char* tenantId ) const;

    /**
     * Returns whether a new tenant client subscription is allowed
     *
     * @param   tenantGuid The guid of the tenant trying to subscribe
     * @param   subscriptionCount The current number of subscriptions the client has
     * @return  whether a new tenant client subscription is allowed
     */
    bool isTenantSubscriptionAllowed( const char* tenantGuid, int subscriptionCount ) const;

    /**
     * Returns the single service instance
     * 
     * @return  The single service instance
     */
    static TenantMetricsService& getInstance();

private:

    /**
     * Updates the entry for the provided tenant in the provided counts by the provided amount.
     *
     * @param   tenantId The tenant identifier
     * @param   adjCount The amount to adjust the tenant count by
     * @param   limit The limit for the tenant count
     * @param   counts The map of tenant ID to count
     * @return  false if the limit has been exceeded, otherwise true
     */
    bool updateTenantLimitCount( const char* tenantId, int adjCount, int limit, unordered_map<std::string,int>& counts );

    /**
     * Determines whether a specified tenant is still within the given limit
     *
     * @param   tenantId The tenant identifier
     * @param   limit The tenant limit
     * @param   counts The current counts for each tenant
     * @return  Whether the specified tenant is within the specified limit
     */
    bool checkTenantWithinLimit( const char* tenantId, int limit, const unordered_map<std::string,int>& counts ) const;

    /**
     * Sends a DXL event notifying that a tenant limit has been exceeded
     *
     * @param   tenantId The tenant identifier
     * @param   limitType The type of limit the tenant has exceeded
     */
    void sendLimitExceededEvent( const char* tenantId, const char* limitType ) const;

    /** The count of sent bytes per tenant */
    sentBytesByTenant_t m_sentBytesPerTenant;
    /** The count of connections per tenant */
    connectionsByTenant_t m_connectionsPerTenant;
    /** The count of services per tenant */
    servicesByTenant_t m_servicesPerTenant;
};

} /* namespace metrics */
} /* namespace broker */
} /* namespace dxl */

#endif /* TENANTMETRICSSERVICE_H_ */
