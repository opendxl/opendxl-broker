/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef DUMPSERVICESTATEEVENTHANDLER_H_
#define DUMPSERVICESTATEEVENTHANDLER_H_

#include "core/include/CoreOnPublishMessageHandler.h"

namespace dxl {
namespace broker {
namespace message {
namespace handler {

/**
 * Handler for dumping the services state
 */
class DumpServiceStateEventHandler : public dxl::broker::core::CoreOnPublishMessageHandler
{
public:
    /** Constructor */
    DumpServiceStateEventHandler() {}

    /** Destructor */
    virtual ~DumpServiceStateEventHandler() {}

    /** {@inheritDoc} */
    bool onPublishMessage( const char* sourceId, const char* canonicalSourceId, bool isBridge, uint8_t contextFlags,
        const char* topic, struct cert_hashes *certHashes ) const;
};

} /* namespace handler */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* DUMPSERVICESTATEEVENTHANDLER_H_ */
