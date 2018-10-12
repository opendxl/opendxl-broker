/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#include "include/SimpleLog.h"
#include "util/include/FileUtil.h"
#include <cstring>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include "dxlcommon.h"

using namespace std;
using namespace dxl::broker::util;

/** {@inheritDoc} */
bool FileUtil::fileExists( const string &fileName )
{
    bool retVal( false );
    struct stat buf;

    if( stat( fileName.c_str(), &buf ) == 0 )
    {
        retVal = true;
    }

    return retVal;
}

/** {@inheritDoc} */
bool FileUtil::deleteFile( const string &fileName )
{
    return !unlink( fileName.c_str() );
}

/** {@inheritDoc} */
bool FileUtil::renameFile( const string &oldFileName, const string &newFileName )
{
    return !rename( oldFileName.c_str(), newFileName.c_str() );
}

/** {@inheritDoc} */
bool FileUtil::updateFile( 
    const string& fileContent,
    const string& file,
    const string& tmpFile,
    const string& backupFile )
{
    // Create temporary file
    if( !FileUtil::writeStringToFile( fileContent, tmpFile ) )
    {
        SL_START << "Error writing temp file: " << tmpFile << SL_ERROR_END;
        return false;
    }

    // Delete backup file (if it exists)
    if( FileUtil::fileExists( backupFile ) )
    {
        if( !FileUtil::deleteFile( backupFile ) )
        {
            SL_START << "Error deleting backup file: " << backupFile << SL_ERROR_END;
            return false;
        }
    }

    // Rename file to backup file (if it exists)
    if( FileUtil::fileExists( file ) )
    {
        if( !FileUtil::renameFile( file, backupFile ) )
        {
            SL_START << "Error renaming file to backup file: " << file 
                << ", " << backupFile << SL_ERROR_END;
            return false;
        }
    }

    // Rename temp to file
    if( !FileUtil::renameFile( tmpFile, file ) )
    {
        SL_START << "Error renaming temp to file: " << tmpFile
            << ", " << file << SL_ERROR_END;
        
        // Attempt to restore backup file (if it exists)
        if( FileUtil::fileExists( backupFile ) )
        {
            if( !FileUtil::renameFile( backupFile, file ) )
            {
                SL_START << "Error renaming backup to file: " << backupFile
                    << ", " << file << SL_ERROR_END;
            }
        }

        return false;        
    }

    return true;
}

/** {@inheritDoc} */
string FileUtil::getFileAsString( const string &fileName )
{
    string retVal;
    ifstream file( fileName.c_str(), ios::in | ios::binary );

    if( file )
    {
        // get length of file:
        file.seekg( 0, file.end );
        int length = (int)file.tellg();
        file.seekg( 0, file.beg );

        // create a temporary buffer
        char * buffer = new char[ length + 1 ];        
        memset( buffer, 0, length + 1 );

        // read data as a block:
        file.read( buffer, length );

        if( file )
        {
            retVal = buffer;
        }
        else
        {
            SL_START << "dxlbroker only read " << file.gcount() << " bytes of " << fileName << "." << SL_ERROR_END;
        }

        file.close();

        delete[] buffer;
    }
    else
    {
        SL_START << "dxlbroker failed to open " << fileName << ", " << strerror(errno) << SL_ERROR_END;
    }

    return retVal;
}

/** {@inheritDoc} */
bool FileUtil::writeStringToFile( const string &data, const string &fileName  )
{
    ofstream file( fileName.c_str(), ios::out | ios::binary | ios::trunc );
    if( file )
    {
        file << data;
    }
    else
    {
        SL_START << "dxlbroker failed to open " << fileName << ", " << strerror(errno) 
            << SL_ERROR_END;

        return false;
    }

    file.close();

    return true;
}
