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


#ifndef BACKUPRESTOREBASEFRAME_H
#define BACKUPRESTOREBASEFRAME_H

#include <wx/wx.h>
#include <wx/thread.h>

#include <memory>

#include "core/Observer.h"
#include "gui/BaseFrame.h"
#include "metadata/database.h"
#include "metadata/MetadataClasses.h"

class FileTextControl;
class LogTextControl;

class BackupRestoreBaseFrame: public BaseFrame, public Observer
{
public:
    enum MsgKind {
        progress_message,
        important_message,
        error_message
    };

    enum {
        ID_thread_output = 500,
        ID_thread_finished
    };

    // make sure that thread gets deleted
    virtual bool Destroy();
protected:
    wxArrayString msgsM;
    wxArrayInt msgKindsM;
    bool verboseMsgsM;

    DatabasePtr getDatabase() const;

    void cancelBackupRestore();
    void clearLog();
    virtual void doReadConfigSettings(const wxString& prefix);
    virtual void doWriteConfigSettings(const wxString& prefix) const;
    virtual const wxString getStorageName() const;
    // set threadM if thread was successfully created and started,
    // otherwise delete the thread
    bool startThread(std::unique_ptr<wxThread> thread);
    bool getThreadRunning() const;

    void threadOutputMsg(const wxString msg, MsgKind kind);
    virtual void updateControls();
    BackupRestoreBaseFrame(wxWindow* parent, DatabasePtr db);
private:
    DatabaseWeakPtr databaseM;
    wxThread* threadM;

    wxCriticalSection critsectM;
    wxArrayString threadMsgsM;
    wxLongLong threadMsgTimeMillisM;
    void addThreadMsg(const wxString msg, bool& notificationNeeded);
    void updateMessages(size_t firstmsg, size_t lastmsg);

    // observer stuff
    virtual void subjectRemoved(Subject* subject);
    virtual void update();

protected:
    enum {
        ID_text_ctrl_filename = 101,
        ID_button_browse,
        ID_button_showlog,
        ID_text_ctrl_log,
        ID_checkbox_showlog,
        ID_button_start,
    };

    wxPanel* panel_controls;
    wxStaticText* label_filename;
    FileTextControl* text_ctrl_filename;
    wxButton* button_browse;
    wxCheckBox* checkbox_showlog;
    wxButton* button_start;
    LogTextControl* text_ctrl_log;
    void setupControls();
private:
    // event handling
    void OnSettingsChange(wxCommandEvent& event);
    void OnThreadFinished(wxCommandEvent& event);
    void OnThreadOutput(wxCommandEvent& event);
    void OnVerboseLogChange(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};

#endif // BACKUPRESTOREBASEFRAME_H
