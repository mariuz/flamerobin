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

Contributor(s):
*/

#ifndef BACKUPRESTOREBASEFRAME_H
#define BACKUPRESTOREBASEFRAME_H
//-----------------------------------------------------------------------------
#include <wx/wx.h>
#include <wx/stc/stc.h>
#include <wx/thread.h>

#include "gui/BaseFrame.h"
#include "metadata/database.h"
#include "metadata/server.h"
//-----------------------------------------------------------------------------
class BackupRestoreBaseFrame: public BaseFrame {
public:
    enum MsgKind {
        progress_message,
        important_message,
        error_message
    };
    enum {
        ID_text_ctrl_filename = 101,
        ID_button_browse,
        ID_button_showlog,
        ID_text_ctrl_log,
        ID_checkbox_showlog,
        ID_button_start,
        ID_button_cancel,

        ID_thread_output,
        ID_thread_finished
    };
    // events
    void OnCancelButtonClick(wxCommandEvent& event);
    void OnSettingsChange(wxCommandEvent& event);
    void OnThreadFinished(wxCommandEvent& event);
    void OnThreadOutput(wxCommandEvent& event);
    void OnVerboseLogChange(wxCommandEvent& event);

    // make sure that thread gets deleted
    virtual bool Destroy();
protected:
    mutable wxString storageNameM;

    Server* serverM;
    Database *databaseM;

    wxThread* threadM;
    wxArrayString msgsM;
    wxArrayInt msgKindsM;
    bool verboseMsgsM;

    void cancelBackupRestore();
    void clearLog();
    virtual void doReadConfigSettings(const wxString& prefix);
    virtual void doWriteConfigSettings(const wxString& prefix) const;
    virtual const wxString getStorageName() const;
    // set threadM if thread was successfully created and started, otherwise delete thread
    bool startThread(wxThread* thread);
    void threadOutputMsg(const wxString msg, MsgKind kind);
    virtual void updateControls() = 0;
    BackupRestoreBaseFrame(wxWindow* parent, Database* db);
private:
    wxCriticalSection critsectM;
    wxArrayString threadMsgsM;
    wxLongLong threadMsgTimeMillisM;
    void addThreadMsg(const wxString msg, bool& notificationNeeded);
    void updateMessages(size_t firstmsg, size_t lastmsg);
protected:
    wxPanel* panel_controls;
    wxStaticText* label_filename;
    wxTextCtrl* text_ctrl_filename;
    wxButton* button_browse;
    wxCheckBox* checkbox_showlog;
    wxButton* button_start;
    wxButton* button_cancel;
    wxStyledTextCtrl* text_ctrl_log;
    void setupControls();

    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif // BACKUPRESTOREBASEFRAME_H
