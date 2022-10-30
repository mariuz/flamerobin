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
#include "gui/StyleGuide.h"
#include "gui/controls/DndTextControls.h"
#include "gui/controls/LogTextControl.h"
#include "metadata/database.h"
#include "metadata/server.h"

BackupRestoreBaseFrame::BackupRestoreBaseFrame(wxWindow* parent,
        DatabasePtr db)
    : ServiceBaseFrame(parent, db)
{
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
    return ServiceBaseFrame::Destroy();
}

void BackupRestoreBaseFrame::doReadConfigSettings(const wxString& prefix)
{
    BaseFrame::doReadConfigSettings(prefix);


    wxString strValue;
    bool boolValue;
    int intValue;

    strValue = "";
    config().getValue(prefix + Config::pathSeparator + "backupfilename", strValue);
    if (!strValue.empty())
        text_ctrl_filename->SetValue(strValue);

    
    boolValue = false;
    config().getValue(prefix + Config::pathSeparator + "metadata", boolValue);
    checkbox_metadata->SetValue(boolValue);
    
    boolValue = false;
    config().getValue(prefix + Config::pathSeparator + "verboselog", boolValue);
    checkbox_showlog->SetValue(boolValue);
    intValue = 0;
    config().getValue(prefix + Config::pathSeparator + "verboselog_interval", intValue);
    spinctrl_showlogInterval->SetValue(intValue);

    strValue = "";
    config().getValue(prefix + Config::pathSeparator + "cryptplugin_name", strValue);
    if (!strValue.empty())
        textCtrl_crypt->SetValue(strValue);
    
    strValue = ""; 
    config().getValue(prefix + Config::pathSeparator + "keyholder_name", strValue);
    if (!strValue.empty())
        textCtrl_keyholder->SetValue(strValue);
    
    strValue = "";
    config().getValue(prefix + Config::pathSeparator + "key_name", strValue);
    if (!strValue.empty())
        textCtrl_keyname->SetValue(strValue);

    strValue = "";
    config().getValue(prefix + Config::pathSeparator + "skipdata", strValue);
    if (!strValue.empty())
        textCtrl_skipdata->SetValue(strValue);

    strValue = "";
    config().getValue(prefix + Config::pathSeparator + "includedata", strValue);
    if (!strValue.empty())
        textCtrl_includedata->SetValue(strValue);


    boolValue = false;
    config().getValue(prefix + Config::pathSeparator + "static_time", boolValue);
    checkbox_statictime->SetValue(boolValue);
    boolValue = false;
    config().getValue(prefix + Config::pathSeparator + "static_delta", boolValue);
    checkbox_staticdelta->SetValue(boolValue);
    boolValue = false;
    config().getValue(prefix + Config::pathSeparator + "static_pageread", boolValue);
    checkbox_staticpageread->SetValue(boolValue);
    boolValue = false;
    config().getValue(prefix + Config::pathSeparator + "static_pagewrite", boolValue);
    checkbox_staticpagewrite->SetValue(boolValue);

    intValue = 0;
    config().getValue(prefix + Config::pathSeparator + "parallel_workers", intValue);
    spinctrl_parallelworkers->SetValue(intValue);

}

void BackupRestoreBaseFrame::doWriteConfigSettings(const wxString& prefix) const
{
    BaseFrame::doWriteConfigSettings(prefix);

    config().setValue(prefix + Config::pathSeparator + "backupfilename",
        text_ctrl_filename->GetValue());

    config().setValue(prefix + Config::pathSeparator + "metadata",
        checkbox_metadata->GetValue());
    
    config().setValue(prefix + Config::pathSeparator + "verboselog",
        checkbox_showlog->GetValue());
    config().setValue(prefix + Config::pathSeparator + "verboselog_interval",
        spinctrl_showlogInterval->GetValue());

    config().setValue(prefix + Config::pathSeparator + "cryptplugin_name",
        textCtrl_crypt->GetValue());
    config().setValue(prefix + Config::pathSeparator + "keyholder_name",
        textCtrl_keyholder->GetValue());
    config().setValue(prefix + Config::pathSeparator + "key_name",
        textCtrl_keyname->GetValue());

    config().setValue(prefix + Config::pathSeparator + "skipdata",
        textCtrl_skipdata->GetValue());
    config().setValue(prefix + Config::pathSeparator + "includedata",
        textCtrl_includedata->GetValue());

    config().setValue(prefix + Config::pathSeparator + "static_time",
        checkbox_statictime->GetValue());
    config().setValue(prefix + Config::pathSeparator + "static_delta",
        checkbox_staticdelta->GetValue());
    config().setValue(prefix + Config::pathSeparator + "static_pageread",
        checkbox_staticpageread->GetValue());
    config().setValue(prefix + Config::pathSeparator + "static_pagewrite",
        checkbox_staticpagewrite->GetValue());

    config().setValue(prefix + Config::pathSeparator + "parallel_workers",
        spinctrl_parallelworkers->GetValue());

}


const wxString BackupRestoreBaseFrame::getStorageName() const
{
    if (DatabasePtr db = getDatabase())
        return getName() + Config::pathSeparator + db->getItemPath();
    return wxEmptyString;
}

void BackupRestoreBaseFrame::createControls()
{
    ServiceBaseFrame::createControls();

    label_filename = new wxStaticText(panel_controls, wxID_ANY,
        _("Backup file:"));
    text_ctrl_filename = new FileTextControl(panel_controls,
        ID_text_ctrl_filename, wxEmptyString);
    button_browse = new wxButton(panel_controls, ID_button_browse, _("..."),
        wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

    checkbox_metadata = new wxCheckBox(panel_controls, wxID_ANY,
        _("Only metadata (FB2.5+)"));

    checkbox_showlog = new wxCheckBox(panel_controls, ID_checkbox_showlog,
        _("Show complete log"));

    spinctrl_showlogInterval = new wxSpinCtrl(panel_controls, ID_spinctrl_showlogInterval);
    spinctrl_showlogInterval->SetRange(0, 32767);

    textCtrl_crypt = new wxTextCtrl(panel_controls, wxID_ANY, wxEmptyString);
    textCtrl_keyholder = new wxTextCtrl(panel_controls, wxID_ANY, wxEmptyString);
    textCtrl_keyname = new wxTextCtrl(panel_controls, wxID_ANY, wxEmptyString);

    textCtrl_skipdata = new wxTextCtrl(panel_controls, wxID_ANY, wxEmptyString);
    textCtrl_includedata = new wxTextCtrl(panel_controls, wxID_ANY, wxEmptyString);


    checkbox_statictime = new wxCheckBox(panel_controls, wxID_ANY, _("Time from start"));
    checkbox_staticdelta = new wxCheckBox(panel_controls, wxID_ANY, _("Delta time"));
    checkbox_staticpageread = new wxCheckBox(panel_controls, wxID_ANY, _("Page reads"));
    checkbox_staticpagewrite = new wxCheckBox(panel_controls, wxID_ANY, _("Page writes"));

    spinctrl_parallelworkers = new wxSpinCtrl(panel_controls, ID_spinctrl_parallelworkers);
    spinctrl_parallelworkers->SetRange(0, 32767);
}

void BackupRestoreBaseFrame::layoutControls()
{
    ServiceBaseFrame::layoutControls();

    int wh = text_ctrl_filename->GetMinHeight();
    button_browse->SetSize(wh, wh);

    //sizerFilename = new wxStaticBoxSizer( wxHORIZONTAL, panel_controls, _("General Options"));
    sizerFilename = new wxBoxSizer(wxHORIZONTAL);
    sizerFilename->Add(label_filename, 0, wxALIGN_CENTER_VERTICAL);
    sizerFilename->Add(styleguide().getControlLabelMargin(), 0);
    sizerFilename->Add(text_ctrl_filename, 1, wxALIGN_CENTER_VERTICAL);
    sizerFilename->Add(styleguide().getBrowseButtonMargin(), 0);
    sizerFilename->Add(button_browse, 0, wxALIGN_CENTER_VERTICAL);

    sizerGeneralOptions = new wxStaticBoxSizer(wxVERTICAL, panel_controls, _("General Options"));
    sizerGeneralOptions->Add(0, styleguide().getFrameMargin(wxTOP));


    {
        wxGridSizer* gsizer = new wxGridSizer(1, 4,
            styleguide().getCheckboxSpacing(),
            styleguide().getUnrelatedControlMargin(wxHORIZONTAL));

        gsizer->Add(checkbox_metadata, 0, wxEXPAND);
        gsizer->Add(checkbox_showlog, 0, wxEXPAND);
        {
            wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
            sizer->Add(new wxStaticText(panel_controls, wxID_ANY,
                _("Verbose interval (FB3.0+)")), 0, wxALIGN_CENTER_VERTICAL);
            sizer->Add(styleguide().getControlLabelMargin(), 0);
            sizer->Add(spinctrl_showlogInterval, 1, wxALIGN_CENTER_VERTICAL);

            gsizer->Add(sizer);
        }
        {
            wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
            sizer->Add(new wxStaticText(panel_controls, wxID_ANY,
                _("Parallel (FB3.0+)")), 0, wxALIGN_CENTER_VERTICAL);
            sizer->Add(styleguide().getControlLabelMargin(), 0);
            sizer->Add(spinctrl_parallelworkers, 1, wxALIGN_CENTER_VERTICAL);

            gsizer->Add(sizer);
        }

        sizerGeneralOptions->Add(gsizer);
        sizerGeneralOptions->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
    }

    {
        wxStaticBoxSizer* sizerStatic = new wxStaticBoxSizer(wxVERTICAL, panel_controls, _("Show statistics (FB2.5+)"));
        sizerStatic->Add(0, styleguide().getFrameMargin(wxTOP));
        {
            wxGridSizer* gsizer = new wxGridSizer(1, 4,
                styleguide().getCheckboxSpacing(),
                styleguide().getUnrelatedControlMargin(wxHORIZONTAL));
            gsizer->Add(checkbox_statictime, 0, wxEXPAND);
            gsizer->Add(checkbox_staticdelta, 0, wxEXPAND);
            gsizer->Add(checkbox_staticpageread, 0, wxEXPAND);
            gsizer->Add(checkbox_staticpagewrite, 0, wxEXPAND);

            sizerStatic->Add(gsizer, 0, wxEXPAND);
        }
        sizerGeneralOptions->Add(sizerStatic, 0, wxEXPAND);
        sizerGeneralOptions->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));

    }

    {
        wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
        sizer->Add(new wxStaticText(panel_controls, wxID_ANY,
            _("Skip data for table(s) (FB3.0+)")), 0, wxALIGN_CENTER_VERTICAL);
        sizer->Add(styleguide().getControlLabelMargin(), 0);
        sizer->Add(textCtrl_skipdata, 1, wxALIGN_CENTER_VERTICAL);

        sizerGeneralOptions->Add(sizer, 0, wxEXPAND);
        sizerGeneralOptions->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
    }

    {
        wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
        sizer->Add(new wxStaticText(panel_controls, wxID_ANY,
            _("Include data of table(s) (FB4.0+)")), 0, wxALIGN_CENTER_VERTICAL);
        sizer->Add(styleguide().getControlLabelMargin(), 0);
        sizer->Add(textCtrl_includedata, 1, wxALIGN_CENTER_VERTICAL);

        sizerGeneralOptions->Add(sizer, 0, wxEXPAND);
        sizerGeneralOptions->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
    }

    {
        wxStaticBoxSizer* sizerBox = new wxStaticBoxSizer(wxVERTICAL, panel_controls, _("Encryption (FB4.0+)"));
        sizerBox->Add(0, styleguide().getFrameMargin(wxTOP));

        {
            wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
            sizer->Add(new wxStaticText(panel_controls, wxID_ANY,
                _("Plugin name")), 0, wxALIGN_CENTER_VERTICAL);
            sizer->Add(styleguide().getControlLabelMargin(), 0);
            sizer->Add(textCtrl_crypt, 1, wxALIGN_CENTER_VERTICAL);

            sizerBox->Add(sizer, 0, wxEXPAND);
            sizerBox->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
        }

        {
            wxGridSizer* gsizer = new wxGridSizer(1, 2,
                styleguide().getRelatedControlMargin(wxHORIZONTAL),
                styleguide().getUnrelatedControlMargin(wxHORIZONTAL));

            {
                wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
                sizer->Add(new wxStaticText(panel_controls, wxID_ANY,
                    _("Key plugin ")), 0, wxALIGN_CENTER_VERTICAL);
                sizer->Add(styleguide().getControlLabelMargin(), 0);
                sizer->Add(textCtrl_keyholder, 1, wxALIGN_CENTER_VERTICAL);

                gsizer->Add(sizer, 0, wxEXPAND);
            }

            {
                wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
                sizer->Add(new wxStaticText(panel_controls, wxID_ANY,
                    _("Key for encryption")), 0, wxALIGN_CENTER_VERTICAL);
                sizer->Add(styleguide().getControlLabelMargin(), 0);
                sizer->Add(textCtrl_keyname, 1, wxALIGN_CENTER_VERTICAL);

                gsizer->Add(sizer, 0, wxEXPAND);
            }
            sizerBox->Add(gsizer, 0, wxEXPAND);
            sizerBox->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
        }


        sizerGeneralOptions->Add(sizerBox, 0, wxEXPAND);
        sizerGeneralOptions->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));

    }


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
    ServiceBaseFrame::updateControls();

    bool running = getThreadRunning();

    text_ctrl_filename->Enable(!running);
    button_browse->Enable(!running);

    checkbox_metadata->Enable(!running);
   
    checkbox_showlog->Enable(!running);
    spinctrl_showlogInterval->Enable(!running);

    textCtrl_crypt->Enable(!running);
    textCtrl_keyholder->Enable(!running);
    textCtrl_keyname->Enable(!running);

    textCtrl_skipdata->Enable(!running);
    textCtrl_includedata->Enable(!running);

    checkbox_statictime->Enable(!running);
    checkbox_staticdelta->Enable(!running);
    checkbox_staticpageread->Enable(!running);
    checkbox_staticpagewrite->Enable(!running);

    spinctrl_parallelworkers->Enable(!running);
}

//! event handlers
BEGIN_EVENT_TABLE(BackupRestoreBaseFrame, ServiceBaseFrame)
    EVT_CHECKBOX(BackupRestoreBaseFrame::ID_checkbox_showlog, BackupRestoreBaseFrame::OnVerboseLogChange)
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


BackupRestoreThread::BackupRestoreThread(BackupRestoreBaseFrame* frame,
    wxString server, wxString username, wxString password,
    wxString rolename, wxString charset, wxString dbfilename,
    wxString bkfilename, IBPP::BRF flags, int interval, int parallel,
    wxString skipData, wxString includeData, wxString cryptPluginName,
    wxString keyPlugin, wxString keyEncrypt)
    :
    dbfileM(dbfilename), bkfileM(bkfilename), intervalM(interval), parallelM(parallel),
    skipDataM(skipData), includeDataM(includeData),
    cryptPluginNameM(cryptPluginName), keyPluginM(keyPlugin), keyEncryptM(keyEncrypt),
    ServiceThread(frame, server, username, password, rolename, charset)
{
    // always use verbose flag
    brfM = (IBPP::BRF)((int)flags | (int)IBPP::brVerbose);
}
