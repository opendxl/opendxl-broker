/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include <cstring>

#include "cert/include/ManagedCerts.h"
#include "include/SimpleLog.h"

#include "dxlcommon.h"

namespace dxl {
namespace broker {
namespace cert{

using namespace std;

/** {@inheritDoc} */
ManagedCerts::ManagedCerts( const unordered_set<string>& managedCerts )
{
    m_managedCerts = managedCerts;
}

/** {@inheritDoc} */
ManagedCerts::~ManagedCerts() 
{
}

/** {@inheritDoc} */
bool ManagedCerts::CreateFromFile( const string& filename, shared_ptr<const ManagedCerts>& certs ) 
{
    unordered_set<string> managedCerts;
    certs.reset();

    std::ifstream infile;
    infile.open( filename.c_str() );
    if( infile.is_open() ) 
    {
        while( !infile.eof() ) 
        {
            std::string curLine( "eof" );
            getline( infile, curLine );

            // This is a hack for aix, where the eof stuff is flakey
            if( curLine == "eof" )
                break;

            if( curLine.find_first_not_of( ' ' ) != string::npos )
            {
                if( SL_LOG.isDebugEnabled() )
                {
                    SL_START << "Read cert:" << curLine << SL_DEBUG_END;
                }
                managedCerts.insert( curLine );
            }
        }
    }
    else
    {
        SL_START << "Unable to open certs list file: " << filename << SL_ERROR_END;
        return false;
    }
    infile.close();        

    if( infile.bad() )
    {
        SL_START << "Error reading certs list file: " << filename << SL_ERROR_END;
        return false;
    }
    
    certs.reset( new ManagedCerts( managedCerts ) );
    return true;
}

/** {@inheritDoc} */
bool ManagedCerts::writeToUthash( struct cert_hashes** uthashptr ) const
{
    return writeToUthash( m_managedCerts, uthashptr );
}

/** {@inheritDoc} */
bool ManagedCerts::writeToUthash(
    const unordered_set<std::string>& certs, struct cert_hashes** uthashptr )
{
    *uthashptr = NULL;
    for( auto it = certs.begin(); it != certs.end(); ++it )
    {
        struct cert_hashes* s = NULL;
        s = (struct cert_hashes*)malloc(sizeof(struct cert_hashes));
        if( !s )
        {
            return false;
        }
        s->cert_sha1 = strdup( it->c_str() );
        if( !s->cert_sha1 )
        {
            return false;
        }
        HASH_ADD_KEYPTR( hh, *uthashptr, s->cert_sha1, (unsigned int)strlen(s->cert_sha1), s );
    }

    return true;
}

/** {@inheritDoc} */
bool ManagedCerts::operator==( const ManagedCerts& rhs ) const
{
    return m_managedCerts == rhs.m_managedCerts;
}

/** {@inheritDoc} */
bool ManagedCerts::operator!=( const ManagedCerts& rhs ) const
{
    return !( *this == rhs );
}

}
}
}
