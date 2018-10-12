/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/include/DxlEvent.h"

using namespace dxl::broker::message;

/** {@inheritDoc} */
DxlEvent::DxlEvent( dxl_message_t* msg ) : DxlMessage( msg )
{  
    
}

/** {@inheritDoc} */
DxlEvent::~DxlEvent()
{
}


