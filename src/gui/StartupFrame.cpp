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

// worker thread class to perform database Start
class StartupThread: public wxThread {
public:
    StartupThread(StartupFrame* frame, wxString server, wxString username,
        wxString password, wxString dbfilename,  IBPP::DSM flags);

    virtual void* Entry();
    virtual void OnExit();
private:
    StartupFrame* frameM;
    wxString serverM;
    wxString usernameM;
    wxString passwordM;
    wxString dbfileM;
    IBPP::DSM dsmM;
    void logError(wxString& msg);
    void logImportant(wxString& msg);
    void logProgress(wxString& msg);
};

StartupThread::StartupThread(StartupFrame* frame, wxString server,
        wxString username, wxString password, wxString dbfilename,
        IBPP::DSM flags)
    : wxThread()
{
    frameM = frame;
    serverM = server;
    usernameM = username;
    passwordM = password;
    dbfileM = dbfilename;
    // always use verbose flag
    dsmM = (IBPP::DSM)((int)flags | (int)IBPP::brVerbose);
}

void* StartupThread::Entry()
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
        msg.Printf(_("Database startup started %s"), now.FormatTime().c_str());
        logImportant(msg);
        svc->Restart(wx2std(dbfileM),  dsmM);
        while (true)
        {
            if (TestDestroy())
            {
                now = wxDateTime::Now();
                msg.Printf(_("Database startup canceled %s"),
                    now.FormatTime().c_str());
                logImportant(msg);
                break;
            }
            const char* c = svc->WaitMsg();
            if (c == 0)
            {
                now = wxDateTime::Now();
                msg.Printf(_("Database startup finished %s"),
                    now.FormatTime().c_str());
                logImportant(msg);
                break;
            }
            msg = c;
            logProgress(msg);
        }
        svc->Disconnect();
    }
    catch (IBPP::Exception& e)
    {
        now = wxDateTime::Now();
        msg.Printf(_("Database startup canceled %s due to IBPP exception:\n\n"),
            now.FormatTime().c_str());
        msg += e.what();
        logError(msg);
    }
    catch (...)
    {
        now = wxDateTime::Now();
        msg.Printf(_("Database startup canceled %s due to exception"),
            now.FormatTime().c_str());
        logError(msg);
    }
    return 0;
}

void StartupThread::OnExit()
{
    if (frameM != 0)
    {
        wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED,
            ShutdownStartupBaseFrame::ID_thread_finished);
        wxPostEvent(frameM, event);
    }
}

void StartupThread::logError(wxString& msg)
{
    if (frameM != 0)
        frameM->threadOutputMsg(msg, ShutdownStartupBaseFrame::error_message);
}

void StartupThread::logImportant(wxString& msg)
{
    if (frameM != 0)
        frameM->threadOutputMsg(msg, ShutdownStartupBaseFrame::important_message);
}

void StartupThread::logProgress(wxString& msg)
{
    if (frameM != 0)
        frameM->threadOutputMsg(msg, ShutdownStartupBaseFrame::progress_message);
}

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
    

    wxBoxSizer* sizerButtons = new wxBoxSizer(wxHORIZONTAL);
    sizerButtons->Add(checkbox_showlog, 0, wxALIGN_CENTER_VERTICAL);
    sizerButtons->Add(0, 0, 1, wxEXPAND);
    sizerButtons->Add(button_start);

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
    verboseMsgsM = checkbox_showlog->IsChecked();
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

    int flags = (int)IBPP::dsVerbose; 


    flags |= getDatabaseMode();


    startThread(std::make_unique<StartupThread>(this,
        server->getConnectionString(), username, password,
        database->getPath(), (IBPP::DSM)flags));

    updateControls();
}

