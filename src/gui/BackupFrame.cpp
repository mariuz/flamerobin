/*
  The contents of this file are subject to the Initial Developer's Public
  License Version 1.0 (the "License"); you may not use this file except in
  compliance with the License. You may obtain a copy of the License here:
  http://www.flamerobin.org/license.html.

  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.

  The Original Code is FlameRobin (TM).

  The Initial Developer of the Original Code is Michael Hieke.

  Portions created by the original developer
  are Copyright (C) 2004 Michael Hieke.

  All Rights Reserved.

  $Id$

  Contributor(s): Milan Babuskov, Nando Dessena
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/datetime.h>
#include <wx/filename.h>

#include <algorithm>

#include <ibpp.h>

#include "config/Config.h"
#include "gui/BackupFrame.h"
#include "gui/controls/LogTextControl.h"
#include "metadata/database.h"
#include "metadata/server.h"
#include "styleguide.h"
#include "ugly.h"
//-----------------------------------------------------------------------------
// worker thread class to perform database backup
class BackupThread: public wxThread {
public:
    BackupThread(BackupFrame* frame, wxString server, wxString username,
        wxString password, wxString dbfilename, wxString bkfilename,
        IBPP::BRF flags);

    virtual void* Entry();
    virtual void OnExit();
private:
    BackupFrame* frameM;
    wxString serverM;
    wxString usernameM;
    wxString passwordM;
    wxString dbfileM;
    wxString bkfileM;
    IBPP::BRF brfM;
    void logError(wxString& msg);
    void logImportant(wxString& msg);
    void logProgress(wxString& msg);
};
//-----------------------------------------------------------------------------
BackupThread::BackupThread(BackupFrame* frame, wxString server,
        wxString username, wxString password, wxString dbfilename,
        wxString bkfilename, IBPP::BRF flags)
    : wxThread()
{
    frameM = frame;
    serverM = server;
    usernameM = username;
    passwordM = password;
    dbfileM = dbfilename;
    bkfileM = bkfilename;
    // always use verbose flag
    brfM = (IBPP::BRF)((int)flags | (int)IBPP::brVerbose);
}
//-----------------------------------------------------------------------------
void* BackupThread::Entry()
{
    wxDateTime now;
    wxString msg;

    try
    {
        msg.Printf(_("Connecting to server %s..."), serverM.c_str());
        logImportant(msg);
        IBPP::Service svc = IBPP::ServiceFactory(wx2std(serverM),
            wx2std(usernameM), wx2std(passwordM));
        svc->Connect();

        now = wxDateTime::Now();
        msg.Printf(_("Database backup started %s"), now.FormatTime().c_str());
        logImportant(msg);
        svc->StartBackup(wx2std(dbfileM), wx2std(bkfileM), brfM);
        while (true)
        {
            if (TestDestroy())
            {
                now = wxDateTime::Now();
                msg.Printf(_("Database backup cancelled %s"),
                    now.FormatTime().c_str());
                logImportant(msg);
                break;
            }
            const char* c = svc->WaitMsg();
            if (c == 0)
            {
                now = wxDateTime::Now();
                msg.Printf(_("Database backup finished %s"),
                    now.FormatTime().c_str());
                logImportant(msg);
                break;
            }
            msg = std2wx(c);
            logProgress(msg);
        }
        svc->Disconnect();
    }
    catch (IBPP::Exception& e)
    {
        now = wxDateTime::Now();
        msg.Printf(_("Database backup cancelled %s due to IBPP exception:\n\n"),
            now.FormatTime().c_str());
        msg += std2wx(e.ErrorMessage());
        logError(msg);
    }
    catch (...)
    {
        now = wxDateTime::Now();
        msg.Printf(_("Database backup cancelled %s due to exception"),
            now.FormatTime().c_str());
        logError(msg);
    }
    return 0;
}
//-----------------------------------------------------------------------------
void BackupThread::OnExit()
{
    if (frameM != 0)
    {
        wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED,
            BackupRestoreBaseFrame::ID_thread_finished);
        wxPostEvent(frameM, event);
    }
}
//-----------------------------------------------------------------------------
void BackupThread::logError(wxString& msg)
{
    if (frameM != 0)
        frameM->threadOutputMsg(msg, BackupRestoreBaseFrame::error_message);
}
//-----------------------------------------------------------------------------
void BackupThread::logImportant(wxString& msg)
{
    if (frameM != 0)
        frameM->threadOutputMsg(msg, BackupRestoreBaseFrame::important_message);
}
//-----------------------------------------------------------------------------
void BackupThread::logProgress(wxString& msg)
{
    if (frameM != 0)
        frameM->threadOutputMsg(msg, BackupRestoreBaseFrame::progress_message);
}
//-----------------------------------------------------------------------------
BackupFrame::BackupFrame(wxWindow* parent, Database* db)
    : BackupRestoreBaseFrame(parent, db)
{
    setIdString(this, getFrameId(db));

    wxString s;
    s.Printf(_("Backup Database \"%s:%s\""),
        serverM->getName_().c_str(), databaseM->getName_().c_str());
    SetTitle(s);

    createControls();
    layoutControls();
    updateControls();
}
//-----------------------------------------------------------------------------
//! implementation details
void BackupFrame::createControls()
{
    panel_controls = new wxPanel(this, -1, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL | wxCLIP_CHILDREN | wxNO_FULL_REPAINT_ON_RESIZE);
    label_filename = new wxStaticText(panel_controls, -1, _("Backup file:"));
    text_ctrl_filename = new wxTextCtrl(panel_controls, ID_text_ctrl_filename,
        wxEmptyString);
    button_browse = new wxButton(panel_controls, ID_button_browse, _("..."),
        wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

    checkbox_checksum = new wxCheckBox(panel_controls, -1,
        _("Ignore Checksums"));
    checkbox_limbo = new wxCheckBox(panel_controls, -1,
        _("Ignore Limbo Transactions"));
    checkbox_transport = new wxCheckBox(panel_controls, -1,
        _("Use non-transportable format"));
    checkbox_metadata = new wxCheckBox(panel_controls, -1,
        _("Only backup metadata"));
    checkbox_garbage = new wxCheckBox(panel_controls, -1,
        _("Don't do garbage collection"));
    checkbox_extern = new wxCheckBox(panel_controls, -1,
        _("Convert external tables"));

    checkbox_showlog = new wxCheckBox(panel_controls, ID_checkbox_showlog,
        _("Show complete log"));
    button_start = new wxButton(panel_controls, ID_button_start, _("Backup"));

    text_ctrl_log = new LogTextControl(this, ID_text_ctrl_log);
}
//-----------------------------------------------------------------------------
void BackupFrame::layoutControls()
{
    int wh = text_ctrl_filename->GetMinHeight();
    button_browse->SetSize(wh, wh);

    wxBoxSizer* sizerFilename = new wxBoxSizer(wxHORIZONTAL);
    sizerFilename->Add(label_filename, 0, wxALIGN_CENTER_VERTICAL);
    sizerFilename->Add(styleguide().getControlLabelMargin(), 0);
    sizerFilename->Add(text_ctrl_filename, 1, wxALIGN_CENTER_VERTICAL);
    sizerFilename->Add(styleguide().getBrowseButtonMargin(), 0);
    sizerFilename->Add(button_browse, 0, wxALIGN_CENTER_VERTICAL);

    wxGridSizer* sizerChecks = new wxGridSizer(2, 2,
        styleguide().getCheckboxSpacing(),
        styleguide().getUnrelatedControlMargin(wxHORIZONTAL));
    sizerChecks->Add(checkbox_checksum);
    sizerChecks->Add(checkbox_metadata);
    sizerChecks->Add(checkbox_limbo);
    sizerChecks->Add(checkbox_garbage);
    sizerChecks->Add(checkbox_transport);
    sizerChecks->Add(checkbox_extern);

    wxBoxSizer* sizerButtons = new wxBoxSizer(wxHORIZONTAL);
    sizerButtons->Add(checkbox_showlog, 0, wxALIGN_CENTER_VERTICAL);
    sizerButtons->Add(0, 0, 1, wxEXPAND);
    sizerButtons->Add(button_start);

    wxBoxSizer* sizerPanelV = new wxBoxSizer(wxVERTICAL);
    sizerPanelV->Add(0, styleguide().getFrameMargin(wxTOP));
    sizerPanelV->Add(sizerFilename, 0, wxEXPAND);
    sizerPanelV->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerPanelV->Add(sizerChecks);
    sizerPanelV->Add(0, styleguide().getUnrelatedControlMargin(wxVERTICAL));
    sizerPanelV->Add(sizerButtons, 0, wxEXPAND);
    sizerPanelV->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
    wxBoxSizer* sizerPanelH = new wxBoxSizer(wxHORIZONTAL);
    sizerPanelH->Add(styleguide().getFrameMargin(wxLEFT), 0);
    sizerPanelH->Add(sizerPanelV, 1, wxEXPAND);
    sizerPanelH->Add(styleguide().getFrameMargin(wxRIGHT), 0);
    panel_controls->SetSizerAndFit(sizerPanelH);

    wxBoxSizer* sizerMain = new wxBoxSizer(wxVERTICAL);
    sizerMain->Add(panel_controls, 0, wxEXPAND);
    sizerMain->Add(text_ctrl_log, 1, wxEXPAND);

    // show at least 3 lines of text since it is default size too
    sizerMain->SetItemMinSize(text_ctrl_log,
        -1, 3 * text_ctrl_filename->GetSize().GetHeight());
    SetSizerAndFit(sizerMain);
}
//-----------------------------------------------------------------------------
void BackupFrame::updateControls()
{
    bool running = threadM != 0;
    button_browse->Enable(!running);
    text_ctrl_filename->Enable(!running);
    checkbox_checksum->Enable(!running);
    checkbox_limbo->Enable(!running);
    checkbox_metadata->Enable(!running);
    checkbox_garbage->Enable(!running);
    checkbox_transport->Enable(!running);
    checkbox_extern->Enable(!running);
    button_start->Enable(!running && !text_ctrl_filename->GetValue().empty());
}
//-----------------------------------------------------------------------------
void BackupFrame::doReadConfigSettings(const wxString& prefix)
{
    BackupRestoreBaseFrame::doReadConfigSettings(prefix);
    std::vector<wxString> flags;
    config().getValue(prefix + Config::pathSeparator + wxT("options"), flags);
    if (!flags.empty())
    {
        checkbox_checksum->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), wxT("ignore_checksums")));
        checkbox_limbo->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), wxT("ignore_limbo")));
        checkbox_metadata->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), wxT("metadata_only")));
        checkbox_garbage->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), wxT("no_garbage_collect")));
        checkbox_transport->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), wxT("no_transportable")));
        checkbox_extern->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), wxT("external_tables")));
    }
    updateControls();
}
//-----------------------------------------------------------------------------
void BackupFrame::doWriteConfigSettings(const wxString& prefix) const
{
    BackupRestoreBaseFrame::doWriteConfigSettings(prefix);
    std::vector<wxString> flags;
    if (checkbox_checksum->IsChecked())
        flags.push_back(wxT("ignore_checksums"));
    if (checkbox_limbo->IsChecked())
        flags.push_back(wxT("ignore_limbo"));
    if (checkbox_metadata->IsChecked())
        flags.push_back(wxT("metadata_only"));
    if (checkbox_garbage->IsChecked())
        flags.push_back(wxT("no_garbage_collect"));
    if (checkbox_transport->IsChecked())
        flags.push_back(wxT("no_transportable"));
    if (checkbox_extern->IsChecked())
        flags.push_back(wxT("external_tables"));
    config().setValue(prefix + Config::pathSeparator + wxT("options"), flags);
}
//-----------------------------------------------------------------------------
const wxString BackupFrame::getName() const
{
    return wxT("BackupFrame");
}
//-----------------------------------------------------------------------------
wxString BackupFrame::getFrameId(Database* db)
{
    if (db)
        return wxString(wxT("BackupFrame/") + db->getItemPath());
    else
        return wxEmptyString;
}
//-----------------------------------------------------------------------------
BackupFrame* BackupFrame::findFrameFor(Database* db)
{
    BaseFrame* bf = frameFromIdString(getFrameId(db));
    if (!bf)
        return 0;
    return dynamic_cast<BackupFrame*>(bf);
}
//-----------------------------------------------------------------------------
//! event handlers
BEGIN_EVENT_TABLE(BackupFrame, BackupRestoreBaseFrame)
    EVT_BUTTON(BackupRestoreBaseFrame::ID_button_browse, BackupFrame::OnBrowseButtonClick)
    EVT_BUTTON(BackupRestoreBaseFrame::ID_button_start, BackupFrame::OnStartButtonClick)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void BackupFrame::OnBrowseButtonClick(wxCommandEvent& WXUNUSED(event))
{
    wxFileName origName(text_ctrl_filename->GetValue());
    wxString filename = ::wxFileSelector(_("Select Backup File"),
        origName.GetPath(), origName.GetName(), wxEmptyString,
        _("All files (*.*)|*.*"), wxSAVE | wxOVERWRITE_PROMPT, this);
    if (!filename.empty())
        text_ctrl_filename->SetValue(filename);
}
//-----------------------------------------------------------------------------
void BackupFrame::OnStartButtonClick(wxCommandEvent& WXUNUSED(event))
{
    verboseMsgsM = checkbox_showlog->IsChecked();
    clearLog();

    // TODO: create a global helper function
    //   bool getDatabasePassword(wxFrame* parent, Database* db, wxString password);
    // this would simplify the next lines to
    //   if (!getDatabasePassword(this, databaseM, password))
    //       return;

    wxString password = databaseM->getPassword();
    if (password.empty())
    {
        wxString msg;
        msg.Printf(_("Enter password for user %s:"), databaseM->getUsername().c_str());
        password = ::wxGetPasswordFromUser(msg, _("Connecting To Server"));
        if (password.empty())
            return;
    }

    int flags = (int)IBPP::brVerbose; // this will be ORed in anyway
    if (checkbox_checksum->IsChecked())
        flags |= (int)IBPP::brIgnoreChecksums;
    if (checkbox_limbo->IsChecked())
        flags |= (int)IBPP::brIgnoreLimbo;
    if (checkbox_metadata->IsChecked())
        flags |= (int)IBPP::brMetadataOnly;
    if (checkbox_garbage->IsChecked())
        flags |= (int)IBPP::brNoGarbageCollect;
    if (checkbox_transport->IsChecked())
        flags |= (int)IBPP::brNonTransportable;
    if (checkbox_extern->IsChecked())
        flags |= (int)IBPP::brConvertExtTables;

    BackupThread* thread = new BackupThread(this, serverM->getHostname(),
        databaseM->getUsername(), password, databaseM->getPath(),
        text_ctrl_filename->GetValue(), (IBPP::BRF)flags);
    startThread(thread);
    updateControls();
}
//-----------------------------------------------------------------------------
