/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef FILEUTIL_H_
#define FILEUTIL_H_

#include <string>

namespace dxl {
namespace broker {
/** Namespace for utility-related declarations */
namespace util {

/**
 * File utility methods
 */
class FileUtil
{    
public:
    /**
     * Checks whether the specified file exists
     *
     * @param   fileName The file name
     * @return  Whether the specified file exists
     */
    static bool fileExists( const std::string &fileName );

    /**
     * Deletes the specified file
     *
     * @param   fileName The file name
     * @return  Whether the specified file was successfully deleted
     */
    static bool deleteFile( const std::string &fileName );

    /**
     * Renames the specified file
     *
     * @param   oldFileName The old file name
     * @param   newFileName The new file name
     * @return  Whether the file was renamed successfully
     */
    static bool renameFile( 
        const std::string &oldFileName, const std::string &newFileName );

    /**
     * Safely updates an existing file
     *
     * @param   fileContent The content to update
     * @param   file The target file to write the content to
     * @param   tmpFile The temp file to write the file to initially
     * @param   backupFile The backup file
     */
    static bool updateFile( 
        const std::string& fileContent,
        const std::string& file,
        const std::string& tmpFile,
        const std::string& backupFile );

    /**
     * Returns the contents of the specified file as a string
     *
     * @param   fileName The file name
     * @return  The contents of the specified file as a string
     */
    static std::string getFileAsString( const std::string &fileName );

    /**
     * Writes the specified string to the specified file name
     *
     * @param   data The string to write
     * @param   fileName The file name to write to
     * @return  Whether the write was successful
     */
    static bool writeStringToFile( 
        const std::string &data, const std::string &fileName );
};

} /* namespace util */
} /* namespace broker */
} /* namespace dxl */

#endif /* FILEUTIL_H_ */
