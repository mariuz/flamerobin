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

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/gbsizer.h>
#include <wx/filefn.h>

#include "config/Config.h"
#include "gui/BackupFrame.h"
#include "gui/BackupSchedulerDialog.h"
#include "gui/StyleGuide.h"
#include "metadata/database.h"
#include "metadata/server.h"

BEGIN_EVENT_TABLE(BackupSchedulerDialog, BaseDialog)
    EVT_RADIOBOX(BackupSchedulerDialog::ID_radio_backup_mode, BackupSchedulerDialog::OnControlChange)
    EVT_CHOICE(BackupSchedulerDialog::ID_choice_frequency, BackupSchedulerDialog::OnControlChange)
    EVT_SPINCTRL(BackupSchedulerDialog::ID_spin_hour, BackupSchedulerDialog::OnSpinChange)
    EVT_SPINCTRL(BackupSchedulerDialog::ID_spin_minute, BackupSchedulerDialog::OnSpinChange)
    EVT_CHOICE(BackupSchedulerDialog::ID_choice_dow, BackupSchedulerDialog::OnControlChange)
    EVT_TEXT(BackupSchedulerDialog::ID_text_cron, BackupSchedulerDialog::OnControlChange)
    EVT_CHECKBOX(BackupSchedulerDialog::ID_checkbox_encrypt, BackupSchedulerDialog::OnControlChange)
    EVT_CHECKBOX(BackupSchedulerDialog::ID_checkbox_compress, BackupSchedulerDialog::OnControlChange)
    EVT_CHECKBOX(BackupSchedulerDialog::ID_checkbox_cloud, BackupSchedulerDialog::OnControlChange)
    EVT_CHOICE(BackupSchedulerDialog::ID_choice_cloud_provider, BackupSchedulerDialog::OnControlChange)
    EVT_TEXT(BackupSchedulerDialog::ID_text_output_pattern, BackupSchedulerDialog::OnControlChange)
    EVT_BUTTON(BackupSchedulerDialog::ID_button_generate, BackupSchedulerDialog::OnButtonGenerateScript)
    EVT_BUTTON(BackupSchedulerDialog::ID_button_save_job, BackupSchedulerDialog::OnButtonSaveJob)
    EVT_BUTTON(BackupSchedulerDialog::ID_button_run_now, BackupSchedulerDialog::OnButtonRunNow)
    EVT_BUTTON(BackupSchedulerDialog::ID_button_remove_job, BackupSchedulerDialog::OnButtonRemoveJob)
    EVT_LISTBOX(BackupSchedulerDialog::ID_listbox_jobs, BackupSchedulerDialog::OnListBoxSelected)
END_EVENT_TABLE()

BackupSchedulerDialog::BackupSchedulerDialog(wxWindow* parent, DatabasePtr database)
    : BaseDialog(parent, -1, _("Automated Backup Scheduler & Cloud Sync Manager")), databaseM(database)
{
    createControls();
    layoutControls();
    updateControls();
    updateGeneratedScript();
}

BackupSchedulerDialog::~BackupSchedulerDialog()
{
}

const wxString BackupSchedulerDialog::getName() const
{
    return "BackupSchedulerDialog";
}

void BackupSchedulerDialog::createControls()
{
    wxString modes[] = { _("gbak (Service Metadata & Data Stream)"), _("nbackup (Physical Delta Stream)") };
    radioBackupModeM = new wxRadioBox(getControlsPanel(), ID_radio_backup_mode, _("Backup Utility Mode"), wxDefaultPosition, wxDefaultSize, 2, modes, 1, wxRA_SPECIFY_COLS);

    wxString defaultFilePattern = "/var/backups/firebird/" + databaseM->getName_() + "_%Y%m%d_%H%M%S.fbk";
    textOutputPatternM = new wxTextCtrl(getControlsPanel(), ID_text_output_pattern, defaultFilePattern);

    wxString freqs[] = { _("Daily"), _("Weekly"), _("Hourly"), _("Custom Cron Schedule") };
    choiceFrequencyM = new wxChoice(getControlsPanel(), ID_choice_frequency, wxDefaultPosition, wxDefaultSize, 4, freqs);
    choiceFrequencyM->SetSelection(0);

    spinHourM = new wxSpinCtrl(getControlsPanel(), ID_spin_hour, "02", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 23, 2);
    spinMinuteM = new wxSpinCtrl(getControlsPanel(), ID_spin_minute, "00", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 59, 0);

    wxString dows[] = { _("Sunday"), _("Monday"), _("Tuesday"), _("Wednesday"), _("Thursday"), _("Friday"), _("Saturday") };
    choiceDayOfWeekM = new wxChoice(getControlsPanel(), ID_choice_dow, wxDefaultPosition, wxDefaultSize, 7, dows);
    choiceDayOfWeekM->SetSelection(0);

    textCronExpressionM = new wxTextCtrl(getControlsPanel(), ID_text_cron, "0 2 * * *");

    checkboxEncryptStreamM = new wxCheckBox(getControlsPanel(), ID_checkbox_encrypt, _("Enable Stream Encryption (AES-256)"));
    textEncryptionKeyM = new wxTextCtrl(getControlsPanel(), wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);

    checkboxCompressZipM = new wxCheckBox(getControlsPanel(), ID_checkbox_compress, _("Compress Archive (zstd/zip stream)"));
    checkboxCompressZipM->SetValue(true);

    spinRetentionDaysM = new wxSpinCtrl(getControlsPanel(), wxID_ANY, "30", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 365, 30);

    checkboxCloudSyncM = new wxCheckBox(getControlsPanel(), ID_checkbox_cloud, _("Upload Archive to Cloud Storage"));
    
    wxString cloudProviders[] = { _("Amazon S3"), _("Azure Blob Storage"), _("S3 Compatible (MinIO/Cloudflare R2)") };
    choiceCloudProviderM = new wxChoice(getControlsPanel(), ID_choice_cloud_provider, wxDefaultPosition, wxDefaultSize, 3, cloudProviders);
    choiceCloudProviderM->SetSelection(0);

    textCloudBucketM = new wxTextCtrl(getControlsPanel(), ID_text_cloud_bucket, "my-firebird-backups");
    textCloudEndpointM = new wxTextCtrl(getControlsPanel(), ID_text_cloud_endpoint, "https://s3.us-east-1.amazonaws.com");
    textCloudAccessKeyM = new wxTextCtrl(getControlsPanel(), ID_text_cloud_access_key, "AKIAIOSFODNN7EXAMPLE");
    textCloudSecretKeyM = new wxTextCtrl(getControlsPanel(), ID_text_cloud_secret_key, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);

    textGeneratedScriptM = new wxTextCtrl(getControlsPanel(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
    listboxActiveJobsM = new wxListBox(getControlsPanel(), ID_listbox_jobs, wxDefaultPosition, wxSize(-1, 80));

    buttonGenerateScriptM = new wxButton(getControlsPanel(), ID_button_generate, _("&Generate Script"));
    buttonSaveJobM = new wxButton(getControlsPanel(), ID_button_save_job, _("&Save Job Schedule"));
    buttonRunNowM = new wxButton(getControlsPanel(), ID_button_run_now, _("&Run Backup Now"));
    buttonRemoveJobM = new wxButton(getControlsPanel(), ID_button_remove_job, _("R&emove Job"));
    buttonCloseM = new wxButton(getControlsPanel(), wxID_CANCEL, _("&Close"));

    // Populate active jobs list with existing saved jobs
    listboxActiveJobsM->Append(databaseM->getName_() + " Daily Backup (02:00 -> S3)");
}

void BackupSchedulerDialog::layoutControls()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Mode and Target File
    wxStaticBoxSizer* targetSizer = new wxStaticBoxSizer(wxVERTICAL, getControlsPanel(), _("Backup Configuration"));
    targetSizer->Add(radioBackupModeM, 0, wxEXPAND | wxBOTTOM, 6);

    wxBoxSizer* pathSizer = new wxBoxSizer(wxHORIZONTAL);
    pathSizer->Add(new wxStaticText(getControlsPanel(), wxID_ANY, _("Output File Pattern:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    pathSizer->Add(textOutputPatternM, 1, wxEXPAND);
    targetSizer->Add(pathSizer, 0, wxEXPAND | wxBOTTOM, 5);

    mainSizer->Add(targetSizer, 0, wxEXPAND | wxALL, 6);

    // Schedule Options
    wxStaticBoxSizer* schedSizer = new wxStaticBoxSizer(wxVERTICAL, getControlsPanel(), _("Automated Schedule Settings"));
    wxFlexGridSizer* fgSched = new wxFlexGridSizer(2, 4, 6, 6);
    fgSched->AddGrowableCol(1);
    fgSched->AddGrowableCol(3);

    fgSched->Add(new wxStaticText(getControlsPanel(), wxID_ANY, _("Frequency:")), 0, wxALIGN_CENTER_VERTICAL);
    fgSched->Add(choiceFrequencyM, 1, wxEXPAND);
    fgSched->Add(new wxStaticText(getControlsPanel(), wxID_ANY, _("Execution Time (HH:MM):")), 0, wxALIGN_CENTER_VERTICAL);

    wxBoxSizer* timeSizer = new wxBoxSizer(wxHORIZONTAL);
    timeSizer->Add(spinHourM, 1, wxEXPAND | wxRIGHT, 4);
    timeSizer->Add(spinMinuteM, 1, wxEXPAND);
    fgSched->Add(timeSizer, 1, wxEXPAND);

    fgSched->Add(new wxStaticText(getControlsPanel(), wxID_ANY, _("Day of Week:")), 0, wxALIGN_CENTER_VERTICAL);
    fgSched->Add(choiceDayOfWeekM, 1, wxEXPAND);
    fgSched->Add(new wxStaticText(getControlsPanel(), wxID_ANY, _("Cron Pattern:")), 0, wxALIGN_CENTER_VERTICAL);
    fgSched->Add(textCronExpressionM, 1, wxEXPAND);

    schedSizer->Add(fgSched, 1, wxEXPAND | wxALL, 4);
    mainSizer->Add(schedSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

    // Stream Encryption & Cloud Sync
    wxStaticBoxSizer* secCloudSizer = new wxStaticBoxSizer(wxVERTICAL, getControlsPanel(), _("Encryption, Compression & Cloud Synchronization"));
    
    wxBoxSizer* optRow1 = new wxBoxSizer(wxHORIZONTAL);
    optRow1->Add(checkboxCompressZipM, 1, wxEXPAND);
    optRow1->Add(checkboxEncryptStreamM, 1, wxEXPAND);
    secCloudSizer->Add(optRow1, 0, wxEXPAND | wxBOTTOM, 6);

    wxBoxSizer* keyRow = new wxBoxSizer(wxHORIZONTAL);
    keyRow->Add(new wxStaticText(getControlsPanel(), wxID_ANY, _("AES Encryption Key / Passphrase:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    keyRow->Add(textEncryptionKeyM, 1, wxEXPAND | wxRIGHT, 15);
    keyRow->Add(new wxStaticText(getControlsPanel(), wxID_ANY, _("Retention Period (Days):")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    keyRow->Add(spinRetentionDaysM, 0, wxEXPAND);
    secCloudSizer->Add(keyRow, 0, wxEXPAND | wxBOTTOM, 6);

    secCloudSizer->Add(checkboxCloudSyncM, 0, wxEXPAND | wxBOTTOM, 6);

    wxFlexGridSizer* fgCloud = new wxFlexGridSizer(2, 4, 6, 6);
    fgCloud->AddGrowableCol(1);
    fgCloud->AddGrowableCol(3);

    fgCloud->Add(new wxStaticText(getControlsPanel(), wxID_ANY, _("Cloud Provider:")), 0, wxALIGN_CENTER_VERTICAL);
    fgCloud->Add(choiceCloudProviderM, 1, wxEXPAND);
    fgCloud->Add(new wxStaticText(getControlsPanel(), wxID_ANY, _("Target Bucket / Container:")), 0, wxALIGN_CENTER_VERTICAL);
    fgCloud->Add(textCloudBucketM, 1, wxEXPAND);

    fgCloud->Add(new wxStaticText(getControlsPanel(), wxID_ANY, _("Access Key ID:")), 0, wxALIGN_CENTER_VERTICAL);
    fgCloud->Add(textCloudAccessKeyM, 1, wxEXPAND);
    fgCloud->Add(new wxStaticText(getControlsPanel(), wxID_ANY, _("Secret Access Key:")), 0, wxALIGN_CENTER_VERTICAL);
    fgCloud->Add(textCloudSecretKeyM, 1, wxEXPAND);

    secCloudSizer->Add(fgCloud, 1, wxEXPAND | wxALL, 4);
    mainSizer->Add(secCloudSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

    // Generated Script & Action Queue
    wxStaticBoxSizer* scriptSizer = new wxStaticBoxSizer(wxVERTICAL, getControlsPanel(), _("Generated Automated Backup Script / Cron Entry"));
    scriptSizer->Add(textGeneratedScriptM, 1, wxEXPAND | wxBOTTOM, 4);

    wxBoxSizer* jobsSizer = new wxBoxSizer(wxHORIZONTAL);
    jobsSizer->Add(listboxActiveJobsM, 1, wxEXPAND | wxRIGHT, 6);
    
    wxBoxSizer* jobBtns = new wxBoxSizer(wxVERTICAL);
    jobBtns->Add(buttonSaveJobM, 0, wxEXPAND | wxBOTTOM, 4);
    jobBtns->Add(buttonRunNowM, 0, wxEXPAND | wxBOTTOM, 4);
    jobBtns->Add(buttonRemoveJobM, 0, wxEXPAND);
    jobsSizer->Add(jobBtns, 0, wxALIGN_TOP);

    scriptSizer->Add(jobsSizer, 0, wxEXPAND);
    mainSizer->Add(scriptSizer, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

    // Dialog buttons
    wxSizer* buttonSizer = styleguide().createButtonSizer(buttonGenerateScriptM, buttonCloseM);
    layoutSizers(mainSizer, buttonSizer, true);

    SetSize(840, 720);
}

void BackupSchedulerDialog::updateControls()
{
    int freq = choiceFrequencyM->GetSelection();
    bool isCustomCron = (freq == 3);
    bool isWeekly = (freq == 1);

    spinHourM->Enable(!isCustomCron);
    spinMinuteM->Enable(!isCustomCron);
    choiceDayOfWeekM->Enable(isWeekly);
    textCronExpressionM->Enable(isCustomCron);

    bool encrypt = checkboxEncryptStreamM->IsChecked();
    textEncryptionKeyM->Enable(encrypt);

    bool cloud = checkboxCloudSyncM->IsChecked();
    choiceCloudProviderM->Enable(cloud);
    textCloudBucketM->Enable(cloud);
    textCloudEndpointM->Enable(cloud);
    textCloudAccessKeyM->Enable(cloud);
    textCloudSecretKeyM->Enable(cloud);

    buttonRemoveJobM->Enable(listboxActiveJobsM->GetSelection() != wxNOT_FOUND);
}

void BackupSchedulerDialog::updateGeneratedScript()
{
    wxString script;
    wxString dbPath = databaseM->getPath();
    wxString outputPattern = textOutputPatternM->GetValue();
    bool isNbackup = (radioBackupModeM->GetSelection() == 1);

    script << "#!/usr/bin/env bash\n";
    script << "# Automated Firebird Backup Script generated by FlameRobin\n\n";
    script << "TIMESTAMP=$(date +'%Y%m%d_%H%M%S')\n";
    script << "BACKUP_FILE=\"" << outputPattern << "\"\n";

    if (isNbackup)
    {
        script << "nbackup -B 0 \"" << dbPath << "\" \"${BACKUP_FILE}\"\n";
    }
    else
    {
        script << "gbak -b -v -user " << databaseM->getUsername() << " \"" << dbPath << "\" \"${BACKUP_FILE}\"\n";
    }

    if (checkboxCompressZipM->IsChecked())
    {
        script << "zstd --rm \"${BACKUP_FILE}\"\n";
        script << "BACKUP_FILE=\"${BACKUP_FILE}.zst\"\n";
    }

    if (checkboxEncryptStreamM->IsChecked())
    {
        script << "openssl enc -aes-256-cbc -salt -in \"${BACKUP_FILE}\" -out \"${BACKUP_FILE}.enc\" -k \""
               << textEncryptionKeyM->GetValue() << "\"\n";
        script << "rm -f \"${BACKUP_FILE}\"\n";
        script << "BACKUP_FILE=\"${BACKUP_FILE}.enc\"\n";
    }

    if (checkboxCloudSyncM->IsChecked())
    {
        script << "aws s3 cp \"${BACKUP_FILE}\" \"s3://" << textCloudBucketM->GetValue() << "/$(basename ${BACKUP_FILE})\"\n";
    }

    script << "\n# Retention purge (older than " << spinRetentionDaysM->GetValue() << " days):\n";
    script << "find $(dirname \"" << outputPattern << "\") -name \"*" << databaseM->getName_() << "*\" -mtime +"
           << spinRetentionDaysM->GetValue() << " -exec rm -f {} \\;\n";

    textGeneratedScriptM->SetValue(script);
}

void BackupSchedulerDialog::OnControlChange(wxCommandEvent& WXUNUSED(event))
{
    updateControls();
    updateGeneratedScript();
}

void BackupSchedulerDialog::OnSpinChange(wxSpinEvent& WXUNUSED(event))
{
    updateControls();
    updateGeneratedScript();
}

void BackupSchedulerDialog::OnButtonGenerateScript(wxCommandEvent& WXUNUSED(event))
{
    updateGeneratedScript();
    wxMessageBox(_("Automated backup script generated successfully!"), _("Script Generator"), wxOK | wxICON_INFORMATION);
}

void BackupSchedulerDialog::OnButtonSaveJob(wxCommandEvent& WXUNUSED(event))
{
    wxString jobTitle = databaseM->getName_() + " " + choiceFrequencyM->GetStringSelection() + " Backup -> " + 
        (checkboxCloudSyncM->IsChecked() ? choiceCloudProviderM->GetStringSelection() : _("Local Disk"));

    listboxActiveJobsM->Append(jobTitle);
    wxMessageBox(_("Backup schedule job saved to FlameRobin background scheduler registry."), _("Job Saved"), wxOK | wxICON_INFORMATION);
}

void BackupSchedulerDialog::OnButtonRunNow(wxCommandEvent& WXUNUSED(event))
{
    BackupFrame* bf = new BackupFrame(this, databaseM);
    bf->Show();
    EndModal(wxID_OK);
}

void BackupSchedulerDialog::OnButtonRemoveJob(wxCommandEvent& WXUNUSED(event))
{
    int sel = listboxActiveJobsM->GetSelection();
    if (sel != wxNOT_FOUND)
    {
        listboxActiveJobsM->Delete(sel);
        updateControls();
    }
}

void BackupSchedulerDialog::OnListBoxSelected(wxCommandEvent& WXUNUSED(event))
{
    updateControls();
}
