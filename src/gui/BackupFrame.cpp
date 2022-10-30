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
#include <wx/spinctrl.h>

#include <algorithm>

#include <ibpp.h>

#include "core/StringUtils.h"
#include "config/Config.h"
#include "gui/BackupFrame.h"
#include "gui/controls/DndTextControls.h"
#include "gui/controls/LogTextControl.h"
#include "gui/StyleGuide.h"
#include "gui/UsernamePasswordDialog.h"
#include "metadata/database.h"
#include "metadata/server.h"
#include "ShutdownFrame.h"

BackupFrame::BackupFrame(wxWindow* parent, DatabasePtr db)
    : BackupRestoreBaseFrame(parent, db)
{
    setIdString(this, getFrameId(db));

    wxString databaseName(db->getName_());
    wxString serverName(db->getServer()->getName_());
    SetTitle(wxString::Format(_("Backup Database \"%s:%s\""),
        serverName.c_str(), databaseName.c_str()));

    createControls();
    layoutControls();
    updateControls();

    text_ctrl_filename->SetFocus();
}

//! implementation details
void BackupFrame::createControls()
{
    BackupRestoreBaseFrame::createControls();

    checkbox_checksum = new wxCheckBox(panel_controls, wxID_ANY,
        _("Ignore checksums"));
    checkbox_limbo = new wxCheckBox(panel_controls, wxID_ANY,
        _("Ignore limbo transactions"));
    checkbox_transport = new wxCheckBox(panel_controls, wxID_ANY,
        _("Use non-transportable format"));
    checkbox_garbage = new wxCheckBox(panel_controls, wxID_ANY,
        _("Don't perform garbage collection"));
    checkbox_extern = new wxCheckBox(panel_controls, wxID_ANY,
        _("Convert external tables"));
    checkbox_expand = new wxCheckBox(panel_controls, wxID_ANY,
        _("No data compression"));
    checkbox_olddescription = new wxCheckBox(panel_controls, wxID_ANY,
        _("Save old style metadata descriptions"));
    checkbox_noDBtrigger = new wxCheckBox(panel_controls, wxID_ANY,
        _("Do not run database triggers (FB2.5+)"));
    checkbox_zip = new wxCheckBox(panel_controls, wxID_ANY,
        _("Zip compressed format (FB4.0+)"));



}

void BackupFrame::layoutControls()
{
    
    BackupRestoreBaseFrame::layoutControls();

    wxGridSizer* sizerChecks = new wxGridSizer(3, 3,
        styleguide().getCheckboxSpacing(),
        styleguide().getUnrelatedControlMargin(wxHORIZONTAL));
    sizerChecks->Add(checkbox_checksum, 0, wxEXPAND);
    sizerChecks->Add(checkbox_limbo, 0, wxEXPAND);
    sizerChecks->Add(checkbox_garbage, 0, wxEXPAND);
    sizerChecks->Add(checkbox_transport, 0, wxEXPAND);
    sizerChecks->Add(checkbox_extern, 0, wxEXPAND);
    sizerChecks->Add(checkbox_expand, 0, wxEXPAND);
    sizerChecks->Add(checkbox_olddescription, 0, wxEXPAND);
    sizerChecks->Add(checkbox_noDBtrigger, 0, wxEXPAND);
    sizerChecks->Add(checkbox_zip, 0, wxEXPAND);



    wxBoxSizer* sizerPanelV = new wxBoxSizer(wxVERTICAL);
    sizerPanelV->Add(0, styleguide().getFrameMargin(wxTOP));
    sizerPanelV->Add(sizerFilename, 0, wxEXPAND);
    sizerPanelV->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerPanelV->Add(sizerChecks);
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

void BackupFrame::updateControls()
{
    BackupRestoreBaseFrame::updateControls();

    bool running = getThreadRunning();


    checkbox_checksum->Enable(!running);
    checkbox_limbo->Enable(!running);
    checkbox_garbage->Enable(!running);
    checkbox_transport->Enable(!running);
    checkbox_extern->Enable(!running);

    checkbox_expand->Enable(!running);
    checkbox_olddescription->Enable(!running);
    checkbox_noDBtrigger->Enable(!running);
    checkbox_zip->Enable(!running);

    button_start->Enable(!running && !text_ctrl_filename->GetValue().empty());
}

void BackupFrame::doReadConfigSettings(const wxString& prefix)
{
    BackupRestoreBaseFrame::doReadConfigSettings(prefix);
    wxArrayString flags;
    config().getValue(prefix + Config::pathSeparator + "options", flags);
    if (!flags.empty())
    {
        checkbox_checksum->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), "ignore_checksums"));
        checkbox_limbo->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), "ignore_limbo"));
        checkbox_metadata->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), "metadata_only"));
        checkbox_garbage->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), "no_garbage_collect"));
        checkbox_transport->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), "no_transportable"));
        checkbox_extern->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), "external_tables"));

        checkbox_expand->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), "no_data_compression"));
        checkbox_olddescription->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), "old_description"));
        checkbox_noDBtrigger->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), "no_db_triggers"));
        checkbox_zip->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), "compressed_format"));

    }
    updateControls();
}

void BackupFrame::doWriteConfigSettings(const wxString& prefix) const
{
    BackupRestoreBaseFrame::doWriteConfigSettings(prefix);

    wxArrayString flags;
    if (checkbox_checksum->IsChecked())
        flags.push_back("ignore_checksums");
    if (checkbox_limbo->IsChecked())
        flags.push_back("ignore_limbo");
    if (checkbox_metadata->IsChecked())
        flags.push_back("metadata_only");
    if (checkbox_garbage->IsChecked())
        flags.push_back("no_garbage_collect");
    if (checkbox_transport->IsChecked())
        flags.push_back("no_transportable");
    if (checkbox_extern->IsChecked())
        flags.push_back("external_tables");

    if (checkbox_expand->IsChecked())
        flags.push_back("no_data_compression");
    if (checkbox_olddescription->IsChecked())
        flags.push_back("old_description");
    if (checkbox_noDBtrigger->IsChecked())
        flags.push_back("no_db_triggers");
    if (checkbox_zip->IsChecked())
        flags.push_back("compressed_format");

    config().setValue(prefix + Config::pathSeparator + "options", flags);
}

const wxString BackupFrame::getName() const
{
    return "BackupFrame";
}

/*static*/
wxString BackupFrame::getFrameId(DatabasePtr db)
{
    if (db)
        return wxString("BackupFrame/" + db->getItemPath());
    else
        return wxEmptyString;
}

BackupFrame* BackupFrame::findFrameFor(DatabasePtr db)
{
    BaseFrame* bf = frameFromIdString(getFrameId(db));
    if (!bf)
        return 0;
    return dynamic_cast<BackupFrame*>(bf);
}

//! event handlers
BEGIN_EVENT_TABLE(BackupFrame, BackupRestoreBaseFrame)
    EVT_BUTTON(BackupRestoreBaseFrame::ID_button_browse, BackupFrame::OnBrowseButtonClick)
    EVT_BUTTON(BackupRestoreBaseFrame::ID_button_start, BackupFrame::OnStartButtonClick)
END_EVENT_TABLE()

void BackupFrame::OnBrowseButtonClick(wxCommandEvent& WXUNUSED(event))
{
    wxFileName origName(text_ctrl_filename->GetValue());
    wxString filename = ::wxFileSelector(_("Select Backup File"),
        origName.GetPath(), origName.GetFullName(), "*.fbk",
        _("Backup file (*.fbk)|*.fbk|All files (*.*)|*.*"),
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
    if (!filename.empty())
        text_ctrl_filename->SetValue(filename);
}

void BackupFrame::OnStartButtonClick(wxCommandEvent& WXUNUSED(event))
{
    verboseMsgsM = checkbox_showlog->IsChecked() || spinctrl_showlogInterval->GetValue() > 0;
    clearLog();

    DatabasePtr database = getDatabase();
    wxCHECK_RET(database,
        "Cannot backup unassigned database");
    ServerPtr server = database->getServer();
    wxCHECK_RET(server,
        "Cannot backup database without assigned server");

    wxString username;
    wxString password;
    if (!getConnectionCredentials(this, database, username, password))
        return;
    wxString rolename;
    wxString charset;
    rolename = database->getRole();
    charset = database->getConnectionCharset();

    int flags = (int)IBPP::brVerbose; // this will be ORed in anyway
    if (checkbox_checksum->IsChecked())
        flags |= (int)IBPP::brIgnoreChecksums;
    if (checkbox_limbo->IsChecked())
        flags |= (int)IBPP::brIgnoreLimbo;
    if (checkbox_garbage->IsChecked())
        flags |= (int)IBPP::brNoGarbageCollect;
    if (checkbox_transport->IsChecked())
        flags |= (int)IBPP::brNonTransportable;
    if (checkbox_extern->IsChecked())
        flags |= (int)IBPP::brConvertExtTables;
    if (checkbox_expand->IsChecked())
        flags |= (int)IBPP::brConvertExtTables;
    if (checkbox_olddescription->IsChecked())
        flags |= (int)IBPP::brOldDescriptions;
    if (checkbox_noDBtrigger->IsChecked())
        flags |= (int)IBPP::brNoDBTriggers;
    if (checkbox_zip->IsChecked())
        flags |= (int)IBPP::brZip;

    if (checkbox_metadata->IsChecked())
        flags |= (int)IBPP::brMetadataOnly;

    if (checkbox_statictime->IsChecked())
        flags |= (int)IBPP::brstatistics_time;
    if (checkbox_staticdelta->IsChecked())
        flags |= (int)IBPP::brstatistics_delta;
    if (checkbox_staticpageread->IsChecked())
        flags |= (int)IBPP::brstatistics_pagereads;
    if (checkbox_staticpagewrite->IsChecked())
        flags |= (int)IBPP::brstatistics_pagewrites;

    startThread(std::make_unique<BackupThread>(this,
        server->getConnectionString(), username, password, rolename, charset,
        database->getPath(), text_ctrl_filename->GetValue(),
        (IBPP::BRF)flags, spinctrl_showlogInterval->GetValue(), spinctrl_parallelworkers->GetValue(),
        textCtrl_skipdata->GetValue(), textCtrl_includedata->GetValue(), 
        textCtrl_crypt->GetValue(), textCtrl_keyholder->GetValue(), textCtrl_keyname->GetValue()
        )
    );
    
    updateControls();
}

BackupThread::BackupThread(BackupFrame* frame,
    wxString server, wxString username, wxString password,
    wxString rolename, wxString charset, wxString dbfilename,
    wxString bkfilename, IBPP::BRF flags, int interval, int parallel,
    wxString skipData, wxString includeData, wxString cryptPluginName,
    wxString keyPlugin, wxString keyEncrypt)
    :factorM(0),
    BackupRestoreThread(frame, server, username, password,rolename, charset, 
        dbfilename,bkfilename, flags, interval, parallel, skipData, includeData, cryptPluginName,
        keyPlugin, keyEncrypt)
{
}

void BackupThread::Execute(IBPP::Service svc)
{
    svc->StartBackup(wx2std(dbfileM), wx2std(bkfileM), wx2std(outputFileM),
        factorM, brfM, wx2std(cryptPluginNameM), wx2std(keyPluginM),
        wx2std(keyEncryptM), wx2std(skipDataM), wx2std(includeDataM), 
        intervalM, parallelM
    );
}
