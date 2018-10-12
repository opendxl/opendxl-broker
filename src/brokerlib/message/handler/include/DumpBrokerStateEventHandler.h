/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef DUMPBROKERSTATEEVENTHANDLER_H_
#define DUMPBROKERSTATEEVENTHANDLER_H_

#include "core/include/CoreOnPublishMessageHandler.h"

namespace dxl {
namespace broker {
namespace message {
namespace handler {

/**
 * Handler for dumping the broker state
 */
class DumpBrokerStateEventHandler : public dxl::broker::core::CoreOnPublishMessageHandler
{
public:
    /** Constructor */
    DumpBrokerStateEventHandler() {}

    /** Destructor */
    virtual ~DumpBrokerStateEventHandler() {}

    /** {@inheritDoc} */
    bool onPublishMessage( const char* sourceId, const char* canonicalSourceId, bool isBridge, uint8_t contextFlags,
        const char* topic, struct cert_hashes *certHashes ) const;
};

} /* namespace handler */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* DUMPBROKERSTATEEVENTHANDLER_H_ */
