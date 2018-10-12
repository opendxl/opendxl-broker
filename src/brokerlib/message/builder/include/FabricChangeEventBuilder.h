/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef FABRICCHANGEEVENTBUILDER_H_
#define FABRICCHANGEEVENTBUILDER_H_

#include "message/include/DxlMessageBuilder.h"

namespace dxl {
namespace broker {
namespace message {
namespace builder {

/**
 * Builds a "FabricChangeEvent" message, to publish to other brokers
 */
class FabricChangeEventBuilder : public dxl::broker::message::DxlMessageBuilder
{
public:
    /** Constructor */
    FabricChangeEventBuilder() {}

    /** Destructor */
    virtual ~FabricChangeEventBuilder() {}

    /** {@inheritDoc} */
    const std::shared_ptr<DxlMessage> buildMessage() const;
};

} /* namespace builder */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* FABRICCHANGEEVENTBUILDER_H_ */
