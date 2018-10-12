/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/SimpleLog.h"
#include "include/BrokerSettings.h"
#include "message/include/DxlMessageConstants.h"
#include "message/include/DxlMessageService.h"
#include "message/payload/include/TenantExceedsByteLimitEventPayload.h"
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

        // Fire event
        DxlMessageService& messageService = DxlMessageService::getInstance();
        shared_ptr<DxlEvent> evt = messageService.createEvent();
        TenantExceedsByteLimitEventPayload payload( tenantId );
        evt->setPayload( payload );
        messageService.sendMessage( DxlMessageConstants::CHANNEL_DXL_TENANT_LIMIT_EXCEEDED_EVENT, *evt );
    }

    //SL_START << "(byte limit) Tenant: " << tenantId << ", total=" << total << ", limit=" << limit << SL_ERROR_END;

    return exceeded;
}

/** {@inheritDoc} */
void TenantMetricsService::updateTenantConnectionCount( const char* tenantId, int adjCount )
{
    // Determine tenant limit
    int limit = (int)BrokerSettings::getTenantConnectionLimit();

    // Limit of 0 means unlimited
    if( limit == 0 )
    {
        return;
    }

    // Lookup the total for the tenant
    auto f = m_connectionsPerTenant.find( tenantId );
    if( f == m_connectionsPerTenant.end() )
    {
        // New tenant entry
        m_connectionsPerTenant.insert( make_pair( tenantId, 0 ) );
        f = m_connectionsPerTenant.find( tenantId );
        if( f == m_connectionsPerTenant.end() )
        {
            // Should never happen
            return;
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
        if( SL_LOG.isInfoEnabled() )
        {
            SL_START << "Tenant reached connection limit, Tenant: " 
                << tenantId << ", total=" << f->second << ", limit=" << limit << SL_INFO_END;
        }
    }

    //SL_START << "(conn limit) Tenant: " << tenantId << ", total=" <<  f->second << ", limit=" << limit << SL_ERROR_END;
}

/** {@inheritDoc} */
bool TenantMetricsService::isConnectionAllowed( const char* tenantId ) const
{
    // Determine tenant limit
    int limit = (int)BrokerSettings::getTenantConnectionLimit();

    // Limit of 0 means unlimited
    if( limit == 0 )
    {
        return true;
    }

    auto f = m_connectionsPerTenant.find( tenantId );
    if( f == m_connectionsPerTenant.end() )
    {
        return true;
    }

    return f->second < limit;
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
