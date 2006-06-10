/*
Copyright (c) 2004, 2005, 2006 The FlameRobin Development Team

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

#include "core/StringUtils.h"
//-----------------------------------------------------------------------------
//! converts wxString to std::string
std::string wx2std(const wxString& input, wxMBConv* conv)
{
    if (input.empty())
        return "";
    if (!conv)
        conv = wxConvCurrent;
    return std::string(input.mb_str(*conv));
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
//! converts chars that have special meaning in HTML, so they get displayed
wxString escapeHtmlChars(const wxString& input, bool processNewlines)
{
    if (input.empty())
        return input;
    wxString result;
    size_t start = 0, len = input.length();
    while (start < len)
    {
        size_t stop = start;
        while (stop < len)
        {
            const wxChar c = input[stop];
            if (c == '&' || c == '<' || c == '>' || c == '"'
                || (processNewlines && (c == '\r' || c == '\n')))
            {
                if (stop > start)
                    result += input.Mid(start, stop - start);
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
            result += input.Mid(start, stop - start);
        start = stop;
    }
    return result;
}
//-----------------------------------------------------------------------------
