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

#ifndef FR_BENCHMARKRESULTSVIEW_H
#define FR_BENCHMARKRESULTSVIEW_H

#include <wx/wx.h>

#include <functional>
#include <map>
#include <memory>
#include <vector>

class BenchmarkProgressPanel;
class BenchmarkReportPage;
class wxAuiNotebook;
class wxAuiNotebookEvent;
class wxSimplebook;
class wxWebView;
class wxWebViewEvent;

///
/// The results of the benchmark dialog: a strip of tabs with the progress
/// and the log of the test (without a close button) and one tab for every
/// result, below it what the selected tab shows. All results share one
/// report view: a web view costs a process and tens of MB with Edge. When
/// another result is shown, the scroll position and the opened parts of
/// the page are kept for the one before. The view only shows; what a
/// result is, the dialog knows.
///
class BenchmarkResultsView : public wxPanel
{
public:
    // the tabs that are not results; results have numbers from 1
    static const int progressTab = 0;
    static const int logTab = -1;
    // the actions on the shown result, in the header above it
    enum
    {
        ID_button_copy = 4301,
        ID_button_save,
        ID_button_browser,
        ID_button_close_result
    };

    explicit BenchmarkResultsView(wxWindow* parent);
    ~BenchmarkResultsView();

    BenchmarkProgressPanel* getProgress() const { return progressM; }
    wxTextCtrl* getLog() const { return logM; }
    wxButton* getCopyButton() const { return copyM; }
    wxButton* getSaveButton() const { return saveM; }
    wxButton* getBrowserButton() const { return browserM; }
    wxButton* getCloseButton() const { return closeM; }
    /// which view shows the reports, for the log
    wxString getViewName() const { return viewNameM; }
    /// false if the reports are shown as Markdown text
    bool hasWebView() const;

    /// a new tab at the end
    void addResult(int id, const wxString& label, const wxString& toolTip);
    void setResultLabel(int id, const wxString& label, const wxString& toolTip);
    void removeResult(int id);
    /// the result, progressTab or logTab
    int getSelection() const;
    /// shows the tab and what belongs to it
    void select(int id);
    /// the next or the previous tab, from the last to the first
    void selectNext(bool forward);
    /// while a test runs only the progress and the log can be chosen
    void setLocked(bool locked);
    /// what changes the page of every result (the private information, the
    /// theme): the pages made before are not used any more
    void setPageVariant(const wxString& variant);
    /// what the shown result is, with a longer tool tip if wanted
    void setHeader(const wxString& text, const wxString& toolTip = wxEmptyString);
    /// a message about the shown result, e.g. that it was copied
    void setStatus(const wxString& message);
    /// a message that is never cut
    void setStatusWidth(const wxString& message);

    /// the page of a result: HTML, or Markdown without a web view
    std::function<wxString(int id)> makePage;
    /// a tab was selected, by the user or by select()
    std::function<void(int id)> onSelected;
    /// the close button or the middle mouse button on the tab of a result;
    /// the dialog closes it with removeResult()
    std::function<void(int id)> onCloseResult;
    /// the right mouse button on the tab of a result
    std::function<void(int id)> onResultMenu;
    /// Ctrl and a letter in the report view, which keeps it from the dialog
    std::function<void(char key)> onShortcut;

private:
    wxAuiNotebook* stripM;
    wxSimplebook* bookM;
    BenchmarkProgressPanel* progressM;
    wxTextCtrl* logM;
    wxStaticText* headerM;
    wxStaticText* statusM;
    int statusWidthM = 0;
    wxSizer* sizerHeaderM;
    wxButton* copyM;
    wxButton* saveM;
    wxButton* browserM;
    wxButton* closeM;
    wxWebView* webViewM = nullptr;
    std::unique_ptr<BenchmarkReportPage> reportPageM;
    wxTextCtrl* textM = nullptr;
    wxString viewNameM;
    bool lockedM = false;
    // the tool tips of the results, without the note of a locked tab
    std::map<int, wxString> tipsM;
    // the strip is changed by the program, not by the user
    int changingM = 0;

    // the result whose page is in the view, its state per result ("scroll
    // position|checked inputs|open details"), and the state to restore
    // when the page has loaded
    int shownM = 0;
    std::map<int, wxString> statesM;
    wxString restoreM;
    bool loadedM = false;
    // the scripts that are running: they wait for their result and let
    // events in meanwhile
    int scriptsM = 0;
    void saveState();
#if wxUSE_WEBVIEW
    bool runScript(const wxString& script, wxString* result);
    void restoreState();
#endif
    void OnPageLoaded(wxWebViewEvent& event);
    void OnScriptMessage(wxWebViewEvent& event);

    // the last pages that were made: making one without private
    // information takes up to a second
    struct Page
    {
        int id;
        wxString text;
    };
    std::vector<Page> pagesM;
    wxString variantM;
    const wxString& getPage(int id);
    void showContent(int id);

    int findTab(int id) const;
    int getTabId(size_t index) const;
    void fitStrip();
    void OnPageChanging(wxAuiNotebookEvent& event);
    void OnPageChanged(wxAuiNotebookEvent& event);
    void OnPageClose(wxAuiNotebookEvent& event);
    void OnTabRightUp(wxAuiNotebookEvent& event);
    void OnDPIChanged(wxDPIChangedEvent& event);
};

#endif // FR_BENCHMARKRESULTSVIEW_H
