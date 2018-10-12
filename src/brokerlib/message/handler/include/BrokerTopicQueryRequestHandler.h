/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERTOPICQUERYREQUESTHANDLER_H_
#define BROKERTOPICQUERYREQUESTHANDLER_H_

#include "core/include/CoreOnStoreMessageHandler.h"

namespace dxl {
namespace broker {
namespace message {
namespace handler {

/**
 * Handler for "Broker Topic : query" request messages
 */
class BrokerTopicQueryRequestHandler : public dxl::broker::core::CoreOnStoreMessageHandler
{
public:
    /** Constructor */
    BrokerTopicQueryRequestHandler() {}
    
    /** Destructor */
    virtual ~BrokerTopicQueryRequestHandler() {}
    
    /** {@inheritDoc} */
    bool onStoreMessage( dxl::broker::core::CoreMessageContext* context,
        struct cert_hashes *certHashes ) const;
};

} /* namespace handler */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* BROKERTOPICQUERYREQUESTHANDLER_H_ */