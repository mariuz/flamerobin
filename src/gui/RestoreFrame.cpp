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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

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
#include "core/StringUtils.h"
#include "frutils.h"
#include "gui/controls/DndTextControls.h"
#include "gui/controls/LogTextControl.h"
#include "gui/RestoreFrame.h"
#include "gui/StyleGuide.h"
#include "gui/UsernamePasswordDialog.h"
#include "metadata/database.h"
#include "metadata/server.h"
RestoreFrame::RestoreFrame(wxWindow* parent, DatabasePtr db)
    : BackupRestoreBaseFrame(parent, db)
{
    setIdString(this, getFrameId(db));

    wxString databaseName(db->getName_());
    wxString serverName(db->getServer()->getName_());
    SetTitle(wxString::Format(_("Restore Database \"%s:%s\""),
        serverName.c_str(), databaseName.c_str()));

    createControls();
    layoutControls();
    updateControls();

    text_ctrl_filename->SetFocus();
}

//! implementation details
void RestoreFrame::createControls()
{
    BackupRestoreBaseFrame::createControls();

    checkbox_replace = new wxCheckBox(panel_controls, wxID_ANY,
        _("Replace existing database"));
    checkbox_noshadow = new wxCheckBox(panel_controls, wxID_ANY,
        _("Don't restore shadow files"));
    checkbox_commit = new wxCheckBox(panel_controls, wxID_ANY,
        _("Commit per table"));
    checkbox_deactivate = new wxCheckBox(panel_controls, wxID_ANY,
        _("Deactivate indices"));
    checkbox_validity = new wxCheckBox(panel_controls, wxID_ANY,
        _("Ignore validity constraints"));
    checkbox_space = new wxCheckBox(panel_controls, wxID_ANY,
        _("Use all space"));

    checkbox_fix_fss_data = new wxCheckBox(panel_controls, wxID_ANY,
        _("Fix malformed UNICODE_FSS data"));
    checkbox_fix_fss_data->SetValue(false);

    checkbox_fix_fss_metadata = new wxCheckBox(panel_controls, wxID_ANY,
        _("Fix malformed UNICODE_FSS metadata"));
    checkbox_fix_fss_metadata->SetValue(false);

    checkbox_readonlyDB = new wxCheckBox(panel_controls, wxID_ANY,
        _("Read only access"));

    wxArrayString choices;
    choices.Add(_("None"));
    choices.Add(_("Read only"));
    choices.Add(_("Read write"));
    radiobox_replicamode = new wxRadioBox(panel_controls, wxID_ANY, _("Replica mode (FB4.0+)"),
        wxDefaultPosition, wxDefaultSize, choices, 3);
    radiobox_replicamode->Enable(false);


    label_pagesize = new wxStaticText(panel_controls, wxID_ANY,
        _("Page size:"));
    const wxString pagesize_choices[] = {
        _("Default"), "1024", "2048", "4096", "8192", "16384", "32768"
    };
    choice_pagesize = new wxChoice(panel_controls, wxID_ANY,
        wxDefaultPosition, wxDefaultSize,
        sizeof(pagesize_choices) / sizeof(wxString), pagesize_choices);

    spinctrl_pagebuffers = new wxSpinCtrl(panel_controls, wxID_ANY);
    spinctrl_pagebuffers->SetRange(0, 2147483646);

}

void RestoreFrame::layoutControls()
{
    BackupRestoreBaseFrame::layoutControls();

    /*std::list<wxWindow*> controls;
    controls.push_back(label_filename);
    controls.push_back(label_pagesize);
    adjustControlsMinWidth(controls);
    controls.clear();*/

    wxGridSizer* sizerChecks = new wxGridSizer(3, 3,
        styleguide().getCheckboxSpacing(),
        styleguide().getUnrelatedControlMargin(wxHORIZONTAL));
    sizerChecks->Add(checkbox_replace, 0, wxEXPAND);
    sizerChecks->Add(checkbox_deactivate, 0, wxEXPAND);
    sizerChecks->Add(checkbox_noshadow, 0, wxEXPAND);
    sizerChecks->Add(checkbox_validity, 0, wxEXPAND);
    sizerChecks->Add(checkbox_commit, 0, wxEXPAND);
    sizerChecks->Add(checkbox_space, 0, wxEXPAND);
    sizerChecks->Add(checkbox_fix_fss_data, 0, wxEXPAND);
    sizerChecks->Add(checkbox_fix_fss_metadata, 0, wxEXPAND);
    sizerChecks->Add(checkbox_readonlyDB, 0, wxEXPAND);



    wxBoxSizer* sizerCombo = new wxBoxSizer(wxHORIZONTAL);
    sizerCombo->Add(label_pagesize, 0, wxALIGN_CENTER_VERTICAL);
    sizerCombo->Add(styleguide().getControlLabelMargin(), 0);
    sizerCombo->Add(choice_pagesize, 1, wxEXPAND);
    sizerCombo->Add(styleguide().getRelatedControlMargin(wxHORIZONTAL), 0);
    {
        wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
        sizer->Add(new wxStaticText(panel_controls, wxID_ANY,
            _("Page buffers")), 0, wxALIGN_CENTER_VERTICAL);
        sizer->Add(styleguide().getControlLabelMargin(), 0);
        sizer->Add(spinctrl_pagebuffers, 1, wxALIGN_CENTER_VERTICAL);

        sizerCombo->Add(sizer);
    }




    wxBoxSizer* sizerPanelV = new wxBoxSizer(wxVERTICAL);
    sizerPanelV->Add(0, styleguide().getFrameMargin(wxTOP));
    sizerPanelV->Add(sizerFilename, 0, wxEXPAND);
    sizerPanelV->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerPanelV->Add(sizerChecks);
    sizerPanelV->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerPanelV->Add(sizerCombo);
    sizerPanelV->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));

    sizerPanelV->Add(radiobox_replicamode, 0, wxEXPAND);
    sizerPanelV->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));

    sizerPanelV->Add(sizerGeneralOptions, 0, wxEXPAND);
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

void RestoreFrame::updateControls()
{
    BackupRestoreBaseFrame::updateControls();

    bool running = getThreadRunning();
    button_browse->Enable(!running);
    text_ctrl_filename->Enable(!running);
    checkbox_replace->Enable(!running);
    checkbox_deactivate->Enable(!running);
    checkbox_noshadow->Enable(!running);
    checkbox_validity->Enable(!running);
    checkbox_commit->Enable(!running);
    checkbox_space->Enable(!running);
    choice_pagesize->Enable(!running);
    checkbox_fix_fss_data->Enable(!running);
    checkbox_fix_fss_metadata->Enable(!running);
    checkbox_readonlyDB->Enable(!running);
    spinctrl_pagebuffers->Enable(!running);
    
    //radiobox_replicamode->Enable(!running);

    DatabasePtr db = getDatabase();
    
    button_start->Enable(!running && !text_ctrl_filename->GetValue().empty()
        && db && !db->isConnected());
}

void RestoreFrame::doReadConfigSettings(const wxString& prefix)
{
    BackupRestoreBaseFrame::doReadConfigSettings(prefix);

    wxString pagesize;
    config().getValue(prefix + Config::pathSeparator + "pagesize", pagesize);
    int selindex = -1;
    if (!pagesize.empty())
        selindex = choice_pagesize->FindString(pagesize);
    // select default pagesize of 1024 if invalid selindex
    choice_pagesize->SetSelection(selindex >= 0 ? selindex : 0);

    wxArrayString flags;
    config().getValue(prefix + Config::pathSeparator + "options", flags);
    if (!flags.empty())
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
        checkbox_fix_fss_data->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), "fix_fss_data"));
        checkbox_fix_fss_metadata->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), "fix_fss_metadata"));
        checkbox_readonlyDB->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), "readonlyDB"));
    }

    int intValue = 0;
    config().getValue(prefix + Config::pathSeparator + "page_buffers", intValue);
    spinctrl_pagebuffers->SetValue(intValue);

    updateControls();
}

void RestoreFrame::doWriteConfigSettings(const wxString& prefix) const
{
    BackupRestoreBaseFrame::doWriteConfigSettings(prefix);
    
    config().setValue(prefix + Config::pathSeparator + "pagesize",
        choice_pagesize->GetStringSelection());

    wxArrayString flags;
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
    if (checkbox_fix_fss_data->IsChecked())
        flags.push_back("fix_fss_data");
    if (checkbox_fix_fss_metadata->IsChecked())
        flags.push_back("fix_fss_metadata");
    if (checkbox_readonlyDB->IsChecked())
        flags.push_back("readonlyDB");

    config().setValue(prefix + Config::pathSeparator + "options", flags);

    config().setValue(prefix + Config::pathSeparator + "page_buffers",
        spinctrl_pagebuffers->GetValue());

}

const wxString RestoreFrame::getName() const
{
    return "RestoreFrame";
}

/*static*/
wxString RestoreFrame::getFrameId(DatabasePtr db)
{
    if (db)
        return wxString("RestoreFrame/" + db->getItemPath());
    else
        return wxEmptyString;
}

RestoreFrame* RestoreFrame::findFrameFor(DatabasePtr db)
{
    BaseFrame* bf = frameFromIdString(getFrameId(db));
    if (!bf)
        return 0;
    return dynamic_cast<RestoreFrame*>(bf);
}

BEGIN_EVENT_TABLE(RestoreFrame, BackupRestoreBaseFrame)
    EVT_BUTTON(BackupRestoreBaseFrame::ID_button_browse, RestoreFrame::OnBrowseButtonClick)
    EVT_BUTTON(BackupRestoreBaseFrame::ID_button_start, RestoreFrame::OnStartButtonClick)
END_EVENT_TABLE()

void RestoreFrame::OnBrowseButtonClick(wxCommandEvent& WXUNUSED(event))
{
    wxFileName origName(text_ctrl_filename->GetValue());
    wxString filename = ::wxFileSelector(_("Select Backup File"),
        origName.GetPath(), origName.GetFullName(), "*.fbk",
        _("Backup file (*.fbk, *.gbk)|*.fbk;*.gbk|All files (*.*)|*.*"),
        wxFD_OPEN, this);
    if (!filename.empty())
        text_ctrl_filename->SetValue(filename);
}

void RestoreFrame::OnStartButtonClick(wxCommandEvent& WXUNUSED(event))
{
    verboseMsgsM = checkbox_showlog->IsChecked() || spinctrl_showlogInterval->GetValue() > 0;
    clearLog();

    DatabasePtr database = getDatabase();
    wxCHECK_RET(database,
        "Cannot restore unassigned database");
    ServerPtr server = database->getServer();
    wxCHECK_RET(server,
        "Cannot restore database without assigned server");

    wxString username;
    wxString password;
    if (!getConnectionCredentials(this, database, username, password))
        return;
    wxString rolename;
    wxString charset;
    rolename = database->getRole();
    charset = database->getConnectionCharset();

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

    if (checkbox_fix_fss_data->IsChecked())
        flags |= (int)IBPP::brFix_Fss_Data;
    if (checkbox_fix_fss_metadata->IsChecked())
        flags |= (int)IBPP::brFix_Fss_Metadata;
    if (checkbox_readonlyDB->IsChecked())
        flags |= (int)IBPP::brDatabase_readonly;

    if (checkbox_statictime->IsChecked())
        flags |= (int)IBPP::brstatistics_time;
    if (checkbox_staticdelta->IsChecked())
        flags |= (int)IBPP::brstatistics_delta;
    if (checkbox_staticpageread->IsChecked())
        flags |= (int)IBPP::brstatistics_pagereads;
    if (checkbox_staticpagewrite->IsChecked())
        flags |= (int)IBPP::brstatistics_pagewrites;

    
    /*switch (radiobox_replicamode->GetSelection())
    {
    case 0: 
        flags |= (int)IBPP::brReplicaMode_none; 
        break;
    case 1:
        flags |= (int)IBPP::brReplicaMode_readonly; 
        break;
    case 2:
        flags |= (int)IBPP::brReplicaMode_readwrite;
        break;
    }*/

    if (checkbox_metadata->IsChecked())
        flags |= (int)IBPP::brMetadataOnly;

    unsigned long pagesize;
    if (!choice_pagesize->GetStringSelection().ToULong(&pagesize))
        pagesize = 0;

    startThread(std::make_unique<RestoreThread>(this,
        server->getConnectionString(), username, password, rolename, charset,
        text_ctrl_filename->GetValue(), database->getPath(), pagesize, spinctrl_pagebuffers->GetValue(),
        (IBPP::BRF)flags, spinctrl_showlogInterval->GetValue(), spinctrl_parallelworkers->GetValue(),
        textCtrl_skipdata->GetValue(), textCtrl_includedata->GetValue(),
        textCtrl_crypt->GetValue(), textCtrl_keyholder->GetValue(), textCtrl_keyname->GetValue()
        )
    );
    updateControls();
}

RestoreThread::RestoreThread(RestoreFrame* frame, wxString 
    server, wxString username, wxString password, wxString 
    rolename, wxString charset, wxString bkfilename, wxString dbfilename, 
    int pagesize, int pagebuffers, IBPP::BRF flags, int interval, int parallel,
    wxString skipData, wxString includeData, wxString cryptPluginName, wxString keyPlugin, 
    wxString keyEncrypt)
    :pagesizeM(pagesize), pagebuffersM(pagebuffers),
    BackupRestoreThread(frame, server, username, password, rolename, charset,
        dbfilename, bkfilename, flags, interval, parallel, skipData, includeData, cryptPluginName,
        keyPlugin, keyEncrypt)

{
}

void RestoreThread::Execute(IBPP::Service svc)
{
    svc->StartRestore(wx2std(bkfileM), wx2std(dbfileM), wx2std(outputFileM),
        pagesizeM, pagebuffersM, brfM,
        wx2std(cryptPluginNameM), wx2std(keyPluginM),
        wx2std(keyEncryptM), wx2std(skipDataM), wx2std(includeDataM), 
        intervalM, parallelM
    );
}
