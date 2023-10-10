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

#ifndef EXECUTESQLFRAME_H
#define EXECUTESQLFRAME_H

#include <wx/wx.h>
#include <wx/filename.h>
#include <wx/grid.h>
#include <wx/image.h>
#include <wx/notebook.h>
#include <wx/splitter.h>
#include <wx/stc/stc.h>

#include <ibpp.h>

#include "core/Observer.h"
#include "core/StringUtils.h"
#include "controls/DataGridTable.h"
#include "gui/BaseFrame.h"
#include "gui/EditBlobDialog.h"
#include "gui/FindDialog.h"
#include "sql/SqlStatement.h"
#include "statementHistory.h"
#include "map"

class CommandManager;
class Database;
class DataGrid;
class ExecuteSqlFrame;

class SqlEditor: public SearchableEditor
{
private:
    void setup();
public:
    SqlEditor(wxWindow *parent, wxWindowID id);
    void markText(int start, int end);
    void setChars(bool firebirdIdentifierOnly);
    void setFont();

    bool hasSelection();

    void OnContextMenu(wxContextMenuEvent& event);
    void OnKillFocus(wxFocusEvent& event);
    DECLARE_EVENT_TABLE()
};

class ExecuteSqlFrame: public BaseFrame, public Observer
{
public:
    ExecuteSqlFrame(wxWindow* parent, int id, wxString title, DatabasePtr db,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxDEFAULT_FRAME_STYLE);

    bool loadSqlFile(const wxString& filename);
    bool setSql(wxString sql);

    void executeAllStatements(bool autoExecute = false);

    virtual bool Show(bool show = TRUE);

    Database* getDatabase() const;
private:

    virtual bool doCanClose();
    virtual void doBeforeDestroy();

    // query parsing and execution
    void prepareAndExecute(bool prepareOnly = false);
    bool parseStatements(const wxString& statements, bool autoExecute = false,
        bool prepareOnly = false, int selectionOffset = 0);
    bool execute(wxString sql, const wxString& terminator,
        bool prepareOnly = false);

    std::vector<SqlStatement> executedStatementsM;
    std::map<std::string, wxString> parameterSaveList;
    std::map<std::string, wxString> parameterSaveListOptionNull;
    wxFileName filenameM;
    wxDateTime filenameModificationTimeM;

    void compareCounts(IBPP::DatabaseCounts& one, IBPP::DatabaseCounts& two);

    void showProperties(wxString objectName);

    typedef enum { ttNormal, ttSql, ttError } TextType;
    void log(wxString s, TextType type = ttNormal);     // write messages to textbox
    void clearLogBeforeExecution();

    void splitScreen();
    Database* databaseM;

    StatementHistory::Position historyPositionM;
    wxString localBuffer;

    bool autoCommitM;
    bool inTransactionM;
    IBPP::Transaction transactionM;
    IBPP::Statement statementM;
    IBPP::TIL transactionIsolationLevelM;
    IBPP::TLR transactionLockResolutionM;
    IBPP::TAM transactionAccessModeM;
    bool showStatisticsM;
    void inTransaction(bool started);       // changes controls (enable/disable)
    bool commitTransaction();
    bool rollbackTransaction();

    void autoComplete(bool force);
    void autoCompleteColumns(int pos, int len = 0);
    void OnSqlEditUpdateUI(wxStyledTextEvent& event);
    void OnSqlEditCharAdded(wxStyledTextEvent& event);      // autocomplete stuff
    void OnSqlEditChanged(wxStyledTextEvent& event);        // update title
    void OnSqlEditStartDrag(wxStyledTextEvent& event);      // enable click&remove selection
    wxString keywordsM;     // text used for autocomplete
    void setKeywords();
    void buildMainMenu(CommandManager& cm);
    void buildToolbar(CommandManager& cm);

    bool doUpdateFocusedControlM;
    enum ViewMode { vmNotebook, vmEditor, vmLogCtrl, vmGrid, vmGridEditor };
    ViewMode viewModeM;
    void setViewMode(ViewMode mode);
    void setViewMode(bool splitView, ViewMode mode);
    void updateViewMode();

    bool updateEditorCaretPosM;
    bool updateFrameTitleM;
    void updateFrameTitle();

    // blob-editor-timer
    enum {
        TIMER_ID_UPDATE_BLOB = 1
    };
    wxTimer timerBlobEditorM;
    // blob-editor dialog
    EditBlobDialog* editBlobDlgM;
    // blob-editor event
    void OnBlobEditorUpdate(wxTimerEvent& event);
    // blob-editor function to update blob-editor value
    void closeBlobEditor(bool saveBlobValue);
    void updateBlobEditor();

    // events
    void OnActivate(wxActivateEvent& event);
    void OnChildFocus(wxChildFocusEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnGridCellChange(wxGridEvent& event);
    void OnGridInvalidateAttributeCache(wxCommandEvent& event);
    void OnGridRowCountChanged(wxCommandEvent& event);
    void OnGridStatementExecuted(wxCommandEvent& event);
    void OnGridSum(wxCommandEvent& event);
    void OnGridLabelLeftDClick(wxGridEvent& event);
    void OnSplitterUnsplit(wxSplitterEvent& event);
    void OnIdle(wxIdleEvent& event);

    // menu events
    void OnMenuNew(wxCommandEvent& event);
    void OnMenuOpen(wxCommandEvent& event);
    void OnMenuSaveOrSaveAs(wxCommandEvent& event);
    void OnMenuClose(wxCommandEvent& event);

    void OnMenuUndo(wxCommandEvent& event);
    void OnMenuUpdateUndo(wxUpdateUIEvent& event);
    void OnMenuRedo(wxCommandEvent& event);
    void OnMenuUpdateRedo(wxUpdateUIEvent& event);
    void OnMenuCut(wxCommandEvent& event);
    void OnMenuUpdateCut(wxUpdateUIEvent& event);
    void OnMenuCopy(wxCommandEvent& event);
    void OnMenuCopyWithHeader(wxCommandEvent& event);
    void OnMenuUpdateCopy(wxUpdateUIEvent& event);
    void OnMenuPaste(wxCommandEvent& event);
    void OnMenuUpdatePaste(wxUpdateUIEvent& event);
    void OnMenuDelete(wxCommandEvent& event);
    void OnMenuUpdateDelete(wxUpdateUIEvent& event);
    void OnMenuSelectAll(wxCommandEvent& event);
    void OnMenuReplace(wxCommandEvent& event);

    void OnMenuSelectView(wxCommandEvent& event);
    void OnMenuUpdateSelectView(wxUpdateUIEvent& event);
    void OnMenuSplitView(wxCommandEvent& event);
    void OnMenuUpdateSplitView(wxUpdateUIEvent& event);
    void OnMenuSetEditorFont(wxCommandEvent& event);
    void OnMenuToggleWrap(wxCommandEvent& event);

    void OnMenuHistoryNext(wxCommandEvent& event);
    void OnMenuUpdateHistoryNext(wxUpdateUIEvent& event);
    void OnMenuHistoryPrev(wxCommandEvent& event);
    void OnMenuUpdateHistoryPrev(wxUpdateUIEvent& event);
    void OnMenuHistorySearch(wxCommandEvent& event);

    void OnMenuExecute(wxCommandEvent& event);
    void OnMenuShowPlan(wxCommandEvent& event);
    void OnMenuShowStatistics(wxCommandEvent& event);
    void OnMenuUpdateShowStatistics(wxUpdateUIEvent& event);
    void OnMenuExecuteSelection(wxCommandEvent& event);
    void OnMenuExecuteFromCursor(wxCommandEvent& event);
    void OnMenuCommit(wxCommandEvent& event);
    void OnMenuRollback(wxCommandEvent& event);
    void OnMenuUpdateWhenInTransaction(wxUpdateUIEvent& event);
    void OnMenuUpdateWhenExecutePossible(wxUpdateUIEvent& event);
    void OnMenuTransactionIsolationLevel(wxCommandEvent& event);
    void OnMenuUpdateTransactionIsolationLevel(wxUpdateUIEvent& event);
    void OnMenuTransactionLockResolution(wxCommandEvent& event);
    void OnMenuUpdateTransactionLockResolution(wxUpdateUIEvent& event);
    void OnMenuTransactionReadOnly(wxCommandEvent& event);
    void OnMenuUpdateTransactionReadOnly(wxUpdateUIEvent& event);

    void OnMenuGridInsertRow(wxCommandEvent& event);
    void OnMenuUpdateGridInsertRow(wxUpdateUIEvent& event);
    void OnMenuGridDeleteRow(wxCommandEvent& event);
    void OnMenuUpdateGridDeleteRow(wxUpdateUIEvent& event);
    void OnMenuGridSetFieldToNULL(wxCommandEvent& WXUNUSED(event));
    void OnMenuGridEditBlob(wxCommandEvent& event);
    void OnMenuGridImportBlob(wxCommandEvent& event);
    void OnMenuGridExportBlob(wxCommandEvent& event);
    void OnMenuUpdateGridCellIsBlob(wxUpdateUIEvent& event);
    void OnMenuGridCopyAsInList(wxCommandEvent& event);
    void OnMenuGridCopyAsInsert(wxCommandEvent& event);
    void OnMenuGridCopyAsUpdate(wxCommandEvent& event);
    void OnMenuGridCopyAsUpdateInsert(wxCommandEvent& event);
    void OnMenuGridSaveAsHtml(wxCommandEvent& event);
    void OnMenuGridSaveAsCsv(wxCommandEvent& event);
    void OnMenuGridGridHeaderFont(wxCommandEvent& event);
    void OnMenuGridGridCellFont(wxCommandEvent& event);
    void OnMenuGridFetchAll(wxCommandEvent& event);
    void OnMenuGridCancelFetchAll(wxCommandEvent& event);
    void OnMenuUpdateGridHasSelection(wxUpdateUIEvent& event);
    void OnMenuUpdateGridHasData(wxUpdateUIEvent& event);
    void OnMenuUpdateGridFetchAll(wxUpdateUIEvent& event);
    void OnMenuUpdateGridCancelFetchAll(wxUpdateUIEvent& event);
    void OnMenuUpdateGridCanSetFieldToNULL(wxUpdateUIEvent& event);

    void OnMenuFindSelectedObject(wxCommandEvent& event);

    void set_properties();
    void do_layout();

private:
    // observer stuff
    virtual void subjectRemoved(Subject* subject);
    virtual void update();

protected:
    enum {
        ID_grid_data = 101,
        ID_stc_sql
    };

    bool closeWhenTransactionDoneM;
    bool loadingM;

    wxPanel* panel_contents;

    wxSplitterWindow* splitter_window_1;
    SqlEditor* styled_text_ctrl_sql;
    wxNotebook* notebook_1;
    wxPanel* notebook_pane_1;
    wxPanel* notebook_pane_2;
    DataGrid* grid_data;
    wxStyledTextCtrl* styled_text_ctrl_stats;

    wxStatusBar* statusbar_1;

    wxMenuBar* menuBarM;
    wxToolBar* toolBarM;

    virtual const wxString getName() const;
    virtual void doReadConfigSettings(const wxString& prefix);
    virtual void doWriteConfigSettings(const wxString& prefix) const;
    virtual const wxRect getDefaultRect() const;

    DECLARE_EVENT_TABLE()
};

#endif // EXECUTESQLFRAME_H
