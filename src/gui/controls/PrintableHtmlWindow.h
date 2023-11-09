/*
  Copyright (c) 2004-2022 The FlameRobin Development Team

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
*/

#ifndef FR_PRINTABLE_HTML_WINDOW_H
#define FR_PRINTABLE_HTML_WINDOW_H

#include <wx/wx.h>
#include <wx/wxhtml.h>
#include <wx/webview.h>

class wxHtmlEasyPrinting;

class HtmlPrinter
{
private:
    wxHtmlEasyPrinting *prnM;
    HtmlPrinter();
public:
    static wxHtmlEasyPrinting *getHEP();
    ~HtmlPrinter();
};

class PrintableHtmlWindow: public wxHtmlWindow
{
private:
    wxString pageSourceM;
    wxString tempLinkM;     // set before context menu pops up, and used in handler for menu item
public:
    PrintableHtmlWindow(wxWindow* parent, wxWindowID id = wxID_ANY);
    void setPageSource(const wxString& html);
protected:
    virtual void OnLinkClicked(const wxHtmlLinkInfo& link);

    void OnRightUp(wxMouseEvent& event);
    void OnMenuCopy(wxCommandEvent& event);
    void OnMenuCopyAllHtml(wxCommandEvent& event);
    void OnMenuNewWindow(wxCommandEvent& event);
    void OnMenuNewTab(wxCommandEvent& event);
    void OnMenuSave(wxCommandEvent& event);
    void OnMenuPrint(wxCommandEvent& event);
    void OnMenuPreview(wxCommandEvent& event);
    DECLARE_EVENT_TABLE()
};

#endif
