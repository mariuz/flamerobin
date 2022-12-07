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
#include "gui/StartupFrame.h"
#include "gui/controls/DndTextControls.h"
#include "gui/controls/LogTextControl.h"
#include "gui/StyleGuide.h"
#include "gui/UsernamePasswordDialog.h"
#include "metadata/database.h"
#include "metadata/server.h"

StartupFrame::StartupFrame(wxWindow* parent, DatabasePtr db)
    : ShutdownStartupBaseFrame(parent, db)
{
    setIdString(this, getFrameId(db));

    wxString databaseName(db->getName_());
    wxString serverName(db->getServer()->getName_());
    SetTitle(wxString::Format(_("Startup Database \"%s:%s\""),
        serverName.c_str(), databaseName.c_str()));

    createControls();
    layoutControls();
    updateControls();
}

//! implementation details
void StartupFrame::createControls()
{
    ShutdownStartupBaseFrame::createControls();

    button_start->SetLabelText(_("Startup Start"));

}

void StartupFrame::layoutControls()
{
    
    ShutdownStartupBaseFrame::layoutControls();

    /*wxBoxSizer* sizerButtons = new wxBoxSizer(wxHORIZONTAL);
    sizerButtons->Add(checkbox_showlog, 0, wxALIGN_CENTER_VERTICAL);
    sizerButtons->Add(0, 0, 1, wxEXPAND);
    sizerButtons->Add(button_start);*/

    wxBoxSizer* sizerPanelV = new wxBoxSizer(wxVERTICAL);
    sizerPanelV->Add(0, styleguide().getFrameMargin(wxTOP));
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

void StartupFrame::updateControls()
{
    ShutdownStartupBaseFrame::updateControls();
}

void StartupFrame::doReadConfigSettings(const wxString& prefix)
{
    ShutdownStartupBaseFrame::doReadConfigSettings(prefix);
    wxArrayString flags;
    config().getValue(prefix + Config::pathSeparator + "options", flags);
    updateControls();
}

void StartupFrame::doWriteConfigSettings(const wxString& prefix) const
{
    ShutdownStartupBaseFrame::doWriteConfigSettings(prefix);
    wxArrayString flags;
    config().setValue(prefix + Config::pathSeparator + "options", flags);
}

const wxString StartupFrame::getName() const
{
    return "StartFrame";
}

/*static*/
wxString StartupFrame::getFrameId(DatabasePtr db)
{
    if (db)
        return wxString("StartFrame/" + db->getItemPath());
    else
        return wxEmptyString;
}

StartupFrame* StartupFrame::findFrameFor(DatabasePtr db)
{
    BaseFrame* bf = frameFromIdString(getFrameId(db));
    if (!bf)
        return 0;
    return dynamic_cast<StartupFrame*>(bf);
}

//! event handlers
BEGIN_EVENT_TABLE(StartupFrame, ShutdownStartupBaseFrame)
    EVT_BUTTON(ShutdownStartupBaseFrame::ID_button_start, StartupFrame::OnStartButtonClick)
END_EVENT_TABLE()


void StartupFrame::OnStartButtonClick(wxCommandEvent& WXUNUSED(event))
{
    verboseMsgsM = true;
    clearLog();

    DatabasePtr database = getDatabase();
    wxCHECK_RET(database,
        "Cannot startup unassigned database");
    ServerPtr server = database->getServer();
    wxCHECK_RET(server,
        "Cannot startup database without assigned server");

    wxString username;
    wxString password;
    if (!getConnectionCredentials(this, database, username, password))
        return;
    wxString rolename;
    wxString charset;
    rolename = database->getRole();
    charset = database->getConnectionCharset();

    int flags = (int)IBPP::dsVerbose; 


    flags |= getDatabaseMode();


    startThread(std::make_unique<StartupThread>(this,
        server->getConnectionString(), username, password, rolename, charset,
        database->getPath(), (IBPP::DSM)flags));

    updateControls();
}

StartupThread::StartupThread(StartupFrame* frame, 
    wxString server, wxString username, wxString password, 
    wxString rolename, wxString charset, wxString 
    dbfilename, IBPP::DSM flags)
    :ShutdownStartupThread(frame, server, username, password,
        rolename, charset, dbfilename, flags)
{
}

void StartupThread::Execute(IBPP::Service svc)
{
    svc->Restart(wx2std(dbfileM), dsmM);
}
