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

#include <wx/aui/auibook.h>
#include <wx/simplebook.h>
#include <wx/statline.h>
#if wxUSE_WEBVIEW
#include <wx/webview.h>
#endif

#include <algorithm>

#include "gui/BenchmarkProgressPanel.h"
#include "gui/BenchmarkReportPage.h"
#include "gui/BenchmarkResultsView.h"

namespace
{

// the pages of what the selected tab shows
enum { progressPage, resultPage, logPage };

// an empty page of the strip that only carries the number of its tab
class ResultTab : public wxPanel
{
public:
    ResultTab(wxWindow* parent, int id) : wxPanel(parent), idM(id) {}
    int getResultId() const { return idM; }

private:
    int idM;
};

// the tabs of the platform, without a close button on the progress and the
// log, which belong to the test and cannot be closed
class ResultTabArt : public wxAuiDefaultTabArt
{
public:
    // a new one: wxWidgets 3.3 deletes the copy constructor of the flat tab art
    virtual wxAuiTabArt* Clone() override { return new ResultTabArt(); }
    virtual void DrawTab(wxDC& dc, wxWindow* wnd, const wxAuiNotebookPage& page,
        const wxRect& rect, int closeButtonState, wxRect* outTabRect,
        wxRect* outButtonRect, int* xExtent) override
    {
        const ResultTab* tab = dynamic_cast<const ResultTab*>(page.window);
        const bool fixed = !tab || tab->getResultId() <= 0;
        wxAuiDefaultTabArt::DrawTab(dc, wnd, page, rect,
            fixed ? wxAUI_BUTTON_STATE_HIDDEN : closeButtonState, outTabRect,
            outButtonRect, xExtent);
    }
};

// reads the state of the shown report: the scroll position, the checked
// inputs (the views "Overview / Detailed report" and the opened parts) and
// the opened <details>, as "y|ids|indexes"; also for Internet Explorer
const char* const saveStateScript = "(function() { var s = [], d = [], i, "
    "c = document.getElementsByTagName('input'); for (i = 0; i < c.length; i++) "
    "{ if (c[i].checked && c[i].id) { s.push(c[i].id); } } "
    "c = document.getElementsByTagName('details'); for (i = 0; i < c.length; i++) "
    "{ if (c[i].open) { d.push(i); } } var y = window.pageYOffset "
    "|| document.documentElement.scrollTop || document.body.scrollTop || 0; "
    "return Math.round(y) + '|' + s.join(',') + '|' + d.join(','); })()";

// the keys of the dialog that a page with the focus keeps for itself
// (Edge, WebKit): it hands them to the dialog
const char* const keyScript = "document.addEventListener('keydown', function(e) { "
    "if (!e.ctrlKey || e.altKey || e.metaKey) { return; } var k = e.keyCode, a = ''; "
    "if (k == 9) { a = e.shiftKey ? 'previous' : 'next'; } "
    "else if (e.shiftKey) { return; } else if (k == 33) { a = 'previous'; } "
    "else if (k == 34) { a = 'next'; } else if (k == 87) { a = 'close'; } "
    "else if (k == 79) { a = 'open'; } if (!a) { return; } "
    "var h = window.frbench || (window.webkit && window.webkit.messageHandlers "
    "&& window.webkit.messageHandlers.frbench); "
    "if (h) { e.preventDefault(); h.postMessage(a); } }, true);";

// the script that restores a state read with saveStateScript; everything
// but the characters of numbers and ids is left out
wxString getRestoreStateScript(const wxString& state)
{
    wxString clean;
    for (const wxUniChar c : state)
    {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
            || c == '_' || c == '-' || c == ',' || c == '|')
        {
            clean += c;
        }
    }
    const wxString y = clean.BeforeFirst('|');
    const wxString ids = clean.AfterFirst('|').BeforeFirst('|');
    const wxString open = clean.AfterFirst('|').AfterFirst('|');
    long top = 0;
    if (!y.ToLong(&top) || top < 0)
        top = 0;
    wxString idList;
    for (const wxString& id : wxSplit(ids, ',', '\0'))
    {
        if (!id.empty())
            idList += (idList.empty() ? "'" : ",'") + id + "'";
    }
    wxString openList;
    for (const wxString& index : wxSplit(open, ',', '\0'))
    {
        long n = 0;
        if (index.ToLong(&n) && n >= 0)
            openList += (openList.empty() ? "" : ",") + wxString::Format("%ld", n);
    }
    return "(function() { var ids = [" + idList + "], o = [" + openList + "], i, e, "
        "d = document.getElementsByTagName('details'); for (i = 0; i < ids.length; i++) "
        "{ e = document.getElementById(ids[i]); if (e) { e.checked = true; } } "
        "for (i = 0; i < o.length; i++) { if (d[o[i]]) { d[o[i]].open = true; } } "
        + wxString::Format("window.scrollTo(0, %ld); return 1; })()", top);
}

// an explaining text, quieter than a label, cut with an ellipsis where the
// dialog is narrow
wxStaticText* createQuietText(wxWindow* parent)
{
    wxStaticText* label = new wxStaticText(parent, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END | wxST_NO_AUTORESIZE);
    const wxColour foreground = parent->GetForegroundColour();
    const wxColour background = parent->GetBackgroundColour();
    label->SetForegroundColour(getReadableBenchmarkColour(mixBenchmarkColours(foreground,
        background, 0.68), foreground, background));
    return label;
}

} // namespace

BenchmarkResultsView::BenchmarkResultsView(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    // the strip only: its pages are empty, what a tab shows is below it;
    // no moving and no splitting of tabs
    stripM = new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxAUI_NB_TOP | wxAUI_NB_SCROLL_BUTTONS | wxAUI_NB_CLOSE_ON_ALL_TABS
        | wxAUI_NB_WINDOWLIST_BUTTON | wxAUI_NB_MIDDLE_CLICK_CLOSE);
    stripM->SetArtProvider(new ResultTabArt);
    stripM->AddPage(new ResultTab(stripM, progressTab), _("Progress"), true);
    stripM->AddPage(new ResultTab(stripM, logTab), _("Log"), false);
#if wxCHECK_VERSION(3, 3, 0)
    // wxWidgets 3.3 hides the close button of a locked tab; before, the tab
    // art above does it
    stripM->SetPageKind(0, wxAuiTabKind::Locked);
    stripM->SetPageKind(1, wxAuiTabKind::Locked);
#endif

    bookM = new wxSimplebook(this, wxID_ANY);
    progressM = new BenchmarkProgressPanel(bookM);
    bookM->AddPage(progressM, wxEmptyString);

    // the shown result, with what can be done with it above it
    wxPanel* pageResult = new wxPanel(bookM);
    wxBoxSizer* sizerResult = new wxBoxSizer(wxVERTICAL);
    const int gap = FromDIP(6);
    headerM = new wxStaticText(pageResult, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxST_ELLIPSIZE_END | wxST_NO_AUTORESIZE);
    // the message after a copy has the room it needs first
    headerM->SetMinSize(wxSize(FromDIP(20), -1));
    statusM = createQuietText(pageResult);
    copyM = new wxButton(pageResult, ID_button_copy, _("Copy as &Markdown"));
    saveM = new wxButton(pageResult, ID_button_save, _("Sa&ve report..."));
    browserM = new wxButton(pageResult, ID_button_browser, _("Open in &browser"));
    browserM->SetToolTip(_("Shows the report in the web browser of this computer. "
        "The page is written to the temporary directory; without \"Hide private "
        "information\" it contains the names of computers and paths."));
    closeM = new wxButton(pageResult, ID_button_close_result, _("C&lose result"));
    closeM->SetToolTip(_("Closes the shown result (Ctrl+W)."));
    wxBoxSizer* sizerHeader = new wxBoxSizer(wxHORIZONTAL);
    sizerHeaderM = sizerHeader;
    sizerHeader->Add(headerM, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));
    sizerHeader->Add(statusM, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(12));
    sizerHeader->Add(copyM, 0, wxRIGHT, gap);
    sizerHeader->Add(saveM, 0, wxRIGHT, gap);
    sizerHeader->Add(browserM, 0, wxRIGHT, gap);
    sizerHeader->Add(closeM, 0);
    sizerResult->Add(sizerHeader, 0, wxEXPAND | wxALL, gap);
    sizerResult->Add(new wxStaticLine(pageResult), 0, wxEXPAND);

#if wxUSE_WEBVIEW
    wxString backend;
    webViewM = createBenchmarkReportView(pageResult, backend);
    if (webViewM)
    {
        const bool internetExplorer = backend == wxWebViewBackendIE;
        reportPageM.reset(new BenchmarkReportPage(webViewM, internetExplorer));
        webViewM->Bind(wxEVT_WEBVIEW_LOADED, &BenchmarkResultsView::OnPageLoaded, this);
        // Internet Explorer passes the keys on by itself
        if (!internetExplorer && webViewM->AddScriptMessageHandler("frbench")
            && webViewM->AddUserScript(keyScript))
        {
            webViewM->Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED,
                &BenchmarkResultsView::OnScriptMessage, this);
        }
        if (internetExplorer)
        {
            viewNameM = "Internet Explorer";
            wxStaticText* hint = new wxStaticText(pageResult, wxID_ANY,
                _("The Microsoft Edge WebView2 Runtime is not installed on this "
                  "computer, so the report is shown with Internet Explorer in a "
                  "simpler form. \"Open in browser\" shows it in your web browser."));
            hint->Wrap(FromDIP(600));
            sizerResult->Add(hint, 0, wxEXPAND | wxALL, gap);
        }
        else
            viewNameM = wxWebView::GetBackendVersionInfo(backend).GetVersionString();
        sizerResult->Add(webViewM, 1, wxEXPAND);
    }
#endif
    if (!webViewM)
    {
        viewNameM = _("text, no web view is available");
        textM = new wxTextCtrl(pageResult, wxID_ANY, wxEmptyString, wxDefaultPosition,
            wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
        sizerResult->Add(textM, 1, wxEXPAND);
    }
    pageResult->SetSizer(sizerResult);
    bookM->AddPage(pageResult, wxEmptyString);

    // every message of the run, also the measurements as they come
    logM = new wxTextCtrl(bookM, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
    bookM->AddPage(logM, wxEmptyString);

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(stripM, 0, wxEXPAND);
    sizer->Add(bookM, 1, wxEXPAND);
    SetSizer(sizer);
    fitStrip();

    stripM->Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGING, &BenchmarkResultsView::OnPageChanging, this);
    stripM->Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, &BenchmarkResultsView::OnPageChanged, this);
    stripM->Bind(wxEVT_AUINOTEBOOK_PAGE_CLOSE, &BenchmarkResultsView::OnPageClose, this);
    stripM->Bind(wxEVT_AUINOTEBOOK_TAB_RIGHT_UP, &BenchmarkResultsView::OnTabRightUp, this);
    Bind(wxEVT_DPI_CHANGED, &BenchmarkResultsView::OnDPIChanged, this);
}

BenchmarkResultsView::~BenchmarkResultsView()
{
#if wxUSE_WEBVIEW
    if (webViewM)
    {
        webViewM->Unbind(wxEVT_WEBVIEW_LOADED, &BenchmarkResultsView::OnPageLoaded, this);
        webViewM->Unbind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED,
            &BenchmarkResultsView::OnScriptMessage, this);
    }
#endif
}

bool BenchmarkResultsView::hasWebView() const
{
    return webViewM != nullptr;
}

void BenchmarkResultsView::fitStrip()
{
    // as high as its tabs: the pages of the strip are empty; not wider
    // than the dialog, the arrows and the list reach every tab
    const int height = stripM->GetTabCtrlHeight() + 2;
    stripM->SetMinSize(wxSize(FromDIP(200), height));
    stripM->SetMaxSize(wxSize(-1, height));
    Layout();
}

void BenchmarkResultsView::OnDPIChanged(wxDPIChangedEvent& event)
{
    event.Skip();
    // the tab art measures its tabs once: a new one for the new resolution
    CallAfter([this]()
    {
        stripM->SetArtProvider(new ResultTabArt);
        fitStrip();
    });
}

int BenchmarkResultsView::getTabId(size_t index) const
{
    const ResultTab* tab = dynamic_cast<const ResultTab*>(stripM->GetPage(index));
    return tab ? tab->getResultId() : progressTab;
}

int BenchmarkResultsView::findTab(int id) const
{
    for (size_t i = 0; i < stripM->GetPageCount(); ++i)
    {
        if (getTabId(i) == id)
            return int(i);
    }
    return wxNOT_FOUND;
}

void BenchmarkResultsView::addResult(int id, const wxString& label, const wxString& toolTip)
{
    ++changingM;
    stripM->AddPage(new ResultTab(stripM, id), label, false);
    --changingM;
    tipsM[id] = toolTip;
    setResultLabel(id, label, toolTip);
}

void BenchmarkResultsView::setResultLabel(int id, const wxString& label,
    const wxString& toolTip)
{
    const int index = findTab(id);
    if (index == wxNOT_FOUND)
        return;
    tipsM[id] = toolTip;
    stripM->SetPageText(index, label);
    // what a locked tab is, also where nothing else says it
    stripM->SetPageToolTip(index, lockedM ? toolTip + "\n" + _("A result can be shown "
        "again when the test has finished, so that it does not disturb the measurement.")
        : toolTip);
}

void BenchmarkResultsView::removeResult(int id)
{
    const int index = findTab(id);
    if (index == wxNOT_FOUND)
        return;
    const bool selected = getSelection() == id;
    ++changingM;
    stripM->DeletePage(index);
    --changingM;
    statesM.erase(id);
    tipsM.erase(id);
    pagesM.erase(std::remove_if(pagesM.begin(), pagesM.end(),
        [id](const Page& p) { return p.id == id; }), pagesM.end());
    if (shownM == id)
    {
        shownM = 0;
        restoreM.clear();
    }
    // the strip chose a neighbour on its own; the caller selects
    if (selected)
        select(progressTab);
}

int BenchmarkResultsView::getSelection() const
{
    const int index = stripM->GetSelection();
    return index == wxNOT_FOUND ? progressTab : getTabId(size_t(index));
}

void BenchmarkResultsView::select(int id)
{
    // a script waits for its result and lets events in: not in between
    if (scriptsM)
    {
        CallAfter([this, id]() { select(id); });
        return;
    }
    const int index = findTab(id);
    if (index == wxNOT_FOUND)
        return;
    if (index != stripM->GetSelection())
    {
        saveState();
        ++changingM;
        stripM->ChangeSelection(size_t(index));
        --changingM;
    }
    showContent(id);
    if (onSelected)
        onSelected(id);
}

void BenchmarkResultsView::selectNext(bool forward)
{
    const int count = int(stripM->GetPageCount());
    const int current = std::max(0, stripM->GetSelection());
    for (int i = 1; i < count; ++i)
    {
        const int index = (current + (forward ? i : count - i)) % count;
        const int id = getTabId(size_t(index));
        if (lockedM && id > 0)
            continue;
        select(id);
        return;
    }
}

void BenchmarkResultsView::setLocked(bool locked)
{
    lockedM = locked;
    const std::map<int, wxString> tips = tipsM;
    for (const auto& tip : tips)
    {
        const int index = findTab(tip.first);
        if (index != wxNOT_FOUND)
            setResultLabel(tip.first, stripM->GetPageText(index), tip.second);
    }
}

void BenchmarkResultsView::setPageVariant(const wxString& variant)
{
    if (variant == variantM)
        return;
    variantM = variant;
    pagesM.clear();
    // the shown result again, where it was
    const int id = getSelection();
    if (id > 0)
    {
        saveState();
        shownM = 0;
        showContent(id);
    }
}

void BenchmarkResultsView::setHeader(const wxString& text, const wxString& toolTip)
{
    headerM->SetLabel(text);
    // the whole text where it is cut
    headerM->SetToolTip(toolTip.empty() ? text : toolTip);
}

void BenchmarkResultsView::setStatus(const wxString& message)
{
    statusM->SetLabel(message);
    // the whole message where it is cut
    statusM->SetToolTip(message);
    // a short message is never cut, a longer one at the width of the
    // short ones: the dialog does not get wider
    const int width = message.empty() ? 0 : statusM->GetTextExtent(message).x;
    statusM->SetMinSize(wxSize(std::min(width, statusWidthM), -1));
    sizerHeaderM->Layout();
}

void BenchmarkResultsView::setStatusWidth(const wxString& message)
{
    statusWidthM = statusM->GetTextExtent(message).x;
}

const wxString& BenchmarkResultsView::getPage(int id)
{
    auto it = std::find_if(pagesM.begin(), pagesM.end(),
        [id](const Page& p) { return p.id == id; });
    if (it == pagesM.end())
    {
        // without private information a page takes up to a second
        wxBusyCursor wait;
        pagesM.push_back({ id, makePage ? makePage(id) : wxString() });
        // four pages are about 2 MB
        if (pagesM.size() > 4)
            pagesM.erase(pagesM.begin());
    }
    else if (it + 1 != pagesM.end())
    {
        // the last one used stays longest
        Page page = *it;
        pagesM.erase(it);
        pagesM.push_back(page);
    }
    return pagesM.back().text;
}

void BenchmarkResultsView::showContent(int id)
{
    if (id == progressTab)
    {
        bookM->ChangeSelection(progressPage);
        return;
    }
    if (id == logTab)
    {
        bookM->ChangeSelection(logPage);
        return;
    }
    bookM->ChangeSelection(resultPage);
    if (shownM == id)
        return;
    shownM = id;
    loadedM = false;
    auto state = statesM.find(id);
    restoreM = state == statesM.end() ? wxString() : state->second;
    const wxString& page = getPage(id);
#if wxUSE_WEBVIEW
    if (webViewM)
    {
        reportPageM->show(page);
        return;
    }
#endif
    textM->SetValue(page);
    textM->ShowPosition(0);
}

void BenchmarkResultsView::saveState()
{
#if wxUSE_WEBVIEW
    // only the page in the view, and only once it has loaded: until then
    // the state to restore is still the right one
    if (!webViewM || shownM <= 0 || getSelection() != shownM || scriptsM)
        return;
    if (!loadedM)
    {
        statesM[shownM] = restoreM;
        return;
    }
    // the script lets events in: the state belongs to the result it was
    // read from, and not to another one shown in the meantime
    const int id = shownM;
    wxString state;
    // a script that fails costs only the state
    if (runScript(saveStateScript, &state) && state.Contains("|") && shownM == id)
        statesM[id] = state;
#endif
}

#if wxUSE_WEBVIEW
bool BenchmarkResultsView::runScript(const wxString& script, wxString* result)
{
    ++scriptsM;
    const bool done = reportPageM->runScript(script, result);
    --scriptsM;
    return done;
}

void BenchmarkResultsView::restoreState()
{
    if (!loadedM || restoreM.empty())
        return;
    // after the script that is running now
    if (scriptsM)
    {
        const int id = shownM;
        CallAfter([this, id]()
        {
            if (shownM == id)
                restoreState();
        });
        return;
    }
    const wxString script = getRestoreStateScript(restoreM);
    restoreM.clear();
    runScript(script, nullptr);
}
#endif

#if wxUSE_WEBVIEW
void BenchmarkResultsView::OnPageLoaded(wxWebViewEvent& event)
{
    event.Skip();
    if (!reportPageM->isShownPage(event.GetURL()))
        return;
    loadedM = true;
    restoreState();
}

void BenchmarkResultsView::OnScriptMessage(wxWebViewEvent& event)
{
    const wxString key = event.GetString();
    if (key == "next" || key == "previous")
        selectNext(key == "next");
    else if ((key == "close" || key == "open") && onShortcut)
        onShortcut(key == "close" ? 'W' : 'O');
}
#else
void BenchmarkResultsView::OnPageLoaded(wxWebViewEvent& WXUNUSED(event))
{
}

void BenchmarkResultsView::OnScriptMessage(wxWebViewEvent& WXUNUSED(event))
{
}
#endif

void BenchmarkResultsView::OnPageChanging(wxAuiNotebookEvent& event)
{
    if (changingM)
    {
        event.Skip();
        return;
    }
    const int id = event.GetSelection() == wxNOT_FOUND ? progressTab
        : getTabId(size_t(event.GetSelection()));
    // making and showing a report would disturb the measurement
    if (lockedM && id > 0)
    {
        event.Veto();
        wxBell();
        return;
    }
    saveState();
    event.Skip();
}

void BenchmarkResultsView::OnPageChanged(wxAuiNotebookEvent& event)
{
    event.Skip();
    if (changingM)
        return;
    const int id = getSelection();
    showContent(id);
    if (onSelected)
        onSelected(id);
}

void BenchmarkResultsView::OnPageClose(wxAuiNotebookEvent& event)
{
    // the dialog closes a result, it may ask first; the progress and the
    // log cannot be closed
    event.Veto();
    const int id = event.GetSelection() == wxNOT_FOUND ? progressTab
        : getTabId(size_t(event.GetSelection()));
    if (id > 0 && !lockedM && onCloseResult)
        CallAfter([this, id]() { onCloseResult(id); });
}

void BenchmarkResultsView::OnTabRightUp(wxAuiNotebookEvent& event)
{
    const int id = event.GetSelection() == wxNOT_FOUND ? progressTab
        : getTabId(size_t(event.GetSelection()));
    if (id > 0 && !lockedM && onResultMenu)
        onResultMenu(id);
}
