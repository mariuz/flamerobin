/*
  Copyright (c) 2004-2007 The FlameRobin Development Team

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

#ifndef FR_STRINGUTILS_H
#define FR_STRINGUTILS_H
//-----------------------------------------------------------------------------
#include <wx/string.h>

#include <string>

class wxMBConv;
//-----------------------------------------------------------------------------
std::string wx2std(const wxString& input, wxMBConv* conv = wxConvCurrent);
wxString std2wx(const std::string& input, wxMBConv* conv = wxConvCurrent);
//-----------------------------------------------------------------------------
//! converts chars that have special meaning in HTML, so they get displayed
wxString escapeHtmlChars(const wxString& input, bool processNewlines = true);
//-----------------------------------------------------------------------------
//! returns string suitable for HTML META charset tag (used only if no
//  conversion to UTF-8 is available, i.e. in non-Unicode build
wxString getHtmlCharset();
//-----------------------------------------------------------------------------
// a helper class to manage the wxMBConv object necessary to translate from
// and to the database connection charset
class DatabaseToSystemCharsetConversion
{
private:
    wxString connectionCharsetM;
    wxMBConv* converterM;
public:
    DatabaseToSystemCharsetConversion();
    ~DatabaseToSystemCharsetConversion();

    wxMBConv* getConverter();
    static wxString mapCharset(const wxString& connectionCharset);
    void setConnectionCharset(const wxString& connectionCharset);
};
//-----------------------------------------------------------------------------
#endif // FR_STRINGUTILS_H
