/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef TENANTEXCEEDSBYTELIMITEVENTPAYLOAD_H_
#define TENANTEXCEEDSBYTELIMITEVENTPAYLOAD_H_

#include "json/include/JsonReader.h"
#include "json/include/JsonWriter.h"

namespace dxl {
namespace broker {
namespace message {
namespace payload {

/**
 * Payload for a "Tenant exceeds byte limit event" message
 */
class TenantExceedsByteLimitEventPayload :     
    public dxl::broker::json::JsonReader,
    public dxl::broker::json::JsonWriter
{
public:
    /** 
     * Constructor
     *
     * @param   tenantId The tenant identifier
     */
    explicit TenantExceedsByteLimitEventPayload( 
        const std::string& tenantId = "" )
    {
        m_tenantId = tenantId;
    }

    /** Destructor */
    virtual ~TenantExceedsByteLimitEventPayload() {}

    /**
     * Returns the identifier of the tenant that exceeded the limit
     *
     * @return  The identifier of the tenant that exceeded the limit
     */
    std::string getTenantId() const { return m_tenantId; }

    /** {@inheritDoc} */
    void read( const Json::Value& in );

    /** {@inheritDoc} */
    void write( Json::Value& out ) const;    

    /** Equals operator */
    bool operator==( const TenantExceedsByteLimitEventPayload& rhs ) const;

    /** Not equals operator */
    bool operator!= (const TenantExceedsByteLimitEventPayload& rhs ) const {
        return !( *this == rhs );
    }

private:
    /** The tenant identifier */
    std::string m_tenantId;
};

} /* namespace payload */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* TENANTEXCEEDSBYTELIMITEVENTPAYLOAD_H_ */
