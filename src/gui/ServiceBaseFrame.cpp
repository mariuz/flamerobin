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

#include <ibpp.h>

#include "config/Config.h"
#include "core/ArtProvider.h"
#include "core/StringUtils.h"
#include "gui/ServiceBaseFrame.h"
#include "gui/controls/DndTextControls.h"
#include "gui/controls/LogTextControl.h"
#include "gui/StyleGuide.h"
#include "metadata/database.h"
#include "metadata/server.h"
#include <frutils.h>

ServiceBaseFrame::ServiceBaseFrame(wxWindow* parent,
        DatabasePtr db)
    : BaseFrame(parent, wxID_ANY, wxEmptyString), databaseM(db), threadM(0)
{
    wxASSERT(db);
    db->attachObserver(this, false);

    threadMsgTimeMillisM = 0;
    verboseMsgsM = true;

    SetIcon(wxArtProvider::GetIcon(ART_Backup, wxART_FRAME_ICON));

}

//! implementation details
void ServiceBaseFrame::addThreadMsg(const wxString msg,
    bool& notificationNeeded)
{
    notificationNeeded = false;
    wxLongLong millisNow = ::wxGetLocalTimeMillis();

    wxCriticalSectionLocker locker(critsectM);
    threadMsgsM.Add(msg);
    // we post no more than 10 events per second to prevent flooding of
    // the message queue, and to keep the frame responsive for user interaction
    if ((millisNow - threadMsgTimeMillisM).GetLo() > 100)
    {
        threadMsgTimeMillisM = millisNow;
        notificationNeeded = true;
    }
}

void ServiceBaseFrame::cancelThread()
{
    if (threadM != 0)
    {
        threadM->Delete();
        threadM = 0;
    }
}

void ServiceBaseFrame::clearLog()
{
    msgKindsM.Clear();
    msgsM.Clear();
    text_ctrl_log->ClearAll();
}

bool ServiceBaseFrame::Destroy()
{
    cancelThread();
    return BaseFrame::Destroy();
}


DatabasePtr ServiceBaseFrame::getDatabase() const
{
    return databaseM.lock();
}


bool ServiceBaseFrame::getThreadRunning() const
{
    return threadM != 0;
}

void ServiceBaseFrame::subjectRemoved(Subject* subject)
{
    DatabasePtr db = getDatabase();
    if (!db || !db->isConnected() || subject == db.get())
        Close();
}

bool ServiceBaseFrame::startThread(std::unique_ptr<wxThread> thread)
{
    wxASSERT(threadM == 0);
    if (wxTHREAD_NO_ERROR != thread->Create())
    {
        ::wxMessageBox(_("Error creating thread!"), _("Error"),
            wxOK | wxICON_ERROR);
        return false;
    }
    if (wxTHREAD_NO_ERROR != thread->Run())
    {
        ::wxMessageBox(_("Error starting thread!"), _("Error"),
            wxOK | wxICON_ERROR);
        return false;
    }
    threadM = thread.release();
    return true;
}

void ServiceBaseFrame::threadOutputMsg(const wxString msg, MsgKind kind)
{
    wxString s(msg);
    switch (kind)
    {
    case error_message:
        s.Prepend("e");
        break;
    case important_message:
        s.Prepend("i");
        break;
    case progress_message:
        s.Prepend("p");
        break;
    default:
        wxASSERT(false);
        return;
    }
    bool doPostMsg = false;
    addThreadMsg(s, doPostMsg);
    if (doPostMsg)
    {
        wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, ID_thread_output);
        wxPostEvent(this, event);
    }
}

void ServiceBaseFrame::createControls()
{
    panel_controls = new wxPanel(this, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxTAB_TRAVERSAL | wxCLIP_CHILDREN);

    button_start = new wxButton(panel_controls, ID_button_start,
        _("&Start Backup"));

    text_ctrl_log = new LogTextControl(this, ID_text_ctrl_log);

}

void ServiceBaseFrame::layoutControls()
{
    sizerButtons = new wxBoxSizer(wxHORIZONTAL);
    
    /*sizerButtons->Add(checkbox_showlog, 0, wxALIGN_CENTER_VERTICAL);
  
    sizerButtons->Add(styleguide().getControlLabelMargin(), 0);
    sizerButtons->Add(caption_showlogInterval, 0, wxALIGN_CENTER_VERTICAL);

    sizerButtons->Add(styleguide().getControlLabelMargin(), 0);
    sizerButtons->Add(spinctrl_showlogInterval, 0, wxALIGN_CENTER_VERTICAL);*/

    sizerButtons->Add(0, 0, 1, wxEXPAND);
    sizerButtons->Add(button_start);

}

void ServiceBaseFrame::update()
{
    DatabasePtr db = getDatabase();
    if (db)
        updateControls();
    else
        Close();
}

void ServiceBaseFrame::updateControls()
{
    bool running = getThreadRunning();
    
    button_start->Enable(!running);
}

void ServiceBaseFrame::updateMessages(size_t firstmsg, size_t lastmsg)
{
    if (lastmsg > msgsM.GetCount())
        lastmsg = msgsM.GetCount();
    for (size_t i = firstmsg; i < lastmsg; i++)
    {
        switch ((MsgKind)msgKindsM[i])
        {
            case progress_message:
                if (verboseMsgsM)
                    text_ctrl_log->logMsg(msgsM[i]);
                break;
            case important_message:
                text_ctrl_log->logImportantMsg(msgsM[i]);
                break;
            case error_message:
                text_ctrl_log->logErrorMsg(msgsM[i]);
                break;
        }
    }
}


//! event handlers
BEGIN_EVENT_TABLE(ServiceBaseFrame, BaseFrame)
    EVT_MENU(ServiceBaseFrame::ID_thread_finished, ServiceBaseFrame::OnThreadFinished)
    EVT_MENU(ServiceBaseFrame::ID_thread_output, ServiceBaseFrame::OnThreadOutput)
END_EVENT_TABLE()

void ServiceBaseFrame::OnSettingsChange(wxCommandEvent& WXUNUSED(event))
{
    if (IsShown())
        updateControls();
}

void ServiceBaseFrame::OnThreadFinished(wxCommandEvent& event)
{
    threadM = 0;
    OnThreadOutput(event);
    updateControls();
}

void ServiceBaseFrame::OnThreadOutput(wxCommandEvent& WXUNUSED(event))
{
    wxCriticalSectionLocker locker(critsectM);
    threadMsgTimeMillisM = ::wxGetLocalTimeMillis();

    size_t first = msgsM.GetCount();
    for (size_t i = 0; i < threadMsgsM.GetCount(); i++)
    {
        wxString s(threadMsgsM[i]);
        if (s.Length() == 0)
            continue;
        switch ((wxChar)s[0])
        {
            case 'e':
                msgKindsM.Add((int)error_message);
                break;
            case 'i':
                msgKindsM.Add((int)important_message);
                break;
            case 'p':
                msgKindsM.Add((int)progress_message);
                break;
        }
        // this depends on server type, so just in case...
        if (s.Last() != '\n')
            s.Append('\n');
        msgsM.Add(s.Mid(1));
    }
    threadMsgsM.Clear();

    updateMessages(first, msgsM.GetCount());
}

ServiceThread::ServiceThread(ServiceBaseFrame* frame, wxString server,
    wxString username, wxString password, wxString rolename,
    wxString charset) :
    frameM(frame), serverM(server), usernameM(username), passwordM(password),
    rolenameM(rolename), charsetM(charset),
    wxThread()

{
}

void* ServiceThread::Entry()
{
    wxDateTime now;
    wxString msg;

    try
    {
        msg.Printf(_("Connecting to server %s..."), serverM.c_str());
        logImportant(msg);
        IBPP::Service svc = IBPP::ServiceFactory(wx2std(serverM),
            wx2std(usernameM), wx2std(passwordM), wx2std(rolenameM), wx2std(charsetM),
            wx2std(getClientLibrary())
        );
        svc->Connect();

        now = wxDateTime::Now();
        msg.Printf(_("Database restore started %s"), now.FormatTime().c_str());
        logImportant(msg);
        Execute(svc);
        while (true)
        {
            if (TestDestroy())
            {
                now = wxDateTime::Now();
                msg.Printf(_("Database restore canceled %s"),
                    now.FormatTime().c_str());
                logImportant(msg);
                break;
            }
            const char* c = svc->WaitMsg();
            if (c == 0)
            {
                now = wxDateTime::Now();
                msg.Printf(_("Database restore finished %s"),
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
        msg.Printf(_("Database restore canceled %s due to IBPP exception:\n\n"),
            now.FormatTime().c_str());
        msg += e.what();
        logError(msg);
    }
    catch (...)
    {
        now = wxDateTime::Now();
        msg.Printf(_("Database restore canceled %s due to exception"),
            now.FormatTime().c_str());
        logError(msg);
    }
    return 0;
}

void ServiceThread::OnExit()
{
    if (frameM != 0)
    {
        wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED,
            ServiceBaseFrame::ID_thread_finished);
        wxPostEvent(frameM, event);
    }

}

void ServiceThread::logError(wxString& msg)
{
    if (frameM != 0)
        frameM->threadOutputMsg(msg, ServiceBaseFrame::error_message);
}

void ServiceThread::logImportant(wxString& msg)
{
    if (frameM != 0)
        frameM->threadOutputMsg(msg, ServiceBaseFrame::important_message);
}

void ServiceThread::logProgress(wxString& msg)
{
    if (frameM != 0)
        frameM->threadOutputMsg(msg, ServiceBaseFrame::progress_message);

}

