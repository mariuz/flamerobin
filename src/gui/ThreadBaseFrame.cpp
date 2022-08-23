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
#include "gui/ThreadBaseFrame.h"
#include "gui/controls/DndTextControls.h"
#include "gui/controls/LogTextControl.h"
#include "metadata/database.h"
#include "metadata/server.h"

ThreadBaseFrame::ThreadBaseFrame(wxWindow* parent,
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
void ThreadBaseFrame::addThreadMsg(const wxString msg,
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

void ThreadBaseFrame::cancelThread()
{
    if (threadM != 0)
    {
        threadM->Delete();
        threadM = 0;
    }
}

void ThreadBaseFrame::clearLog()
{
    msgKindsM.Clear();
    msgsM.Clear();
    text_ctrl_log->ClearAll();
}

bool ThreadBaseFrame::Destroy()
{
    cancelThread();
    return BaseFrame::Destroy();
}


DatabasePtr ThreadBaseFrame::getDatabase() const
{
    return databaseM.lock();
}


bool ThreadBaseFrame::getThreadRunning() const
{
    return threadM != 0;
}

void ThreadBaseFrame::subjectRemoved(Subject* subject)
{
    DatabasePtr db = getDatabase();
    if (!db || !db->isConnected() || subject == db.get())
        Close();
}

bool ThreadBaseFrame::startThread(std::unique_ptr<wxThread> thread)
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

void ThreadBaseFrame::threadOutputMsg(const wxString msg, MsgKind kind)
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

void ThreadBaseFrame::createControls()
{
    panel_controls = new wxPanel(this, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxTAB_TRAVERSAL | wxCLIP_CHILDREN);

    checkbox_showlog = new wxCheckBox(panel_controls, ID_checkbox_showlog,
        _("Show complete log"));
    button_start = new wxButton(panel_controls, ID_button_start,
        _("&Start Backup"));

    text_ctrl_log = new LogTextControl(this, ID_text_ctrl_log);

}

void ThreadBaseFrame::layoutControls()
{
}

void ThreadBaseFrame::update()
{
    DatabasePtr db = getDatabase();
    if (db)
        updateControls();
    else
        Close();
}

void ThreadBaseFrame::updateControls()
{
    bool running = getThreadRunning();
    
    button_start->Enable(!running);
    checkbox_showlog->Enable(!running);
}

void ThreadBaseFrame::updateMessages(size_t firstmsg, size_t lastmsg)
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
BEGIN_EVENT_TABLE(ThreadBaseFrame, BaseFrame)
    EVT_MENU(ThreadBaseFrame::ID_thread_finished, ThreadBaseFrame::OnThreadFinished)
    EVT_MENU(ThreadBaseFrame::ID_thread_output, ThreadBaseFrame::OnThreadOutput)
END_EVENT_TABLE()

void ThreadBaseFrame::OnSettingsChange(wxCommandEvent& WXUNUSED(event))
{
    if (IsShown())
        updateControls();
}

void ThreadBaseFrame::OnThreadFinished(wxCommandEvent& event)
{
    threadM = 0;
    OnThreadOutput(event);
    updateControls();
}

void ThreadBaseFrame::OnThreadOutput(wxCommandEvent& WXUNUSED(event))
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


