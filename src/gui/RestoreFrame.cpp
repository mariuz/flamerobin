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

  Contributor(s):
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

#include <algorithm>
#include <wx/datetime.h>
#include "ibpp.h"

#include "RestoreFrame.h"
#include "config.h"
#include "frutils.h"
#include "styleguide.h"
#include "ugly.h"

//-----------------------------------------------------------------------------
// worker thread class to perform database restore
class RestoreThread: public wxThread {
public:
    RestoreThread(RestoreFrame* frame, std::string server, std::string username,
        std::string password, std::string bkfilename, std::string dbfilename,
        int pagesize, IBPP::BRF flags);

    virtual void* Entry();
    virtual void OnExit();
private:
    RestoreFrame* frameM;
    std::string serverM;
    std::string usernameM;
    std::string passwordM;
    std::string bkfileM;
    std::string dbfileM;
    int pagesizeM;
    IBPP::BRF brfM;
    void logError(wxString& msg);
    void logImportant(wxString& msg);
    void logProgress(wxString& msg);
};
//-----------------------------------------------------------------------------
RestoreThread::RestoreThread(RestoreFrame* frame, std::string server, std::string username,
    std::string password, std::string bkfilename, std::string dbfilename, int pagesize, IBPP::BRF flags):
    wxThread()
{
    frameM = frame;
    serverM = server;
    usernameM = username;
    passwordM = password;
    bkfileM = bkfilename;
    dbfileM = dbfilename;
    pagesizeM = pagesize;
    // always use verbose flag to make cancelling possible
    brfM = (IBPP::BRF)((int)flags | (int)IBPP::brVerbose);
}
//-----------------------------------------------------------------------------
void* RestoreThread::Entry()
{
    wxDateTime now;
    wxString msg;

    try
    {
        msg.Printf(_("Connecting to server %s..."), serverM.c_str());
        logImportant(msg);
        IBPP::Service svc = IBPP::ServiceFactory(serverM, usernameM, passwordM);
		svc->Connect();

        now = wxDateTime::Now();
        msg.Printf(_("Database restore started %s"), now.FormatTime().c_str());
        logImportant(msg);
        svc->StartRestore(bkfileM, dbfileM, pagesizeM, brfM);
        while (true)
        {
            if (TestDestroy())
            {
                now = wxDateTime::Now();
                msg.Printf(_("Database restore cancelled %s"), now.FormatTime().c_str());
                logImportant(msg);
                break;
            }
            const char* c = svc->WaitMsg();
            if (c == 0)
            {
                now = wxDateTime::Now();
                msg.Printf(_("Database restore finished %s"), now.FormatTime().c_str());
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
        msg.Printf(_("Database restore cancelled %s due to IBPP exception:\n\n"), now.FormatTime().c_str());
        msg += std2wx(e.ErrorMessage());
        logError(msg);
    }
    catch (...)
    {
        now = wxDateTime::Now();
        msg.Printf(_("Database restore cancelled %s due to exception"), now.FormatTime().c_str());
        logError(msg);
    }
    return 0;
}
//-----------------------------------------------------------------------------
void RestoreThread::OnExit()
{
    if (frameM != 0)
    {
        wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, BackupRestoreBaseFrame::ID_thread_finished);
        wxPostEvent(frameM, event);
    }
}
//-----------------------------------------------------------------------------
void RestoreThread::logError(wxString& msg)
{
    if (frameM != 0)
        frameM->threadOutputMsg(msg, BackupRestoreBaseFrame::error_message);
}
//-----------------------------------------------------------------------------
void RestoreThread::logImportant(wxString& msg)
{
    if (frameM != 0)
        frameM->threadOutputMsg(msg, BackupRestoreBaseFrame::important_message);
}
//-----------------------------------------------------------------------------
void RestoreThread::logProgress(wxString& msg)
{
    if (frameM != 0)
        frameM->threadOutputMsg(msg, BackupRestoreBaseFrame::progress_message);
}
//-----------------------------------------------------------------------------
RestoreFrame::RestoreFrame(wxWindow* parent, YDatabase* db):
    BackupRestoreBaseFrame(parent, db)
{
    wxString s;
    s.Printf(_("Restore Database \"%s:%s\""),
        serverM->getName().c_str(), databaseM->getName().c_str());
	SetTitle(s);

    panel_controls = new wxPanel(this, -1, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL|wxCLIP_CHILDREN|wxNO_FULL_REPAINT_ON_RESIZE);
    label_filename = new wxStaticText(panel_controls, -1, _("Backup file:"));
    text_ctrl_filename = new wxTextCtrl(panel_controls, ID_text_ctrl_filename, _(""));
    button_browse = new wxButton(panel_controls, ID_button_browse, _("..."), wxDefaultPosition,
        wxDefaultSize, wxBU_EXACTFIT);

    checkbox_replace = new wxCheckBox(panel_controls, -1, _("Replace existing database"));
    checkbox_deactivate = new wxCheckBox(panel_controls, -1, _("Deactivate indices"));
    checkbox_noshadow = new wxCheckBox(panel_controls, -1, _("Don't restore shadow files"));
    checkbox_validity = new wxCheckBox(panel_controls, -1, _("Ignore validity constraints"));
    checkbox_commit = new wxCheckBox(panel_controls, -1, _("Commit per table"));
    checkbox_space = new wxCheckBox(panel_controls, -1, _("Use all space"));
    label_pagesize = new wxStaticText(panel_controls, -1, _("Page size:"));
    const wxString pagesize_choices[] = {
        wxT("1024"), wxT("2048"), wxT("4096"), wxT("8192"), wxT("16384")
    };
    combobox_pagesize = new wxComboBox(panel_controls, -1, wxT(""), wxDefaultPosition, wxDefaultSize,
        sizeof(pagesize_choices) / sizeof(wxString), pagesize_choices, wxCB_DROPDOWN|wxCB_READONLY);

    combobox_showlog = new wxCheckBox(panel_controls, ID_checkbox_showlog, _("Show complete log"));
    button_start = new wxButton(panel_controls, ID_button_start, _("Restore"));
    button_cancel = new wxButton(panel_controls, ID_button_cancel, _("Cancel"));

    text_ctrl_log = new wxStyledTextCtrl(this, ID_text_ctrl_log);

    do_layout();
    updateControls();
	setupControls();
}
//-----------------------------------------------------------------------------
void RestoreFrame::do_layout()
{
    int wh = text_ctrl_filename->GetMinHeight();
    button_browse->SetSize(wh, wh);

    std::list<wxWindow*> controls;
    controls.push_back(label_filename);
    controls.push_back(label_pagesize);
    adjustControlsMinWidth(controls);
    controls.clear();

    wxBoxSizer* sizerFilename = new wxBoxSizer(wxHORIZONTAL);
    sizerFilename->Add(label_filename, 0, wxALIGN_CENTER_VERTICAL);
	sizerFilename->Add(styleguide().getControlLabelMargin(), 0);
    sizerFilename->Add(text_ctrl_filename, 1, wxALIGN_CENTER_VERTICAL);
    sizerFilename->Add(styleguide().getBrowseButtonMargin(), 0);
    sizerFilename->Add(button_browse, 0, wxALIGN_CENTER_VERTICAL);

    wxGridSizer* sizerChecks = new wxGridSizer(2, 2,
        styleguide().getCheckboxSpacing(),
        styleguide().getUnrelatedControlMargin(wxHORIZONTAL));
    sizerChecks->Add(checkbox_replace);
    sizerChecks->Add(checkbox_deactivate);
    sizerChecks->Add(checkbox_noshadow);
    sizerChecks->Add(checkbox_validity);
    sizerChecks->Add(checkbox_commit);
    sizerChecks->Add(checkbox_space);

    wxBoxSizer* sizerCombo = new wxBoxSizer(wxHORIZONTAL);
    sizerCombo->Add(label_pagesize, 0, wxALIGN_CENTER_VERTICAL);
    sizerCombo->Add(styleguide().getControlLabelMargin(), 0);
    sizerCombo->Add(combobox_pagesize, 1, wxEXPAND);

    wxBoxSizer* sizerButtons = new wxBoxSizer(wxHORIZONTAL);
    sizerButtons->Add(combobox_showlog, 0, wxALIGN_CENTER_VERTICAL);
    sizerButtons->Add(0, 0, 1, wxEXPAND);
    sizerButtons->Add(button_start);
    sizerButtons->Add(styleguide().getBetweenButtonsMargin(wxHORIZONTAL), 0);
    sizerButtons->Add(button_cancel);

    wxBoxSizer* sizerPanelV = new wxBoxSizer(wxVERTICAL);
    sizerPanelV->Add(0, styleguide().getFrameMargin(wxTOP));
    sizerPanelV->Add(sizerFilename, 0, wxEXPAND);
    sizerPanelV->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerPanelV->Add(sizerChecks);
    sizerPanelV->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerPanelV->Add(sizerCombo);
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
    // show at least one line of text
    sizerMain->SetItemMinSize(text_ctrl_log, -1, text_ctrl_filename->GetSize().GetHeight());
    SetSizerAndFit(sizerMain);
}
//-----------------------------------------------------------------------------
void RestoreFrame::doReadConfigSettings(const std::string& prefix)
{
    BackupRestoreBaseFrame::doReadConfigSettings(prefix);

    std::string pagesize;
    int selindex = -1;
    if (config().getValue(prefix + "::pagesize", pagesize) && !pagesize.empty())
        selindex = combobox_pagesize->FindString(std2wx(pagesize));
    // select default pagesize of 1024 if invalid selindex
    combobox_pagesize->SetSelection(selindex >= 0 ? selindex : 0);

    std::vector<std::string> flags;
    if (config().getValue(prefix + "::options", flags) && !flags.empty())
    {
        checkbox_replace->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), "replace"));
        checkbox_deactivate->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), "deactivate_indices"));
        checkbox_noshadow->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), "no_shadow"));
        checkbox_validity->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), "no_constraints"));
        checkbox_commit->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), "commit_per_table"));
        checkbox_space->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), "use_all_space"));
    }
    updateControls();
}
//-----------------------------------------------------------------------------
void RestoreFrame::doWriteConfigSettings(const std::string& prefix) const
{
    BackupRestoreBaseFrame::doWriteConfigSettings(prefix);
    config().setValue(prefix + "::pagesize", wx2std(combobox_pagesize->GetStringSelection()));

    std::vector<std::string> flags;
    if (checkbox_replace->IsChecked())
        flags.push_back("replace");
    if (checkbox_deactivate->IsChecked())
        flags.push_back("deactivate_indices");
    if (checkbox_noshadow->IsChecked())
        flags.push_back("no_shadow");
    if (checkbox_validity->IsChecked())
        flags.push_back("no_constraints");
    if (checkbox_commit->IsChecked())
        flags.push_back("commit_per_table");
    if (checkbox_space->IsChecked())
        flags.push_back("use_all_space");
    config().setValue(prefix + "::options", flags);
}
//-----------------------------------------------------------------------------
const std::string RestoreFrame::getName() const
{
    return "RestoreFrame";
}
//-----------------------------------------------------------------------------
void RestoreFrame::updateControls()
{
    bool running = threadM != 0;
    button_browse->Enable(!running);
    text_ctrl_filename->Enable(!running);
    checkbox_replace->Enable(!running);
    checkbox_deactivate->Enable(!running);
    checkbox_noshadow->Enable(!running);
    checkbox_validity->Enable(!running);
    checkbox_commit->Enable(!running);
    checkbox_space->Enable(!running);
    combobox_pagesize->Enable(!running);
    button_start->Enable(!running && !text_ctrl_filename->GetValue().empty());
    button_cancel->Enable(running);
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(RestoreFrame, BackupRestoreBaseFrame)
    EVT_BUTTON(BackupRestoreBaseFrame::ID_button_browse, RestoreFrame::OnBrowseButtonClick)
    EVT_BUTTON(BackupRestoreBaseFrame::ID_button_start, RestoreFrame::OnStartButtonClick)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void RestoreFrame::OnBrowseButtonClick(wxCommandEvent& WXUNUSED(event))
{
	wxString filename = ::wxFileSelector(_("Select Backup File"), wxT(""), wxT(""),
        wxT(""), _("All files (*.*)|*.*"), 0, this);
	if (!filename.empty())
		text_ctrl_filename->SetValue(filename);
}
//-----------------------------------------------------------------------------
void RestoreFrame::OnStartButtonClick(wxCommandEvent& WXUNUSED(event))
{
    verboseMsgsM = combobox_showlog->IsChecked();
    clearLog();

// TODO: create a global helper function
//   bool getDatabasePassword(wxFrame* parent, YDatabase* db, wxString password);
// this would simplify the next lines to
//   if (!getDatabasePassword(this, databaseM, password))
//       return;

    std::string password = databaseM->getPassword();
    if (password.empty())
    {
		wxString msg;
        msg.Printf(_("Enter password for user %s:"), databaseM->getUsername().c_str());
		password = wx2std(::wxGetPasswordFromUser(msg, _("Connecting To Server")));
        if (password.empty())
            return;
    }

    int flags = (int)IBPP::brVerbose; // this will be ORed in anyway
    if (checkbox_replace->IsChecked())
        flags |= (int)IBPP::brReplace;
    if (checkbox_deactivate->IsChecked())
        flags |= (int)IBPP::brDeactivateIdx;
    if (checkbox_noshadow->IsChecked())
        flags |= (int)IBPP::brNoShadow;
    if (checkbox_validity->IsChecked())
        flags |= (int)IBPP::brNoValidity;
    if (checkbox_commit->IsChecked())
        flags |= (int)IBPP::brPerTableCommit;
    if (checkbox_space->IsChecked())
        flags |= (int)IBPP::brUseAllSpace;

    unsigned long pagesize;
    if (!combobox_pagesize->GetValue().ToULong(&pagesize))
        pagesize = 0;

    RestoreThread* thread = new RestoreThread(this, serverM->getName(), databaseM->getUsername(),
        password, wx2std(text_ctrl_filename->GetValue()), databaseM->getPath(), pagesize, (IBPP::BRF)flags);
    startThread(thread);
    updateControls();
}
//-----------------------------------------------------------------------------
