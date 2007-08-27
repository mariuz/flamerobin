/*
  Copyright (c) 2004-2007 The FlameRobin Development Team

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


  $Id$

*/

#ifndef EXECUTESQLFRAME_H
#define EXECUTESQLFRAME_H
//-----------------------------------------------------------------------------
#include <wx/wx.h>
#include <wx/grid.h>
#include <wx/image.h>
#include <wx/notebook.h>
#include <wx/splitter.h>
#include <wx/stc/stc.h>

#include <ibpp.h>

#include "core/Observer.h"
#include "core/StringUtils.h"
#include "gui/BaseFrame.h"
#include "gui/FindDialog.h"
#include "logger.h"
#include "sql/SqlStatement.h"
#include "metadata/database.h"
#include "statementHistory.h"
//-----------------------------------------------------------------------------
class DataGrid;
class ExecuteSqlFrame;
//-----------------------------------------------------------------------------
class SqlEditor: public SearchableEditor
{
private:
    ExecuteSqlFrame *frameM;
    void setup();
public:
    enum { ID_MENU_UNDO = 300, ID_MENU_REDO, ID_MENU_CUT, ID_MENU_COPY, ID_MENU_PASTE, ID_MENU_DELETE,
        ID_MENU_SELECT_ALL, ID_MENU_EXECUTE_SELECTED, ID_MENU_FIND_SELECTED, ID_MENU_WRAP, ID_MENU_SET_FONT,
        ID_MENU_FIND
    };

    SqlEditor(wxWindow *parent, wxWindowID id, ExecuteSqlFrame *frame);
    void markText(int start, int end);
    void OnContextMenu(wxContextMenuEvent& event);
    void OnKillFocus(wxFocusEvent& event);
    void OnMenuUndo(wxCommandEvent& event);
    void OnMenuRedo(wxCommandEvent& event);
    void OnMenuCut(wxCommandEvent& event);
    void OnMenuCopy(wxCommandEvent& event);
    void OnMenuPaste(wxCommandEvent& event);
    void OnMenuDelete(wxCommandEvent& event);
    void OnMenuSelectAll(wxCommandEvent& event);
    void OnMenuFindSelected(wxCommandEvent& event);
    void OnMenuExecuteSelected(wxCommandEvent& event);
    void OnMenuFind(wxCommandEvent& event);
    void OnMenuSetFont(wxCommandEvent& event);
    void OnMenuWrap(wxCommandEvent& event);
    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
class ExecuteSqlFrame: public BaseFrame, public Observer
{
public:
    Database *getDatabase();
    void showProperties(wxString objectName);
    enum {
        ID_button_new = 101,
        ID_button_load,
        ID_button_save,
        ID_button_saveas,
        ID_button_prev,
        ID_button_next,
        ID_button_history,
        ID_button_execute,
        ID_button_commit,
        ID_button_rollback,
        ID_button_toggle,
        ID_button_delete,
        ID_button_insert,
        ID_button_plan,
        ID_grid_data,
        ID_stc_sql
    };

    // query parsing and execution
    void executeAllStatements(bool autoExecute = false);
    void prepareAndExecute(bool prepareOnly = false);
    bool parseStatements(const wxString& statements, bool autoExecute = false,
        bool prepareOnly = false, int selectionOffset = 0);
    bool execute(wxString sql, const wxString& terminator,
        bool prepareOnly = false);
    bool loadSqlFile(const wxString& filename);
    void setSql(wxString sql);
    void clearStats();

    ExecuteSqlFrame(wxWindow* parent, int id, wxString title, Database *db,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxDEFAULT_FRAME_STYLE);

private:
    std::vector<SqlStatement> executedStatementsM;
    wxString filenameM;

    typedef enum { ttNormal, ttSql, ttError } TextType;
    void log(wxString s, TextType type = ttNormal);     // write messages to textbox
    void SplitScreen();
    Database* databaseM;
    DatabaseToSystemCharsetConversion dbCharsetConversionM;
    void setDatabase(Database* db);

    StatementHistory::Position historyPositionM;
    wxString localBuffer;
    void updateHistoryButtons();

    bool autoCommitM;
    bool inTransactionM;
    IBPP::Transaction transactionM;
    IBPP::Statement statementM;
    void InTransaction(bool started);       // changes controls (enable/disable)
    bool commitTransaction();
    void rollbackTransaction();

    void autoComplete(bool force);
    void autoCompleteColumns(int pos, int len = 0);
    void OnSqlEditUpdateUI(wxStyledTextEvent &event);
    void OnSqlEditCharAdded(wxStyledTextEvent &event);      // autocomplete stuff
    void OnSqlEditStartDrag(wxStyledTextEvent& event);      // enable click&remove selection
    wxString keywordsM;     // text used for autocomplete
    void setKeywords();

    // events
    void OnClose(wxCloseEvent& event);
    void OnKeyDown(wxKeyEvent &event);
    void OnButtonNewClick(wxCommandEvent &event);
    void OnButtonLoadClick(wxCommandEvent &event);
    void OnButtonSaveClick(wxCommandEvent &event);
    void OnButtonSaveAsClick(wxCommandEvent &event);
    void OnButtonPrevClick(wxCommandEvent &event);
    void OnButtonNextClick(wxCommandEvent &event);
    void OnButtonHistoryClick(wxCommandEvent &event);
    void OnButtonExecuteClick(wxCommandEvent &event);
    void OnButtonCommitClick(wxCommandEvent &event);
    void OnButtonRollbackClick(wxCommandEvent &event);
    void OnButtonToggleClick(wxCommandEvent &event);
    void OnButtonDeleteClick(wxCommandEvent &event);
    void OnButtonInsertClick(wxCommandEvent &event);
    void OnButtonWrapClick(wxCommandEvent &event);
    void OnButtonPlanClick(wxCommandEvent &event);
    void OnGridRowCountChanged(wxCommandEvent &event);
    void OnGridStatementExecuted(wxCommandEvent &event);

    // begin wxGlade: ExecuteSqlFrame::methods
    void set_properties();
    void do_layout();
    // end wxGlade

protected:
    void removeSubject(Subject* subject);
    void update();
    bool closeWhenTransactionDoneM;
    bool loadingM;

    wxPanel* panel_contents;

    wxBitmapButton* button_new;
    wxBitmapButton* button_load;
    wxBitmapButton* button_save;
    wxBitmapButton* button_saveas;
    wxBitmapButton* button_prev;
    wxBitmapButton* button_next;
    wxBitmapButton* button_history;
    wxButton* button_execute;
    wxButton* button_commit;
    wxButton* button_rollback;
    wxButton* button_plan;
    wxButton* button_toggle;
    wxButton* button_delete;
    wxButton* button_insert;
    wxSplitterWindow* splitter_window_1;
    wxPanel* panel_splitter_top;
    wxPanel* panel_splitter_bottom;
    SqlEditor* styled_text_ctrl_sql;
    wxNotebook* notebook_1;
    wxPanel* notebook_pane_1;
    wxPanel* notebook_pane_2;
    DataGrid* grid_data;
    wxStyledTextCtrl* styled_text_ctrl_stats;

    wxStatusBar* statusbar_1;

    virtual const wxString getName() const;
    virtual void doReadConfigSettings(const wxString& prefix);
    virtual void doWriteConfigSettings(const wxString& prefix) const;
    virtual const wxRect getDefaultRect() const;

    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif // EXECUTESQLFRAME_H
