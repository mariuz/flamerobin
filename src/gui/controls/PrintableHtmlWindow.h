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
#if wxUSE_WEBVIEW
#include <wx/webview.h>
#else
#include <wx/html/htmlwin.h>
#endif

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

class PrintableHtmlWindow: public wxPanel
{
private:
#if wxUSE_WEBVIEW
    wxWebView* webViewM;
#else
    wxHtmlWindow* webViewM;
#endif
    wxString pageSourceM;
    wxString tempFileM;
public:
    PrintableHtmlWindow(wxWindow* parent, wxWindowID id = wxID_ANY);
    virtual ~PrintableHtmlWindow();
    void setPageSource(const wxString& html);

    bool LoadFile(const wxString& filepath);
    void GetViewStart(int* x, int* y);
    void Scroll(int x, int y);
    wxString GetOpenedPageTitle();
    void SetRelatedFrame(wxFrame* frame, const wxString& format);
    void SetRelatedStatusBar(int bar);
protected:
#if wxUSE_WEBVIEW
    void OnWebViewNavigating(wxWebViewEvent& event);
#else
    void OnHtmlLinkClicked(wxHtmlLinkEvent& event);
#endif

    void OnRightUp(wxMouseEvent& event);
    void OnMenuCopy(wxCommandEvent& event);
    void OnMenuCopyAllHtml(wxCommandEvent& event);
#if wxCHECK_VERSION(3, 3, 0) && wxUSE_WEBVIEW
    void OnMenuShowDevTools(wxCommandEvent& event);
#endif
    void OnMenuSave(wxCommandEvent& event);
    void OnMenuPrint(wxCommandEvent& event);
    void OnMenuPreview(wxCommandEvent& event);
    DECLARE_EVENT_TABLE()
};

#endif
