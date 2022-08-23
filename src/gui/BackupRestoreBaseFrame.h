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
#include "gui/ThreadBaseFrame.h"
#include "metadata/database.h"
#include "metadata/MetadataClasses.h"

class FileTextControl;
class LogTextControl;

class BackupRestoreBaseFrame: public ThreadBaseFrame//, public Observer
{
public:

    // make sure that thread gets deleted
    virtual bool Destroy();
protected:

    void cancelBackupRestore();
    //void clearLog();
    virtual void doReadConfigSettings(const wxString& prefix);
    virtual void doWriteConfigSettings(const wxString& prefix) const;
    virtual const wxString getStorageName() const;

    virtual void createControls();
    virtual void layoutControls();
    virtual void updateControls();

    BackupRestoreBaseFrame(wxWindow* parent, DatabasePtr db);
private:


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

    wxStaticText* label_filename;
    FileTextControl* text_ctrl_filename;
    wxButton* button_browse;
private:
    // event handling
    void OnVerboseLogChange(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};

#endif // BACKUPRESTOREBASEFRAME_H
