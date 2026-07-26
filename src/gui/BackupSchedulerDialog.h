/*
  Copyright (c) 2004-2026 The FlameRobin Development Team

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

#ifndef FR_BACKUPSCHEDULERDIALOG_H
#define FR_BACKUPSCHEDULERDIALOG_H

#include <vector>
#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <wx/statline.h>

#include "gui/BaseDialog.h"
#include "metadata/database.h"

class BackupSchedulerDialog : public BaseDialog
{
private:
    DatabasePtr databaseM;

    // Backup mode (gbak / nbackup)
    wxRadioBox* radioBackupModeM;
    wxTextCtrl* textOutputPatternM;

    // Frequency / Schedule
    wxChoice* choiceFrequencyM; // Daily, Weekly, Hourly, Custom Cron
    wxSpinCtrl* spinHourM;
    wxSpinCtrl* spinMinuteM;
    wxChoice* choiceDayOfWeekM;
    wxTextCtrl* textCronExpressionM;

    // Encryption & Compression
    wxCheckBox* checkboxEncryptStreamM;
    wxTextCtrl* textEncryptionKeyM;
    wxCheckBox* checkboxCompressZipM;
    wxSpinCtrl* spinRetentionDaysM;

    // Cloud Sync Options
    wxCheckBox* checkboxCloudSyncM;
    wxChoice* choiceCloudProviderM; // AWS S3, Azure Blob, S3-Compatible / MinIO
    wxTextCtrl* textCloudBucketM;
    wxTextCtrl* textCloudEndpointM;
    wxTextCtrl* textCloudAccessKeyM;
    wxTextCtrl* textCloudSecretKeyM;

    // Preview Script
    wxTextCtrl* textGeneratedScriptM;
    wxListBox* listboxActiveJobsM;

    wxButton* buttonGenerateScriptM;
    wxButton* buttonSaveJobM;
    wxButton* buttonRunNowM;
    wxButton* buttonRemoveJobM;
    wxButton* buttonCloseM;

    void createControls();
    void layoutControls();
    void updateControls();
    void updateGeneratedScript();

protected:
    virtual const wxString getName() const override;

public:
    BackupSchedulerDialog(wxWindow* parent, DatabasePtr database);
    virtual ~BackupSchedulerDialog();

    enum {
        ID_radio_backup_mode = 2100,
        ID_text_output_pattern,
        ID_choice_frequency,
        ID_spin_hour,
        ID_spin_minute,
        ID_choice_dow,
        ID_text_cron,
        ID_checkbox_encrypt,
        ID_text_key,
        ID_checkbox_compress,
        ID_spin_retention,
        ID_checkbox_cloud,
        ID_choice_cloud_provider,
        ID_text_cloud_bucket,
        ID_text_cloud_endpoint,
        ID_text_cloud_access_key,
        ID_text_cloud_secret_key,
        ID_button_generate,
        ID_button_save_job,
        ID_button_run_now,
        ID_button_remove_job,
        ID_listbox_jobs
    };

    void OnControlChange(wxCommandEvent& event);
    void OnSpinChange(wxSpinEvent& event);
    void OnButtonGenerateScript(wxCommandEvent& event);
    void OnButtonSaveJob(wxCommandEvent& event);
    void OnButtonRunNow(wxCommandEvent& event);
    void OnButtonRemoveJob(wxCommandEvent& event);
    void OnListBoxSelected(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};

#endif // FR_BACKUPSCHEDULERDIALOG_H
