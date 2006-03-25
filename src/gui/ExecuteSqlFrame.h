/*
  The contents of this file are subject to the Initial Developer's Public
  License Version 1.0 (the "License"); you may not use this file except in
  compliance with the License. You may obtain a copy of the License here:
  http://www.flamerobin.org/license.html.

  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.

  The Original Code is FlameRobin (TM).

  The Initial Developer of the Original Code is Milan Babuskov.

  Portions created by the original developer
  are Copyright (C) 2004 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s): Nando Dessena
*/

#ifndef EXECUTESQLFRAME_H
#define EXECUTESQLFRAME_H
//-----------------------------------------------------------------------------
#include <wx/wx.h>
#include <wx/dnd.h>
#include <wx/grid.h>
#include <wx/image.h>
#include <wx/notebook.h>
#include <wx/splitter.h>
#include <wx/stc/stc.h>

#include <ibpp.h>

#include "core/Observer.h"
#include "gui/BaseFrame.h"
#include "gui/FindDialog.h"
#include "logger.h"
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
class ExecuteSqlFrame: public BaseFrame, public Observer {
public:
    void setDatabase(Database *db);
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
        ID_button_plan,
        ID_grid_data,
        ID_stc_sql
    };

    // query parsing and execution
    void executeAllStatements(bool autoExecute = false);
    void prepareAndExecute(bool prepareOnly = false);
    bool parseStatements(const wxString& statements, bool autoExecute = false,
        bool prepareOnly = false, int selectionOffset = 0);
    bool execute(wxString sql, bool prepareOnly = false);
    void setSql(wxString sql);

    ExecuteSqlFrame(wxWindow* parent, int id, wxString title, const wxPoint& pos=wxDefaultPosition,
        const wxSize& size=wxDefaultSize, long style=wxDEFAULT_FRAME_STYLE);

private:
    std::vector<ExecutedStatement> executedStatementsM;
    wxString filenameM;

    typedef enum { ttNormal, ttSql, ttError } TextType;
    void log(wxString s, TextType type = ttNormal);     // write messages to textbox
    void SplitScreen();
    Database *databaseM;

    StatementHistory::Position historyPositionM;
    wxString localBuffer;
    void updateHistoryButtons();

    wxString terminatorM;
    bool autoCommitM;
    bool inTransactionM;
    IBPP::Transaction transactionM;
    IBPP::Statement statementM;
    void InTransaction(bool started);       // changes controls (enable/disable)
    void commitTransaction();
    void rollbackTransaction();

    void autoComplete(bool force);
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
    void OnButtonWrapClick(wxCommandEvent &event);
    void OnButtonPlanClick(wxCommandEvent &event);
    void OnGridRowCountChanged(wxCommandEvent &event);

    // begin wxGlade: ExecuteSqlFrame::methods
    void set_properties();
    void do_layout();
    // end wxGlade

protected:
    void removeSubject(Subject* subject);
    void update();
    bool closeWhenTransactionDoneM;

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
class DnDText : public wxTextDropTarget
{
public:
    DnDText(wxStyledTextCtrl *owner, Database *db) { ownerM = owner; databaseM = db; }
    virtual bool OnDropText(wxCoord x, wxCoord y, const wxString& text);

private:
    wxStyledTextCtrl *ownerM;
    Database *databaseM;
};
//-----------------------------------------------------------------------------
#endif // EXECUTESQLFRAME_H
