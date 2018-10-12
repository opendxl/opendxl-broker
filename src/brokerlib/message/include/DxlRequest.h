/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef DXLREQUEST_H_
#define DXLREQUEST_H_

#include "message/include/DxlMessage.h"

namespace dxl {
namespace broker {
namespace message {

/** Forward class reference */
class DxlMessageService;

/**
 * Represents a DXL "Request" message type
 */
class DxlRequest : public DxlMessage
{        
/** The message service is our friend. */
friend class dxl::broker::message::DxlMessageService;

public:    
    /** Destructor */
    virtual ~DxlRequest();

    /**
     * Returns the request's "reply to" topic
     *
     * @return  The request's "reply to" topic
     */
    const char* getReplyToTopic() const;

    /**
     * Sets the destination service identifier
     *
     * @param   serviceId The destination service identifier
     */
    void setDestinationServiceId( const char* serviceId );

    /**
     * Returns the destination service identifier
     *
     * @return  The destination service identifier
     */
    const char* getDestinationServiceId() const;

protected:
    /** 
     * Constructor 
     *
     * @param   msg The underlying message structure
     */
    DxlRequest( dxl_message_t* msg );
};

} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* DXLEVENT_H_ */
