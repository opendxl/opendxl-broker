/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef DXLEVENT_H_
#define DXLEVENT_H_

#include "message/include/DxlMessage.h"

namespace dxl {
namespace broker {
namespace message {

/** Forward class reference */
class DxlMessageService;

/**
 * Represents a DXL "Event" message type
 */
class DxlEvent : public DxlMessage
{        
/** The message service is our friend. */
friend class dxl::broker::message::DxlMessageService;

public:    
    /** Destructor */
    virtual ~DxlEvent();

protected:
    /** 
     * Constructor 
     *
     * @param   msg The underlying message structure
     */
    DxlEvent( dxl_message_t* msg );
};

} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* DXLEVENT_H_ */
