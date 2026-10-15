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

#include <wx/arrstr.h>
#include <wx/clipbrd.h>
#include <wx/collheaderctrl.h>
#include <wx/dir.h>
#include <wx/display.h>
#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/popupwin.h>
#include <wx/simplebook.h>
#include <wx/statline.h>
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

#include <algorithm>
#include <cmath>

#include "config/Config.h"
#include "core/StringUtils.h"
#include "engine/db/IDatabase.h"
#include "gui/BenchmarkDialog.h"
#include "gui/FRStyleManager.h"
#include "gui/StyleGuide.h"
#include "gui/UsernamePasswordDialog.h"
#include "MasterPassword.h"
#include "metadata/server.h"

namespace
{

enum
{
    ID_choice_target = 4101,
    ID_button_start,
    ID_button_cancel,
    ID_button_copy,
    ID_button_save,
    ID_button_browser,
    ID_button_earlier_browse,
    ID_button_compare,
    ID_check_private,
    ID_choice_install,
    ID_header_advanced,
    ID_timer
};

using fr::BenchmarkTarget;

wxString getTargetName(BenchmarkTarget target)
{
    switch (target)
    {
        case BenchmarkTarget::Registered:
            return _("Server of the selected database");
        case BenchmarkTarget::ThisComputer:
            return _("This computer (embedded Firebird)");
        case BenchmarkTarget::OtherServer:
        default:
            return _("Another Firebird server");
    }
}

// the short line next to the list of targets
wxString getTargetHint(BenchmarkTarget target)
{
    switch (target)
    {
        case BenchmarkTarget::Registered:
            return _("Uses the login of the registration.");
        case BenchmarkTarget::ThisComputer:
            return _("No server or login needed.");
        case BenchmarkTarget::OtherServer:
        default:
            return _("Needs a login that may create databases.");
    }
}

// a mix of two colours, with this share of the first one; the colours of
// the dialog come from the system, so that they follow its dark mode and
// its high contrast
wxColour mixColours(const wxColour& first, const wxColour& second, double share)
{
    return wxColour(wxColour::AlphaBlend(first.Red(), second.Red(), share),
        wxColour::AlphaBlend(first.Green(), second.Green(), share),
        wxColour::AlphaBlend(first.Blue(), second.Blue(), share));
}

// the relative luminance and the contrast ratio of WCAG 2
double getLuminance(const wxColour& colour)
{
    auto channel = [](unsigned char value)
    {
        const double share = value / 255.0;
        return share <= 0.03928 ? share / 12.92 : std::pow((share + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * channel(colour.Red()) + 0.7152 * channel(colour.Green())
        + 0.0722 * channel(colour.Blue());
}

double getContrast(const wxColour& first, const wxColour& second)
{
    const double a = getLuminance(first);
    const double b = getLuminance(second);
    return (std::max(a, b) + 0.05) / (std::min(a, b) + 0.05);
}

// the colour, moved towards the text colour until it can be read on the
// background (4.5:1); a theme may have a link colour that is too pale
wxColour getReadableColour(const wxColour& colour, const wxColour& text,
    const wxColour& background)
{
    for (int step = 0; step < 10; ++step)
    {
        const wxColour mixed = mixColours(text, colour, step / 10.0);
        if (getContrast(mixed, background) >= 4.5)
            return mixed;
    }
    return text;
}

// an explaining text, quieter than a label; one that is cut with an
// ellipsis where the dialog is narrow does not make the dialog wider
wxStaticText* createHintText(wxWindow* parent, const wxString& text, bool ellipsize = false)
{
    wxStaticText* label = new wxStaticText(parent, wxID_ANY, text, wxDefaultPosition,
        wxDefaultSize, ellipsize ? wxST_ELLIPSIZE_END | wxST_NO_AUTORESIZE : 0);
    // between the text and the background, nearer to the text
    const wxColour foreground = parent->GetForegroundColour();
    const wxColour background = parent->GetBackgroundColour();
    label->SetForegroundColour(getReadableColour(mixColours(foreground, background, 0.68),
        foreground, background));
    if (ellipsize)
        label->SetMinSize(wxSize(parent->FromDIP(40), -1));
    return label;
}

// the message after "Copy as Markdown"
wxString getCopiedMessage()
{
    return _("Copied as Markdown.");
}

// the explanation behind a "?": a small window below it with a title and
// paragraphs of wrapped text, each with an optional term in bold above it,
// in the colours of the text fields of the system (so it is readable in
// light and dark mode) with a quiet frame. It closes with a click
// elsewhere, and with Escape and Tab (BenchmarkDialog::OnCharHook()).
class HelpPopup : public wxPopupTransientWindow
{
public:
    HelpPopup(wxWindow* parent, wxWindow* button, const wxString& title,
        const std::vector<std::pair<wxString, wxString>>& items);
    wxWindow* getButton() const { return buttonM; }
    // whether it was closed just now: the click on its "?" that closed it
    // must not open it again
    bool closedJustNow() const;

protected:
    virtual void OnDismiss() override;

private:
    wxWindow* buttonM;
    wxLongLong closedM = 0;
};

HelpPopup::HelpPopup(wxWindow* parent, wxWindow* button, const wxString& title,
    const std::vector<std::pair<wxString, wxString>>& items)
    : wxPopupTransientWindow(parent, wxBORDER_NONE), buttonM(button)
{
    // on the screen of the "?" before anything is laid out: with another
    // resolution than the first screen its texts would be wrapped too wide
    Move(button->GetScreenPosition());
    const wxColour background = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    const wxColour foreground = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
    // the frame is the popup itself, one pixel around its panel
    SetBackgroundColour(mixColours(foreground, background, 0.4));
    wxPanel* panel = new wxPanel(this);
    panel->SetBackgroundColour(background);
    panel->SetForegroundColour(foreground);
    const wxFont bold = panel->GetFont().Bold();
    wxBoxSizer* sizerText = new wxBoxSizer(wxVERTICAL);
    auto addText = [this, panel, &foreground, sizerText](const wxString& text,
        const wxFont& font, int below)
    {
        wxStaticText* label = new wxStaticText(panel, wxID_ANY, text);
        if (font.IsOk())
            label->SetFont(font);
        label->SetForegroundColour(foreground);
        label->Wrap(FromDIP(360));
        sizerText->Add(label, 0, wxBOTTOM, below);
    };
    addText(title, bold.Larger(), FromDIP(8));
    for (size_t i = 0; i < items.size(); ++i)
    {
        if (!items[i].first.empty())
            addText(items[i].first, bold, FromDIP(1));
        addText(items[i].second, wxNullFont,
            i + 1 < items.size() ? FromDIP(8) : 0);
    }
    wxBoxSizer* sizerPanel = new wxBoxSizer(wxVERTICAL);
    sizerPanel->Add(sizerText, 1, wxEXPAND | wxALL, FromDIP(12));
    panel->SetSizer(sizerPanel);
    wxBoxSizer* sizerFrame = new wxBoxSizer(wxVERTICAL);
    sizerFrame->Add(panel, 1, wxEXPAND | wxALL, 1);
    SetSizerAndFit(sizerFrame);
}

bool HelpPopup::closedJustNow() const
{
    return !IsShown() && closedM != 0 && wxGetUTCTimeMillis() - closedM < 300;
}

void HelpPopup::OnDismiss()
{
    closedM = wxGetUTCTimeMillis();
}

const wxString pendingDatabasesKey = "BenchmarkPendingDatabases";
const wxString earlierFileKey = "BenchmarkCompareWithFile";
const wxString hidePrivateKey = "BenchmarkHidePrivateInformation";
// the pages for "Open in browser" in the temporary directory
const wxString browserFilePrefix = "flamerobin-benchmark-";

// the inverse of wx2std()
wxString toWxString(const std::string& s)
{
    return wxString(s.c_str(), *wxConvCurrent);
}

// the results in a saved report (Markdown or HTML), or why there are none
fr::BenchmarkSnapshotResult readReportFile(const wxString& path)
{
    wxFFile file(path, "r");
    wxString text;
    if (file.IsOpened() && file.ReadAll(&text, wxConvUTF8))
        return fr::parseBenchmarkSnapshot(text);
    return fr::BenchmarkSnapshotResult();
}

#if wxUSE_WEBVIEW
// on Windows Edge (WebView2) where its runtime is installed, then Internet
// Explorer, which shows the report in a simpler form; nullptr if there is no
// web view, the report is shown as text then
wxWebView* createReportView(wxWindow* parent, wxString& backend)
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
#endif

} // namespace

#if wxUSE_WEBVIEW
// Internet Explorer chooses the mode of a page, and with it how much of the
// style sheet it knows, when it loads the page. SetPage() writes into the
// blank page that was loaded before, which stays in the mode of Internet
// Explorer 7 or 8, whatever the page says. So that the report gets the mode
// of Internet Explorer 11, it is loaded from memory, if that fails from a
// temporary file, and only then written with SetPage(); no setting of the
// system is changed. The other web views simply get the page.
class BenchmarkReportPage
{
public:
    BenchmarkReportPage(wxWebView* view, bool internetExplorer);
    // removes the pages from memory and disk
    ~BenchmarkReportPage();
    void show(const wxString& html);

private:
    wxWebView* viewM;
    bool loadM;
    // the page that is shown and the file name in its URL
    wxString htmlM;
    wxString nameM;
    bool fromFileM = false;
    // the earlier pages stay until the end: Internet Explorer may still be
    // reading one, and wxMemoryFSHandler lends it the memory of the page
    std::vector<wxString> memoryFilesM;
    std::vector<wxString> tempFilesM;

    void showFromFile();
    bool load(const wxString& url);
    void OnNavigating(wxWebViewEvent& event);
    void OnError(wxWebViewEvent& event);
};

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
}

BenchmarkReportPage::~BenchmarkReportPage()
{
    if (!loadM)
        return;
    viewM->Unbind(wxEVT_WEBVIEW_NAVIGATING, &BenchmarkReportPage::OnNavigating, this);
    viewM->Unbind(wxEVT_WEBVIEW_ERROR, &BenchmarkReportPage::OnError, this);
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
    const wxFileName path(wxFileName::GetTempDir(), browserFilePrefix
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
    viewM->CallAfter([this, name]()
    {
        // unless another page was shown in the meantime
        if (name != nameM)
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
#endif

BEGIN_EVENT_TABLE(BenchmarkDialog, BaseDialog)
    EVT_BUTTON(ID_button_start, BenchmarkDialog::OnStart)
    EVT_BUTTON(ID_button_cancel, BenchmarkDialog::OnCancel)
    EVT_BUTTON(ID_button_copy, BenchmarkDialog::OnCopyMarkdown)
    EVT_BUTTON(ID_button_save, BenchmarkDialog::OnSaveReport)
    EVT_BUTTON(ID_button_browser, BenchmarkDialog::OnOpenInBrowser)
    EVT_BUTTON(ID_button_earlier_browse, BenchmarkDialog::OnBrowseEarlier)
    EVT_BUTTON(ID_button_compare, BenchmarkDialog::OnCompareReports)
    EVT_CHECKBOX(ID_check_private, BenchmarkDialog::OnPrivateChanged)
    EVT_CHOICE(ID_choice_install, BenchmarkDialog::OnTargetChanged)
    EVT_COLLAPSIBLEHEADER_CHANGED(ID_header_advanced, BenchmarkDialog::OnAdvancedToggled)
    EVT_BUTTON(wxID_CANCEL, BenchmarkDialog::OnCloseButton)
    EVT_CHOICE(ID_choice_target, BenchmarkDialog::OnTargetChanged)
    EVT_TIMER(ID_timer, BenchmarkDialog::OnTimer)
END_EVENT_TABLE()

BenchmarkDialog::BenchmarkDialog(wxWindow* parent, Database* db)
    : BaseDialog(parent, -1, wxEmptyString), databaseM(db),
      timerM(this, ID_timer)
{
    SetTitle(databaseM ? wxString::Format(_("Performance Benchmark & Diagnosis - %s"),
        databaseM->getName_()) : _("Performance Benchmark & Diagnosis"));
    createControls();
    updateControls();
    // BaseDialog connects its own close handler dynamically, which ends a
    // modal dialog at once; this one is bound later, so it runs first and
    // can wait for the cleanup of a running test
    Bind(wxEVT_CLOSE_WINDOW, &BenchmarkDialog::OnClose, this);
    Bind(wxEVT_CHAR_HOOK, &BenchmarkDialog::OnCharHook, this);
    // the settings start with the target, not with the first text field
    CallAfter([this]() { choice_target->SetFocus(); });
    CallAfter([this]() { dropPendingDatabases(); });
}

BenchmarkDialog::~BenchmarkDialog()
{
    // normally the dialog cannot be closed while the benchmark runs; this
    // is the last line of defence so that the thread never outlives us
    if (workerM.joinable())
    {
        runnerM->requestCancel();
        workerM.join();
    }
}

const wxString BenchmarkDialog::getName() const
{
    return "BenchmarkDialog";
}

const wxString BenchmarkDialog::getStorageName() const
{
    // the size saved before the settings were compact is at least that of
    // the old layout (1104 x 880), which is too large for small screens
    return "BenchmarkDialogCompact";
}

wxButton* BenchmarkDialog::createHelpButton(wxWindow* parent, const wxString& title,
    const HelpItems& items)
{
    // a click, Space or Enter opens the explanation; a tool tip could not be
    // reached with the keyboard or by touch
    wxButton* button = new wxButton(parent, wxID_ANY, "?", wxDefaultPosition,
        wxDefaultSize, wxBU_EXACTFIT | wxBORDER_NONE);
    button->SetFont(button->GetFont().Bold());
    // the colour of links, as readable as a label
    const wxColour text = parent->GetForegroundColour();
    const wxColour background = parent->GetBackgroundColour();
    button->SetForegroundColour(getReadableColour(
        wxSystemSettings::GetColour(wxSYS_COLOUR_HOTLIGHT), text, background));
    // large enough to be hit
    button->SetMinSize(wxSize(std::max(button->GetBestSize().x, FromDIP(24)), -1));
    button->SetToolTip(title);
    button->Bind(wxEVT_BUTTON, [this, button, title, items](wxCommandEvent&)
    {
        showHelp(button, title, items);
    });
    return button;
}

void BenchmarkDialog::showHelp(wxWindow* button, const wxString& title,
    const HelpItems& items)
{
    if (HelpPopup* last = static_cast<HelpPopup*>(popup_help))
    {
        // a click on the "?" of the open explanation closes it: the click
        // has already done that when it gets here
        if (last->IsShown())
            last->Dismiss();
        else if (last->getButton() == button && last->closedJustNow())
            return;
        last->Destroy();
        popup_help = nullptr;
    }
    HelpPopup* popup = new HelpPopup(this, button, title, items);
    popup_help = popup;
    // below the "?"; its right edge at the right edge of the "?" if it would
    // stand out of the dialog on the right
    const wxRect rectButton = button->GetScreenRect();
    const wxRect rectDialog = GetScreenRect();
    const int width = popup->GetSize().x;
    int x = rectButton.x;
    if (x + width > rectDialog.GetRight())
        x = std::max(rectDialog.x, rectButton.GetRight() + 1 - width);
    popup->Position(wxPoint(x, rectButton.y), wxSize(0, rectButton.height));
    popup->Popup();
}

void BenchmarkDialog::OnCharHook(wxKeyEvent& event)
{
    // Escape closes an open explanation, not the dialog; Tab closes it and
    // moves on
    const int key = event.GetKeyCode();
    if (!popup_help || !popup_help->IsShown() || (key != WXK_ESCAPE && key != WXK_TAB))
    {
        event.Skip();
        return;
    }
    popup_help->Dismiss();
    wxWindow* button = static_cast<HelpPopup*>(popup_help)->getButton();
    // the "?" keeps the focus (Windows), or the popup took it (GTK)
    if (wxWindow::FindFocus() == button)
    {
        if (key == WXK_TAB)
            event.Skip();
        return;
    }
    button->SetFocus();
    if (key == WXK_TAB)
    {
        // once the focus is back
        const int direction = event.ShiftDown() ? wxNavigationKeyEvent::IsBackward
            : wxNavigationKeyEvent::IsForward;
        CallAfter([button, direction]() { button->Navigate(direction); });
    }
}

void BenchmarkDialog::createControls()
{
    wxPanel* panel = getControlsPanel();
    wxBoxSizer* sizerMain = new wxBoxSizer(wxVERTICAL);
    const int gap = FromDIP(6);
    // between the lines of the settings
    const int lineGap = FromDIP(8);

    // the settings; after a run they make room for the report
    wxBoxSizer* sizerSettings = new wxBoxSizer(wxVERTICAL);
    sizerSettingsM = sizerSettings;
    // what happens, in one sentence; the details are behind the "?" of the
    // target
    sizerSettings->Add(new wxStaticText(panel, wxID_ANY, _("Creates a temporary "
        "database in the directory below and drops it afterwards; no other database "
        "is changed.")), 0, wxBOTTOM, FromDIP(14));

    // the "?" of the target first, for the width of the column of the "?";
    // in the order of the tab key it follows the list of targets
    wxButton* helpTarget = createHelpButton(panel, _("Where the test runs"), {
        { getTargetName(BenchmarkTarget::Registered), _("The server of the registration, "
            "with its login. Also measures the network to the server.") },
        { getTargetName(BenchmarkTarget::ThisComputer), _("Runs Firebird inside FlameRobin "
            "with the engine of an installation on this computer; no server, no login. "
            "The network is not tested: run it on the server itself.") },
        { getTargetName(BenchmarkTarget::OtherServer), _("A server you enter below. Its "
            "login must be allowed to create databases.") },
        { wxString(), _("The test creates a temporary database (FR_BENCHMARK_*.FDB), runs "
            "in it and drops it afterwards. No other database is changed, but the server "
            "is under load while the test runs.") } });

    // every line has a column of labels, the fields, and a column for the
    // "?" at the right edge: all fields start and end at the same place
    int labelWidth = 0;
    for (const wxString& label : { _("&Test:"), _("Host:"), _("User:"), _("&Firebird:"),
        _("&Directory:"), _("Compa&re with:"), _("Intensity:"), _("Client library:"),
        _("Page size:"), _("Also:") })
    {
        labelWidth = std::max(labelWidth,
            panel->GetTextExtent(wxControl::RemoveMnemonics(label)).x);
    }
    const int helpWidth = helpTarget->GetMinSize().x;
    // the label is created before its fields, so that its mnemonic selects them
    auto createLabel = [labelWidth](wxWindow* parent, const wxString& text)
    {
        wxStaticText* label = new wxStaticText(parent, wxID_ANY, text);
        label->SetMinSize(wxSize(labelWidth, -1));
        return label;
    };
    // the label stands next to the first of several lines of fields
    auto createLine = [this, labelWidth, helpWidth](wxWindow* label, wxSizer* fields,
        wxWindow* help, bool labelOnTop = false)
    {
        wxBoxSizer* line = new wxBoxSizer(wxHORIZONTAL);
        if (label)
        {
            line->Add(label, 0, (labelOnTop ? wxALIGN_TOP : wxALIGN_CENTER_VERTICAL)
                | wxRIGHT, FromDIP(10));
        }
        else
            line->AddSpacer(labelWidth + FromDIP(10));
        line->Add(fields, 1, wxALIGN_CENTER_VERTICAL);
        if (help)
            line->Add(help, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(4));
        else
            line->AddSpacer(helpWidth + FromDIP(4));
        return line;
    };

    // where to test: a list with short names, the details on a page below
    wxStaticText* labelTarget = createLabel(panel, _("&Test:"));
    choice_target = new wxChoice(panel, ID_choice_target);
    for (BenchmarkTarget target : fr::getBenchmarkTargets(databaseM != nullptr))
        choice_target->Append(getTargetName(target));
    choice_target->SetSelection(0);
    helpTarget->MoveAfterInTabOrder(choice_target);
    label_target_hint = createHintText(panel, wxEmptyString, true);
    wxBoxSizer* sizerTarget = new wxBoxSizer(wxHORIZONTAL);
    sizerTarget->Add(choice_target, 0, wxALIGN_CENTER_VERTICAL);
    sizerTarget->Add(label_target_hint, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));
    sizerSettings->Add(createLine(labelTarget, sizerTarget, helpTarget), 0,
        wxEXPAND | wxBOTTOM, lineGap);

    // one page per target, in the order of BenchmarkTarget: the pages are as
    // high as the highest, so nothing moves when the target changes; each
    // page is a line with its own label
    book_target = new wxSimplebook(panel, wxID_ANY);
    installationsM = fr::findBenchmarkFirebirdInstallations();
    for (BenchmarkTarget target : { BenchmarkTarget::Registered,
        BenchmarkTarget::ThisComputer, BenchmarkTarget::OtherServer })
    {
        const fr::BenchmarkTargetDetails details = fr::getBenchmarkTargetDetails(target);
        wxPanel* page = new wxPanel(book_target);
        wxBoxSizer* sizerPage = new wxBoxSizer(wxHORIZONTAL);
        auto addLabel = [page, sizerPage, gap](const wxString& label)
        {
            sizerPage->Add(new wxStaticText(page, wxID_ANY, label), 0,
                wxALIGN_CENTER_VERTICAL | wxRIGHT, gap);
        };
        wxStaticText* labelPage = nullptr;
        if (details.hostAndPort)
        {
            labelPage = createLabel(page, _("Host:"));
            text_host = new wxTextCtrl(page, wxID_ANY, wxEmptyString);
            text_host->SetMinSize(wxSize(FromDIP(110), -1));
            sizerPage->Add(text_host, 3, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(12));
            addLabel(_("Port:"));
            text_port = new wxTextCtrl(page, wxID_ANY, wxEmptyString);
            text_port->SetHint("3050");
            text_port->SetMinSize(text_port->GetSizeFromTextSize(
                text_port->GetTextExtent("65535").x));
            sizerPage->Add(text_port, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(12));
        }
        if (details.login)
        {
            if (labelPage)
                addLabel(_("User:"));
            else
                labelPage = createLabel(page, _("User:"));
            wxTextCtrl* user = new wxTextCtrl(page, wxID_ANY, wxEmptyString);
            user->SetMinSize(wxSize(FromDIP(80), -1));
            sizerPage->Add(user, 3, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(12));
            addLabel(_("Password:"));
            wxTextCtrl* password = new wxTextCtrl(page, wxID_ANY, wxEmptyString,
                wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
            password->SetMinSize(wxSize(FromDIP(80), -1));
            sizerPage->Add(password, 2, wxALIGN_CENTER_VERTICAL);
            if (details.loginOptional)
            {
                user->SetHint(_("from the registration or asked at start"));
                sizerPage->Add(createHintText(page, _("optional")), 0,
                    wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));
            }
            if (target == BenchmarkTarget::Registered)
            {
                text_user_registered = user;
                text_password_registered = password;
            }
            else
            {
                text_user = user;
                text_password = password;
            }
        }
        if (details.installation)
        {
            // the Firebird installations of this computer, for a test without login
            labelPage = createLabel(page, _("&Firebird:"));
            wxArrayString installations;
            for (const auto& i : installationsM)
            {
                wxString label = i.version.empty() ? wxString("Firebird")
                    : "Firebird " + toWxString(i.version);
                label += " (" + toWxString(i.directory);
                if (!i.service.empty())
                {
                    label += ", " + (i.running ? wxString::Format(_("service %s running"),
                        toWxString(i.service)) : wxString::Format(_("service %s"),
                        toWxString(i.service)));
                }
                installations.Add(label + ")");
            }
            installations.Add(_("the client library of the advanced settings"));
            choice_install = new wxChoice(page, ID_choice_install, wxDefaultPosition,
                wxDefaultSize, installations);
            choice_install->SetSelection(0);
            // GTK makes a list as wide as its longest entry
            choice_install->SetMinSize(wxSize(FromDIP(260), -1));
            sizerPage->Add(choice_install, 1, wxALIGN_CENTER_VERTICAL);
        }
        page->SetSizer(createLine(labelPage, sizerPage, nullptr));
        book_target->AddPage(page, wxEmptyString);
    }
    sizerSettings->Add(book_target, 0, wxEXPAND | wxBOTTOM, lineGap);

    wxStaticText* labelDirectory = createLabel(panel, _("&Directory:"));
    // the directory of the selected database, so that its drive is tested
    wxString directory = databaseM ? fr::getBenchmarkDirectoryFromPath(databaseM->getPath())
        : wxString();
    text_directory = new wxTextCtrl(panel, wxID_ANY, directory.empty()
        ? wxFileName::GetTempDir() + wxFileName::GetPathSeparator() : directory);
    wxBoxSizer* sizerDir = new wxBoxSizer(wxHORIZONTAL);
    sizerDir->Add(text_directory, 1, wxALIGN_CENTER_VERTICAL);
    sizerSettings->Add(createLine(labelDirectory, sizerDir, createHelpButton(panel,
        _("Directory"), { { wxString(), _("The directory of the temporary database, "
            "as the server sees it (a path on the server). Its disk is tested, so choose "
            "the directory of your databases.") } })), 0,
        wxEXPAND | wxBOTTOM, lineGap);

    // the intensities in one line, each with its duration
    wxStaticText* labelIntensity = createLabel(panel, _("Intensity:"));
    const wxString intensityHelp[] = {
        _("A first impression."),
        _("The usual choice."),
        _("More data and up to 32 users: a thorough look at a server.")
    };
    HelpItems intensityItems;
    wxBoxSizer* sizerIntensity = new wxBoxSizer(wxHORIZONTAL);
    for (int i = 0; i < 3; ++i)
    {
        const fr::BenchmarkIntensity intensity = fr::BenchmarkIntensity(i);
        radio_intensity[i] = new wxRadioButton(panel, wxID_ANY,
            fr::getBenchmarkIntensityName(intensity), wxDefaultPosition, wxDefaultSize,
            i == 0 ? wxRB_GROUP : 0);
        radio_intensity[i]->Bind(wxEVT_RADIOBUTTON, &BenchmarkDialog::OnDurationChanged, this);
        sizerIntensity->Add(radio_intensity[i], 0, wxALIGN_CENTER_VERTICAL);
        sizerIntensity->Add(createHintText(panel,
            fr::getBenchmarkIntensityDuration(intensity)), 0,
            wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(6));
        if (i < 2)
            sizerIntensity->AddSpacer(FromDIP(24));
        intensityItems.push_back({ wxString::Format("%s (%s)",
            fr::getBenchmarkIntensityName(intensity),
            fr::getBenchmarkIntensityDuration(intensity)), intensityHelp[i] });
    }
    intensityItems.push_back({ wxString(), _("Compare only runs of the same "
        "intensity.") });
    radio_intensity[int(fr::BenchmarkIntensity::Standard)]->SetValue(true);
    sizerSettings->Add(createLine(labelIntensity, sizerIntensity,
        createHelpButton(panel, _("Intensity"), intensityItems)), 0,
        wxEXPAND | wxBOTTOM, lineGap);

    // a saved report to compare the new run with: a single run is not
    // judged, the comparison shows what is faster or slower and why
    wxStaticText* labelEarlier = createLabel(panel, _("Compa&re with:"));
    wxString earlierFile;
    config().getValue(earlierFileKey, earlierFile);
    text_earlier = new wxTextCtrl(panel, wxID_ANY, earlierFile);
    text_earlier->SetHint(_("optional: a saved report"));
    wxBoxSizer* sizerEarlier = new wxBoxSizer(wxHORIZONTAL);
    sizerEarlier->Add(text_earlier, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, gap);
    sizerEarlier->Add(new wxButton(panel, ID_button_earlier_browse, _("Browse...")),
        0, wxALIGN_CENTER_VERTICAL);
    sizerSettings->Add(createLine(labelEarlier, sizerEarlier, createHelpButton(panel,
        _("Compare with an earlier report"), { { wxString(), _("A report saved from "
            "an earlier run (Markdown or HTML), for example before a change on the "
            "server, or from another server.") },
        { wxString(), _("The new report then shows what got faster or slower, what "
            "differs and possible reasons. Leave it empty for no comparison.") } })),
        0, wxEXPAND | wxBOTTOM, lineGap);

    // rarely needed settings, closed at first, below a line that sets them
    // apart; the "?" next to the heading explains all of them
    sizerSettings->Add(new wxStaticLine(panel), 0, wxEXPAND | wxTOP | wxBOTTOM,
        FromDIP(4));
    header_advanced = new wxCollapsibleHeaderCtrl(panel, ID_header_advanced,
        _("Advanced settings"));
    wxBoxSizer* sizerAdvancedLine = new wxBoxSizer(wxHORIZONTAL);
    sizerAdvancedLine->Add(header_advanced, 0, wxALIGN_CENTER_VERTICAL);
    sizerAdvancedLine->Add(createHelpButton(panel, _("Advanced settings"), {
        { _("Client library"), _("The Firebird client (fbclient) FlameRobin uses. To "
            "test this computer without an installation, it selects the Firebird version "
            "and needs the engine plugin next to it.") },
        { _("Page size"), _("Of the temporary database; only the storage and cache tests "
            "depend on it. Automatic uses the page size of the selected database, "
            "otherwise 8192 (Firebird 3: at most 16384). Comparing all page sizes "
            "repeats these tests for each one: a few minutes more.") },
        { _("Read the settings of the selected database"), _("Reads its header, "
            "MON$DATABASE and RDB$DATABASE, on its own server only. Nothing is "
            "written.") },
        { _("Test 50 and 250 users at once"), _("Two more rounds with that many "
            "connections at once: about half a minute more.") } }), 0,
        wxALIGN_CENTER_VERTICAL);
    sizerSettings->Add(sizerAdvancedLine, 0, wxEXPAND | wxBOTTOM, gap);
    panel_advanced = new wxPanel(panel);
    wxWindow* advanced = panel_advanced;
    wxBoxSizer* sizerAdvanced = new wxBoxSizer(wxVERTICAL);

    // the client library decides the Firebird version of a test of this
    // computer without login
    wxStaticText* labelLibrary = createLabel(advanced, _("Client library:"));
    text_clientlib = new wxTextCtrl(advanced, wxID_ANY, databaseM
        ? databaseM->getClientLibrary() : wxString());
    text_clientlib->SetHint(_("default client library"));
    wxBoxSizer* sizerLibrary = new wxBoxSizer(wxHORIZONTAL);
    sizerLibrary->Add(text_clientlib, 1, wxALIGN_CENTER_VERTICAL);
    sizerAdvanced->Add(createLine(labelLibrary, sizerLibrary, nullptr), 0,
        wxEXPAND | wxBOTTOM, lineGap);

    // the page size influences the storage and cache tests; default to the
    // one of the selected database when FlameRobin already knows it
    wxStaticText* labelPageSize = createLabel(advanced, _("Page size:"));
    int knownPageSize = databaseM && databaseM->isConnected()
        ? databaseM->getInfo().getPageSize() : 0;
    if (knownPageSize >= 4096)
        automaticPageSizeM = knownPageSize;
    wxString pageSizes[] = {
        wxString::Format(_("Automatic (%d)"), automaticPageSizeM),
        "4096", "8192", "16384", _("32768 (Firebird 4+)")
    };
    choice_pagesize = new wxChoice(advanced, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, 5, pageSizes);
    choice_pagesize->SetSelection(0);
    check_comparepages = new wxCheckBox(advanced, wxID_ANY, _("Compare all page sizes"));
    check_comparepages->Bind(wxEVT_CHECKBOX, &BenchmarkDialog::OnDurationChanged, this);
    wxBoxSizer* sizerPageSize = new wxBoxSizer(wxHORIZONTAL);
    sizerPageSize->Add(choice_pagesize, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(24));
    sizerPageSize->Add(check_comparepages, 0, wxALIGN_CENTER_VERTICAL);
    sizerAdvanced->Add(createLine(labelPageSize, sizerPageSize, nullptr), 0,
        wxEXPAND | wxBOTTOM, lineGap);

    wxStaticText* labelAlso = createLabel(advanced, _("Also:"));
    check_inspect = new wxCheckBox(advanced, wxID_ANY,
        _("Read the settings of the selected database (read-only)"));
    check_inspect->SetValue(false);
    check_manyusers = new wxCheckBox(advanced, wxID_ANY,
        _("Test 50 and 250 users at once (heavy load)"));
    check_manyusers->Bind(wxEVT_CHECKBOX, &BenchmarkDialog::OnDurationChanged, this);
    // one below the other: side by side they would be the widest line
    wxBoxSizer* sizerAlso = new wxBoxSizer(wxVERTICAL);
    sizerAlso->Add(check_inspect, 0, wxBOTTOM, gap);
    sizerAlso->Add(check_manyusers, 0);
    sizerAdvanced->Add(createLine(labelAlso, sizerAlso, nullptr, true), 0,
        wxEXPAND | wxBOTTOM, gap);
    panel_advanced->SetSizer(sizerAdvanced);
    panel_advanced->Hide();
    sizerSettings->Add(panel_advanced, 0, wxEXPAND);
    sizerMain->Add(sizerSettings, 0, wxEXPAND | wxBOTTOM, gap);

    // progress
    wxBoxSizer* sizerStatus = new wxBoxSizer(wxHORIZONTAL);
    label_status = new wxStaticText(panel, wxID_ANY, wxEmptyString);
    label_elapsed = new wxStaticText(panel, wxID_ANY, wxEmptyString);
    sizerStatus->Add(label_status, 1, wxALIGN_CENTER_VERTICAL);
    sizerStatus->Add(label_elapsed, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(12));
    sizerMain->Add(sizerStatus, 0, wxEXPAND | wxBOTTOM, FromDIP(4));
    gauge_progress = new wxGauge(panel, wxID_ANY, 100, wxDefaultPosition,
        wxSize(-1, FromDIP(14)));
    sizerMain->Add(gauge_progress, 0, wxEXPAND | wxBOTTOM, gap);

    // results
    notebook_results = new wxNotebook(panel, wxID_ANY);

    text_log = new wxTextCtrl(notebook_results, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
    notebook_results->AddPage(text_log, _("Progress"));

    // the report, with what can be done with it above it
    wxPanel* pageReport = new wxPanel(notebook_results);
    wxBoxSizer* sizerReport = new wxBoxSizer(wxVERTICAL);
    check_private = new wxCheckBox(pageReport, ID_check_private,
        _("&Hide private information"));
    check_private->SetToolTip(_("Removes computer and host names, IP addresses, "
        "user names, paths and service names from the report, the copy and the "
        "saved file, so that it can be shared in public. Hardware, versions and "
        "all measurements stay."));
    bool hidePrivate = false;
    config().getValue(hidePrivateKey, hidePrivate);
    check_private->SetValue(hidePrivate);
    label_report_status = createHintText(pageReport, wxEmptyString, true);
    // the answer to a copy is never cut
    label_report_status->SetMinSize(wxSize(
        label_report_status->GetTextExtent(getCopiedMessage()).x, -1));
    button_copy = new wxButton(pageReport, ID_button_copy, _("Copy as &Markdown"));
    button_save = new wxButton(pageReport, ID_button_save, _("Sa&ve report..."));
    button_browser = new wxButton(pageReport, ID_button_browser, _("Open in &browser"));
    button_browser->SetToolTip(_("Shows the report in the web browser of this computer. "
        "The page is written to the temporary directory; without \"Hide private "
        "information\" it contains the names of computers and paths."));
    wxBoxSizer* sizerActions = new wxBoxSizer(wxHORIZONTAL);
    sizerActions->Add(check_private, 0, wxALIGN_CENTER_VERTICAL);
    sizerActions->Add(label_report_status, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT,
        FromDIP(12));
    sizerActions->Add(button_copy, 0, wxRIGHT, gap);
    sizerActions->Add(button_save, 0, wxRIGHT, gap);
    sizerActions->Add(button_browser, 0);
    sizerReport->Add(sizerActions, 0, wxEXPAND | wxALL, gap);
    sizerReport->Add(new wxStaticLine(pageReport), 0, wxEXPAND);

#if wxUSE_WEBVIEW
    wxString backend;
    webview_report = createReportView(pageReport, backend);
    if (webview_report)
    {
        reportPageM.reset(new BenchmarkReportPage(webview_report,
            backend == wxWebViewBackendIE));
        if (backend == wxWebViewBackendIE)
        {
            reportViewM = "Internet Explorer";
            wxStaticText* hint = new wxStaticText(pageReport, wxID_ANY,
                _("The Microsoft Edge WebView2 Runtime is not installed on this "
                  "computer, so the report is shown with Internet Explorer in a "
                  "simpler form. \"Open in browser\" shows it in your web browser."));
            hint->Wrap(FromDIP(600));
            sizerReport->Add(hint, 0, wxEXPAND | wxALL, gap);
        }
        else
            reportViewM = wxWebView::GetBackendVersionInfo(backend).GetVersionString();
        sizerReport->Add(webview_report, 1, wxEXPAND);
    }
#endif
    if (!webview_report)
    {
        reportViewM = _("text, no web view is available");
        text_report = new wxTextCtrl(pageReport, wxID_ANY, wxEmptyString,
            wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
        sizerReport->Add(text_report, 1, wxEXPAND);
    }
    pageReport->SetSizer(sizerReport);
    notebook_results->AddPage(pageReport, _("Report"));

    sizerMain->Add(notebook_results, 1, wxEXPAND);

    // buttons
    button_start = new wxButton(panel, ID_button_start, _("&Start"));
    // the main action; not the default button, so that Enter in a field
    // does not start a test
    button_start->SetFont(button_start->GetFont().Bold());
    button_cancel = new wxButton(panel, ID_button_cancel, _("C&ancel test"));
    button_compare = new wxButton(panel, ID_button_compare, _("Compare t&wo reports..."));
    button_compare->SetToolTip(_("Compares two saved reports, without a new test."));
    button_close = new wxButton(panel, wxID_CANCEL, _("&Close"));

    wxBoxSizer* sizerButtons = new wxBoxSizer(wxHORIZONTAL);
    sizerButtons->Add(button_start, 0, wxRIGHT, gap);
    sizerButtons->Add(button_cancel, 0, wxRIGHT, gap);
    sizerButtons->AddStretchSpacer(1);
    sizerButtons->Add(button_compare, 0, wxRIGHT, gap);
    sizerButtons->Add(button_close, 0);

    layoutSizers(sizerMain, sizerButtons, true);
    // narrow and low enough for a small screen, the report gets the rest
    const wxRect area = wxDisplay(this).GetClientArea();
    SetSize(std::max(GetMinSize().x, std::min(FromDIP(900), area.width)),
        std::max(GetMinSize().y, std::min(FromDIP(560), area.height)));

    // the first target: the server of the selected database, without a
    // registration this computer
    wxCommandEvent dummy;
    OnTargetChanged(dummy);
    label_status->SetLabel(getReadyHint());
}

fr::BenchmarkTarget BenchmarkDialog::getTarget() const
{
    return fr::getBenchmarkTarget(databaseM != nullptr, choice_target->GetSelection());
}

fr::BenchmarkIntensity BenchmarkDialog::getIntensity() const
{
    for (int i = 0; i < 3; ++i)
    {
        if (radio_intensity[i]->GetValue())
            return fr::BenchmarkIntensity(i);
    }
    return fr::BenchmarkIntensity::Standard;
}

void BenchmarkDialog::updateControls()
{
    bool idle = !runningM;
    const fr::BenchmarkTargetDetails details = fr::getBenchmarkTargetDetails(getTarget());
    bool ownLibrary = choice_install->GetSelection() == int(installationsM.size());
    choice_target->Enable(idle);
    // the host, the login and the installation
    book_target->Enable(idle);
    // the installation brings its own client library
    text_clientlib->Enable(idle && (!details.installation || ownLibrary));
    text_directory->Enable(idle);
    choice_pagesize->Enable(idle);
    check_comparepages->Enable(idle);
    check_inspect->Enable(idle && details.readDatabase && databaseM);
    check_manyusers->Enable(idle);
    text_earlier->Enable(idle);
    for (wxRadioButton* radio : radio_intensity)
        radio->Enable(idle);
    button_start->Enable(idle);
    button_cancel->Enable(runningM && !runnerM->isCancelRequested());
    button_compare->Enable(idle);
    button_copy->Enable(idle && (hasReportM || comparisonM));
    button_save->Enable(idle && (hasReportM || comparisonM));
    button_browser->Enable(idle && (hasReportM || comparisonM));
}

wxString BenchmarkDialog::getReadyHint() const
{
    return wxString::Format(_("Ready. The test takes %s."), fr::getBenchmarkDurationHint(
        getIntensity(), check_comparepages->GetValue(), check_manyusers->GetValue()));
}

void BenchmarkDialog::OnDurationChanged(wxCommandEvent& event)
{
    event.Skip();
    // after a run the settings are hidden and the status is the result
    if (!runningM && settingsShownM)
        label_status->SetLabel(getReadyHint());
}

void BenchmarkDialog::setReportStatus(const wxString& message)
{
    label_report_status->SetLabel(message);
    // the whole message where it is cut
    label_report_status->SetToolTip(message);
}

void BenchmarkDialog::OnAdvancedToggled(wxCommandEvent& WXUNUSED(event))
{
    layoutAdvanced();
}

void BenchmarkDialog::showAdvanced(bool show)
{
    if (header_advanced->IsCollapsed() != show)
        return;
    header_advanced->SetCollapsed(!show);
    layoutAdvanced();
}

void BenchmarkDialog::layoutAdvanced()
{
    panel_advanced->Show(!header_advanced->IsCollapsed());
    // the open settings needs more room: the dialog does not get lower than the
    // settings and the progress need, as far as the screen allows
    wxPanel* panel = getControlsPanel();
    const wxRect area = wxDisplay(this).GetClientArea();
    const int needed = std::min(ClientToWindowSize(panel->GetSizer()->CalcMin()).y,
        area.height);
    SetMinSize(wxSize(GetMinSize().x, needed));
    if (GetSize().y < needed)
    {
        wxRect rect = GetRect();
        rect.height = needed;
        rect.y = std::max(area.y, std::min(rect.y, area.GetBottom() + 1 - needed));
        SetSize(rect);
    }
    panel->Layout();
}

void BenchmarkDialog::OnTargetChanged(wxCommandEvent& WXUNUSED(event))
{
    const BenchmarkTarget target = getTarget();
    book_target->ChangeSelection(size_t(target));
    label_target_hint->SetLabel(getTargetHint(target));
    // the client library is in the advanced settings
    if (target == BenchmarkTarget::ThisComputer
        && choice_install->GetSelection() == int(installationsM.size()))
    {
        showAdvanced(true);
    }
    if (target == BenchmarkTarget::OtherServer && text_host->IsEmpty() && databaseM)
    {
        ServerPtr server = databaseM->getServer();
        if (server)
        {
            text_host->SetValue(server->getHostname());
            text_port->SetValue(server->getPort());
        }
    }
    updateControls();
}

bool BenchmarkDialog::buildParams(fr::BenchmarkConnectionParams& params)
{
    const BenchmarkTarget target = getTarget();
    wxString directory = text_directory->GetValue().Trim().Trim(false);
    if (directory.empty())
    {
        wxMessageBox(_("Please enter a directory in which the server can "
            "create the temporary benchmark database, e.g. the directory "
            "of your databases."), _("Performance Benchmark & Diagnosis"),
            wxOK | wxICON_INFORMATION, this);
        text_directory->SetFocus();
        return false;
    }
    params.serverDirectory = directory;
    const int pageSizeValues[] = { automaticPageSizeM, 4096, 8192, 16384, 32768 };
    int selection = choice_pagesize->GetSelection();
    params.pageSize = pageSizeValues[selection >= 0 && selection < 5 ? selection : 0];
    params.comparePageSizes = check_comparepages->GetValue();
    params.manyUsers = check_manyusers->GetValue();
    config().setValue(earlierFileKey, text_earlier->GetValue().Trim().Trim(false));

    ServerPtr server = databaseM ? databaseM->getServer() : ServerPtr();
    wxString serverPart;
    switch (target)
    {
        case BenchmarkTarget::Registered:
            if (server)
            {
                serverPart = server->getConnectionString();
                params.host = wx2std(server->getHostname());
            }
            params.targetDescription = wxString::Format(_("Server of %s (%s)"),
                databaseM->getName_(),
                serverPart.empty() ? _("local") : serverPart);
            break;
        case BenchmarkTarget::ThisComputer:
            params.forceEmbedded = true;
            params.targetDescription = _("This computer, without login");
            break;
        case BenchmarkTarget::OtherServer:
        {
            wxString host = text_host->GetValue().Trim().Trim(false);
            if (host.empty())
            {
                wxMessageBox(_("Please enter the host name of the Firebird "
                    "server."), _("Performance Benchmark & Diagnosis"),
                    wxOK | wxICON_INFORMATION, this);
                text_host->SetFocus();
                return false;
            }
            serverPart = Server::makeConnectionString(host,
                text_port->GetValue().Trim().Trim(false));
            params.host = wx2std(host);
            params.targetDescription = wxString::Format(_("Firebird server %s"),
                serverPart);
            break;
        }
    }
    params.targetDescription += wxString::Format(_(", directory %s"), directory);
    if (!serverPart.empty())
        params.connectionPrefix = wx2std(serverPart + ":");

    fillCredentials(params, target == BenchmarkTarget::Registered);
    if (target == BenchmarkTarget::ThisComputer)
    {
        // the engine inside FlameRobin checks no password; the database
        // belongs to the user of this computer
        params.username.clear();
        params.password.clear();
        params.role.clear();
        params.cryptKeyData.clear();
        const int selection = choice_install->GetSelection();
        if (selection >= 0 && selection < int(installationsM.size()))
            params.clientLibrary = installationsM[selection].clientLibrary;
    }
    // the settings of the selected database are shown with the tested
    // server, which only makes sense when it is the same server
    if (target == BenchmarkTarget::Registered && databaseM && check_inspect->GetValue())
        params.inspectConnectionString = wx2std(databaseM->getConnectionString());
    return true;
}

void BenchmarkDialog::fillCredentials(fr::BenchmarkConnectionParams& params,
    bool fromRegistration)
{
    // the credentials the user entered; the settings of the registration
    // and its login (resolveLogin()) are only sent to the server of the
    // registration
    params.clientLibrary = wx2std(text_clientlib->GetValue().Trim().Trim(false));
    wxTextCtrl* user = fromRegistration ? text_user_registered : text_user;
    if (!user->IsEmpty())
    {
        params.username = wx2std(user->GetValue());
        params.password = wx2std((fromRegistration ? text_password_registered
            : text_password)->GetValue());
    }
    if (!databaseM || !fromRegistration)
        return;
    params.role = wx2std(databaseM->getRole());
    params.charset = wx2std(databaseM->getConnectionCharset());
    params.cryptKeyData = wx2std(databaseM->getCryptKeyData());
}

bool BenchmarkDialog::resolveLogin(fr::BenchmarkConnectionParams& params)
{
    encryptedLoginM = false;
    if (!databaseM)
        return true;
    DatabaseAuthenticationMode& mode = databaseM->getAuthenticationMode();
    fr::IDatabasePtr dal = databaseM->getDALDatabase();
    fr::BenchmarkLoginState state;
    state.userEntered = !text_user_registered->IsEmpty();
    state.trustedUser = mode.getIgnoreUsernamePassword();
    state.connected = databaseM->isConnected() && dal;
    state.alwaysAsk = mode.getAlwaysAskForPassword();
    state.encryptedPassword = mode.getUseEncryptedPassword();
    state.hasStoredPassword = !databaseM->getRawPassword().empty();
    wxString username = databaseM->getUsername();
    wxString password;
    // may ask for the master password, which neither a login entered in
    // this dialog nor trusted authentication needs
    if (!state.userEntered && !state.trustedUser)
        password = databaseM->getDecryptedPassword();
    state.hasUsername = !username.empty();
    state.hasPassword = !password.empty();

    switch (fr::getBenchmarkLoginAction(state))
    {
        case fr::BenchmarkLoginAction::None:
            return true;
        case fr::BenchmarkLoginAction::Abort:
            return false;
        case fr::BenchmarkLoginAction::UseStored:
            if (state.connected)
            {
                params.username = dal->getUsername();
                params.password = dal->getUserPassword();
            }
            else
            {
                params.username = wx2std(username);
                params.password = wx2std(password);
                encryptedLoginM = state.encryptedPassword;
            }
            return true;
        case fr::BenchmarkLoginAction::Ask:
            break;
    }
    // the benchmark creates a database, so another user may be needed
    if (!getConnectionCredentials(this, wxString::Format(_("Login for the server "
        "of %s.\nIt is only used for this test and is not saved."),
        databaseM->getName_()), username, password,
        UsernamePasswordDialog::AllowOtherUsername | UsernamePasswordDialog::AllowTrustedUser))
    {
        return false;
    }
    params.username = wx2std(username);
    params.password = wx2std(password);
    return true;
}

void BenchmarkDialog::addPendingDatabase(const wxString& entry)
{
    wxArrayString entries;
    config().getValue(pendingDatabasesKey, entries);
    if (entries.Index(entry) == wxNOT_FOUND)
        entries.Add(entry);
    config().setValue(pendingDatabasesKey, entries);
}

void BenchmarkDialog::removePendingDatabase(const wxString& entry)
{
    wxArrayString entries;
    config().getValue(pendingDatabasesKey, entries);
    entries.Remove(entry);
    config().setValue(pendingDatabasesKey, entries);
}

void BenchmarkDialog::dropPendingDatabases()
{
    wxArrayString entries;
    if (!config().getValue(pendingDatabasesKey, entries))
        return;
    wxArrayString otherRegistrations;
    std::vector<wxString> otherEntries;
    for (const wxString& entry : entries)
    {
        std::optional<fr::BenchmarkPendingDatabase> parsed =
            fr::BenchmarkPendingDatabase::parse(entry);
        if (!parsed)
        {
            removePendingDatabase(entry);
            continue;
        }
        const fr::BenchmarkPendingDatabase& pending = *parsed;
        const bool ownServer = !pending.registrationId.empty()
            && pending.registrationId == registrationId()
            && isOnRegistrationServer(pending.connectionString);
        if (!pending.embedded && !ownServer)
        {
            otherRegistrations.Add(pending.connectionString);
            otherEntries.push_back(entry);
            continue;
        }
        if (wxMessageBox(wxString::Format(_("An earlier benchmark run was "
            "interrupted and may have left its temporary database behind:"
            "\n\n%s\n\nDrop it now? Only a database with this "
            "FR_BENCHMARK_* name is dropped."), pending.connectionString),
            _("Performance Benchmark & Diagnosis"), wxYES_NO | wxICON_QUESTION, this) != wxYES)
        {
            continue;
        }
        fr::BenchmarkConnectionParams params;
        fillCredentials(params, !pending.embedded);
        // without a login the entry stays for the next time
        if (!pending.embedded && !resolveLogin(params))
            continue;
        params.forceEmbedded = pending.embedded;
        params.clientLibrary = wx2std(pending.clientLibrary);
        if (pending.embedded)
        {
            // the engine inside FlameRobin needs no login
            params.username.clear();
            params.password.clear();
        }
        try
        {
            fr::BenchmarkRunner::dropLeftoverDatabase(params,
                wx2std(pending.connectionString));
            removePendingDatabase(entry);
            wxMessageBox(_("The temporary database was dropped."),
                _("Performance Benchmark & Diagnosis"), wxOK | wxICON_INFORMATION, this);
        }
        catch (const std::exception& e)
        {
            // most likely the file does not exist (any more)
            if (wxMessageBox(wxString::Format(_("The database could not be "
                "opened or dropped:\n\n%s\n\nIf the file still exists, "
                "please delete it manually. Remove it from this list?"),
                wxString::FromUTF8(e.what())), _("Performance Benchmark & Diagnosis"),
                wxYES_NO | wxICON_WARNING, this) == wxYES)
            {
                removePendingDatabase(entry);
            }
        }
    }

    // the credentials of this registration are not sent to the servers of
    // benchmarks that were started from other registrations or for another
    // server
    if (!otherRegistrations.empty() && wxMessageBox(wxString::Format(
        _("Earlier benchmark runs on other servers may have left these "
          "temporary databases behind:"
          "\n\n%s\n\nTo drop them, open the benchmark test of the "
          "database of that server, or delete the files manually."
          "\n\nRemove them from this list?"),
        wxJoin(otherRegistrations, '\n', '\0')), _("Performance Benchmark & Diagnosis"),
        wxYES_NO | wxNO_DEFAULT | wxICON_INFORMATION, this) == wxYES)
    {
        for (const wxString& entry : otherEntries)
            removePendingDatabase(entry);
    }
}

void BenchmarkDialog::OnStart(wxCommandEvent& WXUNUSED(event))
{
    // after a run the button shows the settings again for a new test
    if (!settingsShownM)
    {
        showSettings(true);
        return;
    }
    fr::BenchmarkConnectionParams params;
    if (!buildParams(params))
        return;

    const bool registered = getTarget() == BenchmarkTarget::Registered;
    if (registered && databaseM && databaseM->isProductionEnvironment())
    {
        if (wxMessageBox(_("The selected database is marked as production "
            "environment. The benchmark puts heavy load on its server for a "
            "few minutes (the database itself is not touched). Continue?"),
            _("Performance Benchmark & Diagnosis"), wxYES_NO | wxICON_QUESTION, this) != wxYES)
        {
            return;
        }
    }
    // asked for only now, after every other check
    if (registered && !resolveLogin(params))
        return;

    const fr::BenchmarkIntensity intensity = getIntensity();
    runnerM = std::make_unique<fr::BenchmarkRunner>(params, intensity);
    pendingEntriesM.clear();
    for (const auto& connectionString : runnerM->getAllConnectionStrings())
    {
        fr::BenchmarkPendingDatabase pending;
        pending.embedded = params.forceEmbedded;
        // only a run on the server of the registration may use its
        // credentials for the cleanup
        pending.registrationId = registered ? registrationId() : wxString();
        pending.clientLibrary = toWxString(params.clientLibrary);
        pending.connectionString = toWxString(connectionString);
        pendingEntriesM.push_back(pending.toString());
        addPendingDatabase(pendingEntriesM.back());
    }
    runningM = true;
    hasReportM = false;
    comparisonM.reset();
    closeRequestedM = false;
    loggedResultsM.clear();
    lastMessageM.clear();
    text_log->Clear();
    showReport(fr::BenchmarkReport());
    gauge_progress->SetValue(0);
    notebook_results->SetSelection(0);
    // the settings cannot be changed during the run; the progress gets
    // their room
    showAdvanced(false);
    startTimeM = std::chrono::steady_clock::now();
    label_elapsed->SetLabel(formatElapsed());
    timerM.Start(1000);
    appendLog(wxString::Format(_("Target: %s"), params.targetDescription));
    appendLog(wxString::Format(_("Intensity: %s, page size %d"),
        fr::getBenchmarkIntensityName(intensity), params.pageSize));
    appendLog(wxString::Format(_("Report view: %s"), reportViewM));
    updateControls();

    fr::BenchmarkRunner* runner = runnerM.get();
    workerM = std::thread([this, runner]()
    {
        fr::BenchmarkReport report;
        bool aborted = false;
        try
        {
            report = runner->run([this](const fr::BenchmarkProgress& progress)
            {
                CallAfter([this, progress]() { onProgress(progress); });
            });
        }
        catch (const std::exception& e)
        {
            // run() reports its errors in the report; this only guards the
            // thread boundary (e.g. against std::bad_alloc)
            aborted = true;
            report.errorMessage = wxString::FromUTF8(e.what());
        }
        CallAfter([this, report, aborted]() { onFinished(report, aborted); });
    });
}

void BenchmarkDialog::OnCancel(wxCommandEvent& WXUNUSED(event))
{
    if (!runningM)
        return;
    runnerM->requestCancel();
    label_status->SetLabel(_("Cancelling, the temporary database is being dropped..."));
    appendLog(_("Cancel requested."));
    updateControls();
}

void BenchmarkDialog::requestClose()
{
    closeRequestedM = true;
    wxCommandEvent dummy;
    OnCancel(dummy);
}

void BenchmarkDialog::OnCloseButton(wxCommandEvent& event)
{
    if (runningM)
        requestClose();
    else
        event.Skip();
}

void BenchmarkDialog::OnClose(wxCloseEvent& event)
{
    if (runningM && event.CanVeto())
    {
        event.Veto();
        requestClose();
        return;
    }
    event.Skip();
}

wxString BenchmarkDialog::formatElapsed() const
{
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - startTimeM).count();
    return wxString::Format(_("Elapsed: %02d:%02d"), int(seconds / 60),
        int(seconds % 60));
}

void BenchmarkDialog::OnTimer(wxTimerEvent& WXUNUSED(event))
{
    label_elapsed->SetLabel(formatElapsed());
}

void BenchmarkDialog::appendLog(const wxString& line)
{
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - startTimeM).count();
    text_log->AppendText(wxString::Format("[%02d:%02d] ", int(seconds / 60),
        int(seconds % 60)) + line + "\n");
}

void BenchmarkDialog::onProgress(const fr::BenchmarkProgress& progress)
{
    if (progress.cleaningUp)
        label_status->SetLabel(progress.message);
    else
    {
        label_status->SetLabel(wxString::Format(_("Step %d of %d: %s"),
            progress.step, progress.stepCount, progress.message));
    }
    if (progress.stepCount > 0)
        gauge_progress->SetValue(progress.step * 100 / progress.stepCount);
    if (progress.message != lastMessageM)
    {
        lastMessageM = progress.message;
        appendLog(progress.message);
    }

    // compact intermediate results: every measurement once, when it appears
    for (const auto& r : progress.results)
    {
        wxString key = fr::getBenchmarkCategoryName(r.category) + "|" + r.test;
        if (loggedResultsM.insert(key).second)
        {
            appendLog("    " + fr::getBenchmarkCategoryName(r.category) + " - "
                + r.test + ": " + r.value + "  [" + r.machine + "]");
        }
    }
}

void BenchmarkDialog::onFinished(const fr::BenchmarkReport& report,
    bool aborted)
{
    if (workerM.joinable())
        workerM.join();
    // the runner holds the login of this run, which is not kept
    runnerM.reset();
    timerM.Stop();
    runningM = false;
    reportM = report;
    hasReportM = true;
    label_elapsed->SetLabel(formatElapsed());
    gauge_progress->SetValue(100);
    // like connectDatabase(): a master password that did not work is asked
    // for again at the next start
    if (encryptedLoginM)
    {
        if (report.benchmarkDatabaseCreated)
            MasterPassword::setVerified(true);
        else if (!report.errorMessage.empty() && !MasterPassword::isVerified())
            MasterPassword::reset();
    }

    // an earlier report that cannot be compared, e.g. of an older version
    const wxString earlierFile = text_earlier->GetValue().Trim().Trim(false);
    wxString earlierProblem;
    if (!earlierFile.empty())
    {
        earlierProblem = wxFileExists(earlierFile)
            ? fr::getBenchmarkSnapshotProblem(readReportFile(earlierFile))
            : wxString(_("The file does not exist."));
        if (!earlierProblem.empty())
            appendLog(earlierFile + ": " + earlierProblem);
    }

    if (report.cancelled)
        label_status->SetLabel(_("Cancelled. The results are incomplete."));
    else if (!report.errorMessage.empty())
        label_status->SetLabel(_("Failed. See the log for details."));
    else if (!earlierFile.empty() && !earlierProblem.empty())
    {
        label_status->SetLabel(_("Finished. The earlier report cannot be compared, see "
            "the log."));
    }
    else if (!earlierFile.empty())
    {
        label_status->SetLabel(_("Finished. The overview compares this run with the "
            "earlier report."));
    }
    else
    {
        label_status->SetLabel(_("Finished. Save the report to compare it with later "
            "runs or other servers."));
    }

    if (!report.errorMessage.empty())
    {
        appendLog(_("Error:") + " " + report.errorMessage);
        if (getTarget() == BenchmarkTarget::ThisComputer)
        {
            appendLog(_("Hint: a test of this computer needs a Firebird installation "
                "with the engine plugin (engine12, engine13 or engine14 for Firebird "
                "3, 4/5 or 6 in its plugins folder), and the directory must be "
                "writable for your user. If FlameRobin already used this client "
                "library for another connection, restart FlameRobin and run the test "
                "first."));
        }
    }
    // forget every temporary database that is gone; keep the others so
    // that the next start offers to drop them
    for (size_t i = 0; i < pendingEntriesM.size() && !aborted; ++i)
    {
        auto pending = fr::BenchmarkPendingDatabase::parse(pendingEntriesM[i]);
        bool gone = i == 0
            ? !report.benchmarkDatabaseCreated || report.benchmarkDatabaseDropped
            : pending && std::find(report.undroppedDatabases.begin(),
                report.undroppedDatabases.end(), pending->connectionString)
                == report.undroppedDatabases.end();
        if (gone)
            removePendingDatabase(pendingEntriesM[i]);
    }
    if (aborted)
    {
        appendLog(_("WARNING: the benchmark stopped unexpectedly; the "
            "temporary databases may still exist. The next start of this "
            "dialog offers to drop them."));
    }
    else if (!report.benchmarkDatabaseCreated)
        appendLog(_("No temporary database was created."));
    else if (report.benchmarkDatabaseDropped)
    {
        appendLog(wxString::Format(_("Temporary database %s dropped."),
            report.benchmarkDatabasePath));
    }
    else
    {
        appendLog(wxString::Format(_("WARNING: the temporary database %s "
            "could not be dropped. The next start of this dialog offers to "
            "drop it again; otherwise delete the file manually."),
            report.benchmarkDatabasePath));
    }
    appendLog(wxString::Format(_("Done after %.0f seconds."),
        report.durationSeconds));

    showReport(displayedReport());
    showSettings(false);
    notebook_results->SetSelection(1);  // the report
    updateControls();

    if (closeRequestedM)
        Close();
}

void BenchmarkDialog::showSettings(bool show)
{
    settingsShownM = show;
    if (show && comparisonM)
    {
        comparisonM.reset();
        showReport(hasReportM ? displayedReport() : fr::BenchmarkReport());
    }
    sizerSettingsM->ShowItems(show);
    button_start->SetLabel(show ? _("&Start") : _("&New test"));
    if (show)
    {
        // ready for the next run
        label_status->SetLabel(getReadyHint());
        label_elapsed->SetLabel(wxEmptyString);
        gauge_progress->SetValue(0);
        // the hint of the advanced settings is only shown next to the
        // closed pane
        layoutAdvanced();
    }
    else
        getControlsPanel()->Layout();
}

void BenchmarkDialog::showReport(const fr::BenchmarkReport& report)
{
    // a message such as "copied" was about the report shown before
    setReportStatus(wxEmptyString);
    // an empty report (before a run) shows an empty page
    const bool hasData = report.metrics.settings.connectAttempts > 0;
#if wxUSE_WEBVIEW
    if (webview_report)
    {
        reportPageM->show(hasData
            ? report.toHtml(FRStyleManager::isEffectivelyDark() ? "dark" : "light")
            : wxString("<html><body></body></html>"));
        return;
    }
#endif
    text_report->SetValue(hasData ? report.toMarkdown() : wxString());
    text_report->ShowPosition(0);
}

void BenchmarkDialog::OnCopyMarkdown(wxCommandEvent& WXUNUSED(event))
{
    if (!hasReportM && !comparisonM)
        return;
    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxTextDataObject(comparisonM
            ? displayedComparison().toMarkdown() : displayedReport().toMarkdown()));
        wxTheClipboard->Close();
        setReportStatus(getCopiedMessage());
    }
}

void BenchmarkDialog::OnSaveReport(wxCommandEvent& WXUNUSED(event))
{
    if (!hasReportM && !comparisonM)
        return;
    wxFileDialog fd(this, comparisonM ? _("Save comparison") : _("Save benchmark report"),
        "", comparisonM ? "flamerobin-benchmark-comparison.html" : "flamerobin-benchmark.html",
        _("HTML report (*.html)|*.html|Markdown (*.md)|*.md"),
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (fd.ShowModal() != wxID_OK)
        return;
    const bool markdown = fd.GetFilterIndex() == 1
        || fd.GetPath().Lower().EndsWith(".md");
    wxFFile file(fd.GetPath(), "w");
    if (!file.IsOpened() || !file.Write(displayedText(markdown), wxConvUTF8))
    {
        wxMessageBox(_("The report could not be saved."), _("Performance Benchmark & Diagnosis"),
            wxOK | wxICON_ERROR, this);
    }
}

wxString BenchmarkDialog::displayedText(bool markdown) const
{
    // the saved page follows the theme of the browser that opens it
    if (comparisonM)
    {
        const fr::BenchmarkComparison comparison = displayedComparison();
        return markdown ? comparison.toMarkdown() : comparison.toHtml();
    }
    const fr::BenchmarkReport report = displayedReport();
    return markdown ? report.toMarkdown() : report.toHtml();
}

void BenchmarkDialog::OnOpenInBrowser(wxCommandEvent& WXUNUSED(event))
{
    if (!hasReportM && !comparisonM)
        return;
    // the browser may still read the page after the dialog is closed, so
    // the pages are only removed a day later
    const wxString tempDir = wxFileName::GetTempDir();
    {
        wxLogNull noErrors;
        wxArrayString oldFiles;
        wxDir::GetAllFiles(tempDir, &oldFiles, browserFilePrefix + "*.html", wxDIR_FILES);
        const wxDateTime yesterday = wxDateTime::Now() - wxTimeSpan::Day();
        for (const wxString& oldFile : oldFiles)
        {
            const wxDateTime modified = wxFileName(oldFile).GetModificationTime();
            if (modified.IsValid() && modified < yesterday)
                wxRemoveFile(oldFile);
        }
    }

    wxFileName path(tempDir, browserFilePrefix
        + wxDateTime::Now().Format("%Y%m%d-%H%M%S-")
        + wxString::Format("%lu.html", wxGetProcessId()));
    wxFFile file(path.GetFullPath(), "w");
    if (!file.IsOpened() || !file.Write(displayedText(false), wxConvUTF8) || !file.Close()
        || !wxLaunchDefaultBrowser(wxFileName::FileNameToURL(path)))
    {
        wxMessageBox(_("The report could not be opened in the web browser. Please save "
            "it and open the file."), _("Performance Benchmark & Diagnosis"),
            wxOK | wxICON_ERROR, this);
    }
}

wxString BenchmarkDialog::registrationId() const
{
    return databaseM ? databaseM->getId() : wxString();
}

bool BenchmarkDialog::isOnRegistrationServer(const wxString& connectionString) const
{
    ServerPtr server = databaseM ? databaseM->getServer() : ServerPtr();
    const wxString serverPart = server ? server->getConnectionString() : wxString();
    if (!serverPart.empty())
        return connectionString.StartsWith(serverPart + ":");
    // a local path: no server in front of it, at most a drive letter
    const wxString before = connectionString.BeforeFirst(':');
    return before == connectionString || before.length() <= 1;
}

std::optional<fr::BenchmarkSnapshot> BenchmarkDialog::readSnapshot(const wxString& path,
    bool showErrors)
{
    const fr::BenchmarkSnapshotResult result = readReportFile(path);
    if (result.ok())
        return result.snapshot;
    if (showErrors)
    {
        // older reports are refused with a message, never compared wrongly
        wxMessageBox(path + "\n\n" + fr::getBenchmarkSnapshotProblem(result),
            _("Performance Benchmark & Diagnosis"), wxOK | wxICON_INFORMATION, this);
    }
    return std::nullopt;
}

fr::BenchmarkReport BenchmarkDialog::displayedReport() const
{
    // the earlier report and the private information only change what is
    // shown, copied and saved; the measured report stays as it is
    fr::BenchmarkReport report = reportM;
    const wxString earlierFile = text_earlier->GetValue().Trim().Trim(false);
    if (!earlierFile.empty() && wxFileExists(earlierFile))
    {
        // a report that cannot be compared is named in the log, see onFinished()
        const fr::BenchmarkSnapshotResult earlier = readReportFile(earlierFile);
        if (earlier.ok())
            report.earlier = earlier.snapshot;
    }
    if (check_private->GetValue())
        report = report.anonymized();
    return report;
}

fr::BenchmarkComparison BenchmarkDialog::displayedComparison() const
{
    if (check_private->GetValue())
        return fr::anonymizeBenchmarkComparison(*comparisonM);
    return *comparisonM;
}

void BenchmarkDialog::showComparison()
{
    setReportStatus(wxEmptyString);
#if wxUSE_WEBVIEW
    if (webview_report)
    {
        reportPageM->show(displayedComparison().toHtml(
            FRStyleManager::isEffectivelyDark() ? "dark" : "light"));
        return;
    }
#endif
    text_report->SetValue(displayedComparison().toMarkdown());
    text_report->ShowPosition(0);
}

void BenchmarkDialog::OnPrivateChanged(wxCommandEvent& WXUNUSED(event))
{
    config().setValue(hidePrivateKey, check_private->GetValue());
    if (comparisonM)
        showComparison();
    else if (hasReportM)
        showReport(displayedReport());
}

void BenchmarkDialog::OnBrowseEarlier(wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog fd(this, _("Select an earlier report"), "", "",
        _("Benchmark reports (*.md;*.html)|*.md;*.html;*.htm|All files (*.*)|*.*"),
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (fd.ShowModal() != wxID_OK || !readSnapshot(fd.GetPath(), true))
        return;
    text_earlier->SetValue(fd.GetPath());
    config().setValue(earlierFileKey, fd.GetPath());
    if (hasReportM && !comparisonM)
        showReport(displayedReport());
}

void BenchmarkDialog::OnCompareReports(wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog fd(this, _("Select two saved reports"), "", "",
        _("Benchmark reports (*.md;*.html)|*.md;*.html;*.htm|All files (*.*)|*.*"),
        wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
    if (fd.ShowModal() != wxID_OK)
        return;
    wxArrayString paths;
    fd.GetPaths(paths);
    if (paths.size() != 2)
    {
        wxMessageBox(_("Please select exactly two reports (hold Ctrl while you "
            "click the second one)."), _("Performance Benchmark & Diagnosis"),
            wxOK | wxICON_INFORMATION, this);
        return;
    }
    std::optional<fr::BenchmarkSnapshot> first = readSnapshot(paths[0], true);
    std::optional<fr::BenchmarkSnapshot> second = first ? readSnapshot(paths[1], true)
        : std::nullopt;
    if (!first || !second)
        return;
    // the run with the later date is the newer one
    if (first->date > second->date)
        std::swap(first, second);
    comparisonM = fr::compareBenchmarkSnapshots(*second, *first);
    showComparison();
    showSettings(false);
    notebook_results->SetSelection(notebook_results->GetPageCount() - 1);
    setReportStatus(_("Comparison of two saved reports."));
    // the progress is about a run, which is not shown now
    label_status->SetLabel(wxEmptyString);
    label_elapsed->SetLabel(wxEmptyString);
    gauge_progress->SetValue(0);
    updateControls();
}
