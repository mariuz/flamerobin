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
#include <wx/collpane.h>
#include <wx/dir.h>
#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/simplebook.h>
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
    ID_collapse_advanced,
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

// the line next to the list of targets
wxString getTargetHint(BenchmarkTarget target)
{
    switch (target)
    {
        case BenchmarkTarget::Registered:
            return _("Runs on the server of the registration; its login is used.");
        case BenchmarkTarget::ThisComputer:
            return _("Runs Firebird inside FlameRobin; no login, the network is not tested.");
        case BenchmarkTarget::OtherServer:
        default:
            return _("Runs on the server below; a login with the right to create a "
                "database is needed.");
    }
}

// the colour of the secondary text of the report (BenchmarkHtml.cpp), on
// a light or a dark background
wxColour getMutedColour()
{
    return wxSystemSettings::GetAppearance().IsUsingDarkBackground()
        ? wxColour(0x95, 0x9e, 0xab) : wxColour(0x66, 0x71, 0x83);
}

// a text in the secondary colour; one that is cut with an ellipsis where
// the dialog is narrow does not make the dialog wider
wxStaticText* createMutedText(wxWindow* parent, const wxString& text, bool ellipsize = false)
{
    wxStaticText* label = new wxStaticText(parent, wxID_ANY, text, wxDefaultPosition,
        wxDefaultSize, ellipsize ? wxST_ELLIPSIZE_END | wxST_NO_AUTORESIZE : 0);
    label->SetForegroundColour(getMutedColour());
    if (ellipsize)
        label->SetMinSize(wxSize(parent->FromDIP(40), -1));
    return label;
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
    EVT_COLLAPSIBLEPANE_CHANGED(ID_collapse_advanced, BenchmarkDialog::OnAdvancedToggled)
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

void BenchmarkDialog::createControls()
{
    wxPanel* panel = getControlsPanel();
    wxBoxSizer* sizerMain = new wxBoxSizer(wxVERTICAL);
    const int gap = FromDIP(6);

    // the settings; after a run they make room for the report
    wxBoxSizer* sizerSettings = new wxBoxSizer(wxVERTICAL);
    sizerSettingsM = sizerSettings;
    wxStaticText* info = new wxStaticText(panel, wxID_ANY,
        _("The test creates a temporary database (FR_BENCHMARK_*.FDB) in the "
          "directory below, runs all tests in it and deletes it afterwards. No "
          "other database is changed. The tests put load on the server for a "
          "few minutes."));
    info->Wrap(640);
    sizerSettings->Add(info, 0, wxEXPAND | wxBOTTOM, 8);

    // a label and a field in every line
    wxFlexGridSizer* sizerGrid = new wxFlexGridSizer(2, FromDIP(4), FromDIP(10));
    sizerGrid->AddGrowableCol(1, 1);

    // where to test: a list with short names, the details on a page below
    sizerGrid->Add(new wxStaticText(panel, wxID_ANY, _("&Test:")), 0,
        wxALIGN_CENTER_VERTICAL);
    choice_target = new wxChoice(panel, ID_choice_target);
    for (BenchmarkTarget target : fr::getBenchmarkTargets(databaseM != nullptr))
        choice_target->Append(getTargetName(target));
    choice_target->SetSelection(0);
    label_target_hint = createMutedText(panel, wxEmptyString, true);
    wxBoxSizer* sizerTarget = new wxBoxSizer(wxHORIZONTAL);
    sizerTarget->Add(choice_target, 0, wxALIGN_CENTER_VERTICAL);
    sizerTarget->Add(label_target_hint, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));
    sizerGrid->Add(sizerTarget, 1, wxEXPAND);

    // one page per target, in the order of BenchmarkTarget: the pages are as
    // high as the highest, so nothing moves when the target changes
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
        if (details.hostAndPort)
        {
            addLabel(_("Host:"));
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
            addLabel(_("User:"));
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
                sizerPage->Add(createMutedText(page, _("optional")), 0,
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
            addLabel(_("&Firebird:"));
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
            choice_install->SetToolTip(_("The engine of this installation runs the test, with "
                "its firebird.conf. The installed server keeps running and is not changed."));
            // GTK makes a list as wide as its longest entry
            choice_install->SetMinSize(wxSize(FromDIP(260), -1));
            sizerPage->Add(choice_install, 1, wxALIGN_CENTER_VERTICAL);
        }
        page->SetSizer(sizerPage);
        book_target->AddPage(page, wxEmptyString);
    }
    sizerGrid->AddSpacer(0);
    sizerGrid->Add(book_target, 1, wxEXPAND);

    sizerSettings->Add(sizerGrid, 0, wxEXPAND | wxBOTTOM, 6);

    wxBoxSizer* sizerDir = new wxBoxSizer(wxHORIZONTAL);
    sizerDir->Add(new wxStaticText(panel, wxID_ANY,
        _("Directory for the temporary database (as seen by the server):")),
        0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    // the directory of the selected database, so that its drive is tested
    wxString directory = databaseM ? fr::getBenchmarkDirectoryFromPath(databaseM->getPath())
        : wxString();
    text_directory = new wxTextCtrl(panel, wxID_ANY, directory.empty()
        ? wxFileName::GetTempDir() + wxFileName::GetPathSeparator() : directory);
    text_directory->SetToolTip(_("The disk of this directory is tested. Use the "
        "directory of your databases, so that their disk is measured."));
    sizerDir->Add(text_directory, 1, wxEXPAND);
    sizerSettings->Add(sizerDir, 0, wxEXPAND | wxBOTTOM, 6);

    // a saved report to compare the new run with: a single run is not
    // judged, the comparison shows what is faster or slower and why
    wxBoxSizer* sizerEarlier = new wxBoxSizer(wxHORIZONTAL);
    sizerEarlier->Add(new wxStaticText(panel, wxID_ANY, _("Compare with an earlier report:")),
        0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    wxString earlierFile;
    config().getValue(earlierFileKey, earlierFile);
    text_earlier = new wxTextCtrl(panel, wxID_ANY, earlierFile);
    text_earlier->SetHint(_("optional: a saved report (Markdown or HTML)"));
    text_earlier->SetToolTip(_("A report saved after an earlier run, e.g. before a "
        "change on the server, or the report of another server. The new report then "
        "shows what got faster or slower, what is different and what could be the "
        "reason."));
    sizerEarlier->Add(text_earlier, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    sizerEarlier->Add(new wxButton(panel, ID_button_earlier_browse, _("Bro&wse...")),
        0, wxALIGN_CENTER_VERTICAL);
    sizerSettings->Add(sizerEarlier, 0, wxEXPAND | wxBOTTOM, 6);

    wxString intensities[] = {
        fr::getBenchmarkIntensityName(fr::BenchmarkIntensity::Quick),
        fr::getBenchmarkIntensityName(fr::BenchmarkIntensity::Standard),
        fr::getBenchmarkIntensityName(fr::BenchmarkIntensity::Deep)
    };
    radio_intensity = new wxRadioBox(panel, wxID_ANY, _("Intensity"),
        wxDefaultPosition, wxDefaultSize, 3, intensities, 3, wxRA_SPECIFY_COLS);
    radio_intensity->SetSelection(1);
    radio_intensity->Bind(wxEVT_RADIOBOX, &BenchmarkDialog::OnDurationChanged, this);
    radio_intensity->SetItemToolTip(0, _("Less than a minute: a first impression."));
    radio_intensity->SetItemToolTip(1, _("About two minutes: the usual choice. Compare "
        "runs of the same intensity."));
    radio_intensity->SetItemToolTip(2, _("Several minutes with more data and up to 32 "
        "users: for a thorough look at a server."));
    sizerSettings->Add(radio_intensity, 0, wxEXPAND | wxBOTTOM, 6);

    // rarely needed settings, collapsed at first
    collapse_advanced = new wxCollapsiblePane(panel, ID_collapse_advanced,
        _("Advanced settings"), wxDefaultPosition, wxDefaultSize,
        wxCP_DEFAULT_STYLE | wxCP_NO_TLW_RESIZE);
    wxWindow* advanced = collapse_advanced->GetPane();
    wxBoxSizer* sizerAdvanced = new wxBoxSizer(wxVERTICAL);

    // the client library decides the Firebird version of a test of this
    // computer without login
    wxBoxSizer* sizerLib = new wxBoxSizer(wxHORIZONTAL);
    sizerLib->Add(new wxStaticText(advanced, wxID_ANY, _("Client library:")),
        0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    text_clientlib = new wxTextCtrl(advanced, wxID_ANY, databaseM
        ? databaseM->getClientLibrary() : wxString());
    text_clientlib->SetHint(_("default client library"));
    text_clientlib->SetToolTip(_("The Firebird client library (fbclient) FlameRobin "
        "uses. For a test of this computer without login it needs the engine plugin "
        "next to it, and it selects the Firebird version (3, 4, 5 or 6)."));
    sizerLib->Add(text_clientlib, 1, wxEXPAND);
    sizerAdvanced->Add(sizerLib, 0, wxEXPAND | wxTOP | wxBOTTOM, 6);

    // the page size influences the storage and cache tests; default to the
    // one of the selected database when FlameRobin already knows it
    wxBoxSizer* sizerPageSize = new wxBoxSizer(wxHORIZONTAL);
    sizerPageSize->Add(new wxStaticText(advanced, wxID_ANY,
        _("Page size of the temporary database:")),
        0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
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
    choice_pagesize->SetToolTip(_("The page size only affects the storage "
        "and cache tests; CPU, network and connection tests do not depend on "
        "it. Automatic uses the page size of the selected database when it "
        "is known, otherwise 8192. Firebird 3 supports up to 16384; a larger "
        "value is reduced automatically."));
    choice_pagesize->SetSelection(0);
    sizerPageSize->Add(choice_pagesize, 0, wxRIGHT, 12);
    check_comparepages = new wxCheckBox(advanced, wxID_ANY,
        _("Compare all page sizes (storage and cache tests only)"));
    check_comparepages->SetToolTip(_("Repeats the storage and cache tests in "
        "a separate temporary database for every page size the server "
        "supports, and shows them side by side."));
    check_comparepages->Bind(wxEVT_CHECKBOX, &BenchmarkDialog::OnDurationChanged, this);
    sizerPageSize->Add(check_comparepages, 0, wxALIGN_CENTER_VERTICAL);
    sizerAdvanced->Add(sizerPageSize, 0, wxBOTTOM, 6);

    check_inspect = new wxCheckBox(advanced, wxID_ANY,
        _("Also read the configuration of the selected database (read-only: "
          "page buffers, forced writes, transaction state)"));
    check_inspect->SetToolTip(_("Only for the server of the selected "
        "database: connects to it and reads its header, MON$DATABASE and "
        "RDB$DATABASE. Nothing is written."));
    check_inspect->SetValue(false);
    sizerAdvanced->Add(check_inspect, 0, wxBOTTOM, 6);

    check_manyusers = new wxCheckBox(advanced, wxID_ANY,
        _("Also test 50 and 250 users at once (puts a heavy load on the server)"));
    check_manyusers->SetToolTip(_("Adds two rounds with 50 and 250 connections "
        "working at the same time. Shows whether the server keeps up with many "
        "users; takes about half a minute more."));
    check_manyusers->Bind(wxEVT_CHECKBOX, &BenchmarkDialog::OnDurationChanged, this);
    sizerAdvanced->Add(check_manyusers, 0, wxBOTTOM, 6);
    advanced->SetSizer(sizerAdvanced);
    sizerSettings->Add(collapse_advanced, 0, wxEXPAND | wxBOTTOM, 6);
    sizerMain->Add(sizerSettings, 0, wxEXPAND);

    // progress
    wxBoxSizer* sizerStatus = new wxBoxSizer(wxHORIZONTAL);
    label_status = new wxStaticText(panel, wxID_ANY, wxEmptyString);
    label_elapsed = new wxStaticText(panel, wxID_ANY, wxEmptyString);
    sizerStatus->Add(label_status, 1, wxALIGN_CENTER_VERTICAL);
    sizerStatus->Add(label_elapsed, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);
    sizerMain->Add(sizerStatus, 0, wxEXPAND | wxBOTTOM, 4);
    gauge_progress = new wxGauge(panel, wxID_ANY, 100, wxDefaultPosition,
        wxSize(-1, 14));
    sizerMain->Add(gauge_progress, 0, wxEXPAND | wxBOTTOM, 6);

    // results
    notebook_results = new wxNotebook(panel, wxID_ANY);

    text_log = new wxTextCtrl(notebook_results, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
    notebook_results->AddPage(text_log, _("Progress"));

#if wxUSE_WEBVIEW
    wxString backend;
    webview_report = createReportView(notebook_results, backend);
    if (webview_report)
    {
        notebook_results->AddPage(webview_report, _("Report"));
        reportPageM.reset(new BenchmarkReportPage(webview_report,
            backend == wxWebViewBackendIE));
        if (backend == wxWebViewBackendIE)
        {
            reportViewM = "Internet Explorer";
            wxStaticText* hint = new wxStaticText(panel, wxID_ANY,
                _("The Microsoft Edge WebView2 Runtime is not installed on this "
                  "computer, so the report is shown with Internet Explorer in a "
                  "simpler form. \"Open in browser\" shows it in your web browser."));
            hint->Wrap(900);
            sizerMain->Add(hint, 0, wxEXPAND | wxBOTTOM, 4);
        }
        else
            reportViewM = wxWebView::GetBackendVersionInfo(backend).GetVersionString();
    }
#endif
    if (!webview_report)
    {
        reportViewM = _("text, no web view is available");
        text_report = new wxTextCtrl(notebook_results, wxID_ANY, wxEmptyString,
            wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
        notebook_results->AddPage(text_report, _("Report"));
    }

    sizerMain->Add(notebook_results, 1, wxEXPAND);

    // buttons
    button_start = new wxButton(panel, ID_button_start, _("&Start"));
    button_cancel = new wxButton(panel, ID_button_cancel, _("C&ancel test"));
    check_private = new wxCheckBox(panel, ID_check_private,
        _("&Hide private information"));
    check_private->SetToolTip(_("Removes computer and host names, IP addresses, "
        "user names, paths and service names from the report, the copy and the "
        "saved file, so that it can be shared in public. Hardware, versions and "
        "all measurements stay."));
    bool hidePrivate = false;
    config().getValue(hidePrivateKey, hidePrivate);
    check_private->SetValue(hidePrivate);
    button_compare = new wxButton(panel, ID_button_compare, _("Compare t&wo reports..."));
    button_compare->SetToolTip(_("Compares two saved reports, without a new test."));
    button_copy = new wxButton(panel, ID_button_copy, _("Copy as &Markdown"));
    button_save = new wxButton(panel, ID_button_save, _("Sa&ve report..."));
    button_browser = new wxButton(panel, ID_button_browser, _("Open in &browser"));
    button_browser->SetToolTip(_("Shows the report in the web browser of this computer. "
        "The page is written to the temporary directory; without \"Hide private "
        "information\" it contains the names of computers and paths."));
    button_close = new wxButton(panel, wxID_CANCEL, _("&Close"));

    wxBoxSizer* sizerButtons = new wxBoxSizer(wxHORIZONTAL);
    sizerButtons->Add(button_start, 0, wxRIGHT, 6);
    sizerButtons->Add(button_cancel, 0, wxRIGHT, 6);
    sizerButtons->Add(button_compare, 0, wxRIGHT, 6);
    sizerButtons->AddStretchSpacer(1);
    sizerButtons->Add(check_private, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);
    sizerButtons->Add(button_copy, 0, wxRIGHT, 6);
    sizerButtons->Add(button_save, 0, wxRIGHT, 6);
    sizerButtons->Add(button_browser, 0, wxRIGHT, 6);
    sizerButtons->Add(button_close, 0);

    layoutSizers(sizerMain, sizerButtons, true);
    SetSize(wxSize(980, 880));

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
    radio_intensity->Enable(idle);
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
        fr::BenchmarkIntensity(radio_intensity->GetSelection()),
        check_comparepages->GetValue(), check_manyusers->GetValue()));
}

void BenchmarkDialog::OnDurationChanged(wxCommandEvent& event)
{
    event.Skip();
    // after a run the settings are hidden and the status is the result
    if (!runningM && settingsShownM)
        label_status->SetLabel(getReadyHint());
}

void BenchmarkDialog::OnAdvancedToggled(wxCollapsiblePaneEvent& WXUNUSED(event))
{
    getControlsPanel()->Layout();
}

void BenchmarkDialog::OnTargetChanged(wxCommandEvent& WXUNUSED(event))
{
    const BenchmarkTarget target = getTarget();
    book_target->ChangeSelection(size_t(target));
    label_target_hint->SetLabel(getTargetHint(target));
    label_target_hint->SetToolTip(target == BenchmarkTarget::ThisComputer
        ? _("Runs Firebird directly inside FlameRobin with the engine of a Firebird "
            "installation on this computer. No server connection, no user name and no "
            "password are needed. Tests the hardware, the operating system and the "
            "Firebird configuration of this computer; the network is not tested. Run it "
            "on the server itself.") : getTargetHint(target));
    choice_target->SetToolTip(label_target_hint->GetToolTipText());
    if (target == BenchmarkTarget::ThisComputer
        && choice_install->GetSelection() == int(installationsM.size())
        && collapse_advanced->IsCollapsed())
    {
        collapse_advanced->Expand();
        getControlsPanel()->Layout();
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

    fr::BenchmarkIntensity intensity =
        fr::BenchmarkIntensity(radio_intensity->GetSelection());
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
    }
    getControlsPanel()->Layout();
}

void BenchmarkDialog::showReport(const fr::BenchmarkReport& report)
{
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
        label_status->SetLabel(_("Report copied to the clipboard as Markdown."));
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
    label_status->SetLabel(_("Comparison of two saved reports."));
    updateControls();
}
