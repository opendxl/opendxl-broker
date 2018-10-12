/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef DXLMESSAGEHANDLERS_H_
#define DXLMESSAGEHANDLERS_H_

namespace dxl {
namespace broker {
namespace message {
/** Namespace for handling including DXL messages (events and requests) */
namespace handler {

/**
 * Responsible for registering the standard DXL message handlers (broker state, fabric
 * change, etc.).
 */
class DxlMessageHandlers
{
public:
    /**
     * Registers the standard DXL message handlers
     */
    static void registerHandlers();


    /**
     * Registers the "test-mode" DXL message handlers
     */
    static void registerTestHandlers();    
};

} /* namespace handler */
} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* DXLMESSAGEHANDLERS_H_ */
