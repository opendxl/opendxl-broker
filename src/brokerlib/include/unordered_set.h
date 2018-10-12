/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef UNORDEREDSET_H_
#define UNORDEREDSET_H_

/*
 * The implementation of "unordered set" that we are using.
 *
 * The Boost implementation has shown a significant performance increase
 * over the standard GCC implementation.
 *
 * But, this allows us to switch to the standard version if we choose to in the future.
 */

#include <boost/unordered_set.hpp>
#define unordered_set boost::unordered_set

#endif /* UNORDEREDSET_H_ */
