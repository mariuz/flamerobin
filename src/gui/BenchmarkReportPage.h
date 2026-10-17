/*
  Copyright (c) 2004-2026 The FlameRobin Development Team

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

#ifndef FR_BENCHMARKREPORTPAGE_H
#define FR_BENCHMARKREPORTPAGE_H

#include <wx/wx.h>

#include <memory>
#include <vector>

/// the start of the names of the temporary pages of the benchmark report,
/// those of the report view and those of "Open in browser"
wxString getBenchmarkPagePrefix();

#if wxUSE_WEBVIEW
class wxWebView;
class wxWebViewEvent;

/// the web view of the benchmark report: on Windows Edge (WebView2) where
/// its runtime is installed, then Internet Explorer, which shows the report
/// in a simpler form; nullptr if there is no web view, the report is shown
/// as text then
wxWebView* createBenchmarkReportView(wxWindow* parent, wxString& backend);

///
/// Internet Explorer chooses the mode of a page, and with it how much of the
/// style sheet it knows, when it loads the page. SetPage() writes into the
/// blank page that was loaded before, which stays in the mode of Internet
/// Explorer 7 or 8, whatever the page says. So that the report gets the mode
/// of Internet Explorer 11, it is loaded from memory, if that fails from a
/// temporary file, and only then written with SetPage(); no setting of the
/// system is changed. The other web views simply get the page.
///
class BenchmarkReportPage
{
public:
    BenchmarkReportPage(wxWebView* view, bool internetExplorer);
    // removes the pages from memory and disk
    ~BenchmarkReportPage();
    void show(const wxString& html);
    /// whether a page that has loaded is the one shown now, not one before
    bool isShownPage(const wxString& url) const;
    /// runs a script in the shown page and gives the value of its
    /// expression as text; also with Internet Explorer, where
    /// wxWebView::RunScript() does not work for these pages
    bool runScript(const wxString& expression, wxString* result);

private:
    wxWebView* viewM;
    bool loadM;
    // the page that is shown and the file name in its URL
    wxString htmlM;
    wxString nameM;
    bool fromFileM = false;
    // a sign of life for the calls made after an event
    std::shared_ptr<bool> aliveM = std::make_shared<bool>(true);
    // the last pages: Internet Explorer may still be reading one when the
    // next one starts, and wxMemoryFSHandler lends it the memory of the
    // page; the older ones are removed when the next page has loaded
    std::vector<wxString> memoryFilesM;
    std::vector<wxString> tempFilesM;
    void removeOldPages();

    void showFromFile();
    bool load(const wxString& url);
#ifdef __WXMSW__
    bool runScriptInExplorer(const wxString& expression, wxString* result);
#endif
    void OnNavigating(wxWebViewEvent& event);
    void OnNewWindow(wxWebViewEvent& event);
    void OnError(wxWebViewEvent& event);
    void OnLoaded(wxWebViewEvent& event);
};
#endif

#endif // FR_BENCHMARKREPORTPAGE_H
