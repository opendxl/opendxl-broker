/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "util/include/StringUtil.h"

#include <sstream>

using namespace dxl::broker::util;
using namespace std;

/** {@inheritDoc} */
string StringUtil::toString( uint32_t value )
{
    stringstream ss;
    ss << value;
    return ss.str();
}

/** {@inheritDoc} */
uint32_t StringUtil::asUint32( const string& value )
{
    istringstream iss( value );
    uint32_t u32 = 0;
    iss >> u32;
    return u32;
}