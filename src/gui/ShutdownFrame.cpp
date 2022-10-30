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
#include "gui/ShutdownFrame.h"
#include "gui/controls/DndTextControls.h"
#include "gui/controls/LogTextControl.h"
#include "gui/StyleGuide.h"
#include "gui/UsernamePasswordDialog.h"
#include "metadata/database.h"
#include "metadata/server.h"
ShutdownFrame::ShutdownFrame(wxWindow* parent, DatabasePtr db)
    : ShutdownStartupBaseFrame(parent, db)
{
    setIdString(this, getFrameId(db));

    wxString databaseName(db->getName_());
    wxString serverName(db->getServer()->getName_());
    SetTitle(wxString::Format(_("Shutdown Database \"%s:%s\""),
        serverName.c_str(), databaseName.c_str()));

    createControls();
    layoutControls();
    updateControls();

    //text_ctrl_filename->SetFocus();
}

//! implementation details
void ShutdownFrame::createControls()
{
    ShutdownStartupBaseFrame::createControls();

    button_start->SetLabelText(_("Start Shutdown"));

    checkbox_tran = new wxCheckBox(panel_controls, wxID_ANY,
        _("Prevents any new transactions"));
    checkbox_attach = new wxCheckBox(panel_controls, wxID_ANY,
        _("Prevents any new connections"));
    checkbox_force = new wxCheckBox(panel_controls, wxID_ANY,
        _("No new connections or transactions "));

    label_timeout = new wxStaticText(panel_controls, wxID_ANY, _("Timeout"),
        wxDefaultPosition, wxDefaultSize);

    spinctrl_timeout = new wxSpinCtrl(panel_controls, wxID_ANY);
    spinctrl_timeout->SetRange(0, 32767);
}

void ShutdownFrame::layoutControls()
{
    ShutdownStartupBaseFrame::layoutControls();

    wxBoxSizer* sizerTimeout = new wxBoxSizer(wxHORIZONTAL);
    sizerTimeout->Add(label_timeout, 0, wxALIGN_CENTER_VERTICAL);
    sizerTimeout->Add(styleguide().getControlLabelMargin(), 0);
    sizerTimeout->Add(spinctrl_timeout, 0, wxALIGN_CENTER_VERTICAL);

    wxGridSizer* sizerChecks = new wxGridSizer(3, 1,
        styleguide().getCheckboxSpacing(),
        styleguide().getUnrelatedControlMargin(wxHORIZONTAL));
    sizerChecks->Add(checkbox_tran, 0, wxEXPAND);
    sizerChecks->Add(checkbox_attach, 0, wxEXPAND);
    sizerChecks->Add(checkbox_force, 0, wxEXPAND);

    /*wxBoxSizer* sizerButtons = new wxBoxSizer(wxHORIZONTAL);
    sizerButtons->Add(checkbox_showlog, 0, wxALIGN_CENTER_VERTICAL);
    sizerButtons->Add(0, 0, 1, wxEXPAND);
    sizerButtons->Add(button_start);*/

    wxBoxSizer* sizerPanelV = new wxBoxSizer(wxVERTICAL);
    sizerPanelV->Add(0, styleguide().getFrameMargin(wxTOP));
    sizerPanelV->Add(sizerChecks);
    sizerPanelV->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerPanelV->Add(sizerTimeout, 0, wxEXPAND);
    sizerPanelV->Add(0, styleguide().getUnrelatedControlMargin(wxVERTICAL));
    sizerPanelV->Add(radiobox_state, 0, wxEXPAND);
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

void ShutdownFrame::updateControls()
{
    ShutdownStartupBaseFrame::updateControls();

    bool running = getThreadRunning();

    checkbox_attach->Enable(!running);
    checkbox_tran->Enable(!running);
    checkbox_force->Enable(!running);
}

void ShutdownFrame::doReadConfigSettings(const wxString& prefix)
{
    ShutdownStartupBaseFrame::doReadConfigSettings(prefix);
    wxArrayString flags;
    config().getValue(prefix + Config::pathSeparator + "options", flags);
    if (!flags.empty())
    {
        checkbox_attach->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), "-attach"));
        checkbox_tran->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), "-tran"));
        checkbox_force->SetValue(
            flags.end() != std::find(flags.begin(), flags.end(), "-force"));
    }
    updateControls();
}

void ShutdownFrame::doWriteConfigSettings(const wxString& prefix) const
{
    ShutdownStartupBaseFrame::doWriteConfigSettings(prefix);
    wxArrayString flags;
    if (checkbox_attach->IsChecked())
        flags.push_back("-attach");
    if (checkbox_tran->IsChecked())
        flags.push_back("-tran");
    if (checkbox_force->IsChecked())
        flags.push_back("-force");
    config().setValue(prefix + Config::pathSeparator + "options", flags);
}

const wxString ShutdownFrame::getName() const
{
    return "ShutdownFrame";
}

/*static*/
wxString ShutdownFrame::getFrameId(DatabasePtr db)
{
    if (db)
        return wxString("ShutdownFrame/" + db->getItemPath());
    else
        return wxEmptyString;
}

ShutdownFrame* ShutdownFrame::findFrameFor(DatabasePtr db)
{
    BaseFrame* bf = frameFromIdString(getFrameId(db));
    if (!bf)
        return 0;
    return dynamic_cast<ShutdownFrame*>(bf);
}

//! event handlers
BEGIN_EVENT_TABLE(ShutdownFrame, ShutdownStartupBaseFrame)
    EVT_BUTTON(ShutdownStartupBaseFrame::ID_button_start, ShutdownFrame::OnStartButtonClick)
END_EVENT_TABLE()


void ShutdownFrame::OnStartButtonClick(wxCommandEvent& WXUNUSED(event))
{
    verboseMsgsM = true;
    clearLog();

    DatabasePtr database = getDatabase();
    wxCHECK_RET(database,
        "Cannot shutdown unassigned database");
    ServerPtr server = database->getServer();
    wxCHECK_RET(server,
        "Cannot shutdown database without assigned server");

    wxString username;
    wxString password;
    if (!getConnectionCredentials(this, database, username, password))
        return;

    wxString rolename;
    wxString charset;
    rolename = database->getRole();
    charset = database->getConnectionCharset();

    int flags = (int)IBPP::dsVerbose; 

    if (checkbox_attach->IsChecked())
        flags |= (int)IBPP::dsDenyAttach;
    if (checkbox_tran->IsChecked())
        flags |= (int)IBPP::dsDenyTrans;
    if (checkbox_force->IsChecked())
        flags |= (int)IBPP::dsForce;

    flags |= getDatabaseMode();

    int ltimeout = spinctrl_timeout->GetValue();

    startThread(std::make_unique<ShutdownThread>(this,
        server->getConnectionString(), username, password, rolename, charset,
        database->getPath(), (IBPP::DSM)flags, ltimeout));

    updateControls();
}

ShutdownThread::ShutdownThread(ShutdownFrame* frame,
    wxString server, wxString username, wxString password,
    wxString rolename, wxString charset, wxString dbfilename,
    IBPP::DSM flags, int timeout)
    :timeoutM(timeout),
    ShutdownStartupThread(frame, server, username, password, 
        rolename, charset, dbfilename, flags)
{
}

void ShutdownThread::Execute(IBPP::Service svc)
{
    svc->Shutdown(wx2std(dbfileM), dsmM, timeoutM);
}
