/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "brokerregistry/include/cache.h"
#include <iostream>

namespace dxl {
namespace broker {

/** {@inheritDoc} */
void Cache::add( const std::string &from, const std::string &to, const std::string &next )
{
    if( !from.empty() && !to.empty() && !next.empty() )
    {
        cache::cache_key_t key = std::make_pair( from, to );
        cache_[key] = next;
    }
}

/** {@inheritDoc} */
std::string Cache::get( const std::string &from, const std::string &to ) const
{
    std::string retVal;

    if( !from.empty() && !to.empty() )
    {
        cache::cache_key_t key = std::make_pair( from, to );
        if( exists( key ) ) 
        {
            retVal = cache_.at( key );
        }
    }

    return retVal;
}

/** {@inheritDoc} */
void Cache::invalidate( const std::string &from, const std::string &to )
{
    if( !from.empty() && !to.empty() )
    {
        cache::cache_key_t key = std::make_pair( from, to );
        if( exists( key ) )
        {
            cache_.erase( key );
        }

        /* Invalidate intermediate connections
         *
         *                A
         *               / \
         *              B   C
         *
         * B, C = A
         */
        auto i = cache_.begin();
        while( i != cache_.end() )
        {
            if( ( ( ( * i ).first.first == to ) || ( ( *i ).first.second == to ) ) && 
                ( ( *i ).second == from ) )
            {
                cache_.erase( ( *i ).first );
                i = cache_.begin();
            }
            else
            {
                ++i;
            }
        }
    }
}

/** {@inheritDoc} */
std::ostream & operator <<( std::ostream &out, const Cache &cache )
{
    for( auto i = cache.cache_.begin(); i != cache.cache_.end(); ++i )
    {
        out << "\t" << "Entry:  " << ( *i ).first.first << ", " << ( *i ).first.second << 
            " = " << ( *i ).second << std::endl;
    }

    return out;
}

}
}
