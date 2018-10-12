/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/brokerlib.h"
#include "include/BrokerSettings.h"
#include "include/SimpleLog.h"
#include "cert/include/RevocationService.h"

#include "dxlcommon.h"

using namespace std;
using namespace dxl::broker;
using namespace dxl::broker::cert;
using namespace dxl::broker::core;


/** {@inheritDoc} */
RevocationService& RevocationService::getInstance()
{
    static RevocationService instance;
    return instance;
}

/* {@inheritDoc} */
RevocationService::RevocationService() : m_newCerts( NULL )
{
    // Add maintenance listener
    getCoreInterface()->addMaintenanceListener( this );
}

/* {@inheritDoc} */
bool RevocationService::isRevoked( const char* cert ) const
{
    return m_certs.find( cert ) != m_certs.end();
}

/* {@inheritDoc} */
void RevocationService::addCertificate( const char* cert )
{
    if( m_certs.insert( cert ).second )
    {
        struct cert_hashes* s = NULL;
        s = (struct cert_hashes*)malloc(sizeof(struct cert_hashes));
        if( s )
        {
            s->cert_sha1 = strdup( cert );
            if( s->cert_sha1 )
            {
                HASH_ADD_KEYPTR(hh, m_newCerts, s->cert_sha1, (unsigned int)strlen(s->cert_sha1), s);
            }
        }
    }
}

/** {@inheritDoc} */
bool RevocationService::readFromFile( const string& filename ) 
{
    m_certs.clear();

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
                    SL_START << "Read revoked cert:" << curLine << SL_DEBUG_END;
                }
                m_certs.insert( curLine );
            }
        }
    }
    else
    {
        if( SL_LOG.isDebugEnabled() )
        {
            SL_START << "Unable to open revoked certs file: " << filename << SL_DEBUG_END;
        }
        return false;
    }
    infile.close();        

    if( infile.bad() )
    {
        SL_START << "Error reading revoked certs file: " << filename << SL_ERROR_END;
        return false;
    }
    
    return true;
}

/** {@inheritDoc} */
bool RevocationService::writeToFile( const string& filename ) const 
{
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "RevocationService::writeTofile" << SL_DEBUG_END;
    }

    std::ofstream outfile;
    outfile.open( filename.c_str(), std::ofstream::trunc );
    if( outfile.is_open() ) 
    {
        for( auto it = m_certs.begin(); it != m_certs.end(); ++it ) 
        {
            outfile << *it << "\n";
        }
    }
    else
    {
        SL_START << "Unable to open file for writing: " << filename << SL_ERROR_END;
        return false;
    }
    outfile.close();

    if( outfile.bad() )
    {
        SL_START << "Error writing to file: " << filename << SL_ERROR_END;
        return false;
    }    

    return true;
}

/** {@inheritDoc} */
void RevocationService::onCoreMaintenance( time_t /*time*/ )
{
    if( SL_LOG.isDebugEnabled() )
    {
        SL_START << "RevocationService::onCoreMaintenance" << SL_DEBUG_END;
    }

    if( m_newCerts )
    {
        if( SL_LOG.isDebugEnabled() )
        {
            SL_START << "New revoked certificates" << SL_DEBUG_END;
        }        

        // Store to disk
        writeToFile( BrokerSettings::getRevokedCertsFile() );

        // Revoke certs
        getCoreInterface()->revokeCerts( m_newCerts );

        m_newCerts = NULL;
    }    
}
