/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef DXLERRORRESPONSE_H_
#define DXLERRORRESPONSE_H_

#include "message/include/DxlResponse.h"
#include <ctype.h>

namespace dxl {
namespace broker {
namespace message {

/** Forward class reference */
class DxlMessageService;

/**
 * Represents a DXL "Error Response" message type
 */
class DxlErrorResponse : public DxlResponse
{        
/** The message service is our friend. */
friend class dxl::broker::message::DxlMessageService;

public:    
    /** Destructor */
    virtual ~DxlErrorResponse();
    
    /**
     * Returns the error code for the error message
     *
     * @return  The error code for the error message
     */
    uint32_t getErrorCode() const;

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
     * @param   request The request that is being responded to
     * @param   msg The underlying message structure
     */
    DxlErrorResponse( const DxlRequest* request, dxl_message_t* msg );

    /** 
     * Constructor 
     *
     * @param   msg The underlying message structure
     */
    DxlErrorResponse( dxl_message_t* msg );
};

} /* namespace message */
} /* namespace broker */
} /* namespace dxl */

#endif /* DXLERRORRESPONSE_H_ */
