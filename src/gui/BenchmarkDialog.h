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

#include <chrono>
#include <memory>
#include <optional>
#include <set>
#include <thread>
#include <utility>
#include <vector>

#include "engine/Benchmark.h"
#include "engine/BenchmarkResults.h"
#include "engine/BenchmarkRunner.h"
#include "gui/BaseDialog.h"
#include "metadata/database.h"

class BenchmarkProgressPanel;
class BenchmarkResultsView;
class wxCollapsibleHeaderCtrl;
class wxPopupTransientWindow;
class wxSimplebook;

///
/// Database -> Performance Benchmark & Diagnosis. The benchmark runs on a
/// worker thread in its own temporary database; all GUI updates are
/// marshalled with CallAfter(). Without a database registration (db is
/// nullptr) this computer and other servers can be tested, saved reports
/// opened and compared.
///
class BenchmarkDialog : public BaseDialog
{
public:
    // the results belong to the main window, so that they stay when the
    // dialog is closed
    BenchmarkDialog(wxWindow* parent, Database* db, fr::BenchmarkResultStore& results);
    ~BenchmarkDialog();

protected:
    virtual const wxString getName() const override;
    virtual const wxString getStorageName() const override;

private:
    Database* databaseM;
    std::unique_ptr<fr::BenchmarkRunner> runnerM;
    std::thread workerM;
    bool runningM = false;
    bool closeRequestedM = false;
    std::chrono::steady_clock::time_point startTimeM;
    std::set<wxString> loggedResultsM;
    wxString lastMessageM;
    // the tests of this session, opened files and comparisons
    fr::BenchmarkResultStore& resultsM;
    std::vector<fr::BenchmarkFirebirdInstallation> installationsM;

    // the targets of fr::getBenchmarkTargets(), and the details of every
    // target on a page of its own
    wxChoice* choice_target;
    wxStaticText* label_target_hint;
    wxSimplebook* book_target;
    fr::BenchmarkTarget getTarget() const;
    wxChoice* choice_install;
    // the advanced settings below their heading, closed at first
    wxCollapsibleHeaderCtrl* header_advanced;
    wxPanel* panel_advanced;
    wxTextCtrl* text_host = nullptr;
    wxTextCtrl* text_port = nullptr;
    wxTextCtrl* text_directory;
    // the login for another server, and the optional one for the server of
    // the registration
    wxTextCtrl* text_user = nullptr;
    wxTextCtrl* text_password = nullptr;
    wxTextCtrl* text_user_registered = nullptr;
    wxTextCtrl* text_password_registered = nullptr;
    wxTextCtrl* text_clientlib;
    wxChoice* choice_pagesize;
    wxCheckBox* check_comparepages;
    int automaticPageSizeM = 8192;
    wxCheckBox* check_inspect;
    wxCheckBox* check_manyusers;
    // what the next test is compared with: nothing, a result or a file
    wxChoice* choice_earlier;
    struct EarlierSource
    {
        int id = 0;         // a result
        wxString path;      // else a file
    };
    std::vector<EarlierSource> earlierSourcesM;
    // the last saved report chosen, remembered in the configuration
    wxString earlierFileM;
    void fillEarlier();
    bool chooseEarlierFile();
    // false if a file cannot be read: the test does not start then
    bool readEarlier(std::optional<fr::BenchmarkSnapshot>& baseline);
    // the earlier report of the running test, read at its start
    std::optional<fr::BenchmarkSnapshot> baselineM;
    wxRadioButton* radio_intensity[3];
    fr::BenchmarkIntensity getIntensity() const;

    // the strip of the progress in stages, the whole log of the run and
    // the results, with what the selected tab shows
    BenchmarkResultsView* view_results;
    BenchmarkProgressPanel* panel_progress;
    wxTextCtrl* text_log;
    wxButton* button_start;
    wxButton* button_cancel;
    // in the header of the shown result
    wxButton* button_copy;
    wxButton* button_save;
    wxButton* button_browser;
    wxButton* button_close_result;
    wxButton* button_compare;
    wxButton* button_open;
    wxCheckBox* check_private;
    wxButton* button_close;

    void createControls();
    // a small "?" that shows a longer explanation next to it on a click,
    // also with the keyboard: paragraphs, each with an optional term
    typedef std::vector<std::pair<wxString, wxString>> HelpItems;
    wxButton* createHelpButton(wxWindow* parent, const wxString& title,
        const HelpItems& items);
    void showHelp(wxWindow* button, const wxString& title, const HelpItems& items);
    // the last explanation, open or closed; one at a time
    wxPopupTransientWindow* popup_help = nullptr;
    void updateControls();
    // opens or closes the advanced settings; the dialog grows for the open
    // ones if the screen has room
    void showAdvanced(bool show);
    void layoutAdvanced();
    // how long the test takes, shown before the start
    wxString getReadyHint() const;
    // the stages of a run with the selected settings, shown before the start
    std::vector<fr::BenchmarkStagePlan> getStagePlan() const;
    void showReady();
    void setReportStatus(const wxString& message);
    bool buildParams(fr::BenchmarkConnectionParams& params);
    void fillCredentials(fr::BenchmarkConnectionParams& params, bool fromRegistration);
    // the login for the server of the registration: the saved one, or asked
    // for (only for this run, nothing is saved); false if the user cancelled
    bool resolveLogin(fr::BenchmarkConnectionParams& params);
    // the run uses the saved encrypted password, so its result tells whether
    // the master password is right
    bool encryptedLoginM = false;

    // temporary databases are remembered in the configuration until they
    // are dropped, so that a crash cannot leave them behind unnoticed
    std::vector<wxString> pendingEntriesM;
    static void addPendingDatabase(const wxString& entry);
    static void removePendingDatabase(const wxString& entry);
    void dropPendingDatabases();
    void appendLog(const wxString& line);
    // the settings are hidden after a run, so that the report has room
    wxSizer* sizerSettingsM = nullptr;
    bool settingsShownM = true;
    void showSettings(bool show);
    void addResultTab(int id);
    // after the result was saved
    void updateResultTab(int id);
    // the result of the selected tab; nullptr for the progress and the log
    const fr::BenchmarkResultEntry* getShownResult() const;
    void onResultSelected(int id);
    // asks before a test that is not saved is lost
    void closeResult(int id);
    void closeResults(fr::BenchmarkCloseScope scope, int id);
    enum
    {
        ID_menu_close = 4401,
        ID_menu_close_others,
        ID_menu_close_all,
        ID_menu_save,
        ID_menu_compare
    };
    // the result of the tab the menu belongs to
    int menuResultM = 0;
    void showResultMenu(int id);
    void showFullMessage();
    // false if it was not saved
    bool saveResult(int id);
    // the report of a test with its earlier report, without private
    // information if that is selected: what is shown, copied and saved
    fr::BenchmarkReport displayedReport(const fr::BenchmarkResultEntry& entry) const;
    fr::BenchmarkComparison displayedComparison(const fr::BenchmarkResultEntry& entry) const;
    // a result as a saved file contains it
    wxString displayedText(const fr::BenchmarkResultEntry& entry, bool markdown) const;
    // the page in the view: HTML, or Markdown without a web view
    wxString makePage(int id) const;
    // what changes every page: the private information and the theme
    wxString getPageVariant() const;
    // "Compare results" with these results chosen (0: none)
    void compare(int first, int second);
    // the registration of the selected database, empty without one
    wxString registrationId() const;
    // whether a temporary database is on the server of the registration
    bool isOnRegistrationServer(const wxString& connectionString) const;

    // called on the GUI thread; aborted means that run() itself failed, so
    // nothing is known about the temporary databases
    void onProgress(const fr::BenchmarkProgress& progress);
    void onFinished(const fr::BenchmarkReport& report, bool aborted);
    void requestClose();

    void OnStart(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnCopyMarkdown(wxCommandEvent& event);
    void OnSaveReport(wxCommandEvent& event);
    void OnOpenInBrowser(wxCommandEvent& event);
    void OnCloseResult(wxCommandEvent& event);
    void OnResultMenu(wxCommandEvent& event);
    void OnPrivateChanged(wxCommandEvent& event);
    void OnBrowseEarlier(wxCommandEvent& event);
    void OnEarlierChanged(wxCommandEvent& event);
    void OnCompare(wxCommandEvent& event);
    void OnOpen(wxCommandEvent& event);
    void OnCloseButton(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    // asks about tests that are not saved and drops them; false: stay open
    bool confirmDiscardUnsaved();
    void OnTargetChanged(wxCommandEvent& event);
    void OnDurationChanged(wxCommandEvent& event);
    void OnAdvancedToggled(wxCommandEvent& event);
    void OnCharHook(wxKeyEvent& event);

    DECLARE_EVENT_TABLE()
};

#endif // FR_BENCHMARKDIALOG_H
