/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef _CACHE_H_
#define _CACHE_H_

#include <utility>
#include <string>
#include <map>
#include <iostream>

namespace dxl {
namespace broker {

/** Namespace for caching of broker topology */
namespace cache
{
    /**
     * The type consists of the broker id of the current broker and
     * the broker id to where the message must be posted.
     */
    typedef std::pair<std::string, std::string> cache_key_t;

    /**
     * The type consists of a cache_key_t and the broker id to where to
     * route the message.
     */
    typedef std::map<cache_key_t, std::string> cache_t;
}

/** 
 * Cache containing message routing information 
 */
class Cache
{
public:
    /**
     * Adds to the cache
     *
     * @param   from The broker the message is from
     * @param   to The broker the message is being sent to (final destination)
     * @param   next The next broker to route the message to
     */
    void add( const std::string &from, const std::string &to, const std::string &next );

    /**
     * Returns the next broker to route the message to
     *  
     * @param   from The broker the message is from
     * @param   to The broker the message is being sent to (final destination)
     * @return  The next broker to route the message to, Otherwise "" is returned.
     */
    std::string get( const std::string &from, const std::string &to ) const;

    /** 
     * Clears the cache of all entries. 
     */
    inline void invalidate() { cache_.clear(); }

    /**
     * Invalidates a particular path in the cache
     *
     * @param   from The broker the message is from
     * @param   to The broker the message is being sent to (final destination)
     */
    void invalidate( const std::string &from, const std::string &to );

    /** Print */
    friend std::ostream & operator <<( std::ostream &out, const Cache &cache );

private:

    /**
     * Whether the specified value exists in the cache
     * @param   key The cache key value
     * @return  True if the value is in the cache, otherwise false
     */
    bool exists( const cache::cache_key_t &key ) const { return (cache_.count(key) != 0); }

    /** Cache storage */
    cache::cache_t cache_;
};

} /* namespace broker */ 
} /* namespace dxl */ 

#endif
