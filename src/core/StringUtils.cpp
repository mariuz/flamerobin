/*
  Copyright (c) 2004-2008 The FlameRobin Development Team

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


  $Id$

*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/encconv.h>
#include <wx/fontmap.h>

#include "core/StringUtils.h"
//-----------------------------------------------------------------------------
//! converts wxString to std::string
std::string wx2std(const wxString& input, wxMBConv* conv)
{
    if (input.empty())
        return "";
    if (!conv)
        conv = wxConvCurrent;
    const wxWX2MBbuf buf(input.mb_str(*conv));
    // conversion may fail and return 0, which isn't a safe value to pass 
    // to std:string constructor
    if (!buf)
        return "";
    return std::string(buf);
}
//-----------------------------------------------------------------------------
//! converts std:string to wxString
wxString std2wx(const std::string& input, wxMBConv* conv)
{
    if (input.empty())
        return wxEmptyString;
    if (!conv)
        conv = wxConvCurrent;
    return wxString(input.c_str(), *conv);
}
//-----------------------------------------------------------------------------
wxString std2wxIdentifier(const std::string& input, wxMBConv* conv)
{
    if (input.empty())
        return wxEmptyString;
    if (!conv)
        conv = wxConvCurrent;
    // trim trailing whitespace
    size_t last = input.find_last_not_of(" ");
    return wxString(input.c_str(), *conv,
        (last == std::string::npos) ? std::string::npos : last + 1);
}
//-----------------------------------------------------------------------------
//! converts chars that have special meaning in HTML, so they get displayed
wxString escapeHtmlChars(const wxString& input, bool processNewlines)
{
    if (input.empty())
        return input;
    wxString result;
    wxString::const_iterator start = input.begin();
    while (start != input.end())
    {
        wxString::const_iterator stop = start;
        while (stop != input.end())
        {
            const wxChar c = *stop;
            if (c == '&' || c == '<' || c == '>' || c == '"'
                || (processNewlines && (c == '\r' || c == '\n')))
            {
                if (stop > start)
                    result += wxString(start, stop);
                if (c == '&')
                    result += wxT("&amp;");
                else if (c == '<')
                    result += wxT("&lt;");
                else if (c == '>')
                    result += wxT("&gt;");
                else if (c == '"')
                    result += wxT("&quot;");
                else if (c == '\n')
                    result += wxT("<BR>");
                else if (c == '\r')
                    /* swallow silently */;
                else
                    wxASSERT_MSG(false, wxT("escape not handled"));
                // start processing *after* the replaced character
                ++stop;
                start = stop;
                break;
            }
            ++stop;
        }
        if (stop > start)
            result += wxString(start, stop);
        start = stop;
    }
    return result;
}
//-----------------------------------------------------------------------------
//! returns string suitable for HTML META charset tag (used only if no
//  conversion to UTF-8 is available, i.e. in non-Unicode build
wxString getHtmlCharset()
{
#if !wxUSE_UNICODE
    struct CharsetMapping {
        wxFontEncoding encoding;
        const wxChar* htmlCS;
    };
    static const CharsetMapping mappings[] = {
        { wxFONTENCODING_ISO8859_1, wxT("ISO-8859-1") },
        { wxFONTENCODING_ISO8859_2, wxT("ISO-8859-2") },
        { wxFONTENCODING_ISO8859_3, wxT("ISO-8859-3") },
        { wxFONTENCODING_ISO8859_4, wxT("ISO-8859-4") },
        { wxFONTENCODING_ISO8859_5, wxT("ISO-8859-5") },
        { wxFONTENCODING_ISO8859_6, wxT("ISO-8859-6") },
        { wxFONTENCODING_ISO8859_7, wxT("ISO-8859-7") },
        { wxFONTENCODING_ISO8859_8, wxT("ISO-8859-8") },
        { wxFONTENCODING_ISO8859_9, wxT("ISO-8859-9") },
        { wxFONTENCODING_ISO8859_10, wxT("ISO-8859-10") },
        { wxFONTENCODING_ISO8859_11, wxT("ISO-8859-11") },
        { wxFONTENCODING_ISO8859_12, wxT("ISO-8859-12") },
        { wxFONTENCODING_ISO8859_13, wxT("ISO-8859-13") },
        { wxFONTENCODING_ISO8859_14, wxT("ISO-8859-14") },
        { wxFONTENCODING_ISO8859_15, wxT("ISO-8859-15") },

        { wxFONTENCODING_CP1250, wxT("windows-1250") },
        { wxFONTENCODING_CP1251, wxT("windows-1251") },
        { wxFONTENCODING_CP1252, wxT("windows-1252") },
        { wxFONTENCODING_CP1253, wxT("windows-1253") },
        { wxFONTENCODING_CP1254, wxT("windows-1254") },
        { wxFONTENCODING_CP1255, wxT("windows-1255") },
        { wxFONTENCODING_CP1256, wxT("windows-1256") },
        { wxFONTENCODING_CP1257, wxT("windows-1257") }
    };
    int mappingCount = sizeof(mappings) / sizeof(CharsetMapping);

    wxFontEncoding enc = wxLocale::GetSystemEncoding();
    for (int i = 0; i < mappingCount; i++)
    {
        if (mappings[i].encoding == enc)
            return mappings[i].htmlCS;
    }
#endif
    return wxT("UTF-8");
}
//-----------------------------------------------------------------------------
