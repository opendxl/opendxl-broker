/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef COREUTIL_H_
#define COREUTIL_H_

namespace dxl {
namespace broker {
namespace core {

/**
 * Core messaging utility methods
 */
class CoreUtil
{    
public:
    /** 
     * Begins iteration of the wildcard patterns in the specified topic
     *
     * @param   topic The topic
     * @return  The modified topic (a copy of the topic with some padding)
     */
    static char* iterateWildcardBegin( const char* topic );

    /**
     * Returns the next wildcard pattern (if found)
     *
     * @param   modifiedTopic The modified topic
     * @return  Whether we found the next wildcard pattern (or none were found)
     */
    static bool iterateWildcardNext( char* modifiedTopic );

    /**
     * Completes the iteration (frees the modified topic)
     *
     * @param   modifiedTopic The modified topic that will be freed
     */
    static void iterateWildcardEnd( char *modifiedTopic );

    /**
     * Whether the specified topic is a wildcard topic
     *
     * @param   topic The topic
     * @return  Whether the specified topic is a wildcard topic
     */
    static bool isWildcard( const char* topic );
};
    

} /* namespace core */
} /* namespace broker */
} /* namespace dxl */

#endif /*  COREUTIL_H_ */
