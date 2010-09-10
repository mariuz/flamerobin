/*
  Copyright (c) 2004-2009 The FlameRobin Development Team

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
#include <vector>

class wxMBConv;
//-----------------------------------------------------------------------------
std::string wx2std(const wxString& input, wxMBConv* conv = wxConvCurrent);
wxString std2wx(const std::string& input, wxMBConv* conv = wxConvCurrent);

wxString std2wxIdentifier(const std::string& input, wxMBConv* conv);
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//! returns string suitable for HTML META charset tag (used only if no
//  conversion to UTF-8 is available, i.e. in non-Unicode build
wxString getHtmlCharset();
//-----------------------------------------------------------------------------
class MetadataItem;
class TemplateEngine
{
private:
    MetadataItem *objectM;  // main observed object
    std::vector<MetadataItem *> allowedObjectsM;
    bool flagNextM;
    bool plainTextM;
public:
    TemplateEngine(MetadataItem *m,
        std::vector<MetadataItem *> *allowedObjects = 0);

    void processCommand(wxString cmd, MetadataItem* object,
        wxString& htmlpage, wxWindow *window, bool first);
    void processHtmlCode(wxString& htmlpage, wxString htmlsource,
        MetadataItem* object, wxWindow *window, bool first = true);

    //! converts chars that have special meaning in HTML, so they get displayed
    wxString escapeHtmlChars(const wxString& input, bool processNewlines = true);
    void setPlainText(bool plain);
};
//-----------------------------------------------------------------------------
#endif // FR_STRINGUTILS_H
