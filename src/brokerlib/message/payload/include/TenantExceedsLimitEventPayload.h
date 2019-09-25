/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef TENANTEXCEEDSLIMITEVENTPAYLOAD_H_
#define TENANTEXCEEDSLIMITEVENTPAYLOAD_H_

#include "json/include/JsonReader.h"
#include "json/include/JsonWriter.h"

namespace dxl {
namespace broker {
namespace message {
namespace payload {

/**
 * Payload for a "Tenant exceeds limit event" message
 */
class TenantExceedsLimitEventPayload :
    public dxl::broker::json::JsonReader,
    public dxl::broker::json::JsonWriter
{
public:

    // Value constants for the type
    static const char* TENANT_LIMIT_BYTE;
    static const char* TENANT_LIMIT_SUBSCRIPTIONS;
    static const char* TENANT_LIMIT_SERVICES;
    static const char* TENANT_LIMIT_CONNECTIONS;

    /** 
     * Constructor
     *
     * @param   tenantId The tenant identifier
     */
    explicit TenantExceedsLimitEventPayload(
        const std::string& tenantId = "", const std::string& type = TENANT_LIMIT_BYTE )
    {
        m_tenantId = tenantId;
        m_type = type;
    }

    /** Destructor */
    virtual ~TenantExceedsLimitEventPayload() {}

    /**
     * Returns the identifier of the tenant that exceeded the limit
     *
     * @return  The identifier of the tenant that exceeded the limit
     */
    std::string getTenantId() const { return m_tenantId; }

    /**
     * Returns the type of limit that was exceeded
     *
     * @return  the type of limit that was exceeded
     */
    std::string getType() const { return m_type; }

    /** {@inheritDoc} */
    void read( const Json::Value& in );

    /** {@inheritDoc} */
    void write( Json::Value& out ) const;    

    /** Equals operator */
    bool operator==( const TenantExceedsLimitEventPayload& rhs ) const;

    /** Not equals operator */
    bool operator!= (const TenantExceedsLimitEventPayload& rhs ) const {
        return !( *this == rhs );
    }

private:
    /** The tenant identifier */
    std::string m_tenantId;
    /** The type of limit exceeded */
    std::string m_type;
};

} /* namespace payload */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* TENANTEXCEEDSLIMITEVENTPAYLOAD_H_ */
