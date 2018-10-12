/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "message/payload/include/FabricChangeEventPayload.h"

using namespace dxl::broker::message::payload;

/** {@inheritDoc} */
void FabricChangeEventPayload::write( Json::Value& /* out */ ) const
{
    // Nothing for now...
}

/** {@inheritDoc} */
void FabricChangeEventPayload::read( const Json::Value& /* in */ )
{
    // Nothing for now...
}
