/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef BROKERSTATEEVENTBUILDER_H_
#define BROKERSTATEEVENTBUILDER_H_

#include "message/include/DxlMessageBuilder.h"

namespace dxl {
namespace broker {
namespace message {
/** Namespace for building DXL messages */
namespace builder {

/**
 * Builds a "BrokerStateEvent" message for the local broker. 
 */
class BrokerStateEventBuilder : public dxl::broker::message::DxlMessageBuilder
{
public:
    /** Constructor */
    BrokerStateEventBuilder() {}

    /** Destructor */
    virtual ~BrokerStateEventBuilder() {}

    /** {@inheritDoc} */
    const std::shared_ptr<DxlMessage> buildMessage() const;
};

} /* namespace builder */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* BROKERSTATEEVENTBUILDER_H_ */
