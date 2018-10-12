/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef MESSAGEROUTINGHANDLER_H_
#define MESSAGEROUTINGHANDLER_H_

#include "core/include/CoreOnInsertMessageHandler.h"

namespace dxl {
namespace broker {
namespace message {
namespace handler {

/**
 * Handler for routing messages
 */
class MessageRoutingHandler : public dxl::broker::core::CoreOnInsertMessageHandler
{
public:
    /** Constructor */
    MessageRoutingHandler() {}

    /** Destructor */
    virtual ~MessageRoutingHandler() {}

    /** {@inheritDoc} */
    bool onInsertMessage(
        dxl::broker::core::CoreMessageContext* context, const char* destId,
        const char* canonicalDestId, bool isBridge, uint8_t contextFlags, const char* targetTenantGuid,
        struct cert_hashes *certHashes, bool* isClientMessageEnabled, unsigned char** clientMessage, size_t* clientMessageLen ) const;

private:
    /**
     * Method that determines whether to insert the message when in multi-tenant mode
     *
     * @param   message The DXL message
     * @param   targetContextFlags The flags associated with the context being inserted into
     * @param   context The message context
     * @param   targetTenantGuid The tenant GUID associated with the context being inserted into
     * @return  Whether to allow the insertion of the message
     */
    bool onInsertMultiTenantMessage( DxlMessage* message, uint8_t targetContextFlags,
        dxl::broker::core::CoreMessageContext* context, const char* targetTenantGuid) const;
};

} /* namespace handler */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* MESSAGEROUTINGHANDLER_H_ */
