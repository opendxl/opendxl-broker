/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef DXLMESSAGEBUILDER_H_
#define DXLMESSAGEBUILDER_H_

#include "message/include/DxlMessage.h"
#include <memory>

namespace dxl {
namespace broker {
namespace message {

/**
 * Interface implemented by objects that are responsible for building DXL messages.
 * This allows for the various interactions necessary to create a message 
 * (creating a message type, setting payload, etc.) being centralized in a single 
 * object. For example, a message builder can be passed directly to the "DxlMessageService"
 * to send a message.
 */
class DxlMessageBuilder
{    
public:
    /** Destructor */
    virtual ~DxlMessageBuilder() {};

    /**
     * Builds and returns a DXL message
     *
     * @return  The DXL message that has been built.
     */
    virtual const std::shared_ptr<DxlMessage> buildMessage() const = 0;

protected:
    /** Constructor */
    DxlMessageBuilder() {};    
};

} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* DXLMESSAGEBUILDER_H_ */
