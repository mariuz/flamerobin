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
#include <wx/spinctrl.h>
#include <wx/thread.h>
#include <wx/textctrl.h>

#include <memory>

#include "core/Observer.h"
#include "gui/ServiceBaseFrame.h"
#include "metadata/database.h"
#include "metadata/MetadataClasses.h"

class FileTextControl;
class LogTextControl;

class BackupRestoreBaseFrame: public ServiceBaseFrame//, public Observer
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
        ID_checkbox_showlog,
        ID_spinctrl_showlogInterval,
        ID_button_browse,
        ID_button_showlog,
        ID_spinctrl_parallelworkers


    };

    wxStaticText* label_filename;
    FileTextControl* text_ctrl_filename;
    wxButton* button_browse;

    wxCheckBox* checkbox_metadata;
   
    wxCheckBox* checkbox_showlog;
    wxSpinCtrl* spinctrl_showlogInterval;

    wxTextCtrl* textCtrl_crypt;
    wxTextCtrl* textCtrl_keyholder;
    wxTextCtrl* textCtrl_keyname;

    wxTextCtrl* textCtrl_skipdata;
    wxTextCtrl* textCtrl_includedata;

    wxBoxSizer* sizerFilename;
    wxBoxSizer* sizerGeneralOptions;

    wxCheckBox* checkbox_statictime;
    wxCheckBox* checkbox_staticdelta;
    wxCheckBox* checkbox_staticpageread;
    wxCheckBox* checkbox_staticpagewrite;

    wxSpinCtrl* spinctrl_parallelworkers;


private:
    // event handling
    void OnVerboseLogChange(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};

class BackupRestoreThread : public ServiceThread
{
public:
    BackupRestoreThread(BackupRestoreBaseFrame* frame, wxString server,
        wxString username, wxString password, wxString rolename, wxString charset,
        wxString dbfilename, wxString bkfilename,
        IBPP::BRF flags, int interval, int parallel,
        wxString skipData, wxString includeData,
        wxString cryptPluginName, wxString keyPlugin, wxString keyEncrypt
    );
protected:
    wxString bkfileM;
    wxString dbfileM;
    wxString outputFileM;
    wxString skipDataM;
    wxString includeDataM;
    wxString cryptPluginNameM;
    wxString keyPluginM;
    wxString keyEncryptM;

    int intervalM;
    int parallelM;
    IBPP::BRF brfM;
};

#endif // BACKUPRESTOREBASEFRAME_H
