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
#include "gui/BackupRestoreBaseFrame.h"
#include "gui/controls/DndTextControls.h"
#include "gui/controls/LogTextControl.h"
#include "metadata/database.h"
#include "metadata/server.h"

BackupRestoreBaseFrame::BackupRestoreBaseFrame(wxWindow* parent,
        DatabasePtr db)
    : ThreadBaseFrame(parent, db)
{
    //wxASSERT(db);
    //db->attachObserver(this, false);

    //threadMsgTimeMillisM = 0;
    verboseMsgsM = true;

    // create controls in constructor of descendant class (correct tab order)
    panel_controls = 0;
    checkbox_showlog = 0;
    button_start = 0;
    text_ctrl_log = 0;

    SetIcon(wxArtProvider::GetIcon(ART_Backup, wxART_FRAME_ICON));
}

//! implementation details
void BackupRestoreBaseFrame::cancelBackupRestore()
{
    cancelThread();
}


bool BackupRestoreBaseFrame::Destroy()
{
    cancelBackupRestore();
    return ThreadBaseFrame::Destroy();
}

void BackupRestoreBaseFrame::doReadConfigSettings(const wxString& prefix)
{
    BaseFrame::doReadConfigSettings(prefix);

    bool verbose = true;
    config().getValue(prefix + Config::pathSeparator + "verboselog",
        verbose);
    checkbox_showlog->SetValue(verbose);

    wxString bkfile;
    config().getValue(prefix + Config::pathSeparator + "backupfilename",
        bkfile);
    if (!bkfile.empty())
        text_ctrl_filename->SetValue(bkfile);
}

void BackupRestoreBaseFrame::doWriteConfigSettings(const wxString& prefix) const
{
    BaseFrame::doWriteConfigSettings(prefix);
    config().setValue(prefix + Config::pathSeparator + "verboselog",
        checkbox_showlog->GetValue());
    config().setValue(prefix + Config::pathSeparator + "backupfilename",
        text_ctrl_filename->GetValue());
}


const wxString BackupRestoreBaseFrame::getStorageName() const
{
    if (DatabasePtr db = getDatabase())
        return getName() + Config::pathSeparator + db->getItemPath();
    return wxEmptyString;
}

void BackupRestoreBaseFrame::createControls()
{
    ThreadBaseFrame::createControls();

    label_filename = new wxStaticText(panel_controls, wxID_ANY,
        _("Backup file:"));
    text_ctrl_filename = new FileTextControl(panel_controls,
        ID_text_ctrl_filename, wxEmptyString);
    button_browse = new wxButton(panel_controls, ID_button_browse, _("..."),
        wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

}

void BackupRestoreBaseFrame::layoutControls()
{
}

void BackupRestoreBaseFrame::subjectRemoved(Subject* subject)
{
    DatabasePtr db = getDatabase();
    if (!db || !db->isConnected() || subject == db.get())
        Close();
}

void BackupRestoreBaseFrame::update()
{
    DatabasePtr db = getDatabase();
    if (db)
        updateControls();
    else
        Close();
}

void BackupRestoreBaseFrame::updateControls()
{
    // empty implementation to allow this to be called from update()
    // which could happen in the constructor, when descendant isn't
    // completely initialized yet
}

//! event handlers
BEGIN_EVENT_TABLE(BackupRestoreBaseFrame, BaseFrame)
    EVT_CHECKBOX(BackupRestoreBaseFrame::ID_checkbox_showlog, BackupRestoreBaseFrame::OnVerboseLogChange)
    EVT_MENU(BackupRestoreBaseFrame::ID_thread_finished, BackupRestoreBaseFrame::OnThreadFinished)
    EVT_MENU(BackupRestoreBaseFrame::ID_thread_output, BackupRestoreBaseFrame::OnThreadOutput)
    EVT_TEXT(BackupRestoreBaseFrame::ID_text_ctrl_filename, BackupRestoreBaseFrame::OnSettingsChange)
END_EVENT_TABLE()


void BackupRestoreBaseFrame::OnVerboseLogChange(wxCommandEvent& WXUNUSED(event))
{
    wxBusyCursor wait;
    verboseMsgsM = checkbox_showlog->IsChecked();

    wxWindowUpdateLocker freeze(text_ctrl_log);

    text_ctrl_log->ClearAll();
    updateMessages(0, msgsM.GetCount());
}

