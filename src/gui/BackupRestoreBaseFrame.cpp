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

  Contributor(s): Milan Babuskov
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

#include <wx/timer.h>

#include "BackupRestoreBaseFrame.h"
#include "config.h"
#include "ugly.h"

//-----------------------------------------------------------------------------
BackupRestoreBaseFrame::BackupRestoreBaseFrame(wxWindow* parent, YDatabase* db):
    BaseFrame(parent, -1, _T(""), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE|wxNO_FULL_REPAINT_ON_RESIZE)
{
    databaseM = db;
    serverM = reinterpret_cast<YServer*>(db->getParent());

	threadM = 0;
    threadMsgTimeMillisM = 0;
    verboseMsgsM = true;
    storageNameM = "unassigned";

    // create controls in constructor of descendant class to allow for correct tab order
    panel_controls = 0;
    label_filename = 0;
    text_ctrl_filename = 0;
    button_browse = 0;
    combobox_showlog = 0;
    button_start = 0;
    button_cancel = 0;
    text_ctrl_log = 0;

	#include "backup.xpm"
    wxBitmap bmp(backup_xpm);
    wxIcon icon;
    icon.CopyFromBitmap(bmp);
    SetIcon(icon);
}
//-----------------------------------------------------------------------------
//! some controls need additional setup after descendant frame's controls are created
void BackupRestoreBaseFrame::setupControls()
{
	text_ctrl_log->StyleSetForeground(1, *wxBLUE);
	text_ctrl_log->StyleSetForeground(2, *wxRED);
	text_ctrl_log->SetMarginWidth(1, 0);
	text_ctrl_log->SetWrapMode(wxSTC_WRAP_WORD);
}
//-----------------------------------------------------------------------------
//! implementation details
void BackupRestoreBaseFrame::addThreadMsg(const wxString msg, bool& notificationNeeded)
{
    notificationNeeded = false;
    wxLongLong millisNow = ::wxGetLocalTimeMillis();

    wxCriticalSectionLocker locker(critsectM);
    threadMsgsM.Add(msg);
    // we post no more than 10 events per second to prevent flooding of the message queue
    // and keep the frame responsive for user interaction
    if ((millisNow - threadMsgTimeMillisM).GetLo() > 100)
    {
        threadMsgTimeMillisM = millisNow;
        notificationNeeded = true;
    }
}
//-----------------------------------------------------------------------------
void BackupRestoreBaseFrame::cancelBackupRestore()
{
    if (threadM != 0)
    {
        threadM->Delete();
        threadM = 0;
    }
}
//-----------------------------------------------------------------------------
void BackupRestoreBaseFrame::clearLog()
{
    msgKindsM.Clear();
    msgsM.Clear();
    text_ctrl_log->ClearAll();
}
//-----------------------------------------------------------------------------
bool BackupRestoreBaseFrame::Destroy()
{
    cancelBackupRestore();
    return BaseFrame::Destroy();
}
//-----------------------------------------------------------------------------
void BackupRestoreBaseFrame::doReadConfigSettings(const std::string& prefix)
{
   	BaseFrame::doReadConfigSettings(prefix);

    bool verbose;
	if (!config().getValue(prefix + "::verboselog", verbose))
        verbose = true;
    combobox_showlog->SetValue(verbose);

    std::string bkfile;
    if (config().getValue(prefix + "::backupfilename", bkfile) && !bkfile.empty())
        text_ctrl_filename->SetValue(std2wx(bkfile));
}
//-----------------------------------------------------------------------------
void BackupRestoreBaseFrame::doWriteConfigSettings(const std::string& prefix) const
{
	BaseFrame::doWriteConfigSettings(prefix);
	config().setValue(prefix + "::verboselog", combobox_showlog->GetValue());
    config().setValue(prefix + "::backupfilename", wx2std(text_ctrl_filename->GetValue()));
}
//-----------------------------------------------------------------------------
const std::string BackupRestoreBaseFrame::getStorageName() const
{
	if (storageNameM == "unassigned")
    {
        StorageGranularity g;
	    if (!config().getValue(getName() + "StorageGranularity", g))
		    g = sgFrame;

    	switch (g)
	    {
     	    case sgFrame:
          	    storageNameM = getName();
                break;
		    case sgObject:
                storageNameM = getName() + "::" + databaseM->getItemPath();
                break;
		    default:
			    storageNameM = "";
                break;
	    }
    }
    return storageNameM;
}
//-----------------------------------------------------------------------------
bool BackupRestoreBaseFrame::startThread(wxThread* thread)
{
    wxASSERT(threadM == 0);
    if (wxTHREAD_NO_ERROR != thread->Create())
    {
        ::wxMessageBox(_("Error creating thread!"), _("Error"), wxICON_ERROR);
        delete thread;
        return false;
    }
    if (wxTHREAD_NO_ERROR != thread->Run())
    {
        ::wxMessageBox(_("Error starting thread!"), _("Error"), wxICON_ERROR);
        delete thread;
        return false;
    }
    threadM = thread;
    return true;
}
//-----------------------------------------------------------------------------
void BackupRestoreBaseFrame::threadOutputMsg(const wxString msg, MsgKind kind)
{
    wxString s(msg);
    switch (kind)
    {
        case error_message:
            s.Prepend(wxT("e"));
            break;
        case important_message:
            s.Prepend(wxT("i"));
            break;
        case progress_message:
            s.Prepend(wxT("p"));
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
//-----------------------------------------------------------------------------
void BackupRestoreBaseFrame::updateMessages(size_t firstmsg, size_t lastmsg)
{
    if (lastmsg > msgsM.GetCount())
        lastmsg = msgsM.GetCount();
    bool added = false;
    for (size_t i = firstmsg; i < lastmsg; i++)
    {
		int style = 0;
        switch ((MsgKind)msgKindsM[i])
        {
            case progress_message:
                if (!verboseMsgsM)
                    continue;
                break;
            case important_message:
				style = 1;
                break;
            case error_message:
				style = 2;
                break;
        }
		int startpos = text_ctrl_log->GetLength();
		text_ctrl_log->AddText(msgsM[i] + wxT("\n"));
		int endpos = text_ctrl_log->GetLength();
		text_ctrl_log->StartStyling(startpos, 255);
		text_ctrl_log->SetStyling(endpos-startpos-1, style);
        added = true;
    }

    if (added)
		text_ctrl_log->GotoPos(text_ctrl_log->GetLength());
}
//-----------------------------------------------------------------------------
//! event handlers
BEGIN_EVENT_TABLE(BackupRestoreBaseFrame, BaseFrame)
    EVT_BUTTON(BackupRestoreBaseFrame::ID_button_cancel, BackupRestoreBaseFrame::OnCancelButtonClick)
    EVT_CHECKBOX(BackupRestoreBaseFrame::ID_checkbox_showlog, BackupRestoreBaseFrame::OnVerboseLogChange)
    EVT_MENU(BackupRestoreBaseFrame::ID_thread_finished, BackupRestoreBaseFrame::OnThreadFinished)
    EVT_MENU(BackupRestoreBaseFrame::ID_thread_output, BackupRestoreBaseFrame::OnThreadOutput)
    EVT_TEXT(BackupRestoreBaseFrame::ID_text_ctrl_filename, BackupRestoreBaseFrame::OnSettingsChange)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void BackupRestoreBaseFrame::OnCancelButtonClick(wxCommandEvent& WXUNUSED(event))
{
    cancelBackupRestore();
    updateControls();
}
//-----------------------------------------------------------------------------
void BackupRestoreBaseFrame::OnSettingsChange(wxCommandEvent& WXUNUSED(event))
{
	if (IsShown())
		updateControls();
}
//-----------------------------------------------------------------------------
void BackupRestoreBaseFrame::OnThreadFinished(wxCommandEvent& event)
{
    threadM = 0;
    OnThreadOutput(event);
    updateControls();
}
//-----------------------------------------------------------------------------
void BackupRestoreBaseFrame::OnThreadOutput(wxCommandEvent& WXUNUSED(event))
{
    wxCriticalSectionLocker locker(critsectM);
    threadMsgTimeMillisM = ::wxGetLocalTimeMillis();

    size_t first = msgsM.GetCount();
    for (size_t i = 0; i < threadMsgsM.GetCount(); i++)
    {
        wxString s(threadMsgsM[i]);
        if (s.Length() == 0)
            continue;
        switch (s.GetChar(0))
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
        if (s.Last() == '\n')
            s.RemoveLast();
        msgsM.Add(s.Mid(1));
    }
    threadMsgsM.Clear();

    updateMessages(first, msgsM.GetCount());
}
//-----------------------------------------------------------------------------
void BackupRestoreBaseFrame::OnVerboseLogChange(wxCommandEvent& WXUNUSED(event))
{
	wxBusyCursor wait;
    verboseMsgsM = combobox_showlog->IsChecked();

    text_ctrl_log->Freeze();
    text_ctrl_log->ClearAll();
    updateMessages(0, msgsM.GetCount());
    text_ctrl_log->Thaw();
}
//-----------------------------------------------------------------------------
