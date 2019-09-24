/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/SimpleLog.h"
#include "include/BrokerSettings.h"
#include "message/include/DxlMessageConstants.h"
#include "message/include/DxlMessageService.h"
#include "message/payload/include/TenantExceedsLimitEventPayload.h"
#include "metrics/include/TenantMetricsService.h"

using namespace std;
using namespace dxl::broker::metrics;
using namespace dxl::broker::message;
using namespace dxl::broker::message::payload;

/** {@inheritDoc} */
TenantMetricsService& TenantMetricsService::getInstance()
{
    static TenantMetricsService instance;
    return instance;
}

/** {@inheritDoc} */
bool TenantMetricsService::updateTenantSentByteCount( const char* tenantId, uint32_t byteCount )
{
    // Determine tenant limit
    uint32_t limit = BrokerSettings::getTenantByteLimit();

    // Limit of 0 means unlimited
    if( limit == 0 )
    {
        return false;
    }

    uint32_t oldTotal = 0;
    uint32_t total = 0;

    // Lookup the total for the tenant
    auto f = m_sentBytesPerTenant.find( tenantId );
    if( f == m_sentBytesPerTenant.end() )
    {
        // New tenant entry
        m_sentBytesPerTenant.insert( make_pair( tenantId, byteCount ) );
        total = byteCount;
    }
    else
    {
        oldTotal = total = f->second;

        // Increment if not already exceeding
        if( oldTotal <= limit )
        {
            total += byteCount;
            f->second = total;
        }
    }

    // Check to see if limit is exceeded
    bool exceeded = total > limit;

    // Did this update cause the limit to be exceeded?
    if( ( oldTotal != total ) && exceeded )
    {
        if( SL_LOG.isInfoEnabled() )
        {
            SL_START << "Tenant byte limit exceeded, Tenant: " 
                << tenantId << ", total=" << total << ", limit=" << limit << SL_INFO_END;
        }

        TenantMetricsService::sendLimitExceededEvent( tenantId, TenantExceedsLimitEventPayload::TENANT_LIMIT_BYTE );
    }

    //SL_START << "(byte limit) Tenant: " << tenantId << ", total=" << total << ", limit=" << limit << SL_ERROR_END;

    return exceeded;
}

/** {@inheritDoc} */
bool TenantMetricsService::checkTenantWithinLimit( const char* tenantId, int limit,
        const unordered_map<std::string,int>& counts ) const
{

    // Limit of 0 means unlimited
    if( limit == 0 )
    {
        return true;
    }

    auto f = counts.find( tenantId );
    if( f == counts.end() )
    {
        return true;
    }

    return f->second < limit;
}

/** {@inheritDoc} */
bool TenantMetricsService::updateTenantLimitCount( const char* tenantId, int adjCount,
            int limit, unordered_map<std::string,int>& counts  )
{
    // Limit of 0 means unlimited
    if( limit == 0 )
    {
        return true;
    }

    // Lookup the total services for the tenant
    auto f = counts.find( tenantId );
    if( f == counts.end() )
    {
        // New tenant entry
        counts.insert( make_pair( tenantId, 0 ) );
        f = counts.find( tenantId );
        if( f == counts.end() )
        {
            // Should never happen
            return true;
        }

    }

    int oldCount = f->second;

    f->second += adjCount;
    if( f->second < 0 )
    {
        f->second = 0;
    }
    else if( f->second > limit )
    {
        f->second = limit;
    }

    if( ( oldCount != f->second ) && ( f->second >= limit ) )
    {
        return false;
    }

    //SL_START << "(limit) Tenant: " << tenantId << ", total=" <<  f->second << ", limit=" << limit << SL_ERROR_END;

    return true;
}

/** {@inheritDoc} */
void TenantMetricsService::updateTenantConnectionCount( const char* tenantId, int adjCount )
{
    // Determine tenant limit
    int limit = (int)BrokerSettings::getTenantConnectionLimit();

    if( !TenantMetricsService::updateTenantLimitCount( tenantId, adjCount, limit, m_connectionsPerTenant ) )
    {
        if( SL_LOG.isInfoEnabled() )
        {
            SL_START << "Tenant reached connection limit, Tenant: " 
                << tenantId << ", limit=" << limit << SL_INFO_END;

            TenantMetricsService::sendLimitExceededEvent( tenantId, TenantExceedsLimitEventPayload::TENANT_LIMIT_CONNECTIONS );
        }
    }

    //SL_START << "(conn limit) Tenant: " << tenantId << ", total=" <<  f->second << ", limit=" << limit << SL_ERROR_END;
}

/** {@inheritDoc} */
bool TenantMetricsService::isConnectionAllowed( const char* tenantId ) const
{
    return TenantMetricsService::checkTenantWithinLimit( tenantId,
                (int)BrokerSettings::getTenantConnectionLimit(), m_connectionsPerTenant );
}

/** {@inheritDoc} */
void TenantMetricsService::updateTenantServiceCount( const char* tenantId, int adjCount )
{
    // Determine tenant limit
    int limit = (int)BrokerSettings::getTenantServiceLimit();

    if( !TenantMetricsService::updateTenantLimitCount( tenantId, adjCount, limit, m_servicesPerTenant ) )
    {
        if( SL_LOG.isInfoEnabled() )
        {
            SL_START << "Tenant reached service limit, Tenant: "
                << tenantId << ", limit=" << limit << SL_INFO_END;

            TenantMetricsService::sendLimitExceededEvent( tenantId, TenantExceedsLimitEventPayload::TENANT_LIMIT_SERVICES );
        }
    }

    //SL_START << "(service limit) Tenant: " << tenantId << ", total=" <<  f->second << ", limit=" << limit << SL_ERROR_END;
}

/** {@inheritDoc} */
bool TenantMetricsService::isServiceRegistrationAllowed( const char* tenantId ) const
{
    return TenantMetricsService::checkTenantWithinLimit( tenantId,
                (int)BrokerSettings::getTenantServiceLimit(), m_servicesPerTenant );
}

/** {@inheritDoc} */
bool TenantMetricsService::isTenantSubscriptionAllowed( const char* tenantGuid, int subscriptionCount ) const
{
    bool subscriptionAllowed = BrokerSettings::getTenantClientSubscriptionLimit() == 0 ||
        subscriptionCount < BrokerSettings::getTenantClientSubscriptionLimit();

    if( !subscriptionAllowed )
    {
        TenantMetricsService::sendLimitExceededEvent( tenantGuid, TenantExceedsLimitEventPayload::TENANT_LIMIT_SUBSCRIPTIONS );
    }

    return subscriptionAllowed;
}

/** {@inheritDoc} */
void TenantMetricsService::markTenantExceedsByteCount( const char* tenantId )
{
    // Determine tenant limit
    uint32_t limit = BrokerSettings::getTenantByteLimit();

    // Limit of 0 means unlimited
    if( limit == 0 )
    {
        return;
    }

    if( SL_LOG.isInfoEnabled() )
    {
        SL_START << "Mark tenant exceeds byte count, Tenant: "
            << tenantId << ", limit=" << limit << SL_INFO_END;
    }

    // Lookup the total for the tenant
    auto f = m_sentBytesPerTenant.find( tenantId );
    if( f == m_sentBytesPerTenant.end() )
    {
        // New tenant entry
        m_sentBytesPerTenant.insert( make_pair( tenantId, limit + 1 ) );
    }
    else
    {
        f->second = limit + 1;        
    }
}

/** {@inheritDoc} */
void TenantMetricsService::resetTenantByteCounts()
{
    m_sentBytesPerTenant.clear();
}

/** {@inheritDoc} */
void TenantMetricsService::sendLimitExceededEvent( const char* tenantId, const char* limitType ) const
{
    try
    {
        // TODO: Throttle these so they won't spam
        // Fire event
        DxlMessageService& messageService = DxlMessageService::getInstance();
        shared_ptr<DxlEvent> evt = messageService.createEvent();
        TenantExceedsLimitEventPayload payload( tenantId, limitType );
        evt->setPayload( payload );
        messageService.sendMessage( DxlMessageConstants::CHANNEL_DXL_TENANT_LIMIT_EXCEEDED_EVENT, *evt );
    }
    catch( const exception& ex )
    {
        SL_START << "Error while sending limitExceeded event, " << ex.what() << SL_ERROR_END;
    }
    catch( ... )
    {
        SL_START << "Error while sending limitExceeded event, unknown exception" << SL_ERROR_END;
    }
}