/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef DXLRESPONSE_H_
#define DXLRESPONSE_H_

#include "message/include/DxlRequest.h"

namespace dxl {
namespace broker {
namespace message {

/** Forward class reference */
class DxlMessageService;

/**
 * Represents a DXL "Response" message type
 */
class DxlResponse : public DxlMessage
{        
/** The message service is our friend. */
friend class dxl::broker::message::DxlMessageService;

public:    
    /** Destructor */
    virtual ~DxlResponse();

    /**
     * Returns the destination service identifier
     *
     * @return  The destination service identifier
     */
    virtual const char* getDestinationServiceId() const;

    /**
     * Sets the destination service identifier
     *
     * @param   serviceId The destination service identifier
     */
    void setDestinationServiceId( const char* serviceId );

protected:
    /** 
     * Constructor 
     *
     * @param   request The request that is being responded to
     * @param   msg The underlying message structure
     */
    DxlResponse( const DxlRequest* request, dxl_message_t* msg );

    /** 
     * Constructor 
     *
     * @param   msg The underlying message structure
     */
    DxlResponse( dxl_message_t* msg );
};

} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* DXLRESPONSE_H_ */
