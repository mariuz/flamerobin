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

#include <wx/artprov.h>
#include <wx/dnd.h>
#include <wx/file.h>
#include <wx/fontdlg.h>
#include <wx/stopwatch.h>
#include <wx/tokenzr.h>

#include <algorithm>
#include <map>
#include <vector>

#include "config/Config.h"
#include "core/ArtProvider.h"
#include "core/CodeTemplateProcessor.h"
#include "core/FRError.h"
#include "core/StringUtils.h"
#include "core/URIProcessor.h"
#include "engine/MetadataLoader.h"
#include "gui/AdvancedMessageDialog.h"
#include "gui/CommandIds.h"
#include "gui/CommandManager.h"
#include "gui/controls/ControlUtils.h"
#include "gui/controls/DataGrid.h"
#include "gui/controls/DataGridTable.h"
#include "gui/GUIURIHandlerHelper.h"
#include "gui/MetadataItemPropertiesFrame.h"
#include "gui/ProgressDialog.h"
#include "gui/EditBlobDialog.h"
#include "gui/ExecuteSql.h"
#include "gui/ExecuteSqlFrame.h"
#include "gui/FRLayoutConfig.h"
#include "gui/InsertDialog.h"
#include "gui/InsertParametersDialog.h"
#include "gui/StatementHistoryDialog.h"
#include "gui/StyleGuide.h"
#include "frutils.h"
#include "logger.h"
#include "metadata/column.h"
#include "metadata/CreateDDLVisitor.h"
#include "metadata/database.h"
#include "metadata/exception.h"
#include "metadata/function.h"
#include "metadata/generator.h"
#include "metadata/MetadataItemURIHandlerHelper.h"
#include "metadata/package.h"
#include "metadata/procedure.h"
#include "metadata/server.h"
#include "metadata/table.h"
#include "metadata/view.h"
#include "sql/Identifier.h"
#include "sql/IncompleteStatement.h"
#include "sql/MultiStatement.h"
#include "sql/SelectStatement.h"
#include "sql/SqlStatement.h"
#include "sql/StatementBuilder.h"
#include "statementHistory.h"

class SqlEditorDropTarget : public wxDropTarget
{
public:
    SqlEditorDropTarget(ExecuteSqlFrame* frame, SqlEditor* editor,
        Database* database);
    virtual wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def);
    virtual bool OnDropFiles(wxCoord x, wxCoord y,
        const wxArrayString& filenames);
    virtual bool OnDropText(wxCoord x, wxCoord y, const wxString& text);
private:
    ExecuteSqlFrame* frameM;
    SqlEditor* editorM;
    Database* databaseM;

    wxFileDataObject* fileDataM;
    wxTextDataObject* textDataM;
};

SqlEditorDropTarget::SqlEditorDropTarget(ExecuteSqlFrame* frame,
        SqlEditor* editor, Database* database)
    : frameM(frame), editorM(editor), databaseM(database)
{
    wxDataObjectComposite* dataobj = new wxDataObjectComposite;
    dataobj->Add(fileDataM = new wxFileDataObject);
    dataobj->Add(textDataM = new wxTextDataObject);
    SetDataObject(dataobj);
}

wxDragResult SqlEditorDropTarget::OnData(wxCoord x, wxCoord y,
    wxDragResult def)
{
    if (!GetData())
        return wxDragNone;

    wxDataObjectComposite* dataobj = (wxDataObjectComposite*) m_dataObject;
    // test for wxDF_FILENAME
    if (dataobj->GetReceivedFormat() == fileDataM->GetFormat())
    {
        if (OnDropFiles(x, y, fileDataM->GetFilenames()))
            return def;
    }
    else
    // try everything else as dropped text
    if (OnDropText(x, y, textDataM->GetText()))
        return def;
    return wxDragNone;
}

bool SqlEditorDropTarget::OnDropFiles(wxCoord, wxCoord,
    const wxArrayString& filenames)
{
    return (filenames.GetCount() == 1 && frameM->loadSqlFile(filenames[0]));
}

// TODO: This needs to be reworked to use the tokenizer
//       Perhaps we could have a SelectStatement class, that would be able to:
//       - load the select statement
//       - alter some of it (add new table, column, etc.)
//       - dump statement back to editor
bool SqlEditorDropTarget::OnDropText(wxCoord, wxCoord, const wxString& text)
{
    if (text.Mid(0, 7) != "OBJECT:")
        return false;

    MetadataItem* m;
    if (!wxSscanf(text.Mid(7), "%p", &m))
        return false;
  
    DatabasePtr db = m->getDatabase();
    if (db.get() != databaseM)
    {
        wxMessageBox(_("Cannot use objects from different databases."),
            _("Wrong database."), wxOK | wxICON_WARNING);
        return false;
    }

    wxString columns;
    Table* t = 0;
    Column* c = dynamic_cast<Column*>(m);
    if (c)
        t = c->getTable();
    else
        t = dynamic_cast<Table*>(m);
    if (t == 0)
    {
        wxMessageBox(_("Only tables and table columns can be dropped."),
            _("Object type not supported."), wxOK | wxICON_WARNING);
        return false;
    }

    SelectStatement sstm(editorM->GetText());
    if (!sstm.isValidSelectStatement())
    {
        StatementBuilder sb;
        sb << kwSELECT << ' ' << kwFROM << ' '; // blank statement
        sstm.setStatement(sb);
    }

    // add the column(s)
    if (c)
    {
        sstm.addColumn(t->getQuotedName() + "." + c->getQuotedName());
    }
    else
    {
        t->ensureChildrenLoaded();
        for (ColumnPtrs::const_iterator it = t->begin(); it != t->end(); ++it)
        {
            sstm.addColumn(
                t->getQuotedName() + "." + (*it)->getQuotedName());
        }
    }

    // read in the table names, and find position where FROM clause ends
    std::vector<wxString> tableNames;
    sstm.getTables(tableNames);

    // if table is not there, add it
    if (std::find(tableNames.begin(), tableNames.end(), t->getQuotedName())
        == tableNames.end())
    {
        std::vector<ForeignKey> relatedTables;
        if (Table::tablesRelate(tableNames, t, relatedTables)) // foreign keys
        {
            wxArrayString as;
            for (std::vector<ForeignKey>::iterator it = relatedTables.begin();
                it != relatedTables.end(); ++it)
            {
                wxString addme = (*it).getReferencedTable() + ":  "
                    + (*it).getJoin(false); // false = unquoted
                if (as.Index(addme) == wxNOT_FOUND)
                    as.Add(addme);
            }
            wxString join_list;
            if (as.GetCount() > 1)    // let the user decide
            {
                int selected = ::wxGetSingleChoiceIndex(
                    _("Multiple foreign keys found"),
                    _("Select the desired table"), as);
                if (selected == -1)
                    return false;
                join_list = relatedTables[selected].getJoin(true /*quoted*/);
            }
            else
                join_list = relatedTables[0].getJoin(true);

            // FIXME: dummy test value
            // can_be_null = (check if any of the FK fields can be null)
            bool can_be_null = true;
            wxString joinType = (can_be_null ? "LEFT JOIN":"JOIN");
            sstm.addTable(t->getQuotedName(), joinType, join_list);
        }
        else
        {
            sstm.addTable(t->getQuotedName(), "CARTESIAN", "");
        }
    }

    editorM->SetText(sstm.getStatement());
    return true;
}

// Setup the Scintilla editor
SqlEditor::SqlEditor(wxWindow *parent, wxWindowID id)
    : SearchableEditor(parent, id)
{
    wxString s;
    if (config().getValue("SqlEditorFont", s) && !s.empty())
    {
        wxFont f;
        f.SetNativeFontInfo(s);
        if (f.Ok())
            StyleSetFont(wxSTC_STYLE_DEFAULT, f);
    }
    else
    {
        wxFont font(frlayoutconfig().getEditorFontSize(), wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        StyleSetFont(wxSTC_STYLE_DEFAULT, font);
    }

    int charset;
    if (config().getValue("SqlEditorCharset", charset))
        StyleSetCharacterSet(wxSTC_STYLE_DEFAULT, charset);

    setup();
}

bool SqlEditor::hasSelection()
{
    return GetSelectionStart() != GetSelectionEnd();
}


void SqlEditor::markText(int start, int end)
{
    centerCaret(true);
    GotoPos(end);
    GotoPos(start);
    SetSelectionStart(start);
    SetSelectionEnd(end);
    centerCaret(false);
}

void SqlEditor::setChars(bool firebirdIdentifierOnly)
{
    SetKeyWords(0, SqlTokenizer::getKeywordsString(SqlTokenizer::kwLowerCase));
    wxString chars("_0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz\"$");

    if (!firebirdIdentifierOnly)
    {
        // see Document::SetDefaultCharClasses in stc/Document.cxx
        for (int ch = 0x80; ch < 0x0100; ch++)
            if (isalnum(ch))
                chars += wxChar(ch);
    }
    SetWordChars(chars);
}

//! This code has to be called each time the font has changed, so that the control updates
void SqlEditor::setup()
{
    StyleClearAll();

    StyleSetForeground(wxSTC_SQL_DEFAULT,        wxColour(0x80, 0x00, 0x00));
    StyleSetForeground(wxSTC_SQL_COMMENT,        wxColour(0x00, 0xa0, 0x00));        // multiline comment
    StyleSetForeground(wxSTC_SQL_COMMENTLINE,    wxColour(0x00, 0xa0, 0x00));        // one-line comment
    StyleSetForeground(wxSTC_SQL_COMMENTDOC,     wxColour(0x00, 0xff, 0x00));
    StyleSetForeground(wxSTC_SQL_NUMBER,         wxColour(0x00, 0x00, 0xff));        // number
    StyleSetForeground(wxSTC_SQL_WORD,           wxColour(0x00, 0x00, 0x7f));        // keyword
    StyleSetForeground(wxSTC_SQL_STRING,         wxColour(0x00, 0x00, 0xff));        // 'single quotes'
    StyleSetForeground(wxSTC_SQL_CHARACTER,      wxColour(0xff, 0x00, 0xff));
    StyleSetForeground(wxSTC_SQL_SQLPLUS,        wxColour(0x00, 0x7f, 0x7f));
    StyleSetForeground(wxSTC_SQL_SQLPLUS_PROMPT, wxColour(0xff, 0x00, 0x00));
    StyleSetForeground(wxSTC_SQL_OPERATOR,       wxColour(0x00, 0x00, 0x00));        // ops
    StyleSetForeground(wxSTC_SQL_IDENTIFIER,     wxColour(0x00, 0x00, 0x00));
    
    StyleSetBackground(wxSTC_STYLE_BRACELIGHT,   wxColour(0xff, 0xcc, 0x00));        // brace highlight
    StyleSetBackground(wxSTC_STYLE_BRACEBAD,     wxColour(0xff, 0x33, 0x33));        // brace bad highlight
    
    StyleSetBold(wxSTC_SQL_WORD,         TRUE);
    StyleSetBold(wxSTC_SQL_OPERATOR,     TRUE);
    StyleSetBold(wxSTC_STYLE_BRACELIGHT, TRUE);
    StyleSetBold(wxSTC_STYLE_BRACEBAD,   TRUE);
    
    StyleSetItalic(wxSTC_SQL_COMMENT,     TRUE);
    StyleSetItalic(wxSTC_SQL_COMMENTLINE, TRUE);

    SetLexer(wxSTC_LEX_SQL);
    setChars(false);

    int tabSize = config().get("sqlEditorTabSize", 4);
    SetTabWidth(tabSize);
    SetIndent(tabSize);
    SetUseTabs(false);
    SetTabIndents(true);
    SetBackSpaceUnIndents(true);
    AutoCompSetIgnoreCase(true);
    AutoCompSetAutoHide(true);        // info in ScintillaDoc.html file (in scintilla source package)
    SetMarginWidth(0, 40);            // turn on the linenumbers margin, set width to 40pixels
    SetMarginWidth(1, 0);             // turn off the folding margin
    SetMarginType(0, 1);              // set margin type to linenumbers
    if (config().get("sqlEditorShowEdge", false))
    {
        SetEdgeMode(wxSTC_EDGE_LINE);
        SetEdgeColumn(config().get("sqlEditorEdgeColumn", 50));
    }

    if (!config().get("sqlEditorSmartHomeKey", true))
        CmdKeyAssign(wxSTC_KEY_HOME, wxSTC_SCMOD_NORM, wxSTC_CMD_HOMEDISPLAY);

    centerCaret(false);
}

BEGIN_EVENT_TABLE(SqlEditor, wxStyledTextCtrl)
    EVT_CONTEXT_MENU(SqlEditor::OnContextMenu)
    EVT_KILL_FOCUS(SqlEditor::OnKillFocus)
END_EVENT_TABLE()

void SqlEditor::OnContextMenu(wxContextMenuEvent& event)
{
    if (AutoCompActive() || CallTipActive())
        return;
    SetFocus();

    wxMenu m;
    m.Append(wxID_UNDO, _("&Undo"));
    m.Append(wxID_REDO, _("&Redo"));
    m.AppendSeparator();
    m.Append(wxID_CUT,    _("Cu&t"));
    m.Append(wxID_COPY,   _("&Copy"));
    m.Append(wxID_PASTE,  _("&Paste"));
    m.Append(wxID_DELETE, _("&Delete"));
    m.AppendSeparator();
    m.Append(wxID_SELECTALL,       _("Select &All"));
    m.Append(Cmds::Query_Execute_selection,
        _("E&xecute selected"));

    int slen = GetSelectionEnd() - GetSelectionStart();
    if (slen && slen < 50)     // something (small) is selected
    {
        wxString sel = GetSelectedText().Strip();
        size_t p = sel.find_first_of("\n\r\t");
        if (p != wxString::npos)
            sel.Remove(p);
        m.Append(Cmds::Find_Selected_Object,
            _("S&how properties for ") + sel);
    }
    PopupMenu(&m, calcContextMenuPosition(event.GetPosition(), this));
}

void SqlEditor::OnKillFocus(wxFocusEvent& event)
{
// Milan: this makes STC crash on Mac (tested on Mavericks and Yosemite with wx3.0.x
//        because showing autocomplete box makes the edit control use focus)
#ifndef __WXMAC__
    if (AutoCompActive())
        AutoCompCancel();
#endif
    if (CallTipActive())
        CallTipCancel();
    event.Skip();   // let the STC do it's job
}

void SqlEditor::setFont()
{
    // step 1 of 2: set font
    wxFont f, f2;
    wxString s;        // since we can't get the font from control we ask config() for it
    if (config().getValue("SqlEditorFont", s) && !s.empty())
    {
        f.SetNativeFontInfo(s);
        f2 = ::wxGetFontFromUser(this, f);
    }
    else                // if config() doesn't have it, we'll use the default
    {
        wxFont font(frlayoutconfig().getEditorFontSize(), wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        f2 = ::wxGetFontFromUser(this, font);
    }

    if (!f2.Ok())    // user Canceled
        return;
    StyleSetFont(wxSTC_STYLE_DEFAULT, f2);

    // step 2 of 2: set charset
    std::map<wxString, int> sets;        // create human-readable names from wxSTC charsets
    sets["CHARSET_ANSI"] = 0;
    sets["CHARSET_EASTEUROPE"] = 238;
    sets["CHARSET_GB2312"] = 134;
    sets["CHARSET_HANGUL"] = 129;
    sets["CHARSET_HEBREW"] = 177;
    sets["CHARSET_SHIFTJIS"] = 128;
    #ifdef __WXMSW__
    sets["CHARSET_DEFAULT"] = 1;        // according to scintilla docs these only work on Windows
    sets["CHARSET_BALTIC"] = 186;        // so we won't offer them
    sets["CHARSET_CHINESEBIG5"] = 136;
    sets["CHARSET_GREEK"] = 161;
    sets["CHARSET_MAC"] = 77;
    sets["CHARSET_OEM"] = 255;
    sets["CHARSET_RUSSIAN"] = 204;
    sets["CHARSET_SYMBOL"] = 2;
    sets["CHARSET_TURKISH"] = 162;
    sets["CHARSET_JOHAB"] = 130;
    sets["CHARSET_ARABIC"] = 178;
    sets["CHARSET_VIETNAMESE"] = 163;
    sets["CHARSET_THAI"] = 222;
    #endif
    wxArrayString slist;  // copy to wxArrayString
    slist.Alloc(sets.size());
    std::map<wxString, int>::iterator it;
    for (it = sets.begin(); it != sets.end(); ++it)
        slist.Add((*it).first);

    wxString c = wxGetSingleChoice(_("Select charset to use"), _("Setting font for editor"), slist, this);
    if (c.IsEmpty())    // Canceled
        return;
    it = sets.find(c);
    if (it == sets.end())
        return;        // should never happen

    StyleSetCharacterSet(wxSTC_STYLE_DEFAULT, (*it).second);
    if (wxYES == wxMessageBox(_("Would you like to keep these settings permanently?"), _("SQL Editor"), wxYES_NO|wxICON_QUESTION))
    {
        wxString fontdesc = f2.GetNativeFontInfoDesc();
        if (!fontdesc.IsEmpty())
            config().setValue("SqlEditorFont", fontdesc);
        config().setValue("SqlEditorCharset", (*it).second);
    }
    setup();    // make control accept new settings
    showInformationDialog(wxGetTopLevelParent(this), _("The SQL editor font has been changed."),
        _("This setting affects only the SQL editor font. The font used in the result set data grid has to be changed separately."),
        AdvancedMessageDialogButtonsOk(), config(), "DIALOG_WarnFont", _("Do not show this information again"));
}

class ScrollAtEnd
{
private:
    wxStyledTextCtrl *controlM;
public:
    ScrollAtEnd(wxStyledTextCtrl *c)
        :controlM(c)
    {
    }
    ~ScrollAtEnd()
    {
        scroll();
    }
    void cancel()
    {
        controlM = 0;
    }
    void scroll()
    {
        if (controlM)
            controlM->ScrollToLine(controlM->GetLineCount()-1);
    }
};

// MB: we don't use the 'parent' parameter here, because of some ugly bugs.
//     For example, if user clicks the 'drop trigger' link on the trigger
//     property page, it creates new ExecuteSqlFrame with trigger property
//     page as a parent. When dropping SQL statement is committed, it destroys
//     the trigger object and the observer pattern takes the trigger property
//     page down with it - since it is the parent of ExecuteSqlFrame it takes
//     down the ExecuteSqlFrame as well, and the code in commitTransaction
//     function keeps executing in an invalid object.
//     This is equivallent to 'delete this' in the middle of some function
//
//     I have hard-coded the app's top window as the parent to all SQL frames
//     here as a sure way to prevent the problem. I didn't go into removing
//     the parent parameter everywhere as we might want to find some nice way
//     to make this work, and maybe we won't want to have the top frame as a
//     parent sometime.
ExecuteSqlFrame::ExecuteSqlFrame(wxWindow* WXUNUSED(parent), int id,
        wxString title,
        DatabasePtr db, const wxPoint& pos, const wxSize& size, long style)
    : BaseFrame(wxTheApp->GetTopWindow(), id, title, pos, size, style),
        Observer(), databaseM(db.get())
{
    wxASSERT(db);

    loadingM = true;
    updateEditorCaretPosM = true;
    updateFrameTitleM = true;

    transactionIsolationLevelM = static_cast<IBPP::TIL>(config().get("transactionIsolationLevel", 0));
    transactionLockResolutionM = config().get("transactionLockResolution", true) ? IBPP::lrWait : IBPP::lrNoWait;
    transactionAccessModeM = config().get("transactionAccessMode", false) ? IBPP::amRead : IBPP::amWrite;

    timerBlobEditorM.SetOwner(this, TIMER_ID_UPDATE_BLOB);

    CommandManager cm;
    buildToolbar(cm);
    buildMainMenu(cm);

    panel_contents = new wxPanel(this, -1, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL);
    splitter_window_1 = new wxSplitterWindow(panel_contents, -1);
    styled_text_ctrl_sql = new SqlEditor(splitter_window_1, ID_stc_sql);

    notebook_1 = new wxNotebook(splitter_window_1, -1, wxDefaultPosition,
        wxDefaultSize, 0);
    notebook_pane_1 = new wxPanel(notebook_1, -1);
    styled_text_ctrl_stats = new wxStyledTextCtrl(notebook_pane_1, wxID_ANY,
        wxDefaultPosition, wxDefaultSize, wxBORDER_THEME);
    styled_text_ctrl_stats->SetWrapMode(wxSTC_WRAP_WORD);
    styled_text_ctrl_stats->StyleSetForeground(1, *wxRED);
    styled_text_ctrl_stats->StyleSetForeground(2, *wxBLUE);
    notebook_1->AddPage(notebook_pane_1, _("Statistics"));

    notebook_pane_2 = new wxPanel(notebook_1, -1);
    grid_data = new DataGrid(notebook_pane_2, ID_grid_data);
    notebook_1->AddPage(notebook_pane_2, _("Data"));

    statusbar_1 = CreateStatusBar(4);
    SetStatusBarPane(-1);

    editBlobDlgM = 0;

    set_properties();
    do_layout();

    // observe database object to close on disconnect / destruction
    databaseM->attachObserver(this, false);

    executedStatementsM.clear();
    inTransaction(false);    // enable/disable controls
    setKeywords();           // set words for autocomplete feature

    historyPositionM = StatementHistory::get(databaseM).size();

    // set drop target for DnD
    styled_text_ctrl_sql->SetDropTarget(
        new SqlEditorDropTarget(this, styled_text_ctrl_sql, databaseM));

    setViewMode(false, vmEditor);
    loadingM = false;
}

Database* ExecuteSqlFrame::getDatabase() const
{
    return databaseM;
}

void ExecuteSqlFrame::buildToolbar(CommandManager& cm)
{
    //toolBarM = CreateToolBar( wxTB_FLAT|wxTB_HORIZONTAL|wxTB_TEXT, wxID_ANY );
    toolBarM = CreateToolBar( wxTB_FLAT | wxTB_HORIZONTAL, wxID_ANY );


    wxSize bmpSize(24, 24);
    toolBarM->SetToolBitmapSize(bmpSize);

    toolBarM->AddTool( wxID_NEW, _("New"),
        wxArtProvider::GetBitmap(wxART_NEW, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL, cm.getToolbarHint(_("New window"), wxID_NEW));
    toolBarM->AddTool( wxID_OPEN, _("Open"),
        wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL, cm.getToolbarHint(_("Load a file"), wxID_OPEN));
    toolBarM->AddTool( wxID_SAVE, _("Save"),
        wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL, cm.getToolbarHint(_("Save to file"), wxID_SAVE));
    toolBarM->AddTool( wxID_SAVEAS, _("Save as"),
        wxArtProvider::GetBitmap(wxART_FILE_SAVE_AS, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL, cm.getToolbarHint(_("Save under different name"), wxID_SAVEAS));
    toolBarM->AddSeparator();

    toolBarM->AddTool( wxID_BACKWARD, _("Back"),
        wxArtProvider::GetBitmap(wxART_GO_BACK, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL, cm.getToolbarHint(_("Go to previous statement"), wxID_BACKWARD));
    toolBarM->AddTool( wxID_FORWARD, _("Next"),
        wxArtProvider::GetBitmap(wxART_GO_FORWARD, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL, cm.getToolbarHint(_("Go to next statement"), wxID_FORWARD));
    toolBarM->AddTool( Cmds::History_Search, _("History"),
        wxArtProvider::GetBitmap(ART_History, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL, cm.getToolbarHint(_("Browse and search statement history"), Cmds::History_Search));
    toolBarM->AddSeparator();

    toolBarM->AddTool( Cmds::Query_Execute, _("Execute"),
        wxArtProvider::GetBitmap(ART_ExecuteStatement, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL, cm.getToolbarHint(_("Execute statement(s)"), Cmds::Query_Execute));
    toolBarM->AddTool( Cmds::Query_Show_plan, _("Show plan"),
        wxArtProvider::GetBitmap(ART_ShowExecutionPlan, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL, cm.getToolbarHint(_("Show query execution plan"), Cmds::Query_Show_plan));
    toolBarM->AddTool( Cmds::Query_Commit, _("Commit"),
        wxArtProvider::GetBitmap(ART_CommitTransaction, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL, cm.getToolbarHint(_("Commit transaction"), Cmds::Query_Commit));
    toolBarM->AddTool( Cmds::Query_Rollback, _("Rollback"),
         wxArtProvider::GetBitmap(ART_RollbackTransaction, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL, cm.getToolbarHint(_("Rollback transaction"), Cmds::Query_Rollback));
    toolBarM->AddSeparator();

    toolBarM->AddTool( Cmds::DataGrid_Insert_row, _("Insert row(s)"),
        wxArtProvider::GetBitmap(ART_InsertRow, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL, cm.getToolbarHint(_("Insert row(s) into recordset"), Cmds::DataGrid_Insert_row));
    toolBarM->AddTool( Cmds::DataGrid_Delete_row, _("Delete row(s)"),
        wxArtProvider::GetBitmap(ART_DeleteRow, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL, cm.getToolbarHint(_("Delete row(s) from recordset"), Cmds::DataGrid_Delete_row));
    toolBarM->AddSeparator();

    toolBarM->AddTool( Cmds::View_SplitView, _("Toggle split view"),
        wxArtProvider::GetBitmap(ART_ToggleView, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_CHECK, cm.getToolbarHint(_("Toggle split view"), Cmds::View_SplitView));

    toolBarM->Realize();
}

void ExecuteSqlFrame::buildMainMenu(CommandManager& cm)
{
    menuBarM = new wxMenuBar();

    wxMenu* fileMenu = new wxMenu(); // dynamic menus, created at runtime
    fileMenu->Append(wxID_NEW,
        cm.getMainMenuItemText(_("&New..."), wxID_NEW));
    fileMenu->Append(wxID_OPEN,
        cm.getMainMenuItemText(_("&Open..."), wxID_OPEN));
    fileMenu->Append(wxID_SAVE,
        cm.getMainMenuItemText(_("&Save"), wxID_SAVE));
    fileMenu->Append(wxID_SAVEAS,
        cm.getMainMenuItemText(_("Save &As..."), wxID_SAVEAS));
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_CLOSE,
        cm.getMainMenuItemText(_("&Close"), wxID_CLOSE));
    menuBarM->Append(fileMenu, _("&File"));

    wxMenu* editMenu = new wxMenu();
    editMenu->Append(wxID_UNDO,
        cm.getMainMenuItemText(_("&Undo"), wxID_UNDO));
    editMenu->Append(wxID_REDO,
        cm.getMainMenuItemText(_("&Redo"), wxID_REDO));
    editMenu->AppendSeparator();
    editMenu->Append(wxID_CUT,
        cm.getMainMenuItemText(_("Cu&t"), wxID_CUT));
    editMenu->Append(wxID_COPY,
        cm.getMainMenuItemText(_("&Copy"), wxID_COPY));
    editMenu->Append(wxID_PASTE,
        cm.getMainMenuItemText(_("&Paste"), wxID_PASTE));
    editMenu->Append(wxID_DELETE,
        cm.getMainMenuItemText(_("&Delete"), wxID_DELETE));
    editMenu->AppendSeparator();
    editMenu->Append(wxID_SELECTALL,
        cm.getMainMenuItemText(_("Select &all"), wxID_SELECTALL));
    editMenu->AppendSeparator();
    editMenu->Append(wxID_REPLACE,
        cm.getMainMenuItemText(_("Fi&nd and replace"), wxID_REPLACE));
    menuBarM->Append(editMenu, _("&Edit"));

    wxMenu* viewMenu = new wxMenu();
    viewMenu->AppendRadioItem(Cmds::View_Editor,
        cm.getMainMenuItemText(_("Sql &editor"), Cmds::View_Editor));
    viewMenu->AppendRadioItem(Cmds::View_Statistics,
        cm.getMainMenuItemText(_("&Log view"), Cmds::View_Statistics));
    viewMenu->AppendRadioItem(Cmds::View_Data,
        cm.getMainMenuItemText(_("&Data grid"), Cmds::View_Data));
    viewMenu->AppendSeparator();
    viewMenu->AppendCheckItem(Cmds::View_SplitView,
        cm.getMainMenuItemText(_("&Split view"), Cmds::View_SplitView));
    viewMenu->AppendSeparator();
    viewMenu->Append(Cmds::View_Set_editor_font, _("Set editor &font"));
    viewMenu->AppendSeparator();
    viewMenu->AppendCheckItem(Cmds::View_Wrap_long_lines,
        _("&Wrap long lines"));
    menuBarM->Append(viewMenu, _("&View"));

    wxMenu* historyMenu = new wxMenu();
    historyMenu->Append(wxID_FORWARD, _("&Next"));
    historyMenu->Append(wxID_BACKWARD, _("&Previous"));
    historyMenu->AppendSeparator();
    historyMenu->Append(Cmds::History_Search, _("&Search"));
    historyMenu->AppendSeparator();
    historyMenu->AppendCheckItem(Cmds::History_EnableLogging,
        _("&Enable logging"));
    menuBarM->Append(historyMenu, _("&History"));

    wxMenu* statementMenu = new wxMenu();
    statementMenu->Append(Cmds::Query_Execute,
        cm.getMainMenuItemText(_("&Execute"), Cmds::Query_Execute));
    statementMenu->Append(Cmds::Query_Show_plan,
        cm.getMainMenuItemText(_("Show execution &plan"), Cmds::Query_Show_plan));
    statementMenu->Append(Cmds::Query_Execute_selection,
        cm.getMainMenuItemText(_("Execute &selection"), Cmds::Query_Execute_selection));
    statementMenu->Append(Cmds::Query_Execute_from_cursor,
        cm.getMainMenuItemText(_("Exec&ute from cursor"), Cmds::Query_Execute_from_cursor));
    statementMenu->AppendSeparator();

    wxMenu* stmtPropMenu = new wxMenu();
    stmtPropMenu->AppendRadioItem(Cmds::Query_TransactionConcurrency,
        cm.getMainMenuItemText(_("Concurrency isolation mode"), Cmds::Query_TransactionConcurrency));
    stmtPropMenu->AppendRadioItem(Cmds::Query_TransactionReadDirty,
        cm.getMainMenuItemText(_("Dirty read isolation mode"), Cmds::Query_TransactionReadDirty));
    stmtPropMenu->AppendRadioItem(Cmds::Query_TransactionReadCommitted,
        cm.getMainMenuItemText(_("Read committed isolation mode"), Cmds::Query_TransactionReadCommitted));
    stmtPropMenu->AppendRadioItem(Cmds::Query_TransactionConsistency,
        cm.getMainMenuItemText(_("Consistency isolation mode"), Cmds::Query_TransactionConsistency));
    stmtPropMenu->AppendSeparator();
    stmtPropMenu->AppendCheckItem(Cmds::Query_TransactionLockResolution,
        cm.getMainMenuItemText(_("Wait for lock resolution"), Cmds::Query_TransactionLockResolution));
    stmtPropMenu->AppendSeparator();
    stmtPropMenu->AppendCheckItem(Cmds::Query_TransactionReadOnly,
        cm.getMainMenuItemText(_("Read only transaction"), Cmds::Query_TransactionReadOnly));
    statementMenu->AppendSubMenu(stmtPropMenu, _("Transaction settings"));

    statementMenu->AppendSeparator();
    statementMenu->Append(Cmds::Query_Commit,
        cm.getMainMenuItemText(_("&Commit transaction"), Cmds::Query_Commit));
    statementMenu->Append(Cmds::Query_Rollback,
        cm.getMainMenuItemText(_("&Rollback transaction"), Cmds::Query_Rollback));
    menuBarM->Append(statementMenu, _("&Statement"));

    wxMenu* gridMenu = new wxMenu();
    gridMenu->Append(Cmds::DataGrid_Insert_row,      _("I&nsert row"));
    gridMenu->Append(Cmds::DataGrid_Delete_row,      _("&Delete row"));
    gridMenu->AppendSeparator();
    gridMenu->Append(wxID_COPY,                      _("&Copy"));
    gridMenu->Append(Cmds::DataGrid_Copy_as_insert,  _("Copy &as insert statements"));
    gridMenu->Append(Cmds::DataGrid_Copy_as_update,  _("Copy as &update statements"));
    gridMenu->AppendSeparator();
    gridMenu->Append(Cmds::DataGrid_EditBlob, _("Edit BLOB..."));
    gridMenu->Append(Cmds::DataGrid_ImportBlob, _("Import BLOB from file..."));
    gridMenu->Append(Cmds::DataGrid_ExportBlob, _("Save BLOB to file..."));
    gridMenu->AppendSeparator();
    gridMenu->Append(Cmds::DataGrid_SetFieldToNULL,  _("Set field to &NULL"));
    gridMenu->AppendSeparator();
    gridMenu->Append(Cmds::DataGrid_FetchAll,        _("&Fetch all records"));
    gridMenu->Append(Cmds::DataGrid_CancelFetchAll,  _("&Stop fetching all records"));
    gridMenu->AppendSeparator();
    gridMenu->Append(Cmds::DataGrid_Save_as_html,    _("Save as &html"));
    gridMenu->Append(Cmds::DataGrid_Save_as_csv,     _("Save as cs&v"));
    gridMenu->AppendSeparator();
    gridMenu->Append(Cmds::DataGrid_Set_header_font, _("Set h&eader font"));
    gridMenu->Append(Cmds::DataGrid_Set_cell_font,   _("Set cell f&ont"));
    gridMenu->AppendSeparator();
    gridMenu->AppendCheckItem(Cmds::DataGrid_Log_changes, _("&Log data changes"));
    menuBarM->Append(gridMenu, _("&Grid"));

    SetMenuBar(menuBarM);

    // logging is always enabled by default
    menuBarM->Check(Cmds::History_EnableLogging, true);
}

void ExecuteSqlFrame::set_properties()
{
    SetSize(wxSize(628, 488));

    int statusbar_widths[] = { -2, 100, 60, -1 };
    statusbar_1->SetStatusWidths(4, statusbar_widths);

    statusbar_1->SetStatusText(databaseM->getConnectionInfoString(), 0);
    statusbar_1->SetStatusText("Rows fetched", 1);
    statusbar_1->SetStatusText("Cursor position", 2);
    statusbar_1->SetStatusText("Transaction status", 3);

    grid_data->SetTable(new DataGridTable(statementM, databaseM), true);
    splitter_window_1->Initialize(styled_text_ctrl_sql);
    viewModeM = vmEditor;

    SetIcon(wxArtProvider::GetIcon(ART_ExecuteSqlFrame, wxART_FRAME_ICON));

    closeWhenTransactionDoneM = false;
    autoCommitM = config().get("autoCommitDDL", false);
}

void ExecuteSqlFrame::do_layout()
{
    // log control notebook pane
    wxBoxSizer* sizerPane1 = new wxBoxSizer(wxHORIZONTAL);
    sizerPane1->Add(styled_text_ctrl_stats, 1, wxEXPAND);
    notebook_pane_1->SetSizer(sizerPane1);

    // data grid notebook pane
    wxBoxSizer* sizerPane2 = new wxBoxSizer(wxHORIZONTAL);
    sizerPane2->Add(grid_data, 1, wxEXPAND);
    notebook_pane_2->SetSizer(sizerPane2);

    // splitter is only control in panel_contents
    wxBoxSizer* sizerContents = new wxBoxSizer(wxHORIZONTAL);
    sizerContents->Add(splitter_window_1, 1, wxEXPAND);
    panel_contents->SetSizer(sizerContents);
    sizerContents->Fit(this);
    sizerContents->SetSizeHints(this);
}

bool ExecuteSqlFrame::doCanClose()
{
    bool saveFile = false;
    if (filenameM.IsOk() && styled_text_ctrl_sql->GetModify())
    {
        Raise();
        int res = showQuestionDialog(this, _("Do you want to save changes to the file?"),
            wxString::Format(_("You have made changes to the file\n\n%s\n\nwhich will be lost if you close without saving."),
            filenameM.GetFullPath().c_str()),
            AdvancedMessageDialogButtonsYesNoCancel(_("&Save"), _("Do&n't Save")));
        if (res != wxYES && res != wxNO)
            return false;
        saveFile = res == wxYES;
    }

    if (transactionM != 0 && transactionM->Started())
    {
        Raise();
        int res = showQuestionDialog(this, _("Do you want to commit the active transaction?"),
            _("If you don't commit the transaction then it will be automatically rolled back, and all changes made by statements executed in this transaction will be lost."),
            AdvancedMessageDialogButtonsYesNoCancel(_("&Commit Transaction"), _("&Rollback Transaction")),
            config(), "DIALOG_ActiveTransaction", _("Don't ask again, &always commit/rollback"));
        if (res != wxYES && res != wxNO)
            return false;
        if (res == wxYES && !commitTransaction())
            return false;
    }

    if (saveFile && !styled_text_ctrl_sql->SaveFile(filenameM.GetFullPath()))
        return false;
    return true;
}

void ExecuteSqlFrame::doBeforeDestroy()
{
    // prevent editor from updating the invalid dataset
    if (grid_data->IsCellEditControlEnabled())
        grid_data->EnableCellEditControl(false);
    // make sure that further calls to update() will not call Close() again
    databaseM = 0;
}

void ExecuteSqlFrame::showProperties(wxString objectName)
{
    MetadataItem *m = databaseM->findByName(objectName);
    if (!m)
        m = databaseM->findByName(objectName.Upper());

    if (m)
    {
        MetadataItemPropertiesFrame::showPropertyPage(m);
        return;
    }

    wxMessageBox(
        wxString::Format(_("Object %s has not been found in this database."),
            objectName.c_str()),
        _("Search failed."), wxOK | wxICON_INFORMATION);
}

BEGIN_EVENT_TABLE(ExecuteSqlFrame, wxFrame)
    EVT_STC_UPDATEUI(ExecuteSqlFrame::ID_stc_sql, ExecuteSqlFrame::OnSqlEditUpdateUI)
    EVT_STC_CHARADDED(ExecuteSqlFrame::ID_stc_sql, ExecuteSqlFrame::OnSqlEditCharAdded)
    EVT_STC_CHANGE(ExecuteSqlFrame::ID_stc_sql, ExecuteSqlFrame::OnSqlEditChanged)
    EVT_STC_START_DRAG(ExecuteSqlFrame::ID_stc_sql, ExecuteSqlFrame::OnSqlEditStartDrag)
    EVT_SPLITTER_UNSPLIT(wxID_ANY, ExecuteSqlFrame::OnSplitterUnsplit)
    EVT_CHAR_HOOK(ExecuteSqlFrame::OnKeyDown)
    EVT_CHILD_FOCUS(ExecuteSqlFrame::OnChildFocus)
    EVT_IDLE(ExecuteSqlFrame::OnIdle)
    EVT_ACTIVATE(ExecuteSqlFrame::OnActivate)

    EVT_MENU(wxID_NEW,      ExecuteSqlFrame::OnMenuNew)
    EVT_MENU(wxID_OPEN,     ExecuteSqlFrame::OnMenuOpen)
    EVT_MENU(wxID_SAVE,     ExecuteSqlFrame::OnMenuSaveOrSaveAs)
    EVT_MENU(wxID_SAVEAS,   ExecuteSqlFrame::OnMenuSaveOrSaveAs)
    EVT_MENU(wxID_CLOSE,    ExecuteSqlFrame::OnMenuClose)

    EVT_MENU(wxID_UNDO,     ExecuteSqlFrame::OnMenuUndo)
    EVT_MENU(wxID_REDO,     ExecuteSqlFrame::OnMenuRedo)
    EVT_MENU(wxID_CUT,      ExecuteSqlFrame::OnMenuCut)
    EVT_MENU(wxID_COPY,     ExecuteSqlFrame::OnMenuCopy)
    EVT_MENU(wxID_PASTE,    ExecuteSqlFrame::OnMenuPaste)
    EVT_MENU(wxID_DELETE,   ExecuteSqlFrame::OnMenuDelete)
    EVT_MENU(wxID_SELECTALL,ExecuteSqlFrame::OnMenuSelectAll)
    EVT_MENU(wxID_REPLACE,  ExecuteSqlFrame::OnMenuReplace)

    EVT_UPDATE_UI(wxID_UNDO,    ExecuteSqlFrame::OnMenuUpdateUndo)
    EVT_UPDATE_UI(wxID_REDO,    ExecuteSqlFrame::OnMenuUpdateRedo)
    EVT_UPDATE_UI(wxID_CUT,     ExecuteSqlFrame::OnMenuUpdateCut)
    EVT_UPDATE_UI(wxID_COPY,    ExecuteSqlFrame::OnMenuUpdateCopy)
    EVT_UPDATE_UI(wxID_PASTE,   ExecuteSqlFrame::OnMenuUpdatePaste)
    EVT_UPDATE_UI(wxID_DELETE,  ExecuteSqlFrame::OnMenuUpdateDelete)

    EVT_MENU(Cmds::View_Editor, ExecuteSqlFrame::OnMenuSelectView)
    EVT_UPDATE_UI(Cmds::View_Editor, ExecuteSqlFrame::OnMenuUpdateSelectView)
    EVT_MENU(Cmds::View_Statistics, ExecuteSqlFrame::OnMenuSelectView)
    EVT_UPDATE_UI(Cmds::View_Statistics, ExecuteSqlFrame::OnMenuUpdateSelectView)
    EVT_MENU(Cmds::View_Data, ExecuteSqlFrame::OnMenuSelectView)
    EVT_UPDATE_UI(Cmds::View_Data, ExecuteSqlFrame::OnMenuUpdateSelectView)
    EVT_MENU(Cmds::View_SplitView, ExecuteSqlFrame::OnMenuSplitView)
    EVT_UPDATE_UI(Cmds::View_SplitView, ExecuteSqlFrame::OnMenuUpdateSplitView)
    EVT_MENU(Cmds::View_Set_editor_font, ExecuteSqlFrame::OnMenuSetEditorFont)
    EVT_MENU(Cmds::View_Wrap_long_lines, ExecuteSqlFrame::OnMenuToggleWrap)

    EVT_MENU(Cmds::Find_Selected_Object,   ExecuteSqlFrame::OnMenuFindSelectedObject)

    EVT_MENU(wxID_FORWARD,         ExecuteSqlFrame::OnMenuHistoryNext)
    EVT_MENU(wxID_BACKWARD,        ExecuteSqlFrame::OnMenuHistoryPrev)
    EVT_MENU(Cmds::History_Search, ExecuteSqlFrame::OnMenuHistorySearch)
    EVT_UPDATE_UI(wxID_FORWARD,    ExecuteSqlFrame::OnMenuUpdateHistoryNext)
    EVT_UPDATE_UI(wxID_BACKWARD,   ExecuteSqlFrame::OnMenuUpdateHistoryPrev)

    EVT_MENU(Cmds::Query_Execute,             ExecuteSqlFrame::OnMenuExecute)
    EVT_MENU(Cmds::Query_Show_plan,           ExecuteSqlFrame::OnMenuShowPlan)
    EVT_MENU(Cmds::Query_Execute_selection,   ExecuteSqlFrame::OnMenuExecuteSelection)
    EVT_MENU(Cmds::Query_Execute_from_cursor, ExecuteSqlFrame::OnMenuExecuteFromCursor)
    EVT_UPDATE_UI(Cmds::Query_Execute,             ExecuteSqlFrame::OnMenuUpdateWhenExecutePossible)
    EVT_UPDATE_UI(Cmds::Query_Show_plan,           ExecuteSqlFrame::OnMenuUpdateWhenExecutePossible)
    EVT_UPDATE_UI(Cmds::Query_Execute_selection,   ExecuteSqlFrame::OnMenuUpdateWhenExecutePossible)
    EVT_UPDATE_UI(Cmds::Query_Execute_from_cursor, ExecuteSqlFrame::OnMenuUpdateWhenExecutePossible)
    EVT_MENU(Cmds::Query_Commit,              ExecuteSqlFrame::OnMenuCommit)
    EVT_MENU(Cmds::Query_Rollback,            ExecuteSqlFrame::OnMenuRollback)
    EVT_UPDATE_UI(Cmds::Query_Commit,         ExecuteSqlFrame::OnMenuUpdateWhenInTransaction)
    EVT_UPDATE_UI(Cmds::Query_Rollback,       ExecuteSqlFrame::OnMenuUpdateWhenInTransaction)
    EVT_MENU_RANGE(Cmds::Query_TransactionConcurrency,      Cmds::Query_TransactionConsistency, ExecuteSqlFrame::OnMenuTransactionIsolationLevel)
    EVT_UPDATE_UI_RANGE(Cmds::Query_TransactionConcurrency, Cmds::Query_TransactionConsistency, ExecuteSqlFrame::OnMenuUpdateTransactionIsolationLevel)
    EVT_MENU(Cmds::Query_TransactionLockResolution,         ExecuteSqlFrame::OnMenuTransactionLockResolution)
    EVT_UPDATE_UI(Cmds::Query_TransactionLockResolution,    ExecuteSqlFrame::OnMenuUpdateTransactionLockResolution)
    EVT_MENU(Cmds::Query_TransactionReadOnly,       ExecuteSqlFrame::OnMenuTransactionReadOnly)
    EVT_UPDATE_UI(Cmds::Query_TransactionReadOnly,  ExecuteSqlFrame::OnMenuUpdateTransactionReadOnly)

    EVT_MENU(Cmds::DataGrid_Insert_row,      ExecuteSqlFrame::OnMenuGridInsertRow)
    EVT_MENU(Cmds::DataGrid_Delete_row,      ExecuteSqlFrame::OnMenuGridDeleteRow)
    EVT_MENU(Cmds::DataGrid_SetFieldToNULL,  ExecuteSqlFrame::OnMenuGridSetFieldToNULL)
    EVT_MENU(Cmds::DataGrid_Copy_with_header,ExecuteSqlFrame::OnMenuCopyWithHeader)
    EVT_MENU(Cmds::DataGrid_Copy_as_insert,  ExecuteSqlFrame::OnMenuGridCopyAsInsert)
    EVT_MENU(Cmds::DataGrid_Copy_as_inList,  ExecuteSqlFrame::OnMenuGridCopyAsInList)
    EVT_MENU(Cmds::DataGrid_Copy_as_update,  ExecuteSqlFrame::OnMenuGridCopyAsUpdate)
    EVT_MENU(Cmds::DataGrid_EditBlob,        ExecuteSqlFrame::OnMenuGridEditBlob)
    EVT_MENU(Cmds::DataGrid_ImportBlob,      ExecuteSqlFrame::OnMenuGridImportBlob)
    EVT_MENU(Cmds::DataGrid_ExportBlob,      ExecuteSqlFrame::OnMenuGridExportBlob)
    EVT_MENU(Cmds::DataGrid_Save_as_html,    ExecuteSqlFrame::OnMenuGridSaveAsHtml)
    EVT_MENU(Cmds::DataGrid_Save_as_csv,     ExecuteSqlFrame::OnMenuGridSaveAsCsv)
    EVT_MENU(Cmds::DataGrid_Set_header_font, ExecuteSqlFrame::OnMenuGridGridHeaderFont)
    EVT_MENU(Cmds::DataGrid_Set_cell_font,   ExecuteSqlFrame::OnMenuGridGridCellFont)
    EVT_MENU(Cmds::DataGrid_FetchAll,        ExecuteSqlFrame::OnMenuGridFetchAll)
    EVT_MENU(Cmds::DataGrid_CancelFetchAll,  ExecuteSqlFrame::OnMenuGridCancelFetchAll)

    EVT_UPDATE_UI(Cmds::DataGrid_Insert_row,     ExecuteSqlFrame::OnMenuUpdateGridInsertRow)
    EVT_UPDATE_UI(Cmds::DataGrid_Delete_row,     ExecuteSqlFrame::OnMenuUpdateGridDeleteRow)
    EVT_UPDATE_UI(Cmds::DataGrid_SetFieldToNULL, ExecuteSqlFrame::OnMenuUpdateGridCanSetFieldToNULL)
    EVT_UPDATE_UI(Cmds::DataGrid_Copy_with_header,ExecuteSqlFrame::OnMenuUpdateGridHasData)
    EVT_UPDATE_UI(Cmds::DataGrid_Copy_as_insert, ExecuteSqlFrame::OnMenuUpdateGridHasData)
    EVT_UPDATE_UI(Cmds::DataGrid_Copy_as_update, ExecuteSqlFrame::OnMenuUpdateGridHasData)
    EVT_UPDATE_UI(Cmds::DataGrid_EditBlob,       ExecuteSqlFrame::OnMenuUpdateGridCellIsBlob)
    EVT_UPDATE_UI(Cmds::DataGrid_ImportBlob,     ExecuteSqlFrame::OnMenuUpdateGridCellIsBlob)
    EVT_UPDATE_UI(Cmds::DataGrid_ExportBlob,     ExecuteSqlFrame::OnMenuUpdateGridCellIsBlob)
    EVT_UPDATE_UI(Cmds::DataGrid_Save_as_html,   ExecuteSqlFrame::OnMenuUpdateGridHasSelection)
    EVT_UPDATE_UI(Cmds::DataGrid_Save_as_csv,    ExecuteSqlFrame::OnMenuUpdateGridHasSelection)
    EVT_UPDATE_UI(Cmds::DataGrid_FetchAll,       ExecuteSqlFrame::OnMenuUpdateGridFetchAll)
    EVT_UPDATE_UI(Cmds::DataGrid_CancelFetchAll, ExecuteSqlFrame::OnMenuUpdateGridCancelFetchAll)


    EVT_COMMAND(ExecuteSqlFrame::ID_grid_data, wxEVT_FRDG_ROWCOUNT_CHANGED, \
        ExecuteSqlFrame::OnGridRowCountChanged)
    EVT_COMMAND(ExecuteSqlFrame::ID_grid_data, wxEVT_FRDG_STATEMENT, \
        ExecuteSqlFrame::OnGridStatementExecuted)
    EVT_COMMAND(ExecuteSqlFrame::ID_grid_data, wxEVT_FRDG_INVALIDATEATTR, \
        ExecuteSqlFrame::OnGridInvalidateAttributeCache)
    EVT_COMMAND(ExecuteSqlFrame::ID_grid_data, wxEVT_FRDG_SUM, \
        ExecuteSqlFrame::OnGridSum)

    EVT_GRID_CMD_SELECT_CELL(ExecuteSqlFrame::ID_grid_data, ExecuteSqlFrame::OnGridCellChange)
    EVT_GRID_CMD_LABEL_LEFT_DCLICK(ExecuteSqlFrame::ID_grid_data, ExecuteSqlFrame::OnGridLabelLeftDClick)

    EVT_TIMER(ExecuteSqlFrame::TIMER_ID_UPDATE_BLOB, ExecuteSqlFrame::OnBlobEditorUpdate)
END_EVENT_TABLE()

// Avoiding the annoying thing that you cannot click inside the selection and have it deselected and have caret there
void ExecuteSqlFrame::OnSqlEditStartDrag(wxStyledTextEvent& event)
{
    wxPoint mp = ::wxGetMousePosition();
    int p = styled_text_ctrl_sql->PositionFromPoint(styled_text_ctrl_sql->ScreenToClient(mp));
    styled_text_ctrl_sql->SetSelectionStart(p);    // deselect text
    styled_text_ctrl_sql->SetSelectionEnd(p);
    // cancel drag operation, because drag and drop editing is disabled
    // by our own SqlEditorDropTarget anyway
    event.SetDragText(wxEmptyString);
}

//! display editor col:row in StatusBar and do highlighting of braces ()
void ExecuteSqlFrame::OnSqlEditUpdateUI(wxStyledTextEvent& WXUNUSED(event))
{
    if (loadingM)
        return;

    // mghie: do not update the statusbar from here, because that slows
    //        everything down a lot on Mac OS X
    updateEditorCaretPosM = true;

    // check for braces, and highlight
    int p = styled_text_ctrl_sql->GetCurrentPos();
    int c1 = styled_text_ctrl_sql->GetCharAt(p);
    int c2 = (p > 1 ? styled_text_ctrl_sql->GetCharAt(p-1) : 0);

    if (c2=='(' || c2==')' || c1=='(' || c1==')')
    {
        int sp = (c2=='(' || c2==')') ? p-1 : p;

        int q = styled_text_ctrl_sql->BraceMatch(sp);
        if (q == wxSTC_INVALID_POSITION)
            styled_text_ctrl_sql->BraceBadLight(sp);
        else
            styled_text_ctrl_sql->BraceHighlight(sp, q);

        // remove calltip if needed
        if (styled_text_ctrl_sql->CallTipActive() && (c1==')' || c2==')')
            && q == styled_text_ctrl_sql->CallTipPosAtStart() - 1)
        {
            styled_text_ctrl_sql->CallTipCancel();
        }
    }
    else
        styled_text_ctrl_sql->BraceBadLight(wxSTC_INVALID_POSITION);    // remove light
}

//! returns true if there is a word in "wordlist" that starts with "word"
bool HasWord(wxString word, wxString& wordlist)
{
    word.MakeUpper();

    wxStringTokenizer tkz(wordlist, " ");
    while (tkz.HasMoreTokens())
    {
        if (tkz.GetNextToken().Upper().StartsWith(word))
            return true;
    }
    return false;
}

//! autocomplete stuff
void ExecuteSqlFrame::OnSqlEditCharAdded(wxStyledTextEvent& event)
{
    int pos = styled_text_ctrl_sql->GetCurrentPos();
    if (pos == 0)
        return;

    int c = event.GetKey();
    if (c == '\n')
    {
        if (config().get("sqlEditorAutoIndent", true))
        {
            int lineNum = styled_text_ctrl_sql->LineFromPosition(pos - 1);
            int linestart = styled_text_ctrl_sql->PositionFromLine(lineNum);
            int indpos = styled_text_ctrl_sql->GetLineIndentPosition(lineNum);
            wxString indent(styled_text_ctrl_sql->GetTextRange(linestart,
                indpos));
            int selpos = styled_text_ctrl_sql->GetSelectionStart();
            styled_text_ctrl_sql->InsertText(selpos, indent);
            styled_text_ctrl_sql->GotoPos(selpos + indent.Length());
        }
    }
    else if (c == '(')
    {
        if (config().get("SQLEditorCalltips", true))
        {
            int start = styled_text_ctrl_sql->WordStartPosition(pos - 2, true);
            if (start != -1 && start != pos - 2)
            {
                wxString word = styled_text_ctrl_sql->GetTextRange(start, pos - 1).Upper();
                wxString calltip;
                Procedure* p = dynamic_cast<Procedure*>(databaseM->findByNameAndType(ntProcedure, word));
                if (p)
                    calltip = p->getDefinition();
// TODO: review tip for package, function and udf
                /*
                UDF* f = dynamic_cast<UDF*>(databaseM->findByNameAndType(ntUDF, word)); 
                if (f)
                    calltip = f->getDefinition();
                
                FunctionSQL* f = dynamic_cast<FunctionSQL*>(databaseM->findByNameAndType(ntFunctionSQL, word));
                if (f)
                    calltip = f->getDefinition();
                 */ 
                if (!calltip.empty())
                {
                    styled_text_ctrl_sql->CallTipShow(start, calltip);
                    styled_text_ctrl_sql->CallTipSetHighlight(0, pos - 1 - start);    // start, end
                }
            }
        }
    }
    else
    {
        if (config().get("AutocompleteEnabled", true))
        {
            #ifndef __WXGTK20__
            bool allow = config().get("autoCompleteQuoted", true);
            if (!allow)
            {
                // needed since event that updates the style happens later
                ::wxSafeYield();
                if (styled_text_ctrl_sql->GetStyleAt(pos - 1) != 7)   // not in quotes
                    allow = true;
            }
            #else
            bool allow = true;  // ::wxSafeYield kills the focus on gtk2
                                // so, until we make a parser to detect whether we're
                                // inside quotes or not - this #ifdef stays
            #endif
            if (allow)
            {
                if (styled_text_ctrl_sql->CallTipActive())
                {
                    if (!config().get("AutoCompleteDisableWhenCalltipShown", true))
                        autoComplete(false);
                }
                else
                    autoComplete(false);
            }
        }
    }

    // join table ON __autocomplete FK relation__
    if (c == 'N' && pos > 1 && 'O' == styled_text_ctrl_sql->GetCharAt(pos-2))
    {
        // TODO:
        // autocomplete JOIN table t2 ON ... write FK relation automatically
        // IncompleteStatement is(databaseM, styled_text_ctrl_sql->GetText());
        // wxString join = is.getJoin(pos);
        // if (!join.IsEmpty())
        //    add "join" to sql editor
    }
}

void ExecuteSqlFrame::OnSqlEditChanged(wxStyledTextEvent& WXUNUSED(event))
{
    updateFrameTitleM = true;
}

void ExecuteSqlFrame::autoCompleteColumns(int pos, int len)
{
    int start;
    if (pos > 2 && styled_text_ctrl_sql->GetCharAt(pos-2) == '"')
    {   // first we check if object name is quoted
        start = pos-3;
        while (start > -1 && styled_text_ctrl_sql->GetCharAt(start) != '"')
            --start;
    }
    else
    {   // only allow chars valid for FB identifier
        styled_text_ctrl_sql->setChars(true);
        start = styled_text_ctrl_sql->WordStartPosition(pos-1, true);
        styled_text_ctrl_sql->setChars(false); // reset to default behavior
        if (start == -1)
            return;
    }
    wxString table = styled_text_ctrl_sql->GetTextRange(start, pos-1);
    IncompleteStatement is(databaseM, styled_text_ctrl_sql->GetText());
    wxString columns = is.getObjectColumns(table, pos, len>0 || config().get("autoCompleteLoadColumnsSort", false));//When the user are typing something, you need to sort de result, else intelisense won't work properly
    if (columns.IsEmpty())
        return;
    if (HasWord(styled_text_ctrl_sql->GetTextRange(pos, pos+len), columns))
        styled_text_ctrl_sql->AutoCompShow(len, columns);
}

void ExecuteSqlFrame::autoComplete(bool force)
{
    if (styled_text_ctrl_sql->AutoCompActive())
        return;

    int autoCompleteChars = 1;
    if (!force)
    {
        autoCompleteChars = config().get("AutocompleteChars", 3);
        if (autoCompleteChars <= 0)
            return;
    }

    int pos = styled_text_ctrl_sql->GetCurrentPos();
    int start = styled_text_ctrl_sql->WordStartPosition(pos, true);
    if (start > 1 && styled_text_ctrl_sql->GetCharAt(start - 1) == '.')
    {
// TODO: Autocomplete function/procedure for a package
        autoCompleteColumns(start, pos-start);
        return;
    }

    if (start != -1 && pos - start >= autoCompleteChars)
    {
        // GTK version crashes if nothing matches, so this check must be made for GTK
        // For MSW, it doesn't crash but it flashes on the screen (also not very nice)
        if (HasWord(styled_text_ctrl_sql->GetTextRange(start, pos), keywordsM))
            styled_text_ctrl_sql->AutoCompShow(pos-start, keywordsM);
    }
}

void ExecuteSqlFrame::OnMenuFindSelectedObject(wxCommandEvent& WXUNUSED(event))
{
    wxString sel = styled_text_ctrl_sql->GetSelectedText();
    int p = sel.Find(" ");
    if (p != -1)
        sel.Remove(p);
    showProperties(sel);
}

//! handle function keys
void ExecuteSqlFrame::OnKeyDown(wxKeyEvent& event)
{
    int key = event.GetKeyCode();
    if (!event.HasModifiers() && key == WXK_F3)
    {
        styled_text_ctrl_sql->find(false);
        return;
    }

    if (wxWindow::FindFocus() == styled_text_ctrl_sql)
    {
        if (!styled_text_ctrl_sql->AutoCompActive())
        {
            enum { acSpace=0, acTab };
            int acc = acSpace;
            config().getValue("AutoCompleteKey", acc);
            if (acc == acSpace && event.ControlDown() && key == WXK_SPACE)
            {
                autoComplete(true);
                return;
            }

            // TAB completion works when there is no white space before cursor and there is no selection
            if (acc == acTab && key == WXK_TAB && styled_text_ctrl_sql->GetSelectionStart() == styled_text_ctrl_sql->GetSelectionEnd())
            {
                int p = styled_text_ctrl_sql->GetCurrentPos();
                if (p > 0)
                {
                    int ch = styled_text_ctrl_sql->GetCharAt(p-1);
                    if (ch != ' ' && (ch < 0x09 || ch > 0x0d))        // <- as taken from scintilla/src/Document.cxx
                    {
                        autoComplete(true);
                        return;                    // don't Skip the event
                    }
                }
            }
        }
        else if (key == WXK_RETURN)
        {
            if (!config().get("AutoCompleteWithEnter", true))
                styled_text_ctrl_sql->AutoCompCancel();
        }
    }
    event.Skip();
}

void ExecuteSqlFrame::OnChildFocus(wxChildFocusEvent& WXUNUSED(event))
{
    doUpdateFocusedControlM = true;
}

void ExecuteSqlFrame::OnIdle(wxIdleEvent& event)
{
    if (doUpdateFocusedControlM)
        updateViewMode();
    if (updateEditorCaretPosM)
    {
        updateEditorCaretPosM = false;

        int p = styled_text_ctrl_sql->GetCurrentPos();
        int row = styled_text_ctrl_sql->GetCurrentLine();
        int col = p - styled_text_ctrl_sql->PositionFromLine(row);
        statusbar_1->SetStatusText(wxString::Format("%d : %d",
            row + 1, col + 1), 2);
    }
    if (updateFrameTitleM)
    {
        updateFrameTitleM = false;
        updateFrameTitle();
    }
    event.Skip();
}

void ExecuteSqlFrame::OnActivate(wxActivateEvent& event)
{
    if (event.GetActive() && filenameM.FileExists())
    {
        wxDateTime modified = filenameM.GetModificationTime();
        if (filenameModificationTimeM != modified)
        {
            filenameModificationTimeM = modified;

            Raise();
            int res = showQuestionDialog(this,
                _("Do you want to load external modifications to this file?"),
                wxString::Format(_("The file\n\n%s\n\nhas been modified by another program. Do you want to reload it?\nIf you reload the file now your own modifications will be lost."),
                filenameM.GetFullPath().c_str()),
                AdvancedMessageDialogButtonsOkCancel(_("&Reload"), _("Do&n't Reload")));
            if (res == wxOK)
                loadSqlFile(filenameM.GetFullPath());
        }
    }
    event.Skip();
}

void ExecuteSqlFrame::OnMenuNew(wxCommandEvent& WXUNUSED(event))
{
    ExecuteSqlFrame *eff = new ExecuteSqlFrame(GetParent(), -1,
        _("Execute SQL statements"), databaseM->shared_from_this());
    eff->setSql(styled_text_ctrl_sql->GetSelectedText());
    eff->Show();
}

void ExecuteSqlFrame::OnMenuOpen(wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog fd(this, _("Open File"),
        filenameM.GetPath(), filenameM.GetName(),
        _("SQL script files (*.sql)|*.sql|All files (*.*)|*.*"),
        wxFD_OPEN | wxFD_CHANGE_DIR);
    if (wxID_OK == fd.ShowModal())
        loadSqlFile(fd.GetPath());
}

void ExecuteSqlFrame::OnMenuSaveOrSaveAs(wxCommandEvent& event)
{
    wxString filename(filenameM.GetFullPath());
    if (event.GetId() == wxID_SAVEAS || !filenameM.IsOk())
    {
        wxFileDialog fd(this, _("Save File As"),
            filenameM.GetPath(), filenameM.GetName(),
            _("SQL script files (*.sql)|*.sql|All files (*.*)|*.*"),
            wxFD_SAVE | wxFD_CHANGE_DIR | wxFD_OVERWRITE_PROMPT);
        if (wxID_OK != fd.ShowModal())
            return;
        filename = fd.GetPath();
    }

    bool useAlternativeSaveMode = config().get("useAlternativeSaveMode", false);
    int saveStatus = 0, errorCode = -1;
    
    if (useAlternativeSaveMode)
    {
        wxFile file(filename, wxFile::write);
        if (saveStatus = file.Write(styled_text_ctrl_sql->GetValue()))
        {
            file.Close();
            styled_text_ctrl_sql->SetModified(false);
        }
        else 
            errorCode = file.GetLastError();
    }
    else
    {
        saveStatus = styled_text_ctrl_sql->SaveFile(filename);
        if (! saveStatus )
        {
#ifdef __WINDOWS__
            errorCode = GetLastError();
#endif
        }

    }

    if (saveStatus)
    {
        filenameM = filename;
        filenameModificationTimeM = wxFileName(filenameM).GetModificationTime();
        updateFrameTitleM = true;
        statusbar_1->SetStatusText((_("File saved")), 2);
    }
    else
    {
        throw FRError(wxString::Format(_("Error saving the file, attention for not losing your SQL. Error code: %d"), errorCode));
    }
}

void ExecuteSqlFrame::OnMenuClose(wxCommandEvent& WXUNUSED(event))
{
    Close();
}

void ExecuteSqlFrame::OnMenuUndo(wxCommandEvent& WXUNUSED(event))
{
    if (viewModeM == vmEditor)
        styled_text_ctrl_sql->Undo();
}

void ExecuteSqlFrame::OnMenuUpdateUndo(wxUpdateUIEvent& event)
{
    event.Enable(viewModeM == vmEditor && styled_text_ctrl_sql->CanUndo());
}

void ExecuteSqlFrame::OnMenuRedo(wxCommandEvent& WXUNUSED(event))
{
    if (viewModeM == vmEditor)
        styled_text_ctrl_sql->Redo();
}

void ExecuteSqlFrame::OnMenuUpdateRedo(wxUpdateUIEvent& event)
{
    event.Enable(viewModeM == vmEditor && styled_text_ctrl_sql->CanRedo());
}

void ExecuteSqlFrame::OnMenuCopy(wxCommandEvent& WXUNUSED(event))
{
    if (viewModeM == vmEditor)
        styled_text_ctrl_sql->Copy();
    else if (viewModeM == vmGrid)
        grid_data->copyToClipboard(false);
}

void ExecuteSqlFrame::OnMenuCopyWithHeader(wxCommandEvent& WXUNUSED(event))
{
    if (viewModeM == vmEditor)
        styled_text_ctrl_sql->Copy();
    else if (viewModeM == vmGrid)
        grid_data->copyToClipboard(true);
}

void ExecuteSqlFrame::OnMenuUpdateCopy(wxUpdateUIEvent& event)
{
    bool enableCmd = false;
    if (viewModeM == vmEditor)
        enableCmd = styled_text_ctrl_sql->hasSelection();
    else if (viewModeM == vmGrid)
        enableCmd = grid_data->getDataGridTable() && grid_data->GetNumberRows();
    event.Enable(enableCmd);
}

void ExecuteSqlFrame::OnMenuCut(wxCommandEvent& WXUNUSED(event))
{
    if (viewModeM == vmEditor)
        styled_text_ctrl_sql->Cut();
}

void ExecuteSqlFrame::OnMenuUpdateCut(wxUpdateUIEvent& event)
{
    event.Enable(viewModeM == vmEditor && styled_text_ctrl_sql->hasSelection());
}

void ExecuteSqlFrame::OnMenuDelete(wxCommandEvent& WXUNUSED(event))
{
    if (viewModeM == vmEditor)
        styled_text_ctrl_sql->Clear();
}

void ExecuteSqlFrame::OnMenuUpdateDelete(wxUpdateUIEvent& event)
{
    event.Enable(viewModeM == vmEditor && styled_text_ctrl_sql->hasSelection());
}

void ExecuteSqlFrame::OnMenuPaste(wxCommandEvent& WXUNUSED(event))
{
    if (viewModeM == vmEditor)
        styled_text_ctrl_sql->Paste();
}

void ExecuteSqlFrame::OnMenuUpdatePaste(wxUpdateUIEvent& event)
{
    event.Enable(viewModeM == vmEditor && styled_text_ctrl_sql->CanPaste());
}

void ExecuteSqlFrame::OnMenuSelectAll(wxCommandEvent& WXUNUSED(event))
{
    if (viewModeM == vmEditor)
        styled_text_ctrl_sql->SelectAll();
    else if (viewModeM == vmLogCtrl)
        styled_text_ctrl_stats->SelectAll();
    else if (viewModeM == vmGrid)
        grid_data->SelectAll();
}

void ExecuteSqlFrame::OnMenuReplace(wxCommandEvent &WXUNUSED(event))
{
    styled_text_ctrl_sql->find(true);
}

void ExecuteSqlFrame::OnMenuUpdateWhenInTransaction(wxUpdateUIEvent& event)
{
    event.Enable(inTransactionM && !grid_data->IsCellEditControlEnabled());
}

void ExecuteSqlFrame::OnMenuSelectView(wxCommandEvent& event)
{
    if (event.GetId() == Cmds::View_Editor)
        setViewMode(vmEditor);
    else if (event.GetId() == Cmds::View_Statistics)
        setViewMode(vmLogCtrl);
    else if (event.GetId() == Cmds::View_Data)
        setViewMode(vmGrid);
    else
        wxCHECK_RET(false, "event id not handled");
}

void ExecuteSqlFrame::OnMenuUpdateSelectView(wxUpdateUIEvent& event)
{
    if (event.GetId() == Cmds::View_Editor && viewModeM == vmEditor)
        event.Check(true);
    else if (event.GetId() == Cmds::View_Statistics && viewModeM == vmLogCtrl)
        event.Check(true);
    else if (event.GetId() == Cmds::View_Data && viewModeM == vmGrid)
        event.Check(true);
}

void ExecuteSqlFrame::OnMenuSplitView(wxCommandEvent& WXUNUSED(event))
{
    setViewMode(!splitter_window_1->IsSplit(), viewModeM);
}

void ExecuteSqlFrame::OnMenuUpdateSplitView(wxUpdateUIEvent& event)
{
    event.Check(splitter_window_1->IsSplit());
}

void ExecuteSqlFrame::OnMenuSetEditorFont(wxCommandEvent& WXUNUSED(event))
{
    styled_text_ctrl_sql->setFont();
}

void ExecuteSqlFrame::OnMenuToggleWrap(wxCommandEvent& WXUNUSED(event))
{
    const int mode = styled_text_ctrl_sql->GetWrapMode();
    styled_text_ctrl_sql->SetWrapMode(
        (mode == wxSTC_WRAP_WORD) ? wxSTC_WRAP_NONE : wxSTC_WRAP_WORD);
}

void ExecuteSqlFrame::OnMenuHistoryNext(wxCommandEvent& WXUNUSED(event))
{
    StatementHistory& sh = StatementHistory::get(databaseM);
    if (historyPositionM != sh.size())  // we're already at the end?
    {
        StatementHistory::Position pos = historyPositionM + 1;
        wxString sql(pos == sh.size() ? localBuffer : sh.get(pos));
        if (setSql(sql))
            historyPositionM = pos;
    }
}

void ExecuteSqlFrame::OnMenuHistoryPrev(wxCommandEvent& WXUNUSED(event))
{
    StatementHistory& sh = StatementHistory::get(databaseM);
    if (historyPositionM > 0 && sh.size() > 0)
    {
        if (historyPositionM == sh.size())
        {
            // we're on local buffer => store it
            localBuffer = styled_text_ctrl_sql->GetText();
        }
        StatementHistory::Position pos = historyPositionM - 1;
        if (setSql(sh.get(pos)))
            historyPositionM = pos;
    }
}

void ExecuteSqlFrame::OnMenuHistorySearch(wxCommandEvent& WXUNUSED(event))
{
    StatementHistory& sh = StatementHistory::get(databaseM);
    StatementHistoryDialog *shf = new StatementHistoryDialog(this, &sh);
    if (shf->ShowModal() == wxID_OK)
        setSql(shf->getSql());
}

void ExecuteSqlFrame::OnMenuUpdateHistoryNext(wxUpdateUIEvent& event)
{
    StatementHistory& sh = StatementHistory::get(databaseM);
    event.Enable(sh.size() > historyPositionM);
}

void ExecuteSqlFrame::OnMenuUpdateHistoryPrev(wxUpdateUIEvent& event)
{
    StatementHistory& sh = StatementHistory::get(databaseM);
    event.Enable(historyPositionM > 0 && sh.size() > 0);
}

void ExecuteSqlFrame::OnMenuExecute(wxCommandEvent& WXUNUSED(event))
{
    clearLogBeforeExecution();
    prepareAndExecute(false);
}

void ExecuteSqlFrame::OnMenuShowPlan(wxCommandEvent& WXUNUSED(event))
{
    prepareAndExecute(true);
}

void ExecuteSqlFrame::OnMenuExecuteFromCursor(wxCommandEvent& WXUNUSED(event))
{
    clearLogBeforeExecution();

    wxString sql(
        styled_text_ctrl_sql->GetTextRange(
            styled_text_ctrl_sql->GetCurrentPos(),
            styled_text_ctrl_sql->GetLength()
        )
    );
    parseStatements(sql, false, false, styled_text_ctrl_sql->GetCurrentPos());
}

void ExecuteSqlFrame::OnMenuExecuteSelection(wxCommandEvent& WXUNUSED(event))
{
    clearLogBeforeExecution();
    if (config().get("TreatAsSingleStatement", false))
        execute(styled_text_ctrl_sql->GetSelectedText(), ";");
    else
        parseStatements(styled_text_ctrl_sql->GetSelectedText(),
            false,
            false,
            styled_text_ctrl_sql->GetSelectionStart()
            );
}

void ExecuteSqlFrame::OnMenuGridFetchAll(wxCommandEvent& WXUNUSED(event))
{
    grid_data->fetchAll();
}

void ExecuteSqlFrame::OnMenuGridCancelFetchAll(wxCommandEvent& WXUNUSED(event))
{
    grid_data->cancelFetchAll();
}

void ExecuteSqlFrame::OnMenuUpdateGridCellIsBlob(wxUpdateUIEvent& event)
{
    DataGridTable* dgt = grid_data->getDataGridTable();
    event.Enable(dgt && grid_data->GetNumberRows() &&
        dgt->isBlobColumn(grid_data->GetGridCursorCol()));
}

void ExecuteSqlFrame::closeBlobEditor(bool saveBlobValue)
{
    if ((editBlobDlgM) && (editBlobDlgM->IsShown()))
    {
        if (saveBlobValue)
            editBlobDlgM->Close();
        else
            editBlobDlgM->closeDontSave();
    }
}

void ExecuteSqlFrame::updateBlobEditor()
{
    DataGridTable* dgt = grid_data->getDataGridTable();
    if (!dgt || !grid_data->GetNumberRows())
        return;
    unsigned row = grid_data->GetGridCursorRow();
    unsigned col = grid_data->GetGridCursorCol();

    if (!editBlobDlgM->IsShown())
    {
        editBlobDlgM->Show();
        editBlobDlgM->Update();
    }

    editBlobDlgM->setBlob(grid_data, dgt, &statementM, row, col);
    SetFocus();
    grid_data->SetFocus();
}

void ExecuteSqlFrame::OnMenuGridEditBlob(wxCommandEvent& WXUNUSED(event))
{
    if (!editBlobDlgM)
    {
        editBlobDlgM = new EditBlobDialog(this, databaseM->getCharsetConverter());
    }
    updateBlobEditor();
}

void ExecuteSqlFrame::OnMenuGridExportBlob(wxCommandEvent& WXUNUSED(event))
{
    DataGridTable* dgt = grid_data->getDataGridTable();
    if (!dgt || !grid_data->GetNumberRows())
        return;
    if (!dgt->isBlobColumn(grid_data->GetGridCursorCol()))
        throw FRError(_("Not a BLOB column"));
    wxString filename = ::wxFileSelector(_("Select a file"), "",
        "", "", "*",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
    if (filename.IsEmpty())
        return;

    ProgressDialog pd(this, _("Saving BLOB to file"));
    pd.doShow();
    dgt->exportBlobFile(filename, grid_data->GetGridCursorRow(),
        grid_data->GetGridCursorCol(), &pd);
}

void ExecuteSqlFrame::OnMenuGridImportBlob(wxCommandEvent& WXUNUSED(event))
{
    DataGridTable* dgt = grid_data->getDataGridTable();
    if (!dgt || !grid_data->GetNumberRows())
        return;
    if (!dgt->isBlobColumn(grid_data->GetGridCursorCol()))
        throw FRError(_("Not a BLOB column"));
    wxString filename = ::wxFileSelector(_("Select a file"), "",
        "", "", "*",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST, this);
   if (filename.IsEmpty())
        return;

    ProgressDialog pd(this, _("Importing BLOB from file"));
    pd.doShow();
    dgt->importBlobFile(filename, grid_data->GetGridCursorRow(),
        grid_data->GetGridCursorCol(), &pd);
}

void ExecuteSqlFrame::OnMenuGridInsertRow(wxCommandEvent& WXUNUSED(event))
{
    DataGridTable *tb = grid_data->getDataGridTable();
    if (tb && grid_data->GetNumberCols())
    {
        wxArrayString tables;
        tb->getTableNames(tables);
        wxString tab;
        if (tables.GetCount() == 0)
            throw FRError(_("No valid tables found."));
        if (tables.GetCount() == 1)
            tab = tables[0];
        else
        {   // show list of tables for user to select into which one to insert
            tab = wxGetSingleChoice(_("Select a table"),
                _("Multiple tables found"), tables, this);
            if (tab.IsEmpty())
                return;
        }

        // show dialog to enter values
        InsertDialog* id = new InsertDialog(this, tab, tb, statementM,
            databaseM);
        id->Show();
        Disable();
    }
}

// this returns an array of row numbers of fully selected rows, or the number
// of the active row
wxArrayInt getSelectedGridRows(DataGrid* grid)
{
    wxArrayInt rows;
    if (grid)
    {
        // add fully selected rows
        rows = grid->GetSelectedRows();

        // add rows in selection blocks that span all columns
        wxGridCellCoordsArray tlCells(grid->GetSelectionBlockTopLeft());
        wxGridCellCoordsArray brCells(grid->GetSelectionBlockBottomRight());
        wxASSERT(tlCells.GetCount() == brCells.GetCount());
        for (size_t i = 0; i < tlCells.GetCount(); ++i)
        {
            wxGridCellCoords tl = tlCells[i];
            wxGridCellCoords br = brCells[i];
            if (tl.GetCol() == 0 && br.GetCol() == grid->GetNumberCols() - 1)
            {
                size_t len = br.GetRow() - tl.GetRow() + 1;
                size_t first = rows.GetCount();
                rows.SetCount(first + len);
                for (size_t j = 0; j < len; ++j)
                    rows[first + j] = tl.GetRow() + j;
            }
        }
        // add the row of the active cell if nothing else is selected
        if (!rows.GetCount())
            rows.Add(grid->GetGridCursorRow());
    }
    return rows;
}

void ExecuteSqlFrame::OnMenuGridDeleteRow(wxCommandEvent& WXUNUSED(event))
{
    if (!grid_data->getDataGridTable() || !grid_data->GetNumberRows())
        return;

    // M.B. when this is enabled the grid behaves strange on GTK2 (wx2.8.6)
    // when deleting multiple items. I didn't test other platforms
    // grid_data->BeginBatch();

    wxArrayInt rows(getSelectedGridRows(grid_data));
    size_t count = rows.GetCount();
    if (count > 1)
    {
        bool agreed = wxOK == showQuestionDialog(this,
            _("Do you really want to delete multiple rows?"),
            wxString::Format(_("You have more than one row selected. Are you sure you wish to delete all %d selected rows?"), count),
            AdvancedMessageDialogButtonsOkCancel(_("Delete")));
        if (!agreed)
            return;
    }

    // Since we are not really removing the rows (only changing the color)
    // we can go from first to last. If we decide to revert to old code
    // we should go from last to first.
    for (size_t i = 0; i < rows.GetCount(); i++)
    {
        if (grid_data->DeleteRows(rows[i], 1))
            grid_data->DeselectRow(rows[i]);
        else
            break;
    }

    // grid_data->EndBatch();   // see comment for BeginBatch above
}

void ExecuteSqlFrame::OnMenuGridSetFieldToNULL(wxCommandEvent& WXUNUSED(event))
{
    DataGridTable* dgt = grid_data->getDataGridTable();
    if (!dgt)
        return;

    // get selection into array (cells)
    wxGridCellCoordsArray cells = grid_data->getSelectedCells();

    int count = cells.size();
    if (count == 0)
    {
        wxGridCellCoords curCell(grid_data->GetGridCursorRow(),
            grid_data->GetGridCursorCol());
        cells.push_back(curCell);
    }
    if (count > 1)
    {
        bool agreed = wxOK == showQuestionDialog(this,
            _("Do you really want to set multiple fields to NULL?"),
            wxString::Format(_("You have more than one data field selected. Are you sure you wish to set all %d selected fields to NULL?"), count),
            AdvancedMessageDialogButtonsOkCancel(_("Set to NULL")));
        if (!agreed)
            return;
    }

    // check, if a selected column is readonly
    // -> prepare - get distinct list of columns
    std::set<int> colsReadonly;
    for (int i = 0; i < count; i++)
        colsReadonly.insert(cells[i].GetCol());
    // -> remove all fiels that are nullable and not readonly
    for (auto col = colsReadonly.begin(); col != colsReadonly.end();)
    {
        if (!dgt->isReadonlyColumn(*col) && dgt->isNullableColumn(*col))
            colsReadonly.erase(col++);
        else
            ++col;
    }
    // generate a string for message with column names
    wxString colNames = wxEmptyString;
    for (auto col : colsReadonly)
    {
        if (!colNames.IsEmpty())
            colNames += ", ";
        colNames += dgt->GetColLabelValue(col);
    }
    // -> if colNames != "" the user has readonly columns selected
    // -> we will inform him
    if (colNames != wxEmptyString)
    {
        showQuestionDialog(this,
            _("You have read-only or not nullable fields selected!"),
            wxString::Format(_("The following fields are read-only or not nullable:\n%s\n\nThey can not be set to NULL!"), colNames.c_str()),
            AdvancedMessageDialogButtonsOk());
    }

    // set fields to NULL
    for (int i = 0; i < count; i++)
    {
        int row = cells[i].GetRow();
        int col = cells[i].GetCol();

        // do not set to null if field is not nullable or readonly
        if (colsReadonly.find(col) != colsReadonly.end())
            continue;

        dgt->setValueToNull(row, col);

        // if visible, update BLOB editor
        if (editBlobDlgM && editBlobDlgM->IsShown()
            && grid_data->GetGridCursorCol() == col
            && grid_data->GetGridCursorRow() == row)
        {
            editBlobDlgM->setBlob(grid_data, dgt, &statementM, row, col, false);
        }
    }

    // fields that change from NOT NULL to NULL need to update the text color
    grid_data->refreshAndInvalidateAttributes();
}

void ExecuteSqlFrame::OnMenuGridCopyAsInsert(wxCommandEvent& WXUNUSED(event))
{
    grid_data->copyToClipboardAsInsert();
}

void ExecuteSqlFrame::OnMenuGridCopyAsInList(wxCommandEvent& WXUNUSED(event))
{
    grid_data->copyToClipboardAsInList();
}

void ExecuteSqlFrame::OnMenuGridCopyAsUpdate(wxCommandEvent& WXUNUSED(event))
{
    grid_data->copyToClipboardAsUpdate();
}

void ExecuteSqlFrame::OnMenuGridSaveAsHtml(wxCommandEvent& WXUNUSED(event))
{
    grid_data->saveAsHTML();
}

void ExecuteSqlFrame::OnMenuGridSaveAsCsv(wxCommandEvent& WXUNUSED(event))
{
    CodeTemplateProcessor ctp(0, this);
    wxString code;
    ctp.processTemplateFile(code,
        config().getSysTemplateFileName("save_as_csv"), 0);

    wxString fileName;
    if (!ctp.getConfig().getValue("CSVExportFileName", fileName))
        return;

    int i;
    if (!ctp.getConfig().getValue("CSVFieldDelimiter", i))
        return;
    static const wxChar fieldDelimiters[] = { '\t', ',', ';' };
    if (i < 0 || i >= sizeof(fieldDelimiters) / sizeof(wxChar))
        return;
    wxChar fieldDelimiter(fieldDelimiters[i]);

    if (!ctp.getConfig().getValue("CSVTextDelimiter", i))
        return;
    static const wxChar textDelimiters[] = { '\0', '"', '\'' };
    if (i < 0 || i >= sizeof(textDelimiters) / sizeof(wxChar))
        return;
    wxChar textDelimiter(textDelimiters[i]);

    grid_data->saveAsCSV(fileName, fieldDelimiter, textDelimiter);
}

void ExecuteSqlFrame::OnMenuGridGridHeaderFont(wxCommandEvent& WXUNUSED(event))
{
    grid_data->setHeaderFont();
}

void ExecuteSqlFrame::OnMenuGridGridCellFont(wxCommandEvent& WXUNUSED(event))
{
    grid_data->setCellFont();
}

void ExecuteSqlFrame::OnMenuUpdateGridHasSelection(wxUpdateUIEvent& event)
{
    event.Enable(grid_data->IsSelection());
}

void ExecuteSqlFrame::OnMenuUpdateGridFetchAll(wxUpdateUIEvent& event)
{
    DataGridTable* table = grid_data->getDataGridTable();
    event.Enable(table && table->canFetchMoreRows()
        && !table->getFetchAllRows());
}

void ExecuteSqlFrame::OnMenuUpdateGridCancelFetchAll(wxUpdateUIEvent& event)
{
    DataGridTable* table = grid_data->getDataGridTable();
    event.Enable(table && table->canFetchMoreRows()
        && table->getFetchAllRows());
}

void ExecuteSqlFrame::OnMenuUpdateGridCanSetFieldToNULL(wxUpdateUIEvent& event)
{
    if (DataGridTable* dgt = grid_data->getDataGridTable())
    {
        std::vector<bool> selCols(grid_data->getColumnsWithSelectedCells());
        for (size_t i = 0; i < selCols.size(); i++)
        {
            if (selCols[i] && !dgt->isReadonlyColumn(i))
            {
                event.Enable(true);
                return;
            }
        }
        if (!dgt->isReadonlyColumn(grid_data->GetGridCursorRow()))
        {
            event.Enable(true);
            return;
        }
    }
    event.Enable(false);
}

bool ExecuteSqlFrame::loadSqlFile(const wxString& filename)
{
    if (filenameM.IsOk() && styled_text_ctrl_sql->GetModify())
    {
        Raise();
        int res = showQuestionDialog(this, _("Do you want to save changes to the file?"),
            wxString::Format(_("You have made changes to the file\n\n%s\n\nwhich will be lost if you load another file."),
            filenameM.GetFullPath().c_str()),
            AdvancedMessageDialogButtonsYesNoCancel(_("&Save"), _("Do&n't Save")));
        if (res != wxYES && res != wxNO)
            return false;
        if (res == wxYES && !styled_text_ctrl_sql->SaveFile(filenameM.GetFullPath()))
            return false;
    }

    if (!styled_text_ctrl_sql->LoadFile(filename))
        return false;
    filenameM = filename;
    filenameModificationTimeM = wxFileName(filenameM).GetModificationTime();
    updateFrameTitleM = true;
    return true;
}

//! enable/disable and show/hide controls depending of transaction status
void ExecuteSqlFrame::inTransaction(bool started)
{
    inTransactionM = started;
    splitScreen();
    if (started)
        statusbar_1->SetStatusText(_("Transaction started"), 3);
    else
    {
        grid_data->ClearGrid();
        statusbar_1->SetStatusText(wxEmptyString, 1);
    }
}

bool ExecuteSqlFrame::setSql(wxString sql)
{
    if (filenameM.IsOk() && styled_text_ctrl_sql->GetModify())
    {
        Raise();
        int res = showQuestionDialog(this, _("Do you want to save changes to the file?"),
            wxString::Format(_("You have made changes to the file\n\n%s\n\nwhich will be lost if you set a different statement."),
            filenameM.GetFullPath().c_str()),
            AdvancedMessageDialogButtonsYesNoCancel(_("&Save"), _("Do&n't Save")));
        if (res != wxYES && res != wxNO)
            return false;
        if (res == wxYES && !styled_text_ctrl_sql->SaveFile(filenameM.GetFullPath()))
            return false;
    }

    styled_text_ctrl_sql->SetText(sql);
    styled_text_ctrl_sql->EmptyUndoBuffer();

    filenameM = wxEmptyString;
    filenameModificationTimeM = wxDateTime();
    updateFrameTitleM = true;
    return true;
}

void ExecuteSqlFrame::clearLogBeforeExecution()
{
    if (config().get("SQLEditorExecuteClears", false))
        styled_text_ctrl_stats->ClearAll();
}

void ExecuteSqlFrame::prepareAndExecute(bool prepareOnly)
{
    bool hasSelection = styled_text_ctrl_sql->GetSelectionStart()
        != styled_text_ctrl_sql->GetSelectionEnd();
    bool ok;
    if (hasSelection && config().get("OnlyExecuteSelected", false))
    {
        if (config().get("TreatAsSingleStatement", false))
        {
            ok = execute(styled_text_ctrl_sql->GetSelectedText(), ";",
                prepareOnly);
        }
        else
        {
            ok = parseStatements(styled_text_ctrl_sql->GetSelectedText(),
                false, prepareOnly, styled_text_ctrl_sql->GetSelectionStart());
        }
    }
    else
    {
        ok = parseStatements(styled_text_ctrl_sql->GetText(), false,
            prepareOnly);
    }

    if (ok || config().get("historyStoreUnsuccessful", true))
    {
        // add to history
        StatementHistory& sh = StatementHistory::get(databaseM);
        sh.add(styled_text_ctrl_sql->GetText());
        historyPositionM = sh.size();
    }

    if (!inTransactionM)
        setViewMode(false, vmEditor);
}

//! adapted so we don't have to change all the other code that utilizes SQL editor
void ExecuteSqlFrame::executeAllStatements(bool closeWhenDone)
{
    clearLogBeforeExecution();
    bool ok = parseStatements(styled_text_ctrl_sql->GetText(), closeWhenDone);
    if (config().get("historyStoreGenerated", true) &&
        (ok || config().get("historyStoreUnsuccessful", true)))
    {
        // add buffer to history
        StatementHistory& sh = StatementHistory::get(databaseM);
        sh.add(styled_text_ctrl_sql->GetText());
        historyPositionM = sh.size();
    }

    if (closeWhenDone && autoCommitM && !inTransactionM)
        Close();
}

//! Parses all sql statements in STC
//! when autoexecute is TRUE, program just waits user to click Commit/Rollback and closes window
//! when autocommit DDL is also set then frame is closed at once if commit was successful
bool ExecuteSqlFrame::parseStatements(const wxString& statements,
    bool closeWhenDone, bool prepareOnly, int selectionOffset)
{
    wxBusyCursor cr;
    MultiStatement ms(statements);
    while (true)
    {
        SingleStatement ss = ms.getNextStatement();
        if (!ss.isValid())
            break;

        wxString newTerminator, autoDDLSetting;
        if (ss.isCommitStatement())
        {
            if (!commitTransaction())
                return false;
        }
        else if (ss.isRollbackStatement())
            rollbackTransaction();
        else if (ss.isSetTermStatement(newTerminator))
        {
            if (newTerminator.empty())
            {
                ::wxMessageBox(_("SET TERM command found without terminator.\nStopping further execution."),
                    _("Warning"), wxOK | wxICON_WARNING);
                return false;
            }
        }
        else if (ss.isSetAutoDDLStatement(autoDDLSetting))
        {
            if (autoDDLSetting.CmpNoCase("ON") == 0)
                autoCommitM = true;
            else if (autoDDLSetting.CmpNoCase("OFF") == 0)
                autoCommitM = false;
            else if (autoDDLSetting.empty())
                autoCommitM = !autoCommitM;
            else
            {
                ::wxMessageBox(_("SET AUTODDL command found with invalid parameter (has to be \"ON\" or \"OFF\").\nStopping further execution."),
                    _("Warning"), wxOK | wxICON_WARNING);
                return false;
            }
        }
        else if (!ss.isEmptyStatement()
            && !execute(ss.getSql(), ms.getTerminator(), prepareOnly))
        {
            int stmtStart = selectionOffset + ms.getStart();
            // STC uses UTF-8 internally in Unicode build
            // account for possible differences in string length
            // if system charset != UTF-8
            std::string stmt(wx2std(ss.getSql(), &wxConvUTF8));
            int stmtEnd = stmtStart + stmt.size();
            styled_text_ctrl_sql->markText(stmtStart, stmtEnd);
            styled_text_ctrl_sql->SetFocus();
            return false;
        }
    }

    if (closeWhenDone)
    {
        closeWhenTransactionDoneM = true;
        // TODO: HOWTO focus toolbar button? button_commit->SetFocus();
    }

    ScrollAtEnd sae(styled_text_ctrl_stats);
    log(_("Script execution finished."));
    return true;
}

void ExecuteSqlFrame::OnMenuUpdateWhenExecutePossible(wxUpdateUIEvent& event)
{
    event.Enable(!closeWhenTransactionDoneM);
}

wxString IBPPtype2string(Database *db, IBPP::SDT t, int subtype, int size,
    int scale)
{
    if (scale > 0)
        return wxString::Format("NUMERIC(%d,%d)", size==4 ? 9:18, scale);
    if (t == IBPP::sdString)
    {
        int bpc = db->getCharsetById(subtype).getBytesPerChar();
        return wxString::Format("STRING(%d)", bpc ? size/bpc : size);
    }
    switch (t)
    {
        case IBPP::sdArray:     return "ARRAY";
        case IBPP::sdBlob:      return wxString::Format(
                                    "BLOB SUB_TYPE %d", subtype);
        case IBPP::sdDate:      return "DATE";
        case IBPP::sdTime:      return "TIME";
        case IBPP::sdTimestamp: return "TIMESTAMP";
        case IBPP::sdSmallint:  return "SMALLINT";
        case IBPP::sdInteger:   return "INTEGER";
        case IBPP::sdLargeint:  return "BIGINT";
        case IBPP::sdFloat:     return "FLOAT";
        case IBPP::sdDouble:    return "DOUBLE PRECISION";
        case IBPP::sdTimeTz:    return "TIME WITH TIMEZONE";
        case IBPP::sdTimestampTz: return "TIMESTAMP WITH TIMEZONE";
        case IBPP::sdInt128:    return "INT128";
        case IBPP::sdDec16:     return "DECFLOAT(16)";
        case IBPP::sdDec34:     return "DECFLOAT(34)";
        default:                return "UNKNOWN";
    }
}

void ExecuteSqlFrame::compareCounts(IBPP::DatabaseCounts& one,
    IBPP::DatabaseCounts& two)
{
    for (IBPP::DatabaseCounts::iterator it = two.begin(); it != two.end();
        ++it)
    {
        wxString str_log;
        IBPP::DatabaseCounts::iterator i2 = one.find((*it).first);
        IBPP::CountInfo c;
        IBPP::CountInfo& r1 = (*it).second;
        IBPP::CountInfo& r2 = c;
        if (i2 != one.end())
            r2 = (*i2).second;
        if (r1.inserts > r2.inserts)
            str_log += wxString::Format(_("%d inserts. "), r1.inserts - r2.inserts);
        if (r1.updates > r2.updates)
            str_log += wxString::Format(_("%d updates. "), r1.updates - r2.updates);
        if (r1.deletes > r2.deletes)
            str_log += wxString::Format(_("%d deletes. "), r1.deletes - r2.deletes);
        if (!str_log.IsEmpty())
        {
            wxString relName;
            try
            {
                IBPP::Statement st = IBPP::StatementFactory(
                    databaseM->getIBPPDatabase(), transactionM);
                st->Prepare(
                    "select rdb$relation_name "
                    "from rdb$relations where rdb$relation_id = ?");
                st->Set(1, (*it).first);
                st->Execute();
                if (st->Fetch())
                {
                    std::string s;
                    st->Get(1, s);
                    relName = std2wxIdentifier(s, databaseM->getCharsetConverter());
                }
            }
            catch (...)
            {
            }
            if (relName.IsEmpty())
                relName.Format(_("Relation #%d"), (*it).first);
            log(relName + ": " + str_log, ttSql);
        }
    }
}

wxString millisToTimeString(long millis)
{
    if (millis >= 60 * 1000)
    {
        // round to nearest second by adding 500 millis before truncating
        millis += 500;
        int hh = millis / (60 * 60 * 1000);
        millis -= 60 * 60 * 1000 * hh;
        int mm = millis / (60 * 1000);
        millis -= 60 * 1000 * mm;
        int ss = millis / 1000;
        return wxString::Format("%d:%.2d:%.2d (hh:mm:ss)", hh, mm, ss);
    }
    else
        return wxString::Format("%.3fs", 0.001 * millis);
}

bool ExecuteSqlFrame::execute(wxString sql, const wxString& terminator,
    bool prepareOnly)
{
    ScrollAtEnd sae(styled_text_ctrl_stats);

    // check if sql only contains comments
    SqlTokenizer tk(sql);
    bool hasStatements = false;
    do
    {
        SqlTokenType stt = tk.getCurrentToken();
        if (stt != tkWHITESPACE && stt != tkCOMMENT && stt != tkEOF)
        {
            hasStatements = true;
            break;
        }
    }
    while (tk.nextToken());
    if (!hasStatements)
    {
        log(_("Parsed statement: " + sql), ttSql);
        log(_("Empty statement detected, bailing out..."));
        return true;
    }

    if (styled_text_ctrl_sql->AutoCompActive())
        styled_text_ctrl_sql->AutoCompCancel();    // remove the list if needed
    notebook_1->SetSelection(0);
    wxStopWatch swTotal;
    bool retval = true;
    long waitForParameterInputTime = 0;
    try
    {
        if (transactionM == 0 || !transactionM->Started())
        {
            log(_("Starting transaction..."));

            // fix the IBPP::LogicException "No Database is attached."
            // which happens after a database reconnect
            // (this action detaches the database from all its transactions)
            if (transactionM != 0 && !transactionM->Started())
            {
                try
                {
                    transactionM->Start();
                }
                catch (IBPP::LogicException&)
                {
                    transactionM = 0;
                }
            }

            if (transactionM == 0)
            {
                transactionM = IBPP::TransactionFactory(
                    databaseM->getIBPPDatabase(), transactionAccessModeM,
                    transactionIsolationLevelM, transactionLockResolutionM);
            }
            transactionM->Start();
            inTransaction(true);

            grid_data->EnableEditing(transactionAccessModeM == IBPP::amWrite);
        }

        int fetch1 = 0, mark1 = 0, read1 = 0, write1 = 0, ins1 = 0, upd1 = 0,
            del1 = 0, ridx1 = 0, rseq1 = 0, mem1 = 0;
        int fetch2, mark2, read2, write2, ins2, upd2, del2, ridx2, rseq2, mem2;
        IBPP::DatabaseCounts counts1, counts2;
        bool doShowStats = config().get("SQLEditorShowStats", true);
        if (!prepareOnly && doShowStats)
        {
            databaseM->getIBPPDatabase()->
                Statistics(&fetch1, &mark1, &read1, &write1, &mem1);
            databaseM->getIBPPDatabase()->
                Counts(&ins1, &upd1, &del1, &ridx1, &rseq1);
            databaseM->getIBPPDatabase()->DetailedCounts(counts1);
        }
        grid_data->ClearGrid(); // statement object will be invalidated, so clear the grid
        statementM = IBPP::StatementFactory(databaseM->getIBPPDatabase(), transactionM);
        log(_("Preparing statement: " + sql), ttSql);
        sae.scroll();
        {
            wxStopWatch sw;
            statementM->Prepare(wx2std(sql, databaseM->getCharsetConverter()));
            log(wxString::Format(_("Statement prepared (elapsed time: %s)."),
                millisToTimeString(sw.Time()).c_str()));
        }

        // we don't check IBPP::Select since Firebird 2.0 has a new feature
        // INSERT ... RETURNING which isn't detected as stSelect by IBPP
        bool hasColumns = false;
        try
        {
            int cols = statementM->Columns();
            hasColumns = cols > 0;
            if (doShowStats)
            {
                for (int i = 1; i <= cols; i++)
                {
                    wxString tablename(std2wxIdentifier(statementM->ColumnTable(i),
                        databaseM->getCharsetConverter()));
                    wxString colname(std2wxIdentifier(statementM->ColumnName(i),
                        databaseM->getCharsetConverter()));
                    wxString aliasname(std2wxIdentifier(statementM->ColumnAlias(i),
                        databaseM->getCharsetConverter()));
                    log(wxString::Format(_("Field #%02d: %s.%s Alias:%s Type:%s"),
                        i, tablename.c_str(), colname.c_str(), aliasname.c_str(),
                        IBPPtype2string(
                            databaseM,
                            statementM->ColumnType(i),
                            statementM->ColumnSubtype(i),
                            statementM->ColumnSize(i),
                            statementM->ColumnScale(i)).c_str()
                        ), ttSql);
                }
            }
        }
        catch(IBPP::Exception&)    // reading column info might fail,
        {                          // but we still want to show the plan
        }                          // so we have separate exception handlers

        // for some statements (DDL) it is never available
        // for INSERTs, it is available sometimes (insert into ... select ... )
        // but if it not, IBPP throws an exception
        try
        {
            std::string plan;
            statementM->Plan(plan);
            log(wxString(plan.c_str(), *databaseM->getCharsetConverter()));
        }
        catch(IBPP::Exception&)
        {
            log(_("Plan not available."));
        }

        if (prepareOnly)
            return true;

        log(wxString::Format(_("Parameters: %zu"), statementM->ParametersByName().size() ));
        //Define parameters here:
        if (statementM->ParametersByName().size() >0)
        {
            //Insert parameters here:
            InsertParametersDialog* id = new InsertParametersDialog(this, statementM,
                databaseM, parameterSaveList, parameterSaveListOptionNull);
            id->ShowModal();
            waitForParameterInputTime = id->swWaitForParameterInputTime.Time();
        }

        log(wxEmptyString);
        log(wxEmptyString);
        log(_("Executing statement..."));
        sae.scroll();
        {
            wxStopWatch sw;
            statementM->Execute();
            log(wxString::Format(_("Statement executed (elapsed time: %s)."),
                millisToTimeString(sw.Time()).c_str()));
        }
        IBPP::STT type = statementM->Type();
        if (hasColumns)            // for select statements: show data
        {
            grid_data->fetchData(transactionAccessModeM == IBPP::amRead);
            setViewMode(vmGrid);
        }

        if (doShowStats)
        {
            databaseM->getIBPPDatabase()->Statistics(
                &fetch2, &mark2, &read2, &write2, &mem2);
            databaseM->getIBPPDatabase()->
                Counts(&ins2, &upd2, &del2, &ridx2, &rseq2);
            log(wxString::Format(
                _("%d fetches, %d marks, %d reads, %d writes."),
                fetch2-fetch1, mark2-mark1, read2-read1, write2-write1));
            log(wxString::Format(
                _("%d inserts, %d updates, %d deletes, %d index, %d seq."),
                ins2-ins1, upd2-upd1, del2-del1, ridx2-ridx1, rseq2-rseq1));
            log(wxString::Format(_("Delta memory: %d bytes."), mem2-mem1));
            databaseM->getIBPPDatabase()->DetailedCounts(counts2);
            compareCounts(counts1, counts2);
        }

        if (type != IBPP::stSelect) // for other statements: show rows affected
        {   // left trim
            wxString::size_type p = sql.find_first_not_of(" \n\t\r");
            if (p != wxString::npos && p > 0)
                sql.erase(0, p);
            if (type == IBPP::stInsert || type == IBPP::stDelete
                || type == IBPP::stExecProcedure || type == IBPP::stUpdate)
            {
                // INSERT INTO..RETURNING and EXECUTE PROCEDURE may throw
                // when they return a single record
                try
                {
                    wxString addon;
                    if (statementM->AffectedRows() % 10 != 1)
                        addon = "s";
                    wxString s = wxString::Format(_("%d row%s affected directly."),
                        statementM->AffectedRows(), addon.c_str());
                    log("" + s);
                    statusbar_1->SetStatusText(s, 1);
                }
                catch (IBPP::Exception&)
                {
                }
            }
            SqlStatement stm(sql, databaseM, terminator);
            if (stm.isDDL())
                type = IBPP::stDDL;
            executedStatementsM.push_back(stm);
            setViewMode(vmEditor);
            if (type == IBPP::stDDL && autoCommitM)
            {
                if (!commitTransaction())
                    retval = false;
            }
        }
    }
    catch(IBPP::Exception& e)
    {
        splitScreen();
        wxString msg(e.what(),
            *databaseM->getCharsetConverter());
        log(_("Error: ") + msg + "\n", ttError);
        retval = false;
    }
    catch (std::exception& e)
    {
        splitScreen();
        log(_("Error: ") + e.what() + "\n", ttError);
        retval = false;
    }
    catch (...)
    {
        splitScreen();
        log(_("SYSTEM ERROR!"), ttError);
        retval = false;
    }

    log(wxString::Format(_("Total execution time: %s"),
        millisToTimeString(swTotal.Time() - waitForParameterInputTime).c_str()));
    return retval;
}

void ExecuteSqlFrame::splitScreen()
{
    if (!splitter_window_1->IsSplit()) // split screen if needed
    {
        splitter_window_1->SplitHorizontally(styled_text_ctrl_sql, notebook_1);
        ::wxYield();
    }
}

void ExecuteSqlFrame::OnMenuTransactionIsolationLevel(wxCommandEvent& event)
{
    if (event.GetId() == Cmds::Query_TransactionConcurrency)
        transactionIsolationLevelM = IBPP::ilConcurrency;
    else if (event.GetId() == Cmds::Query_TransactionConsistency)
        transactionIsolationLevelM = IBPP::ilConsistency;
    else if (event.GetId() == Cmds::Query_TransactionReadCommitted)
        transactionIsolationLevelM = IBPP::ilReadCommitted;
    else if (event.GetId() == Cmds::Query_TransactionReadDirty)
        transactionIsolationLevelM = IBPP::ilReadDirty;

    wxCHECK_RET(transactionM == 0 || !transactionM->Started(),
        "Can't change transaction isolation level while started");
    transactionM = 0;
}

void ExecuteSqlFrame::OnMenuUpdateTransactionIsolationLevel(
    wxUpdateUIEvent& event)
{
    event.Enable(transactionM == 0 || !transactionM->Started());
    if (event.GetId() == Cmds::Query_TransactionConcurrency)
        event.Check(transactionIsolationLevelM == IBPP::ilConcurrency);
    else if (event.GetId() == Cmds::Query_TransactionConsistency)
        event.Check(transactionIsolationLevelM == IBPP::ilConsistency);
    else if (event.GetId() == Cmds::Query_TransactionReadCommitted)
        event.Check(transactionIsolationLevelM == IBPP::ilReadCommitted);
    else if (event.GetId() == Cmds::Query_TransactionReadDirty)
        event.Check(transactionIsolationLevelM == IBPP::ilReadDirty);
}

void ExecuteSqlFrame::OnMenuTransactionLockResolution(wxCommandEvent& event)
{
    transactionLockResolutionM =
        event.IsChecked() ? IBPP::lrWait : IBPP::lrNoWait;

    wxCHECK_RET(transactionM == 0 || !transactionM->Started(),
        "Can't change transaction lock resolution while started");
    transactionM = 0;
}

void ExecuteSqlFrame::OnMenuUpdateTransactionLockResolution(
    wxUpdateUIEvent& event)
{
    event.Enable(transactionM == 0 || !transactionM->Started());
    event.Check(transactionLockResolutionM == IBPP::lrWait);
}

void ExecuteSqlFrame::OnMenuTransactionReadOnly(wxCommandEvent& event)
{
    transactionAccessModeM = event.IsChecked() ? IBPP::amRead : IBPP::amWrite;

    wxCHECK_RET(transactionM == 0 || !transactionM->Started(),
        "Can't change transaction access mode while started");
    transactionM = 0;
}

void ExecuteSqlFrame::OnMenuUpdateTransactionReadOnly(wxUpdateUIEvent& event)
{
    event.Enable(transactionM == 0 || !transactionM->Started());
    event.Check(transactionAccessModeM == IBPP::amRead);
}

void ExecuteSqlFrame::OnMenuCommit(wxCommandEvent& WXUNUSED(event))
{
    // we need this because sometimes, somehow, Close() which is called in
    // commitTransaction() can destroy the object (at least, with wxGTK 2.8.8)
    // before closeWhenTransactionDoneM is checked and if the dummy memory
    // location returns false, we have a crash
    bool doClose = closeWhenTransactionDoneM;
    if (commitTransaction() && !doClose)
        setViewMode(false, vmEditor);
}

bool ExecuteSqlFrame::commitTransaction()
{
    if (transactionM == 0 || !transactionM->Started())    // check
    {
        inTransaction(false);
        return true;    // nothing to commit, but it wasn't error
    }

    closeBlobEditor(true);

    wxBusyCursor cr;
    ScrollAtEnd sae(styled_text_ctrl_stats);

    try
    {
        log(_("Committing transaction..."));
        sae.scroll();
        {
            wxStopWatch sw;
            statementM->Close();
            transactionM->Commit();
            log(wxString::Format(_("Transaction committed (elapsed time: %s)."),
                millisToTimeString(sw.Time()).c_str()));
        }
        statusbar_1->SetStatusText(_("Transaction committed"), 3);
        inTransaction(false);

        SubjectLocker locker(databaseM);
        // log statements, done before parsing in case parsing crashes FR
        if (menuBarM->IsChecked(Cmds::History_EnableLogging))
        {
            for (std::vector<SqlStatement>::const_iterator it =
                executedStatementsM.begin(); it != executedStatementsM.end();
                ++it)
            {
                if (!Logger::logStatement(*it, databaseM))
                    break;
            }
        }

        // parse all successfully executed statements
        for (std::vector<SqlStatement>::const_iterator it =
            executedStatementsM.begin(); it != executedStatementsM.end(); ++it)
        {
            databaseM->parseCommitedSql(*it);
        }

        // possible future version (see database.cpp file for details: ONLY IF FIRST solution is used from database.cpp)
        //for (std::vector<wxString>::const_iterator it = executedStatementsM.begin(); it != executedStatementsM.end(); ++it)
        //    databaseM->addCommitedSql(*it);
        //databaseM->parseAll();

        executedStatementsM.clear();

        // workaround for STC bug with 100% CPU load during Idle(),
        // it was supposed to be fixed in wxWidgets versions 2.5.4 and later,
        // but it looks like it is not (at least for gtk1)
        styled_text_ctrl_stats->SetWrapMode(wxSTC_WRAP_WORD);
        if (closeWhenTransactionDoneM)
        {
            sae.cancel();
            Close();
            return true;
        }
    }
    catch (IBPP::Exception &e)
    {
        splitScreen();
        log(wxString(e.what(), *databaseM->getCharsetConverter()),
            ttError);
        return false;
    }
    catch (std::exception &se)
    {
        splitScreen();
        log(wxString(_("ERROR!\n")) + se.what(), ttError);
        return false;
    }

    notebook_1->SetSelection(0);

    // apparently is has to be at the end to have any effect
    setViewMode(vmEditor);
    return true;
}

void ExecuteSqlFrame::OnMenuRollback(wxCommandEvent& WXUNUSED(event))
{
    wxBusyCursor cr;
    // see comments for OnMenuCommit to learn why this temp. variable is needed
    bool closeIt = closeWhenTransactionDoneM;
    if (rollbackTransaction() && !closeIt)
        setViewMode(false, vmEditor);
}

bool ExecuteSqlFrame::rollbackTransaction()
{
    if (transactionM == 0 || !transactionM->Started())    // check
    {
        executedStatementsM.clear();
        inTransaction(false);
        return true;
    }

    closeBlobEditor(false);

    ScrollAtEnd sae(styled_text_ctrl_stats);

    try
    {
        log(_("Rolling back the transaction..."));
        sae.scroll();
        {
            wxStopWatch sw;
            statementM->Close();
            transactionM->Rollback();
            log(wxString::Format(_("Transaction rolled back (elapsed time: %s)."),
                millisToTimeString(sw.Time()).c_str()));
        }
        statusbar_1->SetStatusText(_("Transaction rolled back"), 3);
        inTransaction(false);
        executedStatementsM.clear();

        if (closeWhenTransactionDoneM)
        {
            sae.cancel();
            Close();
            return true;
        }
    }
    catch (IBPP::Exception &e)
    {
        splitScreen();
        log(wxString(e.what(), *databaseM->getCharsetConverter()),
            ttError);
        return false;
    }
    catch (...)
    {
        splitScreen();
        log(_("ERROR!\nA non-IBPP C++ runtime exception occurred !"), ttError);
        return false;
    }

    notebook_1->SetSelection(0);
    setViewMode(vmEditor);
    return true;
}

void ExecuteSqlFrame::OnMenuUpdateGridInsertRow(wxUpdateUIEvent& event)
{
    DataGridTable* tb = grid_data->getDataGridTable();
    event.Enable(inTransactionM && tb && tb->canInsertRows());
}

void ExecuteSqlFrame::OnMenuUpdateGridHasData(wxUpdateUIEvent& event)
{
    event.Enable(grid_data->getDataGridTable()
        && grid_data->GetNumberRows());
}

void ExecuteSqlFrame::OnMenuUpdateGridDeleteRow(wxUpdateUIEvent& event)
{
    DataGridTable *tb = grid_data->getDataGridTable();
    if (!tb || !grid_data->GetNumberRows())
    {
        event.Enable(false);
        return;
    }

    bool colsSelected = grid_data->GetSelectedCols().GetCount() > 0;
    bool deletableRows = false;

    if (!colsSelected)
    {
        wxArrayInt selRows(getSelectedGridRows(grid_data));
        for (size_t i = 0; !deletableRows && i < selRows.GetCount(); ++i)
        {
            if (tb->canRemoveRow(selRows[i]))
                deletableRows = true;
        }
    }

    event.Enable(!colsSelected && deletableRows);
}

void ExecuteSqlFrame::OnGridCellChange(wxGridEvent& event)
{
    event.Skip();
    // Start timer-event (for updating the blob-value) only if
    // - the blob-dialog is created and visible AND
    // - a different col/row is selected
    if ((editBlobDlgM) && (editBlobDlgM->IsShown()) &&
        ((event.GetCol() != grid_data->GetGridCursorCol()) ||
         (event.GetRow() != grid_data->GetGridCursorRow())))
        timerBlobEditorM.Start(500, true);
}

void ExecuteSqlFrame::OnGridInvalidateAttributeCache(wxCommandEvent& event)
{
    event.Skip();
    grid_data->refreshAndInvalidateAttributes();
}

void ExecuteSqlFrame::OnGridRowCountChanged(wxCommandEvent& event)
{
    wxString s;
    long rowsFetched = event.GetExtraLong();
    s.Printf(_("%ld row(s) fetched"), rowsFetched);
    statusbar_1->SetStatusText(s, 1);

    // TODO: we could make some bool flag, so that this happens only once per execute()
    //       to fix the problem when user does the select, unsplits the window
    //       and then browses the grid, which fetches more records and unsplits again
    if (!splitter_window_1->IsSplit())    // already ok
        return;
    bool selectMaximizeGrid = false;
    config().getValue("SelectMaximizeGrid", selectMaximizeGrid);
    if (selectMaximizeGrid)
    {
        int rowsNeeded = 10;    // default
        config().getValue("MaximizeGridRowsNeeded", rowsNeeded);
        if (rowsFetched >= rowsNeeded)
        {
            //splitScreen();    // not needed atm, might be later (see TODO above)
            setViewMode(false, vmGrid);
        }
    }
}

void ExecuteSqlFrame::OnGridStatementExecuted(wxCommandEvent& event)
{
    ScrollAtEnd sae(styled_text_ctrl_stats);
    log(event.GetString(), ttSql);
    if (menuBarM->IsChecked(Cmds::DataGrid_Log_changes))
    {
        SqlStatement stm(event.GetString(), databaseM);
        executedStatementsM.push_back(stm);
    }
}

void ExecuteSqlFrame::OnGridSum(wxCommandEvent& event)
{
    statusbar_1->SetStatusText(event.GetString(), 3);
}

void ExecuteSqlFrame::OnGridLabelLeftDClick(wxGridEvent& event)
{
    DataGridTable* table = grid_data->getDataGridTable();
    if (!table)
        return;

    int column = 1 + event.GetCol();
    if (column < 1 || column > table->GetNumberCols())
        return;
    SelectStatement sstm(wxString(statementM->Sql().c_str(),
        *databaseM->getCharsetConverter()));

    // rebuild SQL statement with different ORDER BY clause
    sstm.orderBy(column);

    execute(sstm.getStatement(), wxEmptyString);
}

void ExecuteSqlFrame::OnSplitterUnsplit(wxSplitterEvent& WXUNUSED(event))
{
    if (splitter_window_1->GetWindow1() == styled_text_ctrl_sql)
        setViewMode(vmEditor);
    else if (splitter_window_1->GetWindow1() == notebook_1)
    {
        if (notebook_1->GetSelection() == 0)
            setViewMode(vmLogCtrl);
        else
            setViewMode(vmGrid);
    }
}

void ExecuteSqlFrame::update()
{
    if (databaseM && !databaseM->isConnected())
        Close();
}

//! closes window if database is removed (unregistered)
void ExecuteSqlFrame::subjectRemoved(Subject* subject)
{
    if (subject == databaseM)
        Close();
}

static int CaseUnsensitiveCompare(const wxString& one, const wxString& two)
{
    // this would be the right solution, but it doesn't work well as it
    // sorts the underscore character differently
    // return one.CmpNoCase(two);

    // we have to check for underscore first
    int min = one.Length() > two.Length() ? two.Length() : one.Length();
    for (int i = 0; i < min; ++i)
    {
        if (one[i] == '_' && two[i] != '_')
            return 1;
        if (one[i] != '_' && two[i] == '_')
            return -1;
        if (one[i] != two[i])
            return one.CmpNoCase(two);
    }
    return one.CmpNoCase(two);
}
//! Creates a list for autocomplete feature

//! The list consists of:
//! - sql keywords
//! - names of database objects (tables, views, etc.)
//
void ExecuteSqlFrame::setKeywords()
{
    // TODO:
    // we can also make ExecuteSqlFrame observer of YTables/YViews/... objects
    // so it can reload this list if something changes

    wxArrayString as(SqlTokenizer::getKeywords(SqlTokenizer::kwDefaultCase));

    // get list od database objects' names
    std::vector<Identifier> v;
    databaseM->getIdentifiers(v);
    for (std::vector<Identifier>::const_iterator it = v.begin(); it != v.end(); ++it)
        as.Add((*it).getQuoted());
    // The list has to be sorted for autocomplete to work properly
    as.Sort(CaseUnsensitiveCompare);

    keywordsM.clear();                          // create final wxString from array
    keywordsM.Alloc(20480);     // preallocate 20kB
    for (size_t i = 0; i < as.GetCount(); ++i)  // separate words with spaces
        keywordsM += as.Item(i) + " ";
}

//! logs all activity to text control
// this is made a separate function, so we can change the control to any other
// or we can also log to some .txt file, etc.
void ExecuteSqlFrame::log(wxString s, TextType type)
{
    int startpos = styled_text_ctrl_stats->GetLength();
    styled_text_ctrl_stats->SetCurrentPos(startpos);
    styled_text_ctrl_stats->AddText(s + "\n");
    int endpos = styled_text_ctrl_stats->GetLength();

    int style = 0;
    if (type == ttError)
        style = 1;
    if (type == ttSql)
        style = 2;

    styled_text_ctrl_stats->StartStyling(startpos, 0);
    styled_text_ctrl_stats->SetStyling(endpos-startpos-1, style);
}

const wxString ExecuteSqlFrame::getName() const
{
    return "ExecuteSqlFrame";
}

void ExecuteSqlFrame::doReadConfigSettings(const wxString& prefix)
{
    BaseFrame::doReadConfigSettings(prefix);
    int zoom;
    if (config().getValue(prefix + Config::pathSeparator + "zoom", zoom))
        styled_text_ctrl_sql->SetZoom(zoom);
}

void ExecuteSqlFrame::doWriteConfigSettings(const wxString& prefix) const
{
    BaseFrame::doWriteConfigSettings(prefix);
    config().setValue(prefix + Config::pathSeparator + "zoom",
        styled_text_ctrl_sql->GetZoom());
}

const wxRect ExecuteSqlFrame::getDefaultRect() const
{
    return wxRect(-1, -1, 528, 486);
}

bool ExecuteSqlFrame::Show(bool show)
{
    bool retval = BaseFrame::Show(show);
    // bug reported 2008-08-19 by Valdir Marcos: status bar position wrong
    // when ExecuteSqlFrame is created in maximized state
    if (IsMaximized())
        SendSizeEvent();
    return retval;
}

void ExecuteSqlFrame::setViewMode(ViewMode mode)
{
    setViewMode(splitter_window_1->IsSplit(), mode);
}

void ExecuteSqlFrame::setViewMode(bool splitView, ViewMode mode)
{
    wxCHECK_RET(mode == vmEditor || mode == vmLogCtrl || mode == vmGrid,
        "Try to set invalid view mode");
    viewModeM = mode;

    // select notebook pane first (could still be invisible)
    if (mode == vmLogCtrl)
        notebook_1->SetSelection(0);
    else if (mode == vmGrid)
        notebook_1->SetSelection(1);

    // split if necessary
    if (splitView && !splitter_window_1->IsSplit())
    {
        styled_text_ctrl_sql->Show();
        notebook_1->Show();
        splitter_window_1->SplitHorizontally(styled_text_ctrl_sql,
            notebook_1);
    }

    // unsplit or switch panes if necessary
    if (!splitView)
    {
        if (mode == vmEditor)
        {
            if (splitter_window_1->IsSplit())
                splitter_window_1->Unsplit(notebook_1);
            else if (splitter_window_1->GetWindow1() == notebook_1)
            {
                splitter_window_1->ReplaceWindow(notebook_1,
                    styled_text_ctrl_sql);
            }
            styled_text_ctrl_sql->Show();
            notebook_1->Hide();
        }
        else
        {
            if (splitter_window_1->IsSplit())
                splitter_window_1->Unsplit(styled_text_ctrl_sql);
            else if (splitter_window_1->GetWindow1() == styled_text_ctrl_sql)
            {
                splitter_window_1->ReplaceWindow(styled_text_ctrl_sql,
                    notebook_1);
            }
            notebook_1->Show();
            styled_text_ctrl_sql->Hide();
        }
    }

    if (mode == vmEditor)
        styled_text_ctrl_sql->SetFocus();
    else if (mode == vmLogCtrl)
        styled_text_ctrl_stats->SetFocus();
    else if (mode == vmGrid)
        grid_data->SetFocus();
}

void ExecuteSqlFrame::updateViewMode()
{
    doUpdateFocusedControlM = false;

    wxWindow* focused = FindFocus();
    if (focused == styled_text_ctrl_sql)
        viewModeM = vmEditor;
    else if (focused == styled_text_ctrl_stats)
        viewModeM = vmLogCtrl;
    else if (focused == grid_data || grid_data->IsCellEditControlEnabled()
        || focused == grid_data->GetGridWindow()
        || focused == grid_data->GetGridColLabelWindow()
        || focused == grid_data->GetGridRowLabelWindow()
        || focused == grid_data->GetGridCornerLabelWindow())
    {
        viewModeM = vmGrid;
    }
}

void ExecuteSqlFrame::updateFrameTitle()
{
    if (filenameM.IsOk())
    {
        wxString title(filenameM.GetFullName());
        if (styled_text_ctrl_sql->GetModify())
            title += "*";
        SetTitle(title);
        return;
    }

    const wxString text(styled_text_ctrl_sql->GetText());
    if (text.empty())
    {
        SetTitle(_("Execute SQL Statements"));
        return;
    }

    size_t p = text.find("@FR-TITLE@");
    if (p != wxString::npos)
    {
        size_t q = text.find("*/", p);
        if (q == wxString::npos)
            q = text.find_first_of("\n\r", p);
        if (q != wxString::npos)
            SetTitle(text.substr(p+11, q - p - 11));
        else
            SetTitle(text.substr(p+11));
        return;
    }

    SqlTokenizer tk(text);
    const SqlTokenType lookfor[] = {
        kwALTER, kwCREATE, kwDECLARE, kwDROP, kwEXECUTE, kwINSERT,
        kwRECREATE, kwREVOKE, kwGRANT, kwSELECT, kwUPDATE, kwDELETE,
        tkIDENTIFIER
    };
    const wxString namesShort[] = {
        "alt", "cre", "dclr", "drop", "exec",
        "ins", "recr", "rvk", "grnt", "sel",
        "upd", "del"
    };
    const wxString namesVeryShort[] = {
        "a", "c", "decl", "drop", "x",
        "i", "recr", "rev", "grnt", "s",
        "u", "del"
    };
    const wxString* names = 0;
    int setting = config().get("sqlEditorWindowKeywords", 1);
    if (setting == 1)
        names = &namesShort[0];
    else if (setting == 2)
        names = &namesVeryShort[0];

    int cnt = 0;
    wxString title;
    do
    {
        SqlTokenType stt = tk.getCurrentToken();
        for (unsigned int i=0; i<sizeof(lookfor)/sizeof(SqlTokenType); i++)
        {
            if (lookfor[i] == stt)
            {
                if (setting == 3)   // entire statement till end of line
                {
                    p = tk.getCurrentTokenPosition();
                    size_t q = text.find_first_of("\n\r;", p);
                    if (q == wxString::npos)
                        title = text.substr(p);
                    else
                        title = text.substr(p, q-p);
                    cnt = 10;   // flag to exit outer do..while loop
                    break;
                }
                if (cnt == 1)
                    title += " ";
                if (stt == tkIDENTIFIER || setting == 0)
                    title += tk.getCurrentTokenString();
                else
                    title += names[i];
                if (stt == kwSELECT)    // special case, find table name
                    while (tk.getCurrentToken() != kwFROM)
                        if (!tk.jumpToken(true))
                            break;
                if (stt == kwGRANT || stt == kwREVOKE)   // find grantee
                    while (tk.getCurrentToken() != kwON)
                        if (!tk.jumpToken(true))
                            break;
                if (++cnt == 2) // use 2 tokens for title
                    break;
            }
        }
    }
    while (cnt < 2 && tk.jumpToken(true /* skip parenthesis */));
    SetTitle(title);
}

void ExecuteSqlFrame::OnBlobEditorUpdate(wxTimerEvent& WXUNUSED(event))
{
    updateBlobEditor();
}

//! also used to drop constraints
class DropColumnHandler: public URIHandler,
    private MetadataItemURIHandlerHelper, private GUIURIHandlerHelper
{
public:
    DropColumnHandler() {}
    bool handleURI(URI& uri);
private:
    static const DropColumnHandler handlerInstance;
};

const DropColumnHandler DropColumnHandler::handlerInstance;

bool DropColumnHandler::handleURI(URI& uri)
{
    if (uri.action != "drop_field" && uri.action != "drop_constraint")
        return false;

    MetadataItem* c = extractMetadataItemFromURI<MetadataItem>(uri);
    wxWindow* w = getParentWindow(uri);
    if (!c || !w)
        return true;

    Table *t = 0;
    if (uri.action == "drop_field")
    {
        if (Column *cp = dynamic_cast<Column *>(c))
            t = cp->getTable();
    }
    else
    {
        if (Constraint *cs = dynamic_cast<Constraint *>(c))
            t = cs->getTable();
    }
    if (!t)
        return true;

    wxString sql = "ALTER TABLE " + t->getQuotedName() + " DROP ";
    if (uri.action == "drop_constraint")
        sql += "CONSTRAINT ";
    sql += c->getQuotedName();

    wxString msg(wxString::Format(
        _("Are you sure you wish to drop the %s %s?"),
        c->getTypeName().Lower().c_str(),
        c->getName_().c_str()));
    if (wxOK != showQuestionDialog(w, msg,
        _("Once you drop the object it is permanently removed from database."),
        AdvancedMessageDialogButtonsOkCancel(_("&Drop")),
        config(), "DIALOG_ConfirmDrop", _("Always drop without asking")))
    {
        return true;
    }
    execSql(w, _("Dropping field"), c->getDatabase(), sql, true);
    return true;
}

//! drop multiple columns
class DropColumnsHandler: public URIHandler,
    private MetadataItemURIHandlerHelper, private GUIURIHandlerHelper
{
public:
    DropColumnsHandler() {}
    bool handleURI(URI& uri);
private:
    static const DropColumnsHandler handlerInstance;
};

const DropColumnsHandler DropColumnsHandler::handlerInstance;

bool DropColumnsHandler::handleURI(URI& uri)
{
    if (uri.action != "drop_fields")
        return false;

    Table* t = extractMetadataItemFromURI<Table>(uri);
    wxWindow* w = getParentWindow(uri);
    if (!t || !w)
        return true;

    // get list of columns
    wxString sql;
    std::vector<wxString> list;
    if (selectRelationColumnsIntoVector(t, w, list))
    {
        for (std::vector<wxString>::iterator it = list.begin();
            it != list.end(); ++it)
        {
            Identifier temp(*it);
            sql += "ALTER TABLE " + t->getQuotedName() + " DROP "
                + temp.getQuoted() + ";\n";
        }
        execSql(w, _("Dropping fields"), t->getDatabase(), sql, true);
    }
    return true;
}

//! drop any metadata item
class DropObjectHandler: public URIHandler,
    private MetadataItemURIHandlerHelper, private GUIURIHandlerHelper
{
public:
    DropObjectHandler() {}
    bool handleURI(URI& uri);
private:
    static const DropObjectHandler handlerInstance;
};

const DropObjectHandler DropObjectHandler::handlerInstance;

bool DropObjectHandler::handleURI(URI& uri)
{
    if (uri.action != "drop_object")
        return false;

    MetadataItem* m = extractMetadataItemFromURI<MetadataItem>(uri);
    wxWindow* w = getParentWindow(uri);
    if (!m || !w)
        return true;

    wxString msg(wxString::Format(
        _("Are you sure you wish to drop the %s %s?"),
        m->getTypeName().Lower().c_str(),
        m->getName_().c_str()));
    if (wxOK != showQuestionDialog(w, msg,
        _("Once you drop the object it is permanently removed from database."),
        AdvancedMessageDialogButtonsOkCancel(_("&Drop")),
        config(), "DIALOG_ConfirmDrop", _("Always drop without asking")))
    {
        return true;
    }
    execSql(w, _("DROP"), m->getDatabase(), m->getDropSqlStatement(), true);
    return true;
}

//! show DDL in SQL editor
class EditDDLHandler: public URIHandler,
    private MetadataItemURIHandlerHelper, private GUIURIHandlerHelper
{
public:
    EditDDLHandler() {}
    bool handleURI(URI& uri);
private:
    static const EditDDLHandler handlerInstance;
};

const EditDDLHandler EditDDLHandler::handlerInstance;

bool EditDDLHandler::handleURI(URI& uri)
{
    if (uri.action != "edit_ddl")
        return false;

    MetadataItem* m = extractMetadataItemFromURI<MetadataItem>(uri);
    wxWindow* w = getParentWindow(uri);
    if (!m || !w)
        return true;

    // use a single read-only transaction for metadata loading
    DatabasePtr db = m->getDatabase();
    MetadataLoaderTransaction tr(db->getMetadataLoader());

    ProgressDialog pd(w, _("Extracting DDL Definitions"), 2);
    pd.doShow();
    CreateDDLVisitor cdv(&pd);
    m->acceptVisitor(&cdv);
    if (pd.isCanceled())
        return true;

    ExecuteSqlFrame* eff = new ExecuteSqlFrame(wxTheApp->GetTopWindow(), -1,
        "DDL", db);
    eff->setSql(cdv.getSql());
    // ProgressDialog needs to be hidden before ExecuteSqlFrame is shown,
    // otherwise the HTML frame will be raised over the ExecuteSqlFrame
    // when original Z-order is restored after pd has been destroyed
    pd.doHide();
    eff->Show();
    return true;
}

class EditProcedureHandler: public URIHandler,
    private MetadataItemURIHandlerHelper, private GUIURIHandlerHelper
{
public:
    EditProcedureHandler() {}
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const EditProcedureHandler handlerInstance;
};

const EditProcedureHandler EditProcedureHandler::handlerInstance;

bool EditProcedureHandler::handleURI(URI& uri)
{
    if (uri.action != "edit_procedure")
        return false;

    Procedure* p = extractMetadataItemFromURI<Procedure>(uri);
    wxWindow* w = getParentWindow(uri);
    if (!p || !w)
        return true;

    CreateDDLVisitor cdv;
    p->acceptVisitor(&cdv);
    showSql(w->GetParent(), _("Editing stored procedure"), p->getDatabase(),
        cdv.getSuffixSql());
    return true;
}

class EditFunctionHandler : public URIHandler,
    private MetadataItemURIHandlerHelper, private GUIURIHandlerHelper
{
public:
    EditFunctionHandler() {}
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const EditFunctionHandler handlerInstance;
};

const EditFunctionHandler EditFunctionHandler::handlerInstance;

bool EditFunctionHandler::handleURI(URI& uri)
{
    if (uri.action != "edit_function")
        return false;

    FunctionSQL* f = extractMetadataItemFromURI<FunctionSQL>(uri);
    wxWindow* w = getParentWindow(uri);
    if (!f || !w)
        return true;

    CreateDDLVisitor cdv;
    f->acceptVisitor(&cdv);
    showSql(w->GetParent(), _("Editing stored function"), f->getDatabase(),
        cdv.getSuffixSql());
    return true;
}

class AlterViewHandler: public URIHandler,
    private MetadataItemURIHandlerHelper, private GUIURIHandlerHelper
{
public:
    AlterViewHandler() {}
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const AlterViewHandler handlerInstance;
};

const AlterViewHandler AlterViewHandler::handlerInstance;

bool AlterViewHandler::handleURI(URI& uri)
{
    if (uri.action != "alter_relation"
        && uri.action != "alter_field")
    {
        return false;
    }

    Relation* r;
    wxString column;
    if (uri.action == "alter_relation")
        r = extractMetadataItemFromURI<Relation>(uri);
    else
    {
        Column* c = extractMetadataItemFromURI<Column>(uri);
        r = c->getTable();
        column = c->getName_();
    }
    wxWindow* w = getParentWindow(uri);
    if (!r || !w)
        return true;

    showSql(w->GetParent(), _("Altering dependent objects"), r->getDatabase(),
        r->getRebuildSql(column));
    return true;
}

class EditTriggerHandler: public URIHandler,
    private MetadataItemURIHandlerHelper, private GUIURIHandlerHelper
{
public:
    EditTriggerHandler() {}
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const EditTriggerHandler handlerInstance;
};

const EditTriggerHandler EditTriggerHandler::handlerInstance;

bool EditTriggerHandler::handleURI(URI& uri)
{
    if (uri.action != "edit_trigger")
        return false;

    Trigger* t = extractMetadataItemFromURI<Trigger>(uri);
    wxWindow* w = getParentWindow(uri);
    if (!t || !w)
        return true;

    showSql(w->GetParent(), _("Editing trigger"), t->getDatabase(),
        t->getAlterSql());
    return true;
}

class EditGeneratorValueHandler: public URIHandler,
    private MetadataItemURIHandlerHelper, private GUIURIHandlerHelper
{
public:
    EditGeneratorValueHandler() {}
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const EditGeneratorValueHandler handlerInstance;
};

const EditGeneratorValueHandler EditGeneratorValueHandler::handlerInstance;

bool EditGeneratorValueHandler::handleURI(URI& uri)
{
    if (uri.action != "edit_generator_value")
        return false;

    Generator* g = extractMetadataItemFromURI<Generator>(uri);
    wxWindow* w = getParentWindow(uri);
    if (!g || !w)
        return true;

    g->invalidate();
    int64_t oldvalue = g->getValue();
    DatabasePtr db = g->getDatabase();

    wxString value = wxGetTextFromUser(_("Changing generator value"),
        _("Enter new value"),
#ifndef wxLongLong
    // MH: I have no idea if this works on all systems... but it should be better
    // MB: we'll use wxLongLong wherever it is available
        wxLongLong(oldvalue).ToString(), w);
#else
        wxString::Format("%d"), oldvalue), w);
#endif

    if (value != "")
    {
        wxString sql = "SET GENERATOR " + g->getQuotedName()
            + " TO " + value + ";";
        execSql(w, sql, db, sql, true);
    }
    return true;
}

class EditExceptionHandler: public URIHandler,
    private MetadataItemURIHandlerHelper, private GUIURIHandlerHelper
{
public:
    EditExceptionHandler() {}
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const EditExceptionHandler handlerInstance;
};

const EditExceptionHandler EditExceptionHandler::handlerInstance;

bool EditExceptionHandler::handleURI(URI& uri)
{
    if (uri.action != "edit_exception")
        return false;

    Exception* e = extractMetadataItemFromURI<Exception>(uri);
    wxWindow* w = getParentWindow(uri);
    if (!e || !w)
        return true;

    showSql(w->GetParent(), _("Editing exception"), e->getDatabase(),
        e->getAlterSql());
    return true;
}

class IndexActionHandler: public URIHandler,
    private MetadataItemURIHandlerHelper, private GUIURIHandlerHelper
{
public:
    IndexActionHandler() {}
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const IndexActionHandler handlerInstance;
};

const IndexActionHandler IndexActionHandler::handlerInstance;

bool IndexActionHandler::handleURI(URI& uri)
{
    if (uri.action != "index_action")
        return false;

    Index* i = extractMetadataItemFromURI<Index>(uri);
    wxWindow* w = getParentWindow(uri);
    if (!i || !w)
        return true;

    wxString sql;
    wxString type = uri.getParam("type");        // type of operation
    if (type == "DROP")
    {
        wxString msg(wxString::Format(
            _("Are you sure you wish to drop the index %s?"),
            i->getName_().c_str()));
        if (wxOK != showQuestionDialog(w, msg,
            _("Once you drop the object it is permanently removed from database."),
            AdvancedMessageDialogButtonsOkCancel(_("&Drop")),
            config(), "DIALOG_ConfirmDrop", _("Always drop without asking")))
        {
            return true;
        }
        sql = "DROP INDEX " + i->getQuotedName();
    }
    else if (type == "RECOMPUTE")
        sql = "SET STATISTICS INDEX " + i->getQuotedName();
    else if (type == "TOGGLE_ACTIVE")
        sql = "ALTER INDEX " + i->getQuotedName() + (i->isActive() ? " INACTIVE" : " ACTIVE");

    execSql(w, wxEmptyString, i->getDatabase(), sql, true);
    return true;
}

class ActivateTriggersHandler: public URIHandler,
    private MetadataItemURIHandlerHelper, private GUIURIHandlerHelper
{
public:
    ActivateTriggersHandler() {};
    bool handleURI(URI& uri);
private:
    static const ActivateTriggersHandler handlerInstance;
};

const ActivateTriggersHandler ActivateTriggersHandler::handlerInstance;

bool ActivateTriggersHandler::handleURI(URI& uri)
{
    if (uri.action != "activate_triggers"
        && uri.action != "deactivate_triggers")
    {
        return false;
    }

    MetadataItem* mi = extractMetadataItemFromURI<MetadataItem>(uri);
    Relation* r = dynamic_cast<Relation*>(mi);
    Database* d = dynamic_cast<Database*>(mi);
    wxWindow* w = getParentWindow(uri);
    if ((!r && !d) || !w)
        return true;

    std::vector<Trigger*> list;
    if (r)
    {
        r->getTriggers(list, Trigger::afterIUD);
        r->getTriggers(list, Trigger::beforeIUD);
    }
    else
        d->getDatabaseTriggers(list);
    // don't show empty SQL editor if no triggers found
    if (list.empty())
        return true;

    wxString sql;
    for (std::vector<Trigger*>::iterator it = list.begin(); it != list.end();
        ++it)
    {
        sql += "ALTER TRIGGER " + (*it)->getQuotedName() + " ";
        if (uri.action == "deactivate_triggers")
            sql += "IN";
        sql += "ACTIVE;\n";
    }

    execSql(w, wxEmptyString, mi->getDatabase(), sql, true);
    return true;
}

class ActivateTriggerHandler: public URIHandler,
    private MetadataItemURIHandlerHelper, private GUIURIHandlerHelper
{
public:
    ActivateTriggerHandler() {}
    bool handleURI(URI& uri);
private:
    static const ActivateTriggerHandler handlerInstance;
};

const ActivateTriggerHandler ActivateTriggerHandler::handlerInstance;

bool ActivateTriggerHandler::handleURI(URI& uri)
{
    if (uri.action != "activate_trigger" && uri.action != "deactivate_trigger")
        return false;

    Trigger* t = extractMetadataItemFromURI<Trigger>(uri);
    wxWindow* w = getParentWindow(uri);
    if (!t || !w)
        return true;

    wxString sql = "ALTER TRIGGER " + t->getQuotedName() + " ";
    if (uri.action == "deactivate_trigger")
        sql += "IN";
    sql += "ACTIVE;\n";

    execSql(w, wxEmptyString, t->getDatabase(), sql, true);
    return true;
}

class EditPackageHeaderHandler : public URIHandler,
    private MetadataItemURIHandlerHelper, private GUIURIHandlerHelper
{
public:
    EditPackageHeaderHandler() {}
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const EditPackageHeaderHandler handlerInstance;
};

const EditPackageHeaderHandler EditPackageHeaderHandler::handlerInstance;

bool EditPackageHeaderHandler::handleURI(URI& uri)
{
    if (uri.action != "edit_package_header")
        return false;

    Package* p = extractMetadataItemFromURI<Package>(uri);
    wxWindow* w = getParentWindow(uri);
    if (!p || !w)
        return true;

    CreateDDLVisitor cdv;
    p->acceptVisitor(&cdv);
    showSql(w->GetParent(), _("Editing Package Header"), p->getDatabase(),
        p->getAlterHeader());
    return true;
}

class EditPackageBodyHandler : public URIHandler,
    private MetadataItemURIHandlerHelper, private GUIURIHandlerHelper
{
public:
    EditPackageBodyHandler() {}
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const EditPackageBodyHandler handlerInstance;
};

const EditPackageBodyHandler EditPackageBodyHandler::handlerInstance;

bool EditPackageBodyHandler::handleURI(URI& uri)
{
    if (uri.action != "edit_package_body")
        return false;

    Package* p = extractMetadataItemFromURI<Package>(uri);
    wxWindow* w = getParentWindow(uri);
    if (!p || !w)
        return true;

    CreateDDLVisitor cdv;
    p->acceptVisitor(&cdv);
    showSql(w->GetParent(), _("Editing Package Body"), p->getDatabase(),
        p->getAlterBody());
    return true;
}
