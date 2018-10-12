/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef UNORDEREDMAP_H_
#define UNORDEREDMAP_H_

/*
 * The implementation of "unordered map" that we are using.
 *
 * The Boost implementation has shown a significant performance increase
 * over the standard GCC implementation.
 *
 * But, this allows us to switch to the standard version if we choose to in the future.
 */

#include <boost/unordered_map.hpp>
#define unordered_map boost::unordered_map

#endif /* UNORDEREDMAP_H_ */
