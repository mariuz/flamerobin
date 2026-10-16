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

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/ffile.h>
#include <wx/filename.h>
#if wxUSE_WEBVIEW
#include <wx/fs_mem.h>
#include <wx/webview.h>
#include <wx/webviewfshandler.h>
#ifdef __WXMSW__
// WIN32_LEAN_AND_MEAN leaves out the COM declarations that exdisp.h needs
#include <ole2.h>
#include <exdisp.h>
#endif
#endif

#include "gui/BenchmarkReportPage.h"

wxString getBenchmarkPagePrefix()
{
    return "flamerobin-benchmark-";
}

#if wxUSE_WEBVIEW
wxWebView* createBenchmarkReportView(wxWindow* parent, wxString& backend)
{
    const char* const backends[] = {
#ifdef __WXMSW__
        wxWebViewBackendEdge, wxWebViewBackendIE
#else
        wxWebViewBackendDefault
#endif
    };
    for (const char* b : backends)
    {
        if (!wxWebView::IsBackendAvailable(b))
            continue;
        if (wxWebView* view = wxWebView::New(parent, wxID_ANY, wxWebViewDefaultURLStr,
            wxDefaultPosition, wxDefaultSize, b))
        {
            backend = b;
            return view;
        }
    }
    return nullptr;
}

BenchmarkReportPage::BenchmarkReportPage(wxWebView* view, bool internetExplorer)
    : viewM(view), loadM(internetExplorer)
{
    if (!loadM)
        return;
    // one handler for the whole program, like the other file systems
    if (!wxFileSystem::HasHandlerForPath("memory:report.html"))
        wxFileSystem::AddHandler(new wxMemoryFSHandler);
    viewM->RegisterHandler(wxSharedPtr<wxWebViewHandler>(new wxWebViewFSHandler("memory")));
    viewM->Bind(wxEVT_WEBVIEW_NAVIGATING, &BenchmarkReportPage::OnNavigating, this);
    viewM->Bind(wxEVT_WEBVIEW_ERROR, &BenchmarkReportPage::OnError, this);
    viewM->Bind(wxEVT_WEBVIEW_LOADED, &BenchmarkReportPage::OnLoaded, this);
}

BenchmarkReportPage::~BenchmarkReportPage()
{
    if (!loadM)
        return;
    viewM->Unbind(wxEVT_WEBVIEW_NAVIGATING, &BenchmarkReportPage::OnNavigating, this);
    viewM->Unbind(wxEVT_WEBVIEW_ERROR, &BenchmarkReportPage::OnError, this);
    viewM->Unbind(wxEVT_WEBVIEW_LOADED, &BenchmarkReportPage::OnLoaded, this);
    for (const wxString& name : memoryFilesM)
        wxMemoryFSHandler::RemoveFile(name);
    wxLogNull noErrors;
    for (const wxString& path : tempFilesM)
        wxRemoveFile(path);
}

void BenchmarkReportPage::show(const wxString& html)
{
    if (!loadM)
    {
        viewM->SetPage(html, wxEmptyString);
        return;
    }
    // a new name for every page, so that no cached page is shown
    static int pageCount = 0;
    htmlM = html;
    nameM = wxString::Format("flamerobin-benchmark-%d.html", ++pageCount);
    fromFileM = false;
    const wxScopedCharBuffer utf8 = html.utf8_str();
    wxMemoryFSHandler::AddFileWithMimeType(nameM, utf8.data(), utf8.length(), "text/html");
    memoryFilesM.push_back(nameM);
    if (!load("memory:" + nameM))
        showFromFile();
}

void BenchmarkReportPage::showFromFile()
{
    // the same names as the pages for "Open in browser", so that pages
    // left by a crash are removed as well
    static int fileCount = 0;
    const wxFileName path(wxFileName::GetTempDir(), getBenchmarkPagePrefix()
        + wxString::Format("view-%lu-%d.html", wxGetProcessId(), ++fileCount));
    fromFileM = true;
    wxLogNull noErrors;
    wxFFile file(path.GetFullPath(), "w");
    if (file.IsOpened())
    {
        tempFilesM.push_back(path.GetFullPath());
        if (file.Write(htmlM, wxConvUTF8) && file.Close())
        {
            nameM = path.GetFullName();
            if (load(wxFileName::FileNameToURL(path)))
                return;
        }
    }
    nameM.clear();
    viewM->SetPage(htmlM, wxEmptyString);
}

// false if Internet Explorer refuses to load the page
bool BenchmarkReportPage::load(const wxString& url)
{
#ifdef __WXMSW__
    // as wxWebViewIE::LoadURL(), which cannot tell whether it worked
    IWebBrowser2* browser = static_cast<IWebBrowser2*>(viewM->GetNativeBackend());
    if (!browser)
        return false;
    BSTR target = ::SysAllocString(url.wc_str());
    VARIANT none;
    ::VariantInit(&none);
    const HRESULT result = browser->Navigate(target, &none, &none, &none, &none);
    ::SysFreeString(target);
    return SUCCEEDED(result);
#else
    viewM->LoadURL(url);
    return true;
#endif
}

void BenchmarkReportPage::removeOldPages()
{
    // the page shown now and the two before it stay
    const size_t keep = 3;
    if (memoryFilesM.size() > keep)
    {
        const auto last = memoryFilesM.end() - keep;
        for (auto it = memoryFilesM.begin(); it != last; ++it)
            wxMemoryFSHandler::RemoveFile(*it);
        memoryFilesM.erase(memoryFilesM.begin(), last);
    }
    if (tempFilesM.size() > keep)
    {
        wxLogNull noErrors;
        const auto last = tempFilesM.end() - keep;
        for (auto it = tempFilesM.begin(); it != last; ++it)
            wxRemoveFile(*it);
        tempFilesM.erase(tempFilesM.begin(), last);
    }
}

void BenchmarkReportPage::OnNavigating(wxWebViewEvent& event)
{
    // only to the page that is shown now: the back key would show an
    // earlier page, maybe one with the private information
    if (nameM.empty() || !event.GetURL().Contains(nameM))
        event.Veto();
    else
        event.Skip();
}

void BenchmarkReportPage::OnError(wxWebViewEvent& event)
{
    // only for the page that is shown now; it replaces the error page
    if (nameM.empty() || !event.GetURL().Contains(nameM))
    {
        event.Skip();
        return;
    }
    // Internet Explorer does not navigate from within this event
    const wxString name = nameM;
    const std::weak_ptr<bool> alive = aliveM;
    viewM->CallAfter([this, alive, name]()
    {
        // unless the page is gone or another one was shown in the meantime
        if (alive.expired() || name != nameM)
            return;
        if (!fromFileM)
            showFromFile();
        else
        {
            nameM.clear();
            viewM->SetPage(htmlM, wxEmptyString);
        }
    });
}

void BenchmarkReportPage::OnLoaded(wxWebViewEvent& event)
{
    event.Skip();
    // the pages before the one shown now are no longer read
    if (!nameM.empty() && event.GetURL().Contains(nameM))
        removeOldPages();
}
#endif
