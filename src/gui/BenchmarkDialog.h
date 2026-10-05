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

#ifndef FR_BENCHMARKDIALOG_H
#define FR_BENCHMARKDIALOG_H

#include <wx/wx.h>
#include <wx/gauge.h>
#include <wx/notebook.h>
#include <wx/timer.h>

#include <chrono>
#include <memory>
#include <optional>
#include <set>
#include <thread>

#include "engine/Benchmark.h"
#include "engine/BenchmarkRunner.h"
#include "gui/BaseDialog.h"
#include "metadata/database.h"

class wxCollapsiblePane;
class wxCollapsiblePaneEvent;
class wxWebView;

///
/// Database -> Performance Benchmark & Diagnosis. The benchmark runs on a
/// worker thread in its own temporary database; all GUI updates are
/// marshalled with CallAfter(). Without a database registration (db is
/// nullptr) this computer and other servers can be tested, and saved
/// reports compared.
///
class BenchmarkDialog : public BaseDialog
{
public:
    BenchmarkDialog(wxWindow* parent, Database* db);
    ~BenchmarkDialog();

protected:
    virtual const wxString getName() const override;

private:
    Database* databaseM;
    std::unique_ptr<fr::BenchmarkRunner> runnerM;
    std::thread workerM;
    bool runningM = false;
    bool closeRequestedM = false;
    std::chrono::steady_clock::time_point startTimeM;
    wxTimer timerM;
    std::set<wxString> loggedResultsM;
    wxString lastMessageM;
    fr::BenchmarkReport reportM;
    bool hasReportM = false;
    // two saved reports compared without a run
    std::optional<fr::BenchmarkComparison> comparisonM;
    std::vector<fr::BenchmarkFirebirdInstallation> installationsM;

    wxRadioBox* radio_target;
    wxChoice* choice_install;
    wxCollapsiblePane* collapse_advanced;
    wxTextCtrl* text_host;
    wxTextCtrl* text_port;
    wxTextCtrl* text_directory;
    wxTextCtrl* text_user;
    wxTextCtrl* text_password;
    wxTextCtrl* text_clientlib;
    wxChoice* choice_pagesize;
    wxCheckBox* check_comparepages;
    int automaticPageSizeM = 8192;
    wxCheckBox* check_inspect;
    wxCheckBox* check_manyusers;
    wxTextCtrl* text_earlier;
    wxRadioBox* radio_intensity;
    wxStaticText* label_status;
    wxStaticText* label_elapsed;
    wxGauge* gauge_progress;
    wxNotebook* notebook_results;
    wxTextCtrl* text_log;
    // the HTML report, or the Markdown text where no web view is available
    wxWebView* webview_report = nullptr;
    wxTextCtrl* text_report = nullptr;
    wxButton* button_start;
    wxButton* button_cancel;
    wxButton* button_copy;
    wxButton* button_save;
    wxButton* button_compare;
    wxCheckBox* check_private;
    wxButton* button_close;

    void createControls();
    void updateControls();
    bool buildParams(fr::BenchmarkConnectionParams& params);
    void fillCredentials(fr::BenchmarkConnectionParams& params);

    // temporary databases are remembered in the configuration until they
    // are dropped, so that a crash cannot leave them behind unnoticed
    std::vector<wxString> pendingEntriesM;
    static void addPendingDatabase(const wxString& entry);
    static void removePendingDatabase(const wxString& entry);
    void dropPendingDatabases();
    void appendLog(const wxString& line);
    wxString formatElapsed() const;
    // the settings are hidden after a run, so that the report has room
    wxSizer* sizerSettingsM = nullptr;
    bool settingsShownM = true;
    void showSettings(bool show);
    void showReport(const fr::BenchmarkReport& report);
    // the report with the selected earlier report, without private
    // information if that is selected: what is shown, copied and saved
    fr::BenchmarkReport displayedReport() const;
    fr::BenchmarkComparison displayedComparison() const;
    void showComparison();
    std::optional<fr::BenchmarkSnapshot> readSnapshot(const wxString& path, bool showErrors);
    // the registration of the selected database, empty without one
    wxString registrationId() const;

    // called on the GUI thread; aborted means that run() itself failed, so
    // nothing is known about the temporary databases
    void onProgress(const fr::BenchmarkProgress& progress);
    void onFinished(const fr::BenchmarkReport& report, bool aborted);
    void requestClose();

    void OnStart(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnCopyMarkdown(wxCommandEvent& event);
    void OnSaveReport(wxCommandEvent& event);
    void OnPrivateChanged(wxCommandEvent& event);
    void OnBrowseEarlier(wxCommandEvent& event);
    void OnCompareReports(wxCommandEvent& event);
    void OnCloseButton(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnTargetChanged(wxCommandEvent& event);
    void OnAdvancedToggled(wxCollapsiblePaneEvent& event);
    void OnTimer(wxTimerEvent& event);

    DECLARE_EVENT_TABLE()
};

#endif // FR_BENCHMARKDIALOG_H
