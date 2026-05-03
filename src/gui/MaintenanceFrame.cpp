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

#include <wx/spinctrl.h>

#include <algorithm>

#include "config/Config.h"
#include "gui/MaintenanceFrame.h"
#include "gui/controls/LogTextControl.h"
#include "gui/StyleGuide.h"
#include "gui/UsernamePasswordDialog.h"
#include "metadata/database.h"
#include "metadata/server.h"

MaintenanceFrame::MaintenanceFrame(wxWindow* parent, DatabasePtr db)
    : ServiceBaseFrame(parent, db)
{
    setIdString(this, getFrameId(db));

    wxString databaseName(db->getName_());
    wxString serverName(db->getServer()->getName_());
    SetTitle(wxString::Format(_("Database Maintenance \"%s:%s\""),
        serverName.c_str(), databaseName.c_str()));

    createControls();
    layoutControls();
    updateControls();
}

void MaintenanceFrame::createControls()
{
    ServiceBaseFrame::createControls();

    checkbox_sweep = new wxCheckBox(panel_controls, wxID_ANY,
        _("Sweep (Force garbage collection)"));
    checkbox_validate = new wxCheckBox(panel_controls, wxID_ANY,
        _("Validate pages"));
    checkbox_full = new wxCheckBox(panel_controls, wxID_ANY,
        _("Full validation (Validate records)"));
    checkbox_mend = new wxCheckBox(panel_controls, wxID_ANY,
        _("Mend (Prepare for backup)"));
    checkbox_readonly = new wxCheckBox(panel_controls, wxID_ANY,
        _("Read-only validation (Do not fix errors)"));
    checkbox_ignore_checksums = new wxCheckBox(panel_controls, wxID_ANY,
        _("Ignore checksums"));
    checkbox_kill_shadows = new wxCheckBox(panel_controls, wxID_ANY,
        _("Kill unavailable shadows"));

    spinctrl_parallelworkers = new wxSpinCtrl(panel_controls, wxID_ANY);
    spinctrl_parallelworkers->SetRange(0, 32767);
}

void MaintenanceFrame::layoutControls()
{
    ServiceBaseFrame::layoutControls();

    wxGridSizer* sizerChecks = new wxGridSizer(4, 2,
        styleguide().getCheckboxSpacing(),
        styleguide().getUnrelatedControlMargin(wxHORIZONTAL));
    sizerChecks->Add(checkbox_sweep, 0, wxEXPAND);
    sizerChecks->Add(checkbox_validate, 0, wxEXPAND);
    sizerChecks->Add(checkbox_full, 0, wxEXPAND);
    sizerChecks->Add(checkbox_mend, 0, wxEXPAND);
    sizerChecks->Add(checkbox_readonly, 0, wxEXPAND);
    sizerChecks->Add(checkbox_ignore_checksums, 0, wxEXPAND);
    sizerChecks->Add(checkbox_kill_shadows, 0, wxEXPAND);

    wxBoxSizer* sizerParallel = new wxBoxSizer(wxHORIZONTAL);
    sizerParallel->Add(new wxStaticText(panel_controls, wxID_ANY,
        _("Parallel workers (FB5.0+)")), 0, wxALIGN_CENTER_VERTICAL);
    sizerParallel->Add(styleguide().getControlLabelMargin(), 0);
    sizerParallel->Add(spinctrl_parallelworkers, 1, wxALIGN_CENTER_VERTICAL);

    wxBoxSizer* sizerPanelV = new wxBoxSizer(wxVERTICAL);
    sizerPanelV->Add(0, styleguide().getFrameMargin(wxTOP));
    sizerPanelV->Add(sizerChecks);
    sizerPanelV->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerPanelV->Add(sizerParallel, 0, wxEXPAND);
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

    SetSizerAndFit(sizerMain);
}

void MaintenanceFrame::updateControls()
{
    ServiceBaseFrame::updateControls();
    bool running = getThreadRunning();

    checkbox_sweep->Enable(!running);
    checkbox_validate->Enable(!running);
    checkbox_full->Enable(!running);
    checkbox_mend->Enable(!running);
    checkbox_readonly->Enable(!running);
    checkbox_ignore_checksums->Enable(!running);
    checkbox_kill_shadows->Enable(!running);
    spinctrl_parallelworkers->Enable(!running);

    button_start->Enable(!running);
}

void MaintenanceFrame::doReadConfigSettings(const wxString& prefix)
{
    BaseFrame::doReadConfigSettings(prefix);
    wxArrayString flags;
    config().getValue(prefix + Config::pathSeparator + "options", flags);
    if (!flags.empty())
    {
        checkbox_sweep->SetValue(flags.Index("sweep") != wxNOT_FOUND);
        checkbox_validate->SetValue(flags.Index("validate") != wxNOT_FOUND);
        checkbox_full->SetValue(flags.Index("full") != wxNOT_FOUND);
        checkbox_mend->SetValue(flags.Index("mend") != wxNOT_FOUND);
        checkbox_readonly->SetValue(flags.Index("readonly") != wxNOT_FOUND);
        checkbox_ignore_checksums->SetValue(flags.Index("ignore_checksums") != wxNOT_FOUND);
        checkbox_kill_shadows->SetValue(flags.Index("kill_shadows") != wxNOT_FOUND);
    }
    int parallel = 0;
    config().getValue(prefix + Config::pathSeparator + "parallel", parallel);
    spinctrl_parallelworkers->SetValue(parallel);
    updateControls();
}

void MaintenanceFrame::doWriteConfigSettings(const wxString& prefix) const
{
    BaseFrame::doWriteConfigSettings(prefix);
    wxArrayString flags;
    if (checkbox_sweep->IsChecked()) flags.push_back("sweep");
    if (checkbox_validate->IsChecked()) flags.push_back("validate");
    if (checkbox_full->IsChecked()) flags.push_back("full");
    if (checkbox_mend->IsChecked()) flags.push_back("mend");
    if (checkbox_readonly->IsChecked()) flags.push_back("readonly");
    if (checkbox_ignore_checksums->IsChecked()) flags.push_back("ignore_checksums");
    if (checkbox_kill_shadows->IsChecked()) flags.push_back("kill_shadows");
    config().setValue(prefix + Config::pathSeparator + "options", flags);
    config().setValue(prefix + Config::pathSeparator + "parallel", spinctrl_parallelworkers->GetValue());
}

const wxString MaintenanceFrame::getName() const
{
    return "MaintenanceFrame";
}

/*static*/
wxString MaintenanceFrame::getFrameId(DatabasePtr db)
{
    if (db)
        return wxString("MaintenanceFrame/" + db->getItemPath());
    return wxEmptyString;
}

MaintenanceFrame* MaintenanceFrame::findFrameFor(DatabasePtr db)
{
    BaseFrame* bf = frameFromIdString(getFrameId(db));
    return dynamic_cast<MaintenanceFrame*>(bf);
}

BEGIN_EVENT_TABLE(MaintenanceFrame, ServiceBaseFrame)
    EVT_BUTTON(ServiceBaseFrame::ID_button_start, MaintenanceFrame::OnStartButtonClick)
END_EVENT_TABLE()

void MaintenanceFrame::OnStartButtonClick(wxCommandEvent& WXUNUSED(event))
{
    clearLog();
    DatabasePtr database = getDatabase();
    if (!database) return;
    ServerPtr server = database->getServer();
    if (!server) return;

    wxString username, password;
    if (!getConnectionCredentials(this, database, username, password))
        return;

    fr::MaintenanceConfig config;
    config.dbPath = wx2std(database->getPath());
    config.parallel = spinctrl_parallelworkers->GetValue();
    
    int flags = (int)fr::MaintenanceFlags::None;
    if (checkbox_sweep->IsChecked()) flags |= (int)fr::MaintenanceFlags::Sweep;
    if (checkbox_validate->IsChecked()) flags |= (int)fr::MaintenanceFlags::Validate;
    if (checkbox_full->IsChecked()) flags |= (int)fr::MaintenanceFlags::Full;
    if (checkbox_mend->IsChecked()) flags |= (int)fr::MaintenanceFlags::Mend;
    if (checkbox_readonly->IsChecked()) flags |= (int)fr::MaintenanceFlags::ReadOnly;
    if (checkbox_ignore_checksums->IsChecked()) flags |= (int)fr::MaintenanceFlags::IgnoreChecksums;
    if (checkbox_kill_shadows->IsChecked()) flags |= (int)fr::MaintenanceFlags::KillShadows;
    config.flags = (fr::MaintenanceFlags)flags;

    startThread(std::make_unique<MaintenanceThread>(this,
        server->getConnectionString(), username, password,
        database->getRole(), database->getConnectionCharset(),
        config));
    updateControls();
}

MaintenanceThread::MaintenanceThread(MaintenanceFrame* frame,
    wxString server, wxString username, wxString password,
    wxString rolename, wxString charset,
    const fr::MaintenanceConfig& config)
    : ServiceThread(frame, server, username, password, rolename, charset),
    configM(config)
{
}

void MaintenanceThread::Execute(fr::IServicePtr svc)
{
    svc->maintain(configM);
}

wxString MaintenanceThread::getOperationName() const
{
    return _("maintenance");
}
