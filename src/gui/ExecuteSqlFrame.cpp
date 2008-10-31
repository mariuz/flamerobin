/*
  Copyright (c) 2004-2008 The FlameRobin Development Team

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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/artprov.h>
#include <wx/dnd.h>
#include <wx/file.h>
//#include <wx/fontmap.h>
#include <wx/fontdlg.h>
#include <wx/stopwatch.h>
#include <wx/tokenzr.h>

#include <algorithm>
#include <map>
#include <sstream>
#include <vector>

#include "config/Config.h"
#include "core/ArtProvider.h"
#include "core/FRError.h"
#include "core/StringUtils.h"
#include "engine/MetadataLoader.h"
#include "framemanager.h"
#include "gui/AdvancedMessageDialog.h"
#include "gui/CommandIds.h"
#include "gui/CommandManager.h"
#include "gui/controls/DataGrid.h"
#include "gui/controls/DataGridTable.h"
#include "gui/ProgressDialog.h"
#include "gui/ExecuteSql.h"
#include "gui/ExecuteSqlFrame.h"
#include "gui/InsertDialog.h"
#include "gui/StatementHistoryDialog.h"
#include "gui/StyleGuide.h"
#include "frutils.h"
#include "logger.h"
#include "metadata/CreateDDLVisitor.h"
#include "metadata/procedure.h"
#include "metadata/server.h"
#include "metadata/view.h"
#include "sql/Identifier.h"
#include "sql/IncompleteStatement.h"
#include "sql/MultiStatement.h"
#include "sql/SelectStatement.h"
#include "sql/SqlStatement.h"
#include "statementHistory.h"
#include "urihandler.h"
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
SqlEditorDropTarget::SqlEditorDropTarget(ExecuteSqlFrame* frame,
        SqlEditor* editor, Database* database)
    : frameM(frame), editorM(editor), databaseM(database)
{
    wxDataObjectComposite* dataobj = new wxDataObjectComposite;
    dataobj->Add(fileDataM = new wxFileDataObject);
    dataobj->Add(textDataM = new wxTextDataObject);
    SetDataObject(dataobj);
}
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
bool SqlEditorDropTarget::OnDropFiles(wxCoord, wxCoord,
    const wxArrayString& filenames)
{
    return (filenames.GetCount() == 1 && frameM->loadSqlFile(filenames[0]));
}
//-----------------------------------------------------------------------------
// TODO: This needs to be reworked to use the tokenizer
//       Perhaps we could have a SelectStatement class, that would be able to:
//       - load the select statement
//       - alter some of it (add new table, column, etc.)
//       - dump statement back to editor
bool SqlEditorDropTarget::OnDropText(wxCoord, wxCoord, const wxString& text)
{
    if (text.Mid(0, 7) != wxT("OBJECT:"))
        return false;

    long address;
    if (!text.Mid(7).ToLong(&address))
        return false;
    MetadataItem *m = (MetadataItem *)address;

    Database* db = m->findDatabase();
    if (db != databaseM)
    {
        wxMessageBox(_("Cannot use objects from different databases."),
            _("Wrong database."), wxOK | wxICON_WARNING);
        return false;
    }

    Table* t = 0;
    wxString column_list;
    if (m->getType() == ntColumn)
    {
        t = dynamic_cast<Table*>(m->getParent());
        column_list = t->getQuotedName() + wxT(".") + m->getQuotedName();
    }
    if ((m->getType() == ntTable) || (m->getType() == ntSysTable))
    {
        t = dynamic_cast<Table*>(m);
        // TODO: add all columns so that user can remove them easily
        column_list = t->getQuotedName() + wxT(".*");
    }
    if (t == 0)
    {
        wxMessageBox(_("Only tables and table columns can be dropped."),
            _("Object type not supported."), wxOK | wxICON_WARNING);
        return false;
    }

    SelectStatement sstm(editorM->GetText());
    if (!sstm.isValidSelectStatement())
    {
        // question("Invalid SELECT statement, do you wish to overwrite?");
        // if wxNO then
        //  return true;
        // else
            sstm.setStatement(wxT("SELECT FROM ")); // blank statement
    }

    // add the column(s)
    sstm.addColumn(column_list);

    // read in the table names, and find position where FROM clause ends
    std::vector<wxString> tableNames;
    sstm.getTables(tableNames);

    // if table is not there, add it
    if (std::find(tableNames.begin(), tableNames.end(), t->getName_())
        == tableNames.end())
    {
        std::vector<ForeignKey> relatedTables;
        if (Table::tablesRelate(tableNames, t, relatedTables)) // foreign keys
        {
            wxArrayString as;
            for (std::vector<ForeignKey>::iterator it = relatedTables.begin();
                it != relatedTables.end(); ++it)
            {
                wxString addme = (*it).referencedTableM + wxT(":  ")
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
            wxString joinType = (can_be_null ? wxT("LEFT JOIN"):wxT("JOIN"));
            sstm.addTable(t->getQuotedName(), joinType, join_list);
        }
        else
        {
            sstm.addTable(t->getQuotedName(), wxT("CARTESIAN"), wxT(""));
        }
    }

    editorM->SetText(sstm.getStatement());
    return true;
}
//-----------------------------------------------------------------------------
// Setup the Scintilla editor
SqlEditor::SqlEditor(wxWindow *parent, wxWindowID id)
    : SearchableEditor(parent, id)
{
    wxString s;
    if (config().getValue(wxT("SqlEditorFont"), s) && !s.empty())
    {
        wxFont f;
        f.SetNativeFontInfo(s);
        if (f.Ok())
            StyleSetFont(wxSTC_STYLE_DEFAULT, f);
    }
    else
    {
        wxFont font(styleguide().getEditorFontSize(), wxMODERN, wxNORMAL, wxNORMAL);
        StyleSetFont(wxSTC_STYLE_DEFAULT, font);
    }

    int charset;
    if (config().getValue(wxT("SqlEditorCharset"), charset))
        StyleSetCharacterSet(wxSTC_STYLE_DEFAULT, charset);

    setup();
}
//-----------------------------------------------------------------------------
bool SqlEditor::hasSelection()
{
    return GetSelectionStart() != GetSelectionEnd();
}
//-----------------------------------------------------------------------------

void SqlEditor::markText(int start, int end)
{
    centerCaret(true);
    GotoPos(end);
    GotoPos(start);
    SetSelectionStart(start);
    SetSelectionEnd(end);
    centerCaret(false);
}
//-----------------------------------------------------------------------------
void SqlEditor::setChars(bool firebirdIdentifierOnly)
{
    SetKeyWords(0, SqlTokenizer::getKeywordsString(SqlTokenizer::kwLowerCase));
    wxString chars(wxT("_0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz\"$"));

    if (!firebirdIdentifierOnly)
    {
        // see Document::SetDefaultCharClasses in stc/Document.cxx
        for (int ch = 0x80; ch < 0x0100; ch++)
            if (isalnum(ch))
                chars += wxChar(ch);
    }
    SetWordChars(chars);
}
//-----------------------------------------------------------------------------
//! This code has to be called each time the font has changed, so that the control updates
void SqlEditor::setup()
{
    StyleClearAll();
    StyleSetForeground(0,  wxColour(0x80, 0x00, 0x00));
    StyleSetForeground(1,  wxColour(0x00, 0xa0, 0x00));        // multiline comment
    StyleSetForeground(2,  wxColour(0x00, 0xa0, 0x00));        // one-line comment
    StyleSetForeground(3,  wxColour(0x00, 0xff, 0x00));
    StyleSetForeground(4,  wxColour(0x00, 0x00, 0xff));        // number
    StyleSetForeground(5,  wxColour(0x00, 0x00, 0x7f));        // keyword
    StyleSetForeground(6,  wxColour(0x00, 0x00, 0xff));        // 'single quotes'
    StyleSetForeground(7,  wxColour(0xff, 0x00, 0xff));
    StyleSetForeground(8,  wxColour(0x00, 0x7f, 0x7f));
    StyleSetForeground(9,  wxColour(0xff, 0x00, 0x00));
    StyleSetForeground(10, wxColour(0x00, 0x00, 0x00));        // ops
    StyleSetForeground(11, wxColour(0x00, 0x00, 0x00));
    StyleSetBackground(wxSTC_STYLE_BRACELIGHT, wxColour(0xff, 0xcc, 0x00));        // brace highlight
    StyleSetBackground(wxSTC_STYLE_BRACEBAD, wxColour(0xff, 0x33, 0x33));        // brace bad highlight
    StyleSetBold(5,  TRUE);
    StyleSetBold(10, TRUE);
    StyleSetBold(wxSTC_STYLE_BRACELIGHT, TRUE);
    StyleSetBold(wxSTC_STYLE_BRACEBAD, TRUE);
    StyleSetItalic(2, TRUE);
    StyleSetItalic(1, TRUE);
    SetLexer(wxSTC_LEX_SQL);
    setChars(false);

    int tabSize = config().get(wxT("sqlEditorTabSize"), 4);
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
    if (config().get(wxT("sqlEditorShowEdge"), false))
    {
        SetEdgeMode(wxSTC_EDGE_LINE);
        SetEdgeColumn(config().get(wxT("sqlEditorEdgeColumn"), 50));
    }
    centerCaret(false);
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(SqlEditor, wxStyledTextCtrl)
    EVT_CONTEXT_MENU(SqlEditor::OnContextMenu)
    EVT_KILL_FOCUS(SqlEditor::OnKillFocus)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void SqlEditor::OnContextMenu(wxContextMenuEvent& WXUNUSED(event))
{
    if (AutoCompActive() || CallTipActive())
        return;
    SetFocus();

    wxMenu m(0);
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
        size_t p = sel.find_first_of(wxT("\n\r\t"));
        if (p != wxString::npos)
            sel.Remove(p);
        m.Append(Cmds::Find_Selected_Object,
            _("S&how properties for ") + sel);
    }
    PopupMenu(&m, ScreenToClient(::wxGetMousePosition()));
}
//-----------------------------------------------------------------------------
void SqlEditor::OnKillFocus(wxFocusEvent& event)
{
    if (AutoCompActive())
        AutoCompCancel();
    if (CallTipActive())
        CallTipCancel();
    event.Skip();   // let the STC do it's job
}
//-----------------------------------------------------------------------------
void SqlEditor::setFont()
{
    // step 1 of 2: set font
    wxFont f, f2;
    wxString s;        // since we can't get the font from control we ask config() for it
    if (config().getValue(wxT("SqlEditorFont"), s) && !s.empty())
    {
        f.SetNativeFontInfo(s);
        f2 = ::wxGetFontFromUser(this, f);
    }
    else                // if config() doesn't have it, we'll use the default
    {
        wxFont font(styleguide().getEditorFontSize(), wxMODERN, wxNORMAL, wxNORMAL);
        f2 = ::wxGetFontFromUser(this, font);
    }

    if (!f2.Ok())    // user Canceled
        return;
    StyleSetFont(wxSTC_STYLE_DEFAULT, f2);

    // step 2 of 2: set charset
    std::map<wxString, int> sets;        // create human-readable names from wxSTC charsets
    sets[wxT("CHARSET_ANSI")] = 0;
    sets[wxT("CHARSET_EASTEUROPE")] = 238;
    sets[wxT("CHARSET_GB2312")] = 134;
    sets[wxT("CHARSET_HANGUL")] = 129;
    sets[wxT("CHARSET_HEBREW")] = 177;
    sets[wxT("CHARSET_SHIFTJIS")] = 128;
    #ifdef __WXMSW__
    sets[wxT("CHARSET_DEFAULT")] = 1;        // according to scintilla docs these only work on Windows
    sets[wxT("CHARSET_BALTIC")] = 186;        // so we won't offer them
    sets[wxT("CHARSET_CHINESEBIG5")] = 136;
    sets[wxT("CHARSET_GREEK")] = 161;
    sets[wxT("CHARSET_MAC")] = 77;
    sets[wxT("CHARSET_OEM")] = 255;
    sets[wxT("CHARSET_RUSSIAN")] = 204;
    sets[wxT("CHARSET_SYMBOL")] = 2;
    sets[wxT("CHARSET_TURKISH")] = 162;
    sets[wxT("CHARSET_JOHAB")] = 130;
    sets[wxT("CHARSET_ARABIC")] = 178;
    sets[wxT("CHARSET_VIETNAMESE")] = 163;
    sets[wxT("CHARSET_THAI")] = 222;
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
            config().setValue(wxT("SqlEditorFont"), fontdesc);
        config().setValue(wxT("SqlEditorCharset"), (*it).second);
    }
    setup();    // make control accept new settings
    showInformationDialog(wxGetTopLevelParent(this), _("The SQL editor font has been changed."),
        _("This setting affects only the SQL editor font. The font used in the result set data grid has to be changed separately."),
        AdvancedMessageDialogButtonsOk(), config(), wxT("DIALOG_WarnFont"), _("Do not show this information again"));
}
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
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
        Database *db, const wxPoint& pos, const wxSize& size, long style)
    :BaseFrame(wxTheApp->GetTopWindow(), id, title, pos, size, style),
    Observer(), databaseM(db)
{
    loadingM = true;

    CommandManager cm;
    buildToolbar(cm);
    buildMainMenu(cm);

    panel_contents = new wxPanel(this, -1, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL
#ifdef __WXGTK12__
        | wxSUNKEN_BORDER
#endif
    );
    splitter_window_1 = new wxSplitterWindow(panel_contents, -1);
    styled_text_ctrl_sql = new SqlEditor(splitter_window_1, ID_stc_sql);

    notebook_1 = new wxNotebook(splitter_window_1, -1, wxDefaultPosition,
        wxDefaultSize, 0);
    notebook_pane_1 = new wxPanel(notebook_1, -1);
    styled_text_ctrl_stats = new wxStyledTextCtrl(notebook_pane_1, -1);
    styled_text_ctrl_stats->SetWrapMode(wxSTC_WRAP_WORD);
    styled_text_ctrl_stats->StyleSetForeground(1, *wxRED);
    styled_text_ctrl_stats->StyleSetForeground(2, *wxBLUE);
    notebook_1->AddPage(notebook_pane_1, _("Statistics"));

    notebook_pane_2 = new wxPanel(notebook_1, -1);
    grid_data = new DataGrid(notebook_pane_2, ID_grid_data);
    notebook_1->AddPage(notebook_pane_2, _("Data"));

    statusbar_1 = CreateStatusBar(4);
    SetStatusBarPane(-1);

    set_properties();
    do_layout();
    setDatabase(db);    // must come after set_properties
    setViewMode(false, vmEditor);
    loadingM = false;
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::buildToolbar(CommandManager& cm)
{
    //toolBarM = CreateToolBar( wxTB_FLAT|wxTB_HORIZONTAL|wxTB_TEXT, wxID_ANY );
    toolBarM = CreateToolBar( wxTB_FLAT | wxTB_HORIZONTAL, wxID_ANY );

#ifdef __WXGTK20__
    wxSize bmpSize(24, 24);
#else
    wxSize bmpSize(16, 16);
#endif
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
//-----------------------------------------------------------------------------
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
    gridMenu->Append(Cmds::DataGrid_ImportBlob, _("Import BLOB from file..."));
    gridMenu->Append(Cmds::DataGrid_ExportBlob, _("Save BLOB to file..."));
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
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::set_properties()
{
    SetTitle(_("Execute SQL statements"));
    SetSize(wxSize(628, 488));
    int statusbar_widths[] = { -2, 100, 60, -1 };
    statusbar_1->SetStatusWidths(4, statusbar_widths);
    const wxString statusbar_fields[] =
    {
        wxT("user @ database"),
        wxT("rows fetched"),
        wxT("cursor position"),
        wxT("Transaction status")
    };
    for(int i = 0; i < statusbar_1->GetFieldsCount(); ++i)
    {
        statusbar_1->SetStatusText(statusbar_fields[i], i);
    }
    grid_data->SetTable(new DataGridTable(statementM, databaseM), true);
    splitter_window_1->Initialize(styled_text_ctrl_sql);
    viewModeM = vmEditor;
    
    SetIcon(wxArtProvider::GetIcon(ART_ExecuteSqlFrame, wxART_FRAME_ICON));

    keywordsM = wxT("");
    closeWhenTransactionDoneM = false;
    autoCommitM = config().get(wxT("autoCommitDDL"), false);
}
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::showProperties(wxString objectName)
{
    MetadataItem *m = databaseM->findByName(objectName);
    if (!m)
        m = databaseM->findByName(objectName.Upper());

    if (m)
    {
        frameManager().showMetadataPropertyFrame(m);
        return;
    }

    wxMessageBox(
        wxString::Format(_("Object %s has not been found in this database."),
            objectName.c_str()),
        _("Search failed."), wxOK | wxICON_INFORMATION);
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(ExecuteSqlFrame, wxFrame)
    EVT_STC_UPDATEUI(ExecuteSqlFrame::ID_stc_sql, ExecuteSqlFrame::OnSqlEditUpdateUI)
    EVT_STC_CHARADDED(ExecuteSqlFrame::ID_stc_sql, ExecuteSqlFrame::OnSqlEditCharAdded)
    EVT_STC_CHANGE(ExecuteSqlFrame::ID_stc_sql, ExecuteSqlFrame::OnSqlEditChanged)
    EVT_STC_START_DRAG(ExecuteSqlFrame::ID_stc_sql, ExecuteSqlFrame::OnSqlEditStartDrag)
    EVT_SPLITTER_UNSPLIT(wxID_ANY, ExecuteSqlFrame::OnSplitterUnsplit)
    EVT_CHAR_HOOK(ExecuteSqlFrame::OnKeyDown)
    EVT_CHILD_FOCUS(ExecuteSqlFrame::OnChildFocus)
    EVT_CLOSE(ExecuteSqlFrame::OnClose)
    EVT_IDLE(ExecuteSqlFrame::OnIdle)

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

    EVT_MENU(Cmds::DataGrid_Insert_row,      ExecuteSqlFrame::OnMenuGridInsertRow)
    EVT_MENU(Cmds::DataGrid_Delete_row,      ExecuteSqlFrame::OnMenuGridDeleteRow)
    EVT_MENU(Cmds::DataGrid_ImportBlob,      ExecuteSqlFrame::OnMenuGridImportBlob)
    EVT_MENU(Cmds::DataGrid_ExportBlob,      ExecuteSqlFrame::OnMenuGridExportBlob)
    EVT_MENU(Cmds::DataGrid_Copy_as_insert,  ExecuteSqlFrame::OnMenuGridCopyAsInsert)
    EVT_MENU(Cmds::DataGrid_Copy_as_update,  ExecuteSqlFrame::OnMenuGridCopyAsUpdate)
    EVT_MENU(Cmds::DataGrid_Save_as_html,    ExecuteSqlFrame::OnMenuGridSaveAsHtml)
    EVT_MENU(Cmds::DataGrid_Save_as_csv,     ExecuteSqlFrame::OnMenuGridSaveAsCsv)
    EVT_MENU(Cmds::DataGrid_Set_header_font, ExecuteSqlFrame::OnMenuGridGridHeaderFont)
    EVT_MENU(Cmds::DataGrid_Set_cell_font,   ExecuteSqlFrame::OnMenuGridGridCellFont)
    EVT_MENU(Cmds::DataGrid_FetchAll,        ExecuteSqlFrame::OnMenuGridFetchAll)
    EVT_MENU(Cmds::DataGrid_CancelFetchAll,  ExecuteSqlFrame::OnMenuGridCancelFetchAll)

    EVT_UPDATE_UI(Cmds::DataGrid_Insert_row,     ExecuteSqlFrame::OnMenuUpdateGridInsertRow)
    EVT_UPDATE_UI(Cmds::DataGrid_Delete_row,     ExecuteSqlFrame::OnMenuUpdateGridDeleteRow)
    EVT_UPDATE_UI(Cmds::DataGrid_ImportBlob,     ExecuteSqlFrame::OnMenuUpdateGridCellIsBlob)
    EVT_UPDATE_UI(Cmds::DataGrid_ExportBlob,     ExecuteSqlFrame::OnMenuUpdateGridCellIsBlob)
    EVT_UPDATE_UI(Cmds::DataGrid_Copy_as_insert, ExecuteSqlFrame::OnMenuUpdateGridHasData)
    EVT_UPDATE_UI(Cmds::DataGrid_Copy_as_update, ExecuteSqlFrame::OnMenuUpdateGridHasData)
    EVT_UPDATE_UI(Cmds::DataGrid_Save_as_html,   ExecuteSqlFrame::OnMenuUpdateGridHasSelection)
    EVT_UPDATE_UI(Cmds::DataGrid_Save_as_csv,    ExecuteSqlFrame::OnMenuUpdateGridHasSelection)
    EVT_UPDATE_UI(Cmds::DataGrid_FetchAll,       ExecuteSqlFrame::OnMenuUpdateGridFetchAll)
    EVT_UPDATE_UI(Cmds::DataGrid_CancelFetchAll, ExecuteSqlFrame::OnMenuUpdateGridCancelFetchAll)


    EVT_COMMAND(ExecuteSqlFrame::ID_grid_data, wxEVT_FRDG_ROWCOUNT_CHANGED, \
        ExecuteSqlFrame::OnGridRowCountChanged)
    EVT_COMMAND(ExecuteSqlFrame::ID_grid_data, wxEVT_FRDG_STATEMENT, \
        ExecuteSqlFrame::OnGridStatementExecuted)

    EVT_GRID_CMD_LABEL_LEFT_DCLICK(ExecuteSqlFrame::ID_grid_data, ExecuteSqlFrame::OnGridLabelLeftDClick)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
// Avoiding the annoying thing that you cannot click inside the selection and have it deselected and have caret there
void ExecuteSqlFrame::OnSqlEditStartDrag(wxStyledTextEvent& WXUNUSED(event))
{
    wxPoint mp = ::wxGetMousePosition();
    int p = styled_text_ctrl_sql->PositionFromPoint(styled_text_ctrl_sql->ScreenToClient(mp));
    styled_text_ctrl_sql->SetSelectionStart(p);    // deselect text
    styled_text_ctrl_sql->SetSelectionEnd(p);
}
//-----------------------------------------------------------------------------
//! display editor col:row in StatusBar and do highlighting of braces ()
void ExecuteSqlFrame::OnSqlEditUpdateUI(wxStyledTextEvent& WXUNUSED(event))
{
    if (loadingM)
        return;

    // print x:y coordinates in status bar
    int p = styled_text_ctrl_sql->GetCurrentPos();
    int row = styled_text_ctrl_sql->GetCurrentLine();
    int col = p - styled_text_ctrl_sql->PositionFromLine(row);
    statusbar_1->SetStatusText(wxString::Format(wxT("%d : %d"), row+1, col+1), 2);

    // check for braces, and highlight
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
        if (styled_text_ctrl_sql->CallTipActive() && (c1==')' || c2==')') && q == styled_text_ctrl_sql->CallTipPosAtStart() - 1)
            styled_text_ctrl_sql->CallTipCancel();
    }
    else
        styled_text_ctrl_sql->BraceBadLight(wxSTC_INVALID_POSITION);    // remove light
}
//-----------------------------------------------------------------------------
//! returns true if there is a word in "wordlist" that starts with "word"
bool HasWord(wxString word, wxString& wordlist)
{
    word.MakeUpper();

    wxStringTokenizer tkz(wordlist, wxT(" "));
    while (tkz.HasMoreTokens())
        if (tkz.GetNextToken().Upper().StartsWith(word))
            return true;
    return false;
}
//-----------------------------------------------------------------------------
//! autocomplete stuff
void ExecuteSqlFrame::OnSqlEditCharAdded(wxStyledTextEvent& WXUNUSED(event))
{
    int pos = styled_text_ctrl_sql->GetCurrentPos();
    if (pos == 0)
        return;

    int c = styled_text_ctrl_sql->GetCharAt(pos-1);
    if (c == '(')
    {
        if (config().get(wxT("SQLEditorCalltips"), true))
        {
            int start = styled_text_ctrl_sql->WordStartPosition(pos - 2, true);
            if (start != -1 && start != pos - 2)
            {
                wxString word = styled_text_ctrl_sql->GetTextRange(start, pos - 1).Upper();
                wxString calltip;
                Procedure* p = dynamic_cast<Procedure*>(databaseM->findByNameAndType(ntProcedure, word));
                if (p)
                    calltip = p->getDefinition();
                Function* f = dynamic_cast<Function*>(databaseM->findByNameAndType(ntFunction, word));
                if (f)
                    calltip = f->getDefinition();
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
        if (config().get(wxT("AutocompleteEnabled"), true))
        {
            #ifndef __WXGTK20__
            bool allow = config().get(wxT("autoCompleteQuoted"), true);
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
                    if (!config().get(wxT("AutoCompleteDisableWhenCalltipShown"), true))
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
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnSqlEditChanged(wxStyledTextEvent& WXUNUSED(event))
{
    if (filenameM.IsOk())
        return;
    if (styled_text_ctrl_sql->GetTextLength() < 1)
    {
        SetTitle(_("Execute SQL statements"));
        return;
    }

    const wxString& txt = styled_text_ctrl_sql->GetText();
    size_t p = txt.find(wxT("@FR-TITLE@"));
    if (p != wxString::npos)
    {
        size_t q = txt.find(wxT("*/"), p);
        if (q == wxString::npos)
            q = txt.find_first_of(wxT("\n\r"), p);
        if (q != wxString::npos)
            SetTitle(txt.substr(p+11, q - p - 11));
        else
            SetTitle(txt.substr(p+11));
        return;
    }

    SqlTokenizer tk(styled_text_ctrl_sql->GetText());
    const SqlTokenType lookfor[] = {
        kwALTER, kwCREATE, kwDECLARE, kwDROP, kwEXECUTE, kwINSERT,
        kwRECREATE, kwREVOKE, kwGRANT, kwSELECT, kwUPDATE, kwDELETE,
        tkIDENTIFIER
    };
    const wxString namesShort[] = {
        wxT("alt"), wxT("cre"), wxT("dclr"), wxT("drop"), wxT("exec"),
        wxT("ins"), wxT("recr"), wxT("rvk"), wxT("grnt"), wxT("sel"),
        wxT("upd"), wxT("del")
    };
    const wxString namesVeryShort[] = {
        wxT("a"), wxT("c"), wxT("decl"), wxT("drop"), wxT("x"),
        wxT("i"), wxT("recr"), wxT("rev"), wxT("grnt"), wxT("s"),
        wxT("u"), wxT("del")
    };
    const wxString* names = 0;
    int setting = config().get(wxT("sqlEditorWindowKeywords"), 1);
    if (setting == 1)
        names = &namesShort[0];
    else if (setting == 2)
        names = &namesVeryShort[0];

    int cnt = 0;
    wxString title;
    do
    {
        SqlTokenType stt = tk.getCurrentToken();
        for (int i=0; i<sizeof(lookfor)/sizeof(SqlTokenType); i++)
        {
            if (lookfor[i] == stt)
            {
                if (setting == 3)   // entire statement till end of line
                {
                    p = tk.getCurrentTokenPosition();
                    size_t q = txt.find_first_of(wxT("\n\r;"), p);
                    if (q == wxString::npos)
                        title = txt.substr(p);
                    else
                        title = txt.substr(p, q-p);
                    cnt = 10;   // flag to exit outer do..while loop
                    break;
                }
                if (cnt == 1)
                    title += wxT(" ");
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
//-----------------------------------------------------------------------------
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
    wxString columns = is.getObjectColumns(table, pos);
    if (columns.IsEmpty())
        return;
    if (HasWord(styled_text_ctrl_sql->GetTextRange(pos, pos+len), columns))
        styled_text_ctrl_sql->AutoCompShow(len, columns);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::autoComplete(bool force)
{
    if (styled_text_ctrl_sql->AutoCompActive())
        return;

    int autoCompleteChars = 1;
    if (!force)
    {
        autoCompleteChars = config().get(wxT("AutocompleteChars"), 3);
        if (autoCompleteChars <= 0)
            return;
    }

    int pos = styled_text_ctrl_sql->GetCurrentPos();
    int start = styled_text_ctrl_sql->WordStartPosition(pos, true);
    if (start > 1 && styled_text_ctrl_sql->GetCharAt(start - 1) == '.')
    {
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
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuFindSelectedObject(wxCommandEvent& WXUNUSED(event))
{
    wxString sel = styled_text_ctrl_sql->GetSelectedText();
    int p = sel.Find(wxT(" "));
    if (p != -1)
        sel.Remove(p);
    showProperties(sel);
}
//-----------------------------------------------------------------------------
//! handle function keys
void ExecuteSqlFrame::OnKeyDown(wxKeyEvent& event)
{
    wxCommandEvent e;
    int key = event.GetKeyCode();
    if (!event.HasModifiers())
    {
        switch (key)
        {
            case WXK_F3:
                styled_text_ctrl_sql->find(false);
                return;
        };
    }

    if (wxWindow::FindFocus() == styled_text_ctrl_sql)
    {
        if (!styled_text_ctrl_sql->AutoCompActive())
        {
            enum { acSpace=0, acTab };
            int acc = acSpace;
            config().getValue(wxT("AutoCompleteKey"), acc);
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
            if (!config().get(wxT("AutoCompleteWithEnter"), true))
                styled_text_ctrl_sql->AutoCompCancel();
        }
    }
    event.Skip();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnChildFocus(wxChildFocusEvent& WXUNUSED(event))
{
    doUpdateFocusedControlM = true;
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnIdle(wxIdleEvent& event)
{
    if (doUpdateFocusedControlM)
        updateViewMode();
    event.Skip();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnClose(wxCloseEvent& event)
{
    // prevent editor from updating the invalid dataset
    if (grid_data->IsCellEditControlEnabled())
        grid_data->EnableCellEditControl(false);
    BaseFrame::OnClose(event);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuNew(wxCommandEvent& WXUNUSED(event))
{
    ExecuteSqlFrame *eff = new ExecuteSqlFrame(GetParent(), -1,
        _("Execute SQL statements"), databaseM);
    eff->setSql(styled_text_ctrl_sql->GetSelectedText());
    eff->Show();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuOpen(wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog fd(this, _("Open File"),
        filenameM.GetPath(), filenameM.GetName(),
        _("SQL script files (*.sql)|*.sql|All files (*.*)|*.*"),
        wxFD_OPEN | wxFD_CHANGE_DIR);
    if (wxID_OK == fd.ShowModal())
        loadSqlFile(fd.GetPath());
}
//-----------------------------------------------------------------------------
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

    if (styled_text_ctrl_sql->SaveFile(filename))
    {
        if (filename.compare(filenameM.GetFullPath()) != 0)
        {
            filenameM = filename;
            SetTitle(filename);
        }
        statusbar_1->SetStatusText((_("File saved")), 2);
    }
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuClose(wxCommandEvent& WXUNUSED(event))
{
    Close();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUndo(wxCommandEvent& WXUNUSED(event))
{
    if (viewModeM == vmEditor)
        styled_text_ctrl_sql->Undo();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUpdateUndo(wxUpdateUIEvent& event)
{
    event.Enable(viewModeM == vmEditor && styled_text_ctrl_sql->CanUndo());
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuRedo(wxCommandEvent& WXUNUSED(event))
{
    if (viewModeM == vmEditor)
        styled_text_ctrl_sql->Redo();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUpdateRedo(wxUpdateUIEvent& event)
{
    event.Enable(viewModeM == vmEditor && styled_text_ctrl_sql->CanRedo());
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuCopy(wxCommandEvent& WXUNUSED(event))
{
    if (viewModeM == vmEditor)
        styled_text_ctrl_sql->Copy();
    else if (viewModeM == vmGrid)
        grid_data->copyToCB();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUpdateCopy(wxUpdateUIEvent& event)
{
    bool enableCmd = false;
    if (viewModeM == vmEditor)
        enableCmd = styled_text_ctrl_sql->hasSelection();
    else if (viewModeM == vmGrid)
        enableCmd = grid_data->getDataGridTable() && grid_data->GetNumberRows();
    event.Enable(enableCmd);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuCut(wxCommandEvent& WXUNUSED(event))
{
    if (viewModeM == vmEditor)
        styled_text_ctrl_sql->Cut();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUpdateCut(wxUpdateUIEvent& event)
{
    event.Enable(viewModeM == vmEditor && styled_text_ctrl_sql->hasSelection());
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuDelete(wxCommandEvent& WXUNUSED(event))
{
    if (viewModeM == vmEditor)
        styled_text_ctrl_sql->Clear();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUpdateDelete(wxUpdateUIEvent& event)
{
    event.Enable(viewModeM == vmEditor && styled_text_ctrl_sql->hasSelection());
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuPaste(wxCommandEvent& WXUNUSED(event))
{
    if (viewModeM == vmEditor)
        styled_text_ctrl_sql->Paste();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUpdatePaste(wxUpdateUIEvent& event)
{
    event.Enable(viewModeM == vmEditor && styled_text_ctrl_sql->CanPaste());
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuSelectAll(wxCommandEvent& WXUNUSED(event))
{
    if (viewModeM == vmEditor)
        styled_text_ctrl_sql->SelectAll();
    else if (viewModeM == vmLogCtrl)
        styled_text_ctrl_stats->SelectAll();
    else if (viewModeM == vmGrid)
        grid_data->SelectAll();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuReplace(wxCommandEvent &WXUNUSED(event))
{
    styled_text_ctrl_sql->find(true);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUpdateWhenInTransaction(wxUpdateUIEvent& event)
{
    event.Enable(inTransactionM && !grid_data->IsCellEditControlEnabled());
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuSelectView(wxCommandEvent& event)
{
    if (event.GetId() == Cmds::View_Editor)
        setViewMode(vmEditor);
    else if (event.GetId() == Cmds::View_Statistics)
        setViewMode(vmLogCtrl);
    else if (event.GetId() == Cmds::View_Data)
        setViewMode(vmGrid);
    else
        wxCHECK_RET(false, wxT("event id not handled"));
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUpdateSelectView(wxUpdateUIEvent& event)
{
    if (event.GetId() == Cmds::View_Editor && viewModeM == vmEditor)
        event.Check(true);
    else if (event.GetId() == Cmds::View_Statistics && viewModeM == vmLogCtrl)
        event.Check(true);
    else if (event.GetId() == Cmds::View_Data && viewModeM == vmGrid)
        event.Check(true);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuSplitView(wxCommandEvent& WXUNUSED(event))
{
    setViewMode(!splitter_window_1->IsSplit(), viewModeM);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUpdateSplitView(wxUpdateUIEvent& event)
{
    event.Check(splitter_window_1->IsSplit());
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuSetEditorFont(wxCommandEvent& WXUNUSED(event))
{
    styled_text_ctrl_sql->setFont();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuToggleWrap(wxCommandEvent& WXUNUSED(event))
{
    const int mode = styled_text_ctrl_sql->GetWrapMode();
    styled_text_ctrl_sql->SetWrapMode(
        (mode == wxSTC_WRAP_WORD) ? wxSTC_WRAP_NONE : wxSTC_WRAP_WORD);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuHistoryNext(wxCommandEvent& WXUNUSED(event))
{
    StatementHistory& sh = StatementHistory::get(databaseM);
    if (historyPositionM != sh.size())  // we're already at the end?
    {
        historyPositionM++;
        if (historyPositionM == sh.size())
            setSql(localBuffer);
        else
            setSql(sh.get(historyPositionM));
    }
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuHistoryPrev(wxCommandEvent& WXUNUSED(event))
{
    StatementHistory& sh = StatementHistory::get(databaseM);
    if (historyPositionM > 0 && sh.size() > 0)
    {
        if (historyPositionM == sh.size())  // we're on local buffer => store it
            localBuffer = styled_text_ctrl_sql->GetText();
        historyPositionM--;
        setSql(sh.get(historyPositionM));
    }
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuHistorySearch(wxCommandEvent& WXUNUSED(event))
{
    StatementHistory& sh = StatementHistory::get(databaseM);
    StatementHistoryDialog *shf = new StatementHistoryDialog(this, &sh);
    if (shf->ShowModal() == wxID_OK)
        setSql(shf->getSql());
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUpdateHistoryNext(wxUpdateUIEvent& event)
{
    StatementHistory& sh = StatementHistory::get(databaseM);
    event.Enable(sh.size() > historyPositionM);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUpdateHistoryPrev(wxUpdateUIEvent& event)
{
    StatementHistory& sh = StatementHistory::get(databaseM);
    event.Enable(historyPositionM > 0 && sh.size() > 0);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuExecute(wxCommandEvent& WXUNUSED(event))
{
    clearLogBeforeExecution();
    prepareAndExecute(false);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuShowPlan(wxCommandEvent& WXUNUSED(event))
{
    prepareAndExecute(true);
}
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuExecuteSelection(wxCommandEvent& WXUNUSED(event))
{
    clearLogBeforeExecution();
    if (config().get(wxT("TreatAsSingleStatement"), false))
        execute(styled_text_ctrl_sql->GetSelectedText(), wxT(";"));
    else
        parseStatements(styled_text_ctrl_sql->GetSelectedText(),
            false,
            false,
            styled_text_ctrl_sql->GetSelectionStart()
            );
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuGridFetchAll(wxCommandEvent& WXUNUSED(event))
{
    grid_data->fetchAll();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuGridCancelFetchAll(wxCommandEvent& WXUNUSED(event))
{
    grid_data->cancelFetchAll();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUpdateGridCellIsBlob(wxUpdateUIEvent& event)
{
    DataGridTable* dgt = grid_data->getDataGridTable();
    event.Enable(dgt && grid_data->GetNumberRows() &&
        dgt->isBlobColumn(grid_data->GetGridCursorCol()));
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuGridExportBlob(wxCommandEvent& WXUNUSED(event))
{
    DataGridTable* dgt = grid_data->getDataGridTable();
    if (!dgt || !grid_data->GetNumberRows())
        return;
    if (!dgt->isBlobColumn(grid_data->GetGridCursorCol()))
        throw FRError(_("Not a BLOB column"));
    wxString filename = ::wxFileSelector(_("Select a file"), wxT(""),
        wxT(""), wxT(""), wxT("*"),
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
    if (filename.IsEmpty())
        return;

    ProgressDialog pd(this, _("Saving BLOB to file"));
    pd.Show();
    dgt->exportBlobFile(filename, grid_data->GetGridCursorRow(),
        grid_data->GetGridCursorCol(), &pd);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuGridImportBlob(wxCommandEvent& WXUNUSED(event))
{
    DataGridTable* dgt = grid_data->getDataGridTable();
    if (!dgt || !grid_data->GetNumberRows())
        return;
    if (!dgt->isBlobColumn(grid_data->GetGridCursorCol()))
        throw FRError(_("Not a BLOB column"));
    wxString filename = ::wxFileSelector(_("Select a file"), wxT(""),
        wxT(""), wxT(""), wxT("*"),
        wxFD_OPEN | wxFD_FILE_MUST_EXIST, this);
   if (filename.IsEmpty())
        return;

    ProgressDialog pd(this, _("Importing BLOB from file"));
    pd.Show();
    dgt->importBlobFile(filename, grid_data->GetGridCursorRow(),
        grid_data->GetGridCursorCol(), &pd);
}
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuGridCopyAsInsert(wxCommandEvent& WXUNUSED(event))
{
    grid_data->copyToCBAsInsert();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuGridCopyAsUpdate(wxCommandEvent& WXUNUSED(event))
{
    grid_data->copyToCBAsUpdate();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuGridSaveAsHtml(wxCommandEvent& WXUNUSED(event))
{
    grid_data->saveAsHTML();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuGridSaveAsCsv(wxCommandEvent& WXUNUSED(event))
{
    grid_data->saveAsCSV();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuGridGridHeaderFont(wxCommandEvent& WXUNUSED(event))
{
    grid_data->setHeaderFont();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuGridGridCellFont(wxCommandEvent& WXUNUSED(event))
{
    grid_data->setCellFont();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUpdateGridHasSelection(wxUpdateUIEvent& event)
{
    event.Enable(grid_data->IsSelection());
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUpdateGridFetchAll(wxUpdateUIEvent& event)
{
    DataGridTable* table = grid_data->getDataGridTable();
    event.Enable(table && table->canFetchMoreRows()
        && !table->getFetchAllRows());
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUpdateGridCancelFetchAll(wxUpdateUIEvent& event)
{
    DataGridTable* table = grid_data->getDataGridTable();
    event.Enable(table && table->canFetchMoreRows()
        && table->getFetchAllRows());
}
//-----------------------------------------------------------------------------
bool ExecuteSqlFrame::loadSqlFile(const wxString& filename)
{
    if (!styled_text_ctrl_sql->LoadFile(filename))
        return false;
    filenameM = filename;
    SetTitle(filename);
    return true;
}
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::setSql(wxString sql)
{
    styled_text_ctrl_sql->SetText(sql);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::clearLogBeforeExecution()
{
    if (config().get(wxT("SQLEditorExecuteClears"), false))
        styled_text_ctrl_stats->ClearAll();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::prepareAndExecute(bool prepareOnly)
{
    bool hasSelection = styled_text_ctrl_sql->GetSelectionStart()
        != styled_text_ctrl_sql->GetSelectionEnd();
    bool ok;
    if (hasSelection && config().get(wxT("OnlyExecuteSelected"), false))
    {
        if (config().get(wxT("TreatAsSingleStatement"), false))
        {
            ok = execute(styled_text_ctrl_sql->GetSelectedText(), wxT(";"),
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

    if (ok || config().get(wxT("historyStoreUnsuccessful"), true))
    {
        // add to history
        StatementHistory& sh = StatementHistory::get(databaseM);
        sh.add(styled_text_ctrl_sql->GetText());
        historyPositionM = sh.size();
    }

    if (!inTransactionM)
        setViewMode(false, vmEditor);
}
//-----------------------------------------------------------------------------
//! adapted so we don't have to change all the other code that utilizes SQL editor
void ExecuteSqlFrame::executeAllStatements(bool closeWhenDone)
{
    clearLogBeforeExecution();
    bool ok = parseStatements(styled_text_ctrl_sql->GetText(), closeWhenDone);
    if (config().get(wxT("historyStoreGenerated"), true) &&
        (ok || config().get(wxT("historyStoreUnsuccessful"), true)))
    {
        // add buffer to history
        StatementHistory& sh = StatementHistory::get(databaseM);
        sh.add(styled_text_ctrl_sql->GetText());
        historyPositionM = sh.size();
    }

    if (closeWhenDone && autoCommitM && !inTransactionM)
        Close();
}
//-----------------------------------------------------------------------------
//! Parses all sql statements in STC
//! when autoexecute is TRUE, program just waits user to click Commit/Rollback and closes window
//! when autocommit DDL is also set then frame is closed at once if commit was successful
bool ExecuteSqlFrame::parseStatements(const wxString& statements,
    bool closeWhenDone, bool prepareOnly, int selectionOffset)
{
    wxBusyCursor cr;
    MultiStatement ms(statements, wxT(";"));
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
                    _("Warning."), wxOK | wxICON_WARNING);
                return false;
            }
        }
        else if (ss.isSetAutoDDLStatement(autoDDLSetting))
        {
            if (autoDDLSetting == wxT("ON"))
                autoCommitM = true;
            else if (autoDDLSetting == wxT("OFF"))
                autoCommitM = false;
            else
                autoCommitM = !autoCommitM;
        }
        else if (ss.getSql().length() && !execute(ss.getSql(),
            ms.getTerminator(), prepareOnly))
        {
            styled_text_ctrl_sql->markText(selectionOffset + ms.getStart(),
                selectionOffset + ms.getEnd());
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
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUpdateWhenExecutePossible(wxUpdateUIEvent& event)
{
    event.Enable(!closeWhenTransactionDoneM);
}
//-----------------------------------------------------------------------------
wxString IBPPtype2string(Database *db, IBPP::SDT t, int subtype, int size,
    int scale)
{
    if (scale > 0)
        return wxString::Format(wxT("NUMERIC(%d,%d)"), size==4 ? 9:18, scale);
    if (t == IBPP::sdString)
    {
        int bpc = db->getCharsetById(subtype).getBytesPerChar();
        return wxString::Format(wxT("STRING(%d)"), bpc ? size/bpc : size);
    }
    switch (t)
    {
        case IBPP::sdArray:     return wxT("ARRAY");
        case IBPP::sdBlob:      return wxString::Format(
                                    wxT("BLOB SUB_TYPE %d"), subtype);
        case IBPP::sdDate:      return wxT("DATE");
        case IBPP::sdTime:      return wxT("TIME");
        case IBPP::sdTimestamp: return wxT("TIMESTAMP");
        case IBPP::sdSmallint:  return wxT("SMALLINT");
        case IBPP::sdInteger:   return wxT("INTEGER");
        case IBPP::sdLargeint:  return wxT("BIGINT");
        case IBPP::sdFloat:     return wxT("FLOAT");
        case IBPP::sdDouble:    return wxT("DOUBLE PRECISION");
        default:                return wxT("UNKNOWN");
    }
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::compareCounts(IBPP::DatabaseCounts& one,
    IBPP::DatabaseCounts& two)
{
    for (IBPP::DatabaseCounts::iterator it = two.begin(); it != two.end();
        ++it)
    {
        wxString s;
        IBPP::DatabaseCounts::iterator i2 = one.find((*it).first);
        IBPP::CountInfo c;
        IBPP::CountInfo& r1 = (*it).second;
        IBPP::CountInfo& r2 = c;
        if (i2 != one.end())
            r2 = (*i2).second;
        if (r1.inserts > r2.inserts)
            s += wxString::Format(_("%d inserts. "), r1.inserts - r2.inserts);
        if (r1.updates > r2.updates)
            s += wxString::Format(_("%d updates. "), r1.updates - r2.updates);
        if (r1.deletes > r2.deletes)
            s += wxString::Format(_("%d deletes. "), r1.deletes - r2.deletes);
        if (!s.IsEmpty())
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
                    std::string rel;
                    st->Get(1, rel);
                    relName = std2wx(rel).Strip();
                }
            }
            catch (...)
            {
            }
            if (relName.IsEmpty())
                relName.Format(_("Relation #%d"), (*it).first);
            log(relName + wxT(": ") + s, ttSql);
        }
    }
}
//-----------------------------------------------------------------------------
wxString millisToTimeString(long millis)
{
    if (millis >= 60 * 1000)
    {
        int hh = millis / (60 * 60 * 1000);
        millis -= 60 * 60 * 1000 * hh;
        int mm = millis / (60 * 1000);
        millis -= 60 * 1000 * mm;
        int ss = (millis + 500) / 1000;
        return wxString::Format(wxT("%d:%.2d:%.2d (hh:mm:ss)"), hh, mm, ss);
    }
    else
        return wxString::Format(wxT("%.3fs"), 0.001 * millis);
}
//-----------------------------------------------------------------------------
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
        log(_("Parsed query: " + sql), ttSql);
        log(_("Empty statement detected, bailing out..."));
        return true;
    }

    if (styled_text_ctrl_sql->AutoCompActive())
        styled_text_ctrl_sql->AutoCompCancel();    // remove the list if needed
    notebook_1->SetSelection(0);
    wxStopWatch stopwatch;
    bool retval = true;

    try
    {
        if (!inTransactionM)
        {
            log(_("Starting transaction..."));
            transactionM->Start();
            inTransaction(true);
        }

        int fetch1 = 0, mark1 = 0, read1 = 0, write1 = 0, ins1 = 0, upd1 = 0,
            del1 = 0, ridx1 = 0, rseq1 = 0, mem1 = 0;
        int fetch2, mark2, read2, write2, ins2, upd2, del2, ridx2, rseq2, mem2;
        IBPP::DatabaseCounts counts1, counts2;
        bool doShowStats = config().get(wxT("SQLEditorShowStats"), true);
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
        log(_("Preparing query: " + sql), ttSql);
        sae.scroll();
        statementM->Prepare(wx2std(sql, dbCharsetConversionM.getConverter()));
        log(wxString::Format(_("Prepare time: %s"),
            millisToTimeString(stopwatch.Time()).c_str()));

        // we don't check IBPP::Select since Firebird 2.0 has a new feature
        // INSERT ... RETURNING which isn't detected as stSelect by IBPP
        bool hasColumns = false;
        try
        {
            int cols = statementM->Columns();
            for (int i = 1; i <= cols; i++)
            {
                hasColumns = true;
                if (doShowStats)
                {
                    log(wxString::Format(
                        _("Field #%02d: %s.%s Alias:%s Type:%s"),
                        i,
                        std2wx(statementM->ColumnTable(i)).c_str(),
                        std2wx(statementM->ColumnName(i)).c_str(),
                        std2wx(statementM->ColumnAlias(i)).c_str(),
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

        try
        {
            std::string plan;            // for some statements (DDL) it is never available
            statementM->Plan(plan);      // for INSERTs, it is available sometimes (insert into ... select ... )
            log(std2wx(plan));           // but if it not, IBPP throws the exception
        }
        catch(IBPP::Exception&)
        {
            log(_("Plan not available."));
        }

        if (prepareOnly)
            return true;

        log(wxEmptyString);
        log(wxEmptyString);
        log(_("Executing..."));
        sae.scroll();
        statementM->Execute();
        log(_("Done."));

        IBPP::STT type = statementM->Type();
        if (hasColumns)            // for select statements: show data
        {
            grid_data->fetchData(dbCharsetConversionM.getConverter());
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
            wxString::size_type p = sql.find_first_not_of(wxT(" \n\t\r"));
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
                        addon = wxT("s");
                    wxString s = wxString::Format(_("%d row%s affected directly."),
                        statementM->AffectedRows(), addon.c_str());
                    log(wxT("") + s);
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
    catch (IBPP::Exception &e)
    {
        splitScreen();
        log(std2wx(e.ErrorMessage()) + wxT("\n"), ttError);
        retval = false;
    }
    catch (...)
    {
        splitScreen();
        log(_("SYSTEM ERROR!"), ttError);
        retval = false;
    }

    log(wxString::Format(_("Total execution time: %s"),
        millisToTimeString(stopwatch.Time()).c_str()));
    return retval;
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::splitScreen()
{
    if (!splitter_window_1->IsSplit()) // split screen if needed
    {
        splitter_window_1->SplitHorizontally(styled_text_ctrl_sql, notebook_1);
        ::wxYield();
    }
}
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
bool ExecuteSqlFrame::commitTransaction()
{
    wxBusyCursor cr;
    ScrollAtEnd sae(styled_text_ctrl_stats);

    // grid_data->stopFetching();
    if (!transactionM->Started())    // check
    {
        inTransaction(false);
        return true;    // nothing to commit, but it wasn't error
    }

    try
    {
        log(_("Commiting transaction..."));
        sae.scroll();
        transactionM->Commit();
        log(_("Done."));
        statusbar_1->SetStatusText(_("Transaction commited"), 3);
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
        log(std2wx(e.ErrorMessage()), ttError);
        return false;
    }
    catch (std::exception &se)
    {
        splitScreen();
        log(wxString(_("ERROR!\n")) + std2wx(se.what()), ttError);
        return false;
    }

    notebook_1->SetSelection(0);

    // apparently is has to be at the end to have any effect
    setViewMode(vmEditor);
    return true;
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuRollback(wxCommandEvent& WXUNUSED(event))
{
    wxBusyCursor cr;
    // see comments for OnMenuCommit to learn why this temp. variable is needed
    bool closeIt = closeWhenTransactionDoneM;
    if (rollbackTransaction() && !closeIt)
        setViewMode(false, vmEditor);
}
//-----------------------------------------------------------------------------
bool ExecuteSqlFrame::rollbackTransaction()
{
    ScrollAtEnd sae(styled_text_ctrl_stats);

    // grid_data->stopFetching();
    if (!transactionM->Started())    // check
    {
        executedStatementsM.clear();
        inTransaction(false);
        return true;
    }

    try
    {
        log(_("Rolling back the transaction..."));
        sae.scroll();
        transactionM->Rollback();
        log(_("Done."));
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
        log(std2wx(e.ErrorMessage()), ttError);
        return false;
    }
    catch (...)
    {
        splitScreen();
        log(_("ERROR!\nA non-IBPP C++ runtime exception occured !"), ttError);
        return false;
    }

    notebook_1->SetSelection(0);
    setViewMode(vmEditor);
    return true;
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUpdateGridInsertRow(wxUpdateUIEvent& event)
{
    DataGridTable* tb = grid_data->getDataGridTable();
    event.Enable(inTransactionM && tb && tb->canInsertRows());
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUpdateGridHasData(wxUpdateUIEvent& event)
{
    event.Enable(grid_data->getDataGridTable()
        && grid_data->GetNumberRows());
}
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnGridRowCountChanged(wxCommandEvent& event)
{
    wxString s;
    long rowsFetched = event.GetExtraLong();
    s.Printf(_("%d rows fetched"), rowsFetched);
    statusbar_1->SetStatusText(s, 1);

    // TODO: we could make some bool flag, so that this happens only once per execute()
    //       to fix the problem when user does the select, unsplits the window
    //       and then browses the grid, which fetches more records and unsplits again
    if (!splitter_window_1->IsSplit())    // already ok
        return;
    bool selectMaximizeGrid = false;
    config().getValue(wxT("SelectMaximizeGrid"), selectMaximizeGrid);
    if (selectMaximizeGrid)
    {
        int rowsNeeded = 10;    // default
        config().getValue(wxT("MaximizeGridRowsNeeded"), rowsNeeded);
        if (rowsFetched >= rowsNeeded)
        {
            //splitScreen();    // not needed atm, might be later (see TODO above)
            setViewMode(false, vmGrid);
        }
    }
}
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnGridLabelLeftDClick(wxGridEvent& event)
{
    DataGridTable* table = grid_data->getDataGridTable();
    if (!table)
        return;

    int column = 1 + event.GetCol();
    if (column < 1 || column > table->GetNumberCols())
        return;
    SelectStatement sstm(std2wx(statementM->Sql()));

    // rebuild SQL statement with different ORDER BY clause
    sstm.orderBy(column);

    execute(sstm.getStatement(), wxEmptyString);
}
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::update()
{
    if (!databaseM->isConnected())
        Close();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::setDatabase(Database* db)
{
    databaseM = db;
    dbCharsetConversionM.setConnectionCharset(db->getConnectionCharset());

    wxString s = wxString::Format(wxT("%s@%s:%s"), db->getUsername().c_str(),
        db->getServer()->getName_().c_str(), db->getPath().c_str());
    // doesn't seem to work properly as wxToolbar overwrites it
    //statusbar_1->PushStatusText(s, 0);
    statusbar_1->SetStatusText(s, 0);

    transactionM = IBPP::TransactionFactory(databaseM->getIBPPDatabase());
    db->attachObserver(this);    // observe database object

    executedStatementsM.clear();
    inTransaction(false);    // enable/disable controls
    setKeywords();           // set words for autocomplete feature

    historyPositionM = StatementHistory::get(databaseM).size();

    // set drop target for DnD
    styled_text_ctrl_sql->SetDropTarget(
        new SqlEditorDropTarget(this, styled_text_ctrl_sql, databaseM));
}
//-----------------------------------------------------------------------------
//! closes window if database is removed (unregistered)
void ExecuteSqlFrame::removeSubject(Subject* subject)
{
    Observer::removeSubject(subject);
    if (subject == databaseM)
        Close();
}
//-----------------------------------------------------------------------------
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

    keywordsM.Empty();                          // create final wxString from array
    keywordsM.Alloc(20480);     // preallocate 20kB
    for (size_t i = 0; i < as.GetCount(); ++i)  // separate words with spaces
        keywordsM += as.Item(i) + wxT(" ");
}
//-----------------------------------------------------------------------------
//! logs all activity to text control
// this is made a separate function, so we can change the control to any other
// or we can also log to some .txt file, etc.
void ExecuteSqlFrame::log(wxString s, TextType type)
{
    int startpos = styled_text_ctrl_stats->GetLength();
    styled_text_ctrl_stats->SetCurrentPos(startpos);
    styled_text_ctrl_stats->AddText(s + wxT("\n"));
    int endpos = styled_text_ctrl_stats->GetLength();

    int style = 0;
    if (type == ttError)
        style = 1;
    if (type == ttSql)
        style = 2;

    styled_text_ctrl_stats->StartStyling(startpos, 255);
    styled_text_ctrl_stats->SetStyling(endpos-startpos-1, style);
}
//-----------------------------------------------------------------------------
const wxString ExecuteSqlFrame::getName() const
{
    return wxT("ExecuteSqlFrame");
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::doReadConfigSettings(const wxString& prefix)
{
    BaseFrame::doReadConfigSettings(prefix);
    int zoom;
    if (config().getValue(prefix + Config::pathSeparator + wxT("zoom"), zoom))
        styled_text_ctrl_sql->SetZoom(zoom);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::doWriteConfigSettings(const wxString& prefix) const
{
    BaseFrame::doWriteConfigSettings(prefix);
    config().setValue(prefix + Config::pathSeparator + wxT("zoom"),
        styled_text_ctrl_sql->GetZoom());
}
//-----------------------------------------------------------------------------
const wxRect ExecuteSqlFrame::getDefaultRect() const
{
    return wxRect(-1, -1, 528, 486);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::setViewMode(ViewMode mode)
{
    setViewMode(splitter_window_1->IsSplit(), mode);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::setViewMode(bool splitView, ViewMode mode)
{
    wxCHECK_RET(mode == vmEditor || mode == vmLogCtrl || mode == vmGrid,
        wxT("Try to set invalid view mode"));
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
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::updateViewMode()
{
    doUpdateFocusedControlM = false;

    wxWindow* focused = FindFocus();
    if (focused == styled_text_ctrl_sql)
        viewModeM = vmEditor;
    else if (focused == styled_text_ctrl_stats)
        viewModeM = vmLogCtrl;
    else if (focused == grid_data)
        viewModeM = vmGrid;
}
//-----------------------------------------------------------------------------
//! also used to drop constraints
class DropColumnHandler: public URIHandler
{
public:
    bool handleURI(URI& uri);
private:
    static const DropColumnHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const DropColumnHandler DropColumnHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool DropColumnHandler::handleURI(URI& uri)
{
    if (uri.action != wxT("drop_field") && uri.action != wxT("drop_constraint"))
        return false;

    MetadataItem* c = (MetadataItem*)getObject(uri);
    wxWindow* w = getWindow(uri);
    if (!c || !w)
        return true;

    Table *t = 0;
    if (uri.action == wxT("drop_field"))
    {
        Column *cp = dynamic_cast<Column *>(c);
        if (cp)
            t = cp->getTable();
    }
    else
    {
        Constraint *cs = dynamic_cast<Constraint *>(c);
        if (cs)
            t = cs->getTable();
    }
    if (!t)
        return true;

    wxString sql = wxT("ALTER TABLE ") + t->getQuotedName() + wxT(" DROP ");
    if (uri.action == wxT("drop_constraint"))
        sql += wxT("CONSTRAINT ");
    sql += c->getQuotedName();

    wxString msg(wxString::Format(
        _("Are you sure you wish to drop the %s %s?"),
        c->getTypeName().Lower().c_str(),
        c->getName_().c_str()));
    if (wxOK != showQuestionDialog(w, msg,
        _("Once you drop the object it is permanently removed from database."),
        AdvancedMessageDialogButtonsOkCancel(_("&Drop")),
        config(), wxT("DIALOG_ConfirmDrop"), _("Always drop without asking")))
    {
        return true;
    }
    execSql(w, _("Dropping field"), c->findDatabase(), sql, true);
    return true;
}
//-----------------------------------------------------------------------------
//! drop multiple columns
class DropColumnsHandler: public URIHandler
{
public:
    bool handleURI(URI& uri);
private:
    static const DropColumnsHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const DropColumnsHandler DropColumnsHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool DropColumnsHandler::handleURI(URI& uri)
{
    if (uri.action != wxT("drop_fields"))
        return false;

    Table* t = (Table*)getObject(uri);
    wxWindow* w = getWindow(uri);
    if (!t || !w)
        return true;

    // get list of columns
    wxString sql;
    std::vector<wxString> list;
    if (selectRelationColumnsIntoVector(t, w, list))
    {
        for (std::vector<wxString>::iterator it = list.begin(); it != list.end(); ++it)
        {
            Identifier temp(*it);
            sql += wxT("ALTER TABLE ") + t->getQuotedName() + wxT(" DROP ") + temp.getQuoted() + wxT(";\n");
        }
        execSql(w, _("Dropping fields"), t->findDatabase(), sql, true);
    }
    return true;
}
//-----------------------------------------------------------------------------
//! drop any metadata item
class DropObjectHandler: public URIHandler
{
public:
    bool handleURI(URI& uri);
private:
    static const DropObjectHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const DropObjectHandler DropObjectHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool DropObjectHandler::handleURI(URI& uri)
{
    if (uri.action != wxT("drop_object"))
        return false;

    MetadataItem* m = (MetadataItem*)getObject(uri);
    wxWindow* w = getWindow(uri);
    if (!m || !w)
        return true;

    wxString msg(wxString::Format(
        _("Are you sure you wish to drop the %s %s?"),
        m->getTypeName().Lower().c_str(),
        m->getName_().c_str()));
    if (wxOK != showQuestionDialog(w, msg,
        _("Once you drop the object it is permanently removed from database."),
        AdvancedMessageDialogButtonsOkCancel(_("&Drop")),
        config(), wxT("DIALOG_ConfirmDrop"), _("Always drop without asking")))
    {
        return true;
    }
    execSql(w, _("DROP"), m->findDatabase(), m->getDropSqlStatement(), true);
    return true;
}
//-----------------------------------------------------------------------------
//! show DDL in SQL editor
class EditDDLHandler: public URIHandler
{
public:
    bool handleURI(URI& uri);
private:
    static const EditDDLHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const EditDDLHandler EditDDLHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool EditDDLHandler::handleURI(URI& uri)
{
    if (uri.action != wxT("edit_ddl"))
        return false;

    MetadataItem* m = (MetadataItem*)getObject(uri);
    wxWindow* w = getWindow(uri);
    if (!m || !w)
        return true;

    // use a single read-only transaction for metadata loading
    Database* d = m->getDatabase(wxT("EditDDLHandler::handleURI"));
    MetadataLoaderTransaction tr((d) ? d->getMetadataLoader() : 0);

    ProgressDialog pd(w, _("Extracting DDL Definitions"), 2);
    CreateDDLVisitor cdv(&pd);
    m->acceptVisitor(&cdv);
    if (pd.isCanceled())
        return true;

    ExecuteSqlFrame* eff = new ExecuteSqlFrame(wxTheApp->GetTopWindow(), -1,
        wxT("DDL"), m->findDatabase());
    eff->setSql(cdv.getSql());
    // ProgressDialog needs to be hidden before ExecuteSqlFrame is shown,
    // otherwise the HTML frame will be raised over the ExecuteSqlFrame
    // when original Z-order is restored after pd has been destroyed
    pd.Hide();
    eff->Show();
    return true;
}
//-----------------------------------------------------------------------------
class EditProcedureHandler: public URIHandler
{
public:
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const EditProcedureHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const EditProcedureHandler EditProcedureHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool EditProcedureHandler::handleURI(URI& uri)
{
    if (uri.action != wxT("edit_procedure"))
        return false;

    Procedure* p = (Procedure*)getObject(uri);
    wxWindow* w = getWindow(uri);
    if (!p || !w)
        return true;

    CreateDDLVisitor cdv;
    p->acceptVisitor(&cdv);
    showSql(w->GetParent(), _("Editing stored procedure"), p->findDatabase(),
        cdv.getSuffixSql());
    return true;
}
//-----------------------------------------------------------------------------
class AlterViewHandler: public URIHandler
{
public:
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const AlterViewHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const AlterViewHandler AlterViewHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool AlterViewHandler::handleURI(URI& uri)
{
    if (uri.action != wxT("alter_relation"))
        return false;

    Relation* r = (Relation*)getObject(uri);
    wxWindow* w = getWindow(uri);
    if (!r || !w)
        return true;

    showSql(w->GetParent(), _("Altering view"), r->findDatabase(),
        r->getRebuildSql());
    return true;
}
//-----------------------------------------------------------------------------
class EditTriggerHandler: public URIHandler
{
public:
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const EditTriggerHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const EditTriggerHandler EditTriggerHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool EditTriggerHandler::handleURI(URI& uri)
{
    if (uri.action != wxT("edit_trigger"))
        return false;

    Trigger* t = (Trigger*)getObject(uri);
    wxWindow* w = getWindow(uri);
    if (!t || !w)
        return true;

    showSql(w->GetParent(), _("Editing trigger"), t->findDatabase(),
        t->getAlterSql());
    return true;
}
//-----------------------------------------------------------------------------
class EditGeneratorValueHandler: public URIHandler
{
public:
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const EditGeneratorValueHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const EditGeneratorValueHandler EditGeneratorValueHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool EditGeneratorValueHandler::handleURI(URI& uri)
{
    if (uri.action != wxT("edit_generator_value"))
        return false;

    Generator* g = (Generator*)getObject(uri);
    wxWindow* w = getWindow(uri);
    if (!g || !w)
        return true;

    g->loadValue();
    int64_t oldvalue = g->getValue();
    Database* db = g->getDatabase(wxT("EditGeneratorValueHandler::handleURI"));

    wxString value = wxGetTextFromUser(_("Changing generator value"), _("Enter new value"),
#ifndef wxLongLong
    // MH: I have no idea if this works on all systems... but it should be better
    // MB: we'll use wxLongLong wherever it is available
        wxLongLong(oldvalue).ToString(), w);
#else
        wxString::Format(wxT("%d"), oldvalue), w);
#endif

    if (value != wxT(""))
    {
        wxString sql = wxT("SET GENERATOR ") + g->getQuotedName() + wxT(" TO ") + value + wxT(";");
        execSql(w, sql, db, sql, true);
    }
    return true;
}
//-----------------------------------------------------------------------------
class EditExceptionHandler: public URIHandler
{
public:
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const EditExceptionHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const EditExceptionHandler EditExceptionHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool EditExceptionHandler::handleURI(URI& uri)
{
    if (uri.action != wxT("edit_exception"))
        return false;

    Exception* e = (Exception*)getObject(uri);
    wxWindow* w = getWindow(uri);
    if (!e || !w)
        return true;

    showSql(w->GetParent(), _("Editing exception"), e->findDatabase(),
        e->getAlterSql());
    return true;
}
//-----------------------------------------------------------------------------
class IndexActionHandler: public URIHandler
{
public:
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const IndexActionHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const IndexActionHandler IndexActionHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool IndexActionHandler::handleURI(URI& uri)
{
    if (uri.action != wxT("index_action"))
        return false;

    Index* i = (Index*)getObject(uri);
    wxWindow* w = getWindow(uri);
    if (!i || !w)
        return true;

    wxString sql;
    wxString type = uri.getParam(wxT("type"));        // type of operation
    if (type == wxT("DROP"))
    {
        wxString msg(wxString::Format(
            _("Are you sure you wish to drop the index %s?"),
            i->getName_().c_str()));
        if (wxOK != showQuestionDialog(w, msg,
            _("Once you drop the object it is permanently removed from database."),
            AdvancedMessageDialogButtonsOkCancel(_("&Drop")),
            config(), wxT("DIALOG_ConfirmDrop"), _("Always drop without asking")))
        {
            return true;
        }
        sql = wxT("DROP INDEX ") + i->getQuotedName();
    }
    else if (type == wxT("RECOMPUTE"))
        sql = wxT("SET STATISTICS INDEX ") + i->getQuotedName();
    else if (type == wxT("TOGGLE_ACTIVE"))
        sql = wxT("ALTER INDEX ") + i->getQuotedName() + (i->isActive() ? wxT(" INACTIVE") : wxT(" ACTIVE"));

    execSql(w, wxEmptyString, i->findDatabase(), sql, true);
    return true;
}
//-----------------------------------------------------------------------------
class ActivateTriggersHandler: public URIHandler
{
public:
    bool handleURI(URI& uri);
private:
    static const ActivateTriggersHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const ActivateTriggersHandler ActivateTriggersHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool ActivateTriggersHandler::handleURI(URI& uri)
{
    if (uri.action != wxT("activate_triggers")
        && uri.action != wxT("deactivate_triggers"))
    {
        return false;
    }

    MetadataItem *m = (MetadataItem *)getObject(uri);
    Table* t = dynamic_cast<Table*>(m);
    Database* d = dynamic_cast<Database*>(m);
    wxWindow* w = getWindow(uri);
    if ((!t && !d) || !w)
        return true;

    std::vector<Trigger*> list;
    if (t)
    {
        t->getTriggers(list, Trigger::afterTrigger);
        t->getTriggers(list, Trigger::beforeTrigger);
    }
    else
        d->getDatabaseTriggers(list);
    wxString sql;
    for (std::vector<Trigger*>::iterator it = list.begin(); it != list.end();
        ++it)
    {
        sql += wxT("ALTER TRIGGER ") + (*it)->getQuotedName() + wxT(" ");
        if (uri.action == wxT("deactivate_triggers"))
            sql += wxT("IN");
        sql += wxT("ACTIVE;\n");
    }

    execSql(w, wxEmptyString, d ? d : t->findDatabase(), sql, true);
    return true;
}
//-----------------------------------------------------------------------------
class ActivateTriggerHandler: public URIHandler
{
public:
    bool handleURI(URI& uri);
private:
    static const ActivateTriggerHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const ActivateTriggerHandler ActivateTriggerHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool ActivateTriggerHandler::handleURI(URI& uri)
{
    if (uri.action != wxT("activate_trigger") && uri.action != wxT("deactivate_trigger"))
        return false;

    Trigger* t = (Trigger*)getObject(uri);
    wxWindow* w = getWindow(uri);
    if (!t || !w)
        return true;

    wxString sql = wxT("ALTER TRIGGER ") + t->getQuotedName() + wxT(" ");
    if (uri.action == wxT("deactivate_trigger"))
        sql += wxT("IN");
    sql += wxT("ACTIVE;\n");

    execSql(w, wxEmptyString, t->findDatabase(), sql, true);
    return true;
}
//-----------------------------------------------------------------------------
