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

#include <algorithm>
#include <cmath>

#include "config/Config.h"
#include "core/StringUtils.h"
#include "engine/db/IDatabase.h"
#include "gui/BenchmarkDialog.h"
#include "gui/BenchmarkProgressPanel.h"
#include "gui/BenchmarkReportPage.h"
#include "gui/BenchmarkResultsView.h"
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
    ID_header_advanced
};

using ResultsView = BenchmarkResultsView;

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

// an explaining text, quieter than a label; one that is cut with an
// ellipsis where the dialog is narrow does not make the dialog wider
wxStaticText* createHintText(wxWindow* parent, const wxString& text, bool ellipsize = false)
{
    wxStaticText* label = new wxStaticText(parent, wxID_ANY, text, wxDefaultPosition,
        wxDefaultSize, ellipsize ? wxST_ELLIPSIZE_END | wxST_NO_AUTORESIZE : 0);
    // between the text and the background, nearer to the text
    const wxColour foreground = parent->GetForegroundColour();
    const wxColour background = parent->GetBackgroundColour();
    label->SetForegroundColour(getReadableBenchmarkColour(mixBenchmarkColours(foreground,
        background, 0.68), foreground, background));
    if (ellipsize)
        label->SetMinSize(wxSize(parent->FromDIP(40), -1));
    return label;
}

// the style of the report in the dialog: dark where the system colours of
// the dialog are dark, also when FlameRobin chose its dark mode itself
// (wxWidgets 3.3 and later), so that the report looks like the dialog
const char* getReportStyle()
{
    return wxSystemSettings::GetAppearance().IsUsingDarkBackground() ? "dark" : "light";
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
    SetBackgroundColour(mixBenchmarkColours(foreground, background, 0.4));
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

} // namespace

BEGIN_EVENT_TABLE(BenchmarkDialog, BaseDialog)
    EVT_BUTTON(ID_button_start, BenchmarkDialog::OnStart)
    EVT_BUTTON(ID_button_cancel, BenchmarkDialog::OnCancel)
    EVT_BUTTON(ResultsView::ID_button_copy, BenchmarkDialog::OnCopyMarkdown)
    EVT_BUTTON(ResultsView::ID_button_save, BenchmarkDialog::OnSaveReport)
    EVT_BUTTON(ResultsView::ID_button_browser, BenchmarkDialog::OnOpenInBrowser)
    EVT_BUTTON(ResultsView::ID_button_close_result, BenchmarkDialog::OnCloseResult)
    EVT_MENU(ResultsView::ID_button_close_result, BenchmarkDialog::OnCloseResult)
    EVT_BUTTON(ID_button_earlier_browse, BenchmarkDialog::OnBrowseEarlier)
    EVT_BUTTON(ID_button_compare, BenchmarkDialog::OnCompareReports)
    EVT_CHECKBOX(ID_check_private, BenchmarkDialog::OnPrivateChanged)
    EVT_CHOICE(ID_choice_install, BenchmarkDialog::OnTargetChanged)
    EVT_COLLAPSIBLEHEADER_CHANGED(ID_header_advanced, BenchmarkDialog::OnAdvancedToggled)
    EVT_BUTTON(wxID_CANCEL, BenchmarkDialog::OnCloseButton)
    EVT_CHOICE(ID_choice_target, BenchmarkDialog::OnTargetChanged)
END_EVENT_TABLE()

BenchmarkDialog::BenchmarkDialog(wxWindow* parent, Database* db)
    : BaseDialog(parent, -1, wxEmptyString), databaseM(db)
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
    // Ctrl+W closes the shown result, also from the report view
    wxAcceleratorEntry keys[] = {
        wxAcceleratorEntry(wxACCEL_CTRL, 'W', BenchmarkResultsView::ID_button_close_result)
    };
    SetAcceleratorTable(wxAcceleratorTable(1, keys));
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
    button->SetForegroundColour(getReadableBenchmarkColour(
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
    const int key = event.GetKeyCode();
    // the next or previous tab, also where the report view has the focus,
    // which would keep these keys for itself
    if (event.GetModifiers() == wxMOD_CONTROL || event.GetModifiers()
        == (wxMOD_CONTROL | wxMOD_SHIFT))
    {
        const bool shift = event.ShiftDown();
        if (key == WXK_TAB || ((key == WXK_PAGEUP || key == WXK_PAGEDOWN) && !shift))
        {
            view_results->selectNext(key == WXK_TAB ? !shift : key == WXK_PAGEDOWN);
            return;
        }
    }
    // Escape closes an open explanation, not the dialog; Tab closes it and
    // moves on
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

    // the strip of the progress, the log and the results, and what the
    // selected tab shows: the progress is there before the start, so that
    // nothing moves when a test starts
    view_results = new BenchmarkResultsView(panel);
    panel_progress = view_results->getProgress();
    panel_progress->onShowLog = [this]() { view_results->select(ResultsView::logTab); };
    text_log = view_results->getLog();
    button_copy = view_results->getCopyButton();
    button_save = view_results->getSaveButton();
    button_browser = view_results->getBrowserButton();
    button_close_result = view_results->getCloseButton();
    view_results->setStatusWidth(getCopiedMessage());
    view_results->makePage = [this](int id) { return makePage(id); };
    view_results->onSelected = [this](int id) { onResultSelected(id); };
    view_results->onCloseResult = [this](int id) { closeResult(id); };
    view_results->onShortcut = [this](char key)
    {
        wxCommandEvent dummy;
        if (key == 'W')
            OnCloseResult(dummy);
    };
    sizerMain->Add(view_results, 1, wxEXPAND);

    // buttons
    button_start = new wxButton(panel, ID_button_start, _("&Start"));
    // the main action; not the default button, so that Enter in a field
    // does not start a test
    button_start->SetFont(button_start->GetFont().Bold());
    button_cancel = new wxButton(panel, ID_button_cancel, _("C&ancel test"));
    button_compare = new wxButton(panel, ID_button_compare, _("Compare t&wo reports..."));
    button_compare->SetToolTip(_("Compares two saved reports, without a new test."));
    // what is shown, copied and saved of every result
    check_private = new wxCheckBox(panel, ID_check_private,
        _("&Hide private information"));
    check_private->SetToolTip(_("Removes computer and host names, IP addresses, "
        "user names, paths and service names from the report, the copy and the "
        "saved file, so that it can be shared in public. Hardware, versions and "
        "all measurements stay."));
    bool hidePrivate = false;
    config().getValue(hidePrivateKey, hidePrivate);
    check_private->SetValue(hidePrivate);
    view_results->setPageVariant(getPageVariant());
    button_close = new wxButton(panel, wxID_CANCEL, _("&Close"));

    wxBoxSizer* sizerButtons = new wxBoxSizer(wxHORIZONTAL);
    sizerButtons->Add(button_start, 0, wxRIGHT, gap);
    sizerButtons->Add(button_cancel, 0, wxRIGHT, gap);
    sizerButtons->Add(button_compare, 0, wxRIGHT, gap);
    sizerButtons->AddStretchSpacer(1);
    sizerButtons->Add(check_private, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(12));
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
    showReady();
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
    // a saved file is shown as it is: it cannot be copied as Markdown or
    // saved again
    const fr::BenchmarkResultEntry* entry = getShownResult();
    const bool own = entry && entry->kind != fr::BenchmarkResultKind::SavedReport;
    button_copy->Enable(idle && own);
    button_save->Enable(idle && own);
    button_browser->Enable(idle && entry);
    button_close_result->Enable(idle && entry);
}

wxString BenchmarkDialog::getReadyHint() const
{
    return wxString::Format(_("Ready. The test takes %s."), fr::getBenchmarkDurationHint(
        getIntensity(), check_comparepages->GetValue(), check_manyusers->GetValue()));
}

std::vector<fr::BenchmarkStagePlan> BenchmarkDialog::getStagePlan() const
{
    return fr::getBenchmarkStagePlan(getIntensity(), check_comparepages->GetValue(),
        check_manyusers->GetValue());
}

void BenchmarkDialog::showReady()
{
    panel_progress->showReady(getStagePlan(), getReadyHint());
}

void BenchmarkDialog::OnDurationChanged(wxCommandEvent& event)
{
    event.Skip();
    // after a run the settings are hidden and the progress shows the result;
    // an error stays until the next start
    if (!runningM && settingsShownM && panel_progress->isReady())
        showReady();
}

void BenchmarkDialog::setReportStatus(const wxString& message)
{
    view_results->setStatus(message);
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
    // a new test replaces the results before it
    for (const fr::BenchmarkResultEntry& e : std::vector<fr::BenchmarkResultEntry>(
        resultsM.getEntries()))
    {
        view_results->removeResult(e.id);
    }
    resultsM.closeAll();
    closeRequestedM = false;
    loggedResultsM.clear();
    lastMessageM.clear();
    text_log->Clear();
    view_results->select(ResultsView::progressTab);
    // the settings cannot be changed during the run; the progress gets
    // their room
    showAdvanced(false);
    startTimeM = std::chrono::steady_clock::now();
    panel_progress->start(runnerM->getStagePlan());
    appendLog(wxString::Format(_("Target: %s"), params.targetDescription));
    appendLog(wxString::Format(_("Intensity: %s, page size %d"),
        fr::getBenchmarkIntensityName(intensity), params.pageSize));
    appendLog(wxString::Format(_("Report view: %s"), view_results->getViewName()));
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
    panel_progress->cancelRequested();
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

void BenchmarkDialog::appendLog(const wxString& line)
{
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - startTimeM).count();
    text_log->AppendText(wxString::Format("[%02d:%02d] ", int(seconds / 60),
        int(seconds % 60)) + line + "\n");
}

void BenchmarkDialog::onProgress(const fr::BenchmarkProgress& progress)
{
    panel_progress->update(progress);
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
    runningM = false;
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
    std::optional<fr::BenchmarkSnapshot> baseline;
    if (!earlierFile.empty())
    {
        const fr::BenchmarkSnapshotResult earlier = readReportFile(earlierFile);
        earlierProblem = wxFileExists(earlierFile)
            ? fr::getBenchmarkSnapshotProblem(earlier)
            : wxString(_("The file does not exist."));
        if (!earlierProblem.empty())
            appendLog(earlierFile + ": " + earlierProblem);
        else
            baseline = earlier.snapshot;
    }

    using Outcome = BenchmarkProgressPanel::Outcome;
    const Outcome outcome = report.cancelled ? Outcome::Cancelled
        : !report.errorMessage.empty() || aborted ? Outcome::Failed : Outcome::Finished;
    wxString message;
    if (outcome == Outcome::Cancelled)
        message = _("Cancelled. The results are incomplete.");
    else if (outcome == Outcome::Failed)
        message = _("Failed. See the log for details.");
    else if (!earlierFile.empty() && !earlierProblem.empty())
        message = _("Finished. The earlier report cannot be compared, see the log.");
    else if (!earlierFile.empty())
        message = _("Finished. The overview compares this run with the earlier report.");
    else
        message = _("Finished. Save the report to compare it with later runs or other "
            "servers.");

    // the whole error below the stages, with the hint for this computer
    wxString error = report.errorMessage;
    if (!report.errorMessage.empty())
    {
        appendLog(_("Error:") + " " + report.errorMessage);
        if (getTarget() == BenchmarkTarget::ThisComputer)
        {
            const wxString hint = _("Hint: a test of this computer needs a Firebird "
                "installation with the engine plugin (engine12, engine13 or engine14 for "
                "Firebird 3, 4/5 or 6 in its plugins folder), and the directory must be "
                "writable for your user. If FlameRobin already used this client library "
                "for another connection, restart FlameRobin and run the test first.");
            appendLog(hint);
            error += "\n\n" + hint;
        }
    }
    const bool cleanupFailed = !aborted && report.benchmarkDatabaseCreated
        && !report.benchmarkDatabaseDropped;
    panel_progress->finish(outcome, cleanupFailed, message, error);
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

    // a test that measured nothing has only its message
    const int id = resultsM.addRun(report, baseline);
    if (id)
        addResultTab(id);
    showSettings(false);
    // an error stays in sight, with the stage it happened in; the report of
    // the steps before it is on its tab
    if (id && outcome != Outcome::Failed)
        view_results->select(id);
    updateControls();

    if (closeRequestedM)
        Close();
}

void BenchmarkDialog::showSettings(bool show)
{
    settingsShownM = show;
    sizerSettingsM->ShowItems(show);
    button_start->SetLabel(show ? _("&Start") : _("&New test"));
    if (show)
    {
        // ready for the next run
        showReady();
        view_results->select(ResultsView::progressTab);
        // the hint of the advanced settings is only shown next to the
        // closed pane
        layoutAdvanced();
    }
    else
        getControlsPanel()->Layout();
}

void BenchmarkDialog::addResultTab(int id)
{
    if (const fr::BenchmarkResultEntry* entry = resultsM.find(id))
    {
        view_results->addResult(id, fr::getBenchmarkResultLabel(*entry),
            fr::describeBenchmarkResult(*entry));
    }
}

void BenchmarkDialog::updateResultTab(int id)
{
    if (const fr::BenchmarkResultEntry* entry = resultsM.find(id))
    {
        view_results->setResultLabel(id, fr::getBenchmarkResultLabel(*entry),
            fr::describeBenchmarkResult(*entry));
        if (view_results->getSelection() == id)
            view_results->setHeader(fr::getBenchmarkResultHeader(*entry));
    }
}

const fr::BenchmarkResultEntry* BenchmarkDialog::getShownResult() const
{
    return resultsM.find(view_results->getSelection());
}

void BenchmarkDialog::onResultSelected(int id)
{
    resultsM.select(id > 0 ? id : 0);
    if (const fr::BenchmarkResultEntry* entry = resultsM.find(id))
    {
        // the result needs the room of the settings
        if (settingsShownM)
            showSettings(false);
        button_save->SetLabel(entry->kind == fr::BenchmarkResultKind::Comparison
            ? _("Sa&ve comparison...") : _("Sa&ve report..."));
        button_save->GetParent()->Layout();
        view_results->setHeader(fr::getBenchmarkResultHeader(*entry));
        // a message such as "copied" was about the result shown before
        setReportStatus(entry->kind == fr::BenchmarkResultKind::SavedReport
            ? _("\"Hide private information\" does not change a saved file.")
            : wxString());
    }
    updateControls();
}

void BenchmarkDialog::closeResult(int id)
{
    if (runningM)
        return;
    const bool shown = view_results->getSelection() == id;
    const int next = resultsM.close(id);
    view_results->removeResult(id);
    if (shown)
        view_results->select(next > 0 ? next : ResultsView::progressTab);
    updateControls();
}

void BenchmarkDialog::OnCloseResult(wxCommandEvent& WXUNUSED(event))
{
    if (const fr::BenchmarkResultEntry* entry = getShownResult())
        closeResult(entry->id);
}

wxString BenchmarkDialog::getPageVariant() const
{
    return wxString(check_private->GetValue() ? "hidden," : "all,") + getReportStyle();
}

wxString BenchmarkDialog::makePage(int id) const
{
    const fr::BenchmarkResultEntry* entry = resultsM.find(id);
    if (!entry)
        return wxString();
    const bool web = view_results->hasWebView();
    switch (entry->kind)
    {
        case fr::BenchmarkResultKind::Run:
        {
            const fr::BenchmarkReport report = displayedReport(*entry);
            return web ? report.toHtml(getReportStyle()) : report.toMarkdown();
        }
        case fr::BenchmarkResultKind::Comparison:
        {
            const fr::BenchmarkComparison comparison = displayedComparison(*entry);
            return web ? comparison.toHtml(getReportStyle()) : comparison.toMarkdown();
        }
        case fr::BenchmarkResultKind::SavedReport:
        default:
            // as it was saved, in the style of the dialog
            if (web)
                return fr::applyBenchmarkReportTheme(entry->html, getReportStyle());
            return _("A saved HTML report needs a web view. \"Open in browser\" shows "
                "it in your web browser.");
    }
}

void BenchmarkDialog::OnCopyMarkdown(wxCommandEvent& WXUNUSED(event))
{
    const fr::BenchmarkResultEntry* entry = getShownResult();
    if (!entry || entry->kind == fr::BenchmarkResultKind::SavedReport)
        return;
    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxTextDataObject(displayedText(*entry, true)));
        wxTheClipboard->Close();
        setReportStatus(getCopiedMessage());
    }
}

void BenchmarkDialog::OnSaveReport(wxCommandEvent& WXUNUSED(event))
{
    if (const fr::BenchmarkResultEntry* entry = getShownResult())
        saveResult(entry->id);
}

bool BenchmarkDialog::saveResult(int id)
{
    const fr::BenchmarkResultEntry* entry = resultsM.find(id);
    if (!entry || entry->kind == fr::BenchmarkResultKind::SavedReport)
        return false;
    const bool comparison = entry->kind == fr::BenchmarkResultKind::Comparison;
    wxFileDialog fd(this, comparison ? _("Save comparison") : _("Save benchmark report"),
        "", comparison ? "flamerobin-benchmark-comparison.html" : "flamerobin-benchmark.html",
        _("HTML report (*.html)|*.html|Markdown (*.md)|*.md"),
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (fd.ShowModal() != wxID_OK)
        return false;
    const bool markdown = fd.GetFilterIndex() == 1
        || fd.GetPath().Lower().EndsWith(".md");
    wxFFile file(fd.GetPath(), "w");
    if (!file.IsOpened() || !file.Write(displayedText(*entry, markdown), wxConvUTF8)
        || !file.Close())
    {
        wxMessageBox(_("The report could not be saved."), _("Performance Benchmark & Diagnosis"),
            wxOK | wxICON_ERROR, this);
        return false;
    }
    resultsM.markSaved(id, fd.GetPath());
    updateResultTab(id);
    return true;
}

wxString BenchmarkDialog::displayedText(const fr::BenchmarkResultEntry& entry,
    bool markdown) const
{
    // the saved page follows the theme of the browser that opens it
    switch (entry.kind)
    {
        case fr::BenchmarkResultKind::Run:
        {
            const fr::BenchmarkReport report = displayedReport(entry);
            return markdown ? report.toMarkdown() : report.toHtml();
        }
        case fr::BenchmarkResultKind::Comparison:
        {
            const fr::BenchmarkComparison comparison = displayedComparison(entry);
            return markdown ? comparison.toMarkdown() : comparison.toHtml();
        }
        case fr::BenchmarkResultKind::SavedReport:
        default:
            return entry.html;
    }
}

void BenchmarkDialog::OnOpenInBrowser(wxCommandEvent& WXUNUSED(event))
{
    const fr::BenchmarkResultEntry* entry = getShownResult();
    if (!entry)
        return;
    // the browser may still read the page after the dialog is closed, so
    // the pages are only removed a day later
    const wxString tempDir = wxFileName::GetTempDir();
    {
        wxLogNull noErrors;
        wxArrayString oldFiles;
        wxDir::GetAllFiles(tempDir, &oldFiles, getBenchmarkPagePrefix() + "*.html",
            wxDIR_FILES);
        const wxDateTime yesterday = wxDateTime::Now() - wxTimeSpan::Day();
        for (const wxString& oldFile : oldFiles)
        {
            const wxDateTime modified = wxFileName(oldFile).GetModificationTime();
            if (modified.IsValid() && modified < yesterday)
                wxRemoveFile(oldFile);
        }
    }

    wxFileName path(tempDir, getBenchmarkPagePrefix()
        + wxDateTime::Now().Format("%Y%m%d-%H%M%S-")
        + wxString::Format("%lu.html", wxGetProcessId()));
    wxFFile file(path.GetFullPath(), "w");
    if (!file.IsOpened() || !file.Write(displayedText(*entry, false), wxConvUTF8)
        || !file.Close()
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

fr::BenchmarkReport BenchmarkDialog::displayedReport(
    const fr::BenchmarkResultEntry& entry) const
{
    // the earlier report and the private information only change what is
    // shown, copied and saved; the measured report stays as it is
    fr::BenchmarkReport report = *entry.report;
    report.earlier = entry.baseline;
    if (check_private->GetValue())
        report = report.anonymized();
    return report;
}

fr::BenchmarkComparison BenchmarkDialog::displayedComparison(
    const fr::BenchmarkResultEntry& entry) const
{
    if (check_private->GetValue())
        return fr::anonymizeBenchmarkComparison(*entry.comparison);
    return *entry.comparison;
}

void BenchmarkDialog::OnPrivateChanged(wxCommandEvent& WXUNUSED(event))
{
    config().setValue(hidePrivateKey, check_private->GetValue());
    view_results->setPageVariant(getPageVariant());
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
    // two files without a tab of their own
    fr::BenchmarkResultEntry files[2];
    for (int i = 0; i < 2; ++i)
    {
        files[i].kind = fr::BenchmarkResultKind::SavedReport;
        files[i].snapshot = i == 0 ? *first : *second;
        files[i].path = paths[i];
    }
    // the run with the later date is the newer one
    const fr::BenchmarkPair pair = fr::orderBenchmarkPair(files[0], files[1]);
    const int id = resultsM.addComparison(*pair.older, *pair.newer);
    if (!id)
        return;
    addResultTab(id);
    view_results->select(id);
    setReportStatus(_("Comparison of two saved reports."));
    // the progress is about a run, which is not shown now
    panel_progress->showReady(getStagePlan(), wxEmptyString);
    updateControls();
}
