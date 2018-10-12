/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef SIMPLELOG_H_
#define SIMPLELOG_H_

#include <stdarg.h>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <thread>
#include <mutex>
#include "StringUtil.h"
#include "MutexLock.h"


/** Namespace for the simple logger used by the broker */
namespace SimpleLogger {

// Macros to use for logging
#define SL_LOG          SimpleLogger::SimpleLog::Instance()
#define SL_START        SimpleLogger::SimpleLog::SimpleLogContext()
#define SL_FILE_LOC     " (" << __FILE__ << ":" << __LINE__ << ")"
#define SL_DEBUG_END    SL_FILE_LOC << SimpleLogger::debug
#define SL_INFO_END     SL_FILE_LOC << SimpleLogger::info
#define SL_WARN_END     SL_FILE_LOC << SimpleLogger::warn
#define SL_ERROR_END    SL_FILE_LOC << SimpleLogger::error
#define SL_ERROR_THROW  SL_FILE_LOC << SimpleLogger::errorthrow

// Function logger
#define FUNCTION_LOG SimpleLogger::ScopeLogger fnLogger( \
        std::string("Enter ") + __FUNCTION__, \
        std::string("Exit ") + __FUNCTION__ );
    
/** The logging levels */
enum LOG_LEVEL
{
    debug, info, warn, error, errorthrow
};

/**
 * Interface that is implemented by concrete loggers
 */
class SimpleLogListener
{
public:
    /** Destructor */
    virtual ~SimpleLogListener() {}

    /**
     * Writes a log message at the specified level
     *
     * @param   level The log level
     * @param   logText The log text
     */
    virtual void write( LOG_LEVEL level, const char* logText ) = 0;
};

/**
 * Writes log files to disk. Supports rolling log files.
 */
class DiskLogger: public SimpleLogListener
{
public:
    /**
     * Constructs the disk logger
     *
     * @param   logFile The name of the file to write to
     * @param   rollSize Maximum size of the log file
     * @param   maxFiles The maximum number of log files
     */
    DiskLogger( const char* logFile, unsigned int rollSize,
        unsigned int maxFiles ) :
        m_logFileName( logFile ),
        m_rollSize( rollSize ),
        m_maxDepth( maxFiles ) {}

    /**
     * Closes the disk logger
     */
    void close()
    {
        if( !m_outfile.is_open() )
        {
            m_outfile.close();
        }
    }

    /** {@inheritDoc} */
    virtual void write( LOG_LEVEL, const char* logText )
    {
        if( !m_outfile.is_open() )
        {
            m_outfile.open( m_logFileName.c_str(),
            std::ios_base::out | std::ios_base::ate | std::ios_base::app );
        }

        if( m_outfile.is_open() )
        {
            std::streampos pos = m_outfile.tellp();
            if( pos >= m_rollSize )
            {
                m_outfile.close();
                remove( genFileName( m_logFileName, m_maxDepth ).c_str() );
                for( unsigned int i = m_maxDepth; i > 0; i-- )
                {
                    rename( genFileName( m_logFileName, i - 1 ).c_str(),
                    genFileName( m_logFileName, i).c_str() );
                }
            }
        }

        if( !m_outfile.is_open() )
        {
            m_outfile.open( m_logFileName.c_str(),
                std::ios_base::out | std::ios_base::ate | std::ios_base::app );
        }
        if( m_outfile.is_open() )
        {
            m_outfile << logText << std::endl;
        }
    }

protected:
    /**
     * Generates a file name
     *
     * @param   root The root file name
     * @param   suffix The file suffix
     */
    std::string genFileName( const std::string& root, unsigned int suffix )
    {
        std::stringstream filename;
        filename << root;
        if( suffix )
            filename << "." << suffix;
        return filename.str();
    }

    /** The log file name */
    std::string m_logFileName;
    /** The maximum size of a log file */
    unsigned int m_rollSize;
    /** The maximum number of log files */
    unsigned int m_maxDepth;
    /** The output stream */
    std::ofstream m_outfile;
};

/**
 * The class to log messages to (log messages are sent to registered listeners)
 */
class SimpleLog
{
public:
    // Forward reference to context
    class SimpleLogContext;

    /**
     * The single instance
     *
     * @return  The single instance
     */
    static SimpleLog& Instance()
    {
        static SimpleLog instance;
        return instance;
    }

    /**
     * Constructs the logger
     */
    SimpleLog() : m_minLevelOfInterest( debug )
    {
    }

    /**
     * Adds the specified listener to the list of log listeners
     *
     * @param   listener The listener to add
     */
    void addListener( SimpleLogListener* listener )
    {
        if( std::find( m_listeners.begin(), m_listeners.end(), listener )
            == m_listeners.end() )
        m_listeners.push_back( listener );
    }

    /**
     * Adds the specified listener from the list of log listeners
     *
     * @param   listener The listener to remove
     */
    void removeListener( SimpleLogListener* listener )
    {
        std::vector<SimpleLogListener*>::iterator itr = std::find(
            m_listeners.begin(), m_listeners.end(), listener );

        if( itr != m_listeners.end() )
            m_listeners.erase( itr );
    }

    /**
     * Sets the minimum log level of interest
     *
     * @param   minLevelOfInterest The minimum log level of interest
     */
    void setLogLevelFilter( LOG_LEVEL minLevelOfInterest )
    {
        m_minLevelOfInterest = minLevelOfInterest;
    }

    /**
     * Sets the minimum log level of interest as a string
     *
     * @param   minLevelOfInterest The minimum level of interest as a string
     */
    void setLogLevelFilter( std::string minLevelOfInterest )
    {
        if( minLevelOfInterest == "debug" )
        {
            setLogLevelFilter( debug );
        }
        else if( minLevelOfInterest == "info" )
        {
            setLogLevelFilter( info );
        }
        else if( minLevelOfInterest == "warn" )
        {
            setLogLevelFilter( warn );
        }
        else if( minLevelOfInterest == "error" )
        {
            setLogLevelFilter( error );
        }
        else
        {
            SimpleLogger::SimpleLog::SimpleLogContext()
                << "Unknown log level: " << minLevelOfInterest << SL_ERROR_END;
        }
    }

    /**
     * Outputs the current log message to registered listeners
     *
     * @param   context The logging context
     * @param   end The log level
     */
    void outputMessage( const SimpleLogContext& context, LOG_LEVEL end )
    {
        if( m_minLevelOfInterest > end )
        {
            return;
        }

        const std::stringstream& stream = context.getStream();
        std::vector<char> t;
        t.resize( 255 );
        time_t rawtime;
        tm timeinfo;
        rawtime = time( NULL );
        localtime_r( &rawtime, &timeinfo );
        snprintf( &t[0], t.size(), "%02d/%02d/%04d %d:%02d:%02d ",
            timeinfo.tm_mon+1, timeinfo.tm_mday, ( timeinfo.tm_year + 1900 ),
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec );
        std::string theText;
        
        // Output thread information
        // TODO: Make this configurable (enable/disable output)
        std::ostringstream threadId;
        threadId << std::this_thread::get_id();
        theText += "[" + threadId.str() + "] ";

        theText += &t[0];

        switch( end )
        {
            case debug:
                theText += "[D] ";
                break;
            case info:
                theText += "[I] ";
                break;
            case warn:
                theText += "[W] ";
                break;
            case error:
            case errorthrow:
                theText += "[E] ";
                break;
        }

        theText += " ";
        theText += stream.str();
        writeTxt( end, theText );

        if( end == errorthrow )
        {
            std::string error( stream.str() );
            throw std::runtime_error( error );
        }
    }

    /**
     * Whether debug messages are enabled
     *
     * @return  Whether debug messages are enabled
     */
    bool isDebugEnabled() { return m_minLevelOfInterest <= debug; }

    /**
     * Whether info messages are enabled
     *
     * @return  Whether info messages are enabled
     */
    bool isInfoEnabled() { return m_minLevelOfInterest <= info; }

    /**
     * Whether warn messages are enabled
     *
     * @return  Whether warn messages are enabled
     */
    bool isWarnEnabled() { return m_minLevelOfInterest <= warn; }

    /**
     * Whether error messages are enabled
     *
     * @return  Whether error messages are enabled
     */
    bool isErrorEnabled() { return m_minLevelOfInterest <= error; }

protected:
    /** {@inheritDoc} */
    void writeTxt( LOG_LEVEL level, std::string& toWrite )
     {
        // Lock for writes
        dxl::broker::common::MutexLock lock( &m_writeMutex );

        for ( size_t i = 0; i < m_listeners.size(); i++ )
            m_listeners[i]->write(level, toWrite.c_str());
    }

    /** The listeners to log to */
    std::vector<SimpleLogListener*> m_listeners;
    /** The minimum log level */
    LOG_LEVEL m_minLevelOfInterest;
    /** The write mutex */
    std::mutex m_writeMutex;

public:
    /**
     * Context used when writing log messages (thread-specific stream)
     */
    class SimpleLogContext
    {
    public:
        /**
         * Returns the stream to write to
         *
         * @return  The stream to
         */
        const std::stringstream& getStream() const
        {
            return m_stream;
        }

        /**
         * Add int to log message
         *
         * @return  The logging context
         */
        SimpleLogContext& operator<<( int i )
        {
            m_stream << i;
            return *this;
        }

        /**
         * Add unsigned int to log message
         *
         * @return  The logging context
         */
        SimpleLogContext& operator<<( unsigned int uint )
        {
            m_stream << uint;
            return *this;
        }

        /**
         * Add unsigned long to log message
         *
         * @return  The logging context
         */
        SimpleLogContext& operator<<( unsigned long ul )
        {
            m_stream << ul;
            return *this;
        }

        /**
         * Add long to log message
         *
         * @return  The logging context
         */
        SimpleLogContext& operator<<( long l )
        {
            m_stream << l;
            return *this;
        }

        /**
         * Add unsigned long long to log message
         *
         * @return  The logging context
         */
        SimpleLogContext& operator<<( unsigned long long l )
        {
            m_stream << l;
            return *this;
        }

        /**
         * Add long long to log message
         *
         * @return  The logging context
         */
        SimpleLogContext& operator<<( long long l )
        {
            m_stream << l;
            return *this;
        }

        /**
         * Add double to log message
         *
         * @return  The logging context
         */
        SimpleLogContext& operator<<( double i )
        {
            m_stream << i;
            return *this;
        }

        /**
         * Add float to log message
         *
         * @return  The logging context
         */
        SimpleLogContext& operator<<( float i )
        {
            m_stream << i;
            return *this;
        }

        /**
         * Add string to log message
         *
         * @return  The logging context
         */
        SimpleLogContext& operator<<( const std::string& s )
        {
            m_stream << s;
            return *this;
        }

        /**
         * Add string to log message
         *
         * @return  The logging context
         */
        SimpleLogContext& operator<<( const char* sz )
        {
            m_stream << sz;
            return *this;
        }

        /**
         * Add wide string to log message
         *
         * @return  The logging context
         */
        SimpleLogContext& operator<<( const std::wstring& s )
        {
            m_stream << dxl::broker::StringUtil::UnicodeToAnsi(s);
            return *this;
        }

        /**
         * Add wide string to log message
         *
         * @return  The logging context
         */
        SimpleLogContext& operator<<( const wchar_t* sz )
        {
            m_stream << dxl::broker::StringUtil::UnicodeToAnsi(sz);
            return *this;
        }

        /**
         * Ends the log message
         *
         * @param   end The log level for the message
         * @return  The logging context
         */
        SimpleLogContext& operator<<( LOG_LEVEL end )
        {
            SimpleLogger::SimpleLog::Instance().outputMessage( *this, end );
            return *this;
        }

    protected:
        /** The stream associated with the log message */
        std::stringstream m_stream;
    };
};

/**
 * Logger that can be used to log a particular scope (for example a function invocation).
 * See the FUNCTION_LOG macro
 */
class ScopeLogger
{
    public:
        /**
         * Constructs the logger
         *
         * @param   constructText Text to display when scope is started
         * @param   destructText Text to display when scope is exited
         * @param   priority The log level
         */
        ScopeLogger( const std::string& constructText, const std::string& destructText,
            LOG_LEVEL priority = debug ) :
            m_destructText(destructText), m_priority(priority)
        {
            SimpleLogger::SimpleLog::SimpleLogContext() << constructText << m_priority;
        }

        /** Destructor */
        ~ScopeLogger()
        {
            SimpleLogger::SimpleLog::SimpleLogContext() << m_destructText << m_priority;
        }

    protected:
        /** The text to display when scope is exited */
        std::string m_destructText;
        /** The log level */
        LOG_LEVEL m_priority;
};

/**
 * Logger that logs messages to standard out
 */
class StdOutLogger: public SimpleLogger::SimpleLogListener
{
public:
    /** Constructs the logger */
    StdOutLogger() {}

    /** Destructor */
    virtual ~StdOutLogger() {}

    /** {@inheritDoc} */
    virtual void write( SimpleLogger::LOG_LEVEL /* level */,
        const char* logText )
    {
        std::cout << logText << std::endl;
    }
};

}  // namespace SimpleLogger

#endif  // SIMPLELOG_H_
