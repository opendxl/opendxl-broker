/******************************************************************************
 * Copyright (c) 2018 McAfee, LLC - All Rights Reserved.
 *****************************************************************************/

#ifndef STRINGUTIL_H_
#define STRINGUTIL_H_

#include <stdexcept>
#include <vector>
#include <string>

namespace dxl {
namespace broker {

/**
 * String utility methods
 */
class StringUtil
{
public:
    /**
     * Returns a unicode string for the specified ansi string
     *
     * @param   ansi The ansi string
     * @param   unicode The unicode string
     * @return  Whether the conversion succeeded
     */
    static bool AnsiToUnicode( const std::string& ansi, std::wstring& unicode )
    {
        if( ansi.size() == 0 )
        {
            unicode = L"";
            return true;
        }
        bool worked = false;

        size_t sizeNeeded = mbstowcs( NULL, ansi.c_str(), 0 );

        // ensure there's space for the null terminator. Spec isn't clear on this
        sizeNeeded += 1;
        if( sizeNeeded > 0 )
        {
            std::vector<wchar_t> buffer;
            buffer.resize( sizeNeeded, 0 );

            sizeNeeded = mbstowcs( &buffer[0], ansi.c_str(), sizeNeeded );
            if( sizeNeeded > 0 )
            {
                unicode = &buffer[0];
                worked = true;
            }
        }
        return worked;
    }

    /**
     * Returns a unicode string for the specified ansi
     *
     * @param   ansi The ansi string
     * @return  The unicode string
     */
    static std::wstring AnsiToUnicode( const std::string& ansi )
    {
        std::wstring unicode;
        if( !AnsiToUnicode( ansi, unicode ) )
        {
            throw std::runtime_error( "Conversion from ansi to unicode failed" );
        }
        return unicode;
    }

    /**
     * Returns an ansi string for the specified unicode string
     *
     * @param   unicode The unicode string
     * @param   ansi The ansi string
     * @return  Whether the conversion succeeded
     */
    static bool UnicodeToAnsi( const std::wstring& unicode, std::string& ansi )
    {
        if( unicode.size() == 0 )
        {
            ansi = "";
            return true;
        }
        bool worked = false;

        size_t sizeNeeded = wcstombs( NULL, unicode.c_str(), 0 );
        // ensure there's space for the null terminator. Spec isn't clear on this
        sizeNeeded += 1;
        if( sizeNeeded > 0 )
        {
            std::vector<char> buffer;
            buffer.resize( sizeNeeded, 0 );

            sizeNeeded = wcstombs( &buffer[0], unicode.c_str(), sizeNeeded );
            if( sizeNeeded > 0 )
            {
                ansi = &buffer[0];
                worked = true;
            }
        }
        return worked;
    }

    /**
     * Returns a ansi string for the specified unicode string
     *
     * @param   unicode The unicode string
     * @return  The ansi string
     */
    static std::string UnicodeToAnsi( const std::wstring& unicode )
    {
        std::string ansi;
        if( !UnicodeToAnsi( unicode, ansi ) )
        {
            throw std::runtime_error("Conversion from unicode to ansi failed!");
        }
        return ansi;
    }
};

}  // namespace broker
}  // namespace dxl

#endif // STRINGUTIL_H_
