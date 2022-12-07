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

#include <wx/timer.h>
#include <wx/wupdlock.h>

#include "config/Config.h"
#include "core/ArtProvider.h"
#include "gui/ShutdownStartupBaseFrame.h"
#include "gui/controls/DndTextControls.h"
#include "gui/controls/LogTextControl.h"
#include "metadata/database.h"
#include "metadata/server.h"

ShutdownStartupBaseFrame::ShutdownStartupBaseFrame(wxWindow* parent,
        DatabasePtr db)
    : ServiceBaseFrame(parent, db)
{

    verboseMsgsM = true;


    SetIcon(wxArtProvider::GetIcon(ART_Backup, wxART_FRAME_ICON));
}

//! implementation details
void ShutdownStartupBaseFrame::cancelShutdownStartup()
{
    cancelThread();
}


bool ShutdownStartupBaseFrame::Destroy()
{
    cancelShutdownStartup();
    return ServiceBaseFrame::Destroy();
}

void ShutdownStartupBaseFrame::doReadConfigSettings(const wxString& prefix)
{
    BaseFrame::doReadConfigSettings(prefix);

    wxString bkfile;
    config().getValue(prefix + Config::pathSeparator + "state",
        bkfile);
    //if (!bkfile.empty())
    //    text_ctrl_filename->SetValue(bkfile);
}

void ShutdownStartupBaseFrame::doWriteConfigSettings(const wxString& prefix) const
{
    BaseFrame::doWriteConfigSettings(prefix);
    //config().setValue(prefix + Config::pathSeparator + "state",
    //    text_ctrl_filename->GetValue());
}


const wxString ShutdownStartupBaseFrame::getStorageName() const
{
    if (DatabasePtr db = getDatabase())
        return getName() + Config::pathSeparator + db->getItemPath();
    return wxEmptyString;
}

void ShutdownStartupBaseFrame::createControls()
{
    ServiceBaseFrame::createControls();
    
    wxArrayString choices;
    choices.Add("normal");
    choices.Add("single");
    choices.Add("multi");
    choices.Add("full");

    radiobox_state = new wxRadioBox(panel_controls, ID_radiobox_state,
        _("Database mode  (FB 2.0+)"), wxDefaultPosition, wxDefaultSize, choices,
        4, wxHORIZONTAL);
}

void ShutdownStartupBaseFrame::layoutControls()
{
    ServiceBaseFrame::layoutControls();
}

void ShutdownStartupBaseFrame::subjectRemoved(Subject* subject)
{
    DatabasePtr db = getDatabase();
    if (!db || !db->isConnected() || subject == db.get())
        Close();
}

void ShutdownStartupBaseFrame::update()
{
    DatabasePtr db = getDatabase();
    if (db)
        updateControls();
    else
        Close();
}

IBPP::DSM ShutdownStartupBaseFrame::getDatabaseMode()
{
    int dbMode = radiobox_state->GetSelection();
    switch (dbMode)
    {
        case 0 :
            return IBPP::dsNormal;
            break;
        case 1 :
            return IBPP::dsSingle;
            break;
        case 2 :
            return IBPP::dsMulti;
            break;
        case 3 :
            return IBPP::dsFull;
            break;
        default :
            return IBPP::dsNormal;
    }
}

void ShutdownStartupBaseFrame::updateControls()
{
    ServiceBaseFrame::updateControls();

    bool running = getThreadRunning();

    radiobox_state->Enable(!running);

}

//! event handlers
BEGIN_EVENT_TABLE(ShutdownStartupBaseFrame, ServiceBaseFrame)
    EVT_MENU(ShutdownStartupBaseFrame::ID_thread_finished, ShutdownStartupBaseFrame::OnThreadFinished)
    EVT_MENU(ShutdownStartupBaseFrame::ID_thread_output, ShutdownStartupBaseFrame::OnThreadOutput)
END_EVENT_TABLE()


void ShutdownStartupBaseFrame::OnVerboseLogChange(wxCommandEvent& WXUNUSED(event))
{
    wxBusyCursor wait;
    verboseMsgsM = true;

    wxWindowUpdateLocker freeze(text_ctrl_log);

    text_ctrl_log->ClearAll();
    updateMessages(0, msgsM.GetCount());
}

ShutdownStartupThread::ShutdownStartupThread(ShutdownStartupBaseFrame* frame, 
    wxString server, wxString username, wxString password, wxString rolename, 
    wxString charset, wxString dbfilename, IBPP::DSM flags)
    :dbfileM(dbfilename), 
    ServiceThread(frame, server, username, password, rolename, charset)
{
    dsmM = (IBPP::DSM)((int)flags | (int)IBPP::brVerbose);
}
