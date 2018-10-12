/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include <cstdlib>
#include <cstring>
#include "core/include/CoreUtil.h"

using namespace dxl::broker::core;

/** {@inheritDoc} */
char* CoreUtil::iterateWildcardBegin( const char* topic )
{
    if( !topic )
    {
        return NULL;
    }

    int length = (int)strlen( topic ) + 2;
    char* retval = (char*)malloc( length ); 
    if( retval )
    {
        strcpy( retval, topic );
    }
    return retval;
}

/** {@inheritDoc} */
bool CoreUtil::iterateWildcardNext( char* modifiedTopic )
{
    if( !modifiedTopic )
    {
        return false;    
    }

    int length = (int)strlen( modifiedTopic );
    
    if( length == 1 && modifiedTopic[0] == '#' )
    {
        return false;
    }

    int poundPos = 0;
    int searchPos = length - 1;    

    if( length >= 2 &&
        modifiedTopic[searchPos] == '#' &&
        modifiedTopic[searchPos-1] == '/' )
    {
        searchPos -= 2;
    }    

    while( searchPos >= 0 )
    {
        if( modifiedTopic[searchPos] == '/' )
        {
            poundPos = searchPos+1;
            break;
        }
        searchPos--;
    }

    modifiedTopic[poundPos] = '#';
    modifiedTopic[poundPos+1] = '\0';

    return true;
}

/** {@inheritDoc} */
void CoreUtil::iterateWildcardEnd( char *modifiedTopic )
{
    if( modifiedTopic )
    {
        free( modifiedTopic );        
    }
}

/** {@inheritDoc} */
bool CoreUtil::isWildcard( const char* topic )
{
    if( !topic )
    {
        return false;
    }

    int length = (int)strlen( topic );
    return ( length > 0 && topic[length-1] == '#' );
}

