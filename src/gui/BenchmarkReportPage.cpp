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
#include <mshtml.h>
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
    // no page leaves the view, whatever it contains
    viewM->Bind(wxEVT_WEBVIEW_NAVIGATING, &BenchmarkReportPage::OnNavigating, this);
    viewM->Bind(wxEVT_WEBVIEW_NEWWINDOW, &BenchmarkReportPage::OnNewWindow, this);
    if (!loadM)
        return;
    // one handler for the whole program, like the other file systems
    if (!wxFileSystem::HasHandlerForPath("memory:report.html"))
        wxFileSystem::AddHandler(new wxMemoryFSHandler);
    viewM->RegisterHandler(wxSharedPtr<wxWebViewHandler>(new wxWebViewFSHandler("memory")));
    viewM->Bind(wxEVT_WEBVIEW_ERROR, &BenchmarkReportPage::OnError, this);
    viewM->Bind(wxEVT_WEBVIEW_LOADED, &BenchmarkReportPage::OnLoaded, this);
}

BenchmarkReportPage::~BenchmarkReportPage()
{
    viewM->Unbind(wxEVT_WEBVIEW_NAVIGATING, &BenchmarkReportPage::OnNavigating, this);
    viewM->Unbind(wxEVT_WEBVIEW_NEWWINDOW, &BenchmarkReportPage::OnNewWindow, this);
    if (!loadM)
        return;
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

bool BenchmarkReportPage::isShownPage(const wxString& url) const
{
    // the other web views get the page directly
    return !loadM || (!nameM.empty() && url.Contains(nameM));
}

bool BenchmarkReportPage::runScript(const wxString& expression, wxString* result)
{
#ifdef __WXMSW__
    if (loadM)
        return runScriptInExplorer(expression, result);
#endif
    return viewM->RunScript(expression, result);
}

#ifdef __WXMSW__
namespace
{

// releases a COM interface at the end of the scope
template<class T>
class ComHolder
{
public:
    ComHolder() = default;
    ComHolder(const ComHolder&) = delete;
    ComHolder& operator=(const ComHolder&) = delete;
    ~ComHolder() { if (pointerM) pointerM->Release(); }
    T* get() const { return pointerM; }
    T** out() { return &pointerM; }
    T* operator->() const { return pointerM; }
private:
    T* pointerM = nullptr;
};

// frees a BSTR at the end of the scope
class BstrHolder
{
public:
    explicit BstrHolder(const wchar_t* text = nullptr)
        : textM(text ? ::SysAllocString(text) : nullptr) {}
    BstrHolder(const BstrHolder&) = delete;
    BstrHolder& operator=(const BstrHolder&) = delete;
    ~BstrHolder() { if (textM) ::SysFreeString(textM); }
    BSTR get() const { return textM; }
    BSTR* out() { return &textM; }
private:
    BSTR textM;
};

// clears a VARIANT at the end of the scope
class VariantHolder
{
public:
    VariantHolder() { ::VariantInit(&valueM); }
    VariantHolder(const VariantHolder&) = delete;
    VariantHolder& operator=(const VariantHolder&) = delete;
    ~VariantHolder() { ::VariantClear(&valueM); }
    VARIANT* get() { return &valueM; }
private:
    VARIANT valueM;
};

} // namespace

// the value of the expression comes back in the title of the document,
// which the dialog does not show
bool BenchmarkReportPage::runScriptInExplorer(const wxString& expression,
    wxString* result)
{
    IWebBrowser2* browser = static_cast<IWebBrowser2*>(viewM->GetNativeBackend());
    ComHolder<IDispatch> dispatch;
    if (!browser || FAILED(browser->get_Document(dispatch.out())) || !dispatch.get())
        return false;
    ComHolder<IHTMLDocument2> document;
    dispatch->QueryInterface(IID_IHTMLDocument2, reinterpret_cast<void**>(document.out()));
    if (!document.get())
        return false;
    ComHolder<IHTMLWindow2> window;
    if (FAILED(document->get_parentWindow(window.out())) || !window.get())
        return false;
    const wxString code = "document.title = (function() { try { return String("
        + expression + "); } catch (e) { return ''; } })()";
    BstrHolder codeText(code.wc_str());
    BstrHolder language(L"JavaScript");
    VariantHolder value;
    const bool done = SUCCEEDED(window->execScript(codeText.get(), language.get(),
        value.get()));
    BstrHolder title;
    if (done && result && SUCCEEDED(document->get_title(title.out())))
        *result = title.get() ? wxString(title.get()) : wxString();
    return done;
}
#endif

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
    // Edge and WebKit: only the pages of SetPage(), not a page elsewhere
    // that a saved report could lead to
    if (!loadM)
    {
        const wxString url = event.GetURL().Lower();
        if (url.StartsWith("about:") || url.StartsWith("data:"))
            event.Skip();
        else
            event.Veto();
        return;
    }
    // only to the page that is shown now: the back key would show an
    // earlier page, maybe one with the private information
    if (nameM.empty() || !event.GetURL().Contains(nameM))
        event.Veto();
    else
        event.Skip();
}

void BenchmarkReportPage::OnNewWindow(wxWebViewEvent& WXUNUSED(event))
{
    // a link or a script of the page opens no window
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
