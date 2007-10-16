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
#include <wx/datetime.h>
#include <wx/dnd.h>
#include <wx/file.h>
//#include <wx/fontmap.h>
#include <wx/fontdlg.h>
#include <wx/tokenzr.h>

#include <algorithm>
#include <map>
#include <sstream>
#include <vector>

#include "config/Config.h"
#include "core/ArtProvider.h"
#include "core/FRError.h"
#include "core/StringUtils.h"
#include "framemanager.h"
#include "gui/AdvancedMessageDialog.h"
#include "gui/CommandIds.h"
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
#include "sql/SimpleParser.h"
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
#if wxCHECK_VERSION(2, 8, 0)
    dataobj->Add(fileDataM = new wxFileDataObject);
#endif
    dataobj->Add(textDataM = new wxTextDataObject);
    SetDataObject(dataobj);
}
//-----------------------------------------------------------------------------
wxDragResult SqlEditorDropTarget::OnData(wxCoord x, wxCoord y,
    wxDragResult def)
{
    if (!GetData())
        return wxDragNone;

#if wxCHECK_VERSION(2, 8, 0)
    wxDataObjectComposite* dataobj = (wxDataObjectComposite*) m_dataObject;
    // test for wxDF_FILENAME
    if (dataobj->GetReceivedFormat() == fileDataM->GetFormat())
    {
        if (OnDropFiles(x, y, fileDataM->GetFilenames()))
            return def;
    }
    else
#endif
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

    Database *db = m->getDatabase();
    if (db != databaseM)
    {
        wxMessageBox(_("Cannot use objects from different databases."), _("Wrong database."), wxOK | wxICON_WARNING);
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
        column_list = t->getQuotedName() + wxT(".*");
    }
    if (t == 0)
    {
        wxMessageBox(_("Only tables and table columns can be dropped."), _("Object type not supported."), wxOK | wxICON_WARNING);
        return false;
    }

    // setup complete. now the actual stuff:
    wxString sql = editorM->GetText().Upper();

    // currently we don't support having comments and quotes (it's complicated)
    //if (!SimpleParser::stripSql(sql))
    //    return true;

    wxString::size_type psel, pfrom;
    psel = sql.find(wxT("SELECT"));
    if (psel == wxString::npos)                            // simple select statement
    {
        sql = wxT("SELECT ") + column_list + wxT("\nFROM ") + t->getQuotedName();
        editorM->SetText(sql);
        return true;
    }

    pfrom = sql.find(wxT("FROM"), psel);
    if (pfrom == wxString::npos)
    {
        wxMessageBox(_("SELECT present, but FROM missing."), _("Unable to parse the statement"), wxOK | wxICON_WARNING);
        return true;
    }

    // read in the table names, and find position where FROM clause ends
    std::vector<wxString> tableNames;
    wxString::size_type from_end = pfrom + SimpleParser::getTableNames(tableNames, sql.substr(pfrom));

    // if table is not there, add it
    if (std::find(tableNames.begin(), tableNames.end(), t->getName_()) == tableNames.end())
    {
        std::vector<ForeignKey> relatedTables;
        if (Table::tablesRelate(tableNames, t, relatedTables))    // foreign keys
        {
            wxArrayString as;
            for (std::vector<ForeignKey>::iterator it = relatedTables.begin(); it != relatedTables.end(); ++it)
            {
                wxString addme = (*it).referencedTableM + wxT(":  ") + (*it).getJoin(false); // false = unquoted
                if (as.Index(addme) == wxNOT_FOUND)
                    as.Add(addme);
            }
            wxString join_list;
            if (as.GetCount() > 1)    // let the user decide
            {

                int selected = ::wxGetSingleChoiceIndex(_("Multiple foreign keys found"),
                    _("Select the desired table"), as);
                if (selected == -1)
                    return false;
                join_list = relatedTables[selected].getJoin(true); // true = quoted
            }
            else
                join_list = relatedTables[0].getJoin(true);

            // FIXME: dummy test value
            // can_be_null = (check if any of the FK fields can be null)
            bool can_be_null = true;

            wxString insert = (can_be_null ? wxT(" LEFT") : wxT(""));
            insert += wxT(" JOIN ") + t->getQuotedName() + wxT(" ON ") + join_list;
            insert = wxT("\n") + insert + wxT(" ");
            sql.insert(from_end, insert);
        }
        else
        {
            sql.insert(pfrom + 5, wxT(" "));
            if (!tableNames.empty())
                sql.insert(pfrom + 5, wxT(","));
            sql.insert(pfrom + 5, t->getQuotedName());
        }
    }

    // add columns to SELECT. Possible solutions include either psel+8 or pfrom + 1. I picked pfrom + 1.
    sql.insert(pfrom, wxT(",\n"));
    sql.insert(pfrom+1, column_list);

    editorM->SetText(sql);
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
    SetKeyWords(0, Identifier::getKeywords(true));    // true = lower_case
    wxString chars(wxT("_0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz\"$"));
    // see Document::SetDefaultCharClasses in stc/Document.cxx
    for (int ch = 0x80; ch < 0x0100; ch++)
        if (isalnum(ch))
            chars += wxChar(ch);
    SetWordChars(chars);

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

    m.AppendSeparator();
    m.Append(wxID_REPLACE,          _("&Find and replace"));
    m.Append(Cmds::View_Set_editor_font, _("Set F&ont"));
    m.AppendCheckItem(Cmds::View_Wrap_long_lines, _("&Wrap"));
    if (wxSTC_WRAP_WORD == GetWrapMode())
        m.Check(Cmds::View_Wrap_long_lines, true);

    // disable stuff
    m.Enable(wxID_UNDO, CanUndo());
    m.Enable(wxID_REDO, CanRedo());
    if (slen == 0)        // nothing is selected
    {
        m.Enable(wxID_CUT,              false);
        m.Enable(wxID_COPY,             false);
        m.Enable(wxID_DELETE,           false);
        m.Enable(Cmds::Query_Execute_selection, false);
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
ExecuteSqlFrame::ExecuteSqlFrame(wxWindow* parent, int id, wxString title,
        Database *db, const wxPoint& pos, const wxSize& size, long style)
    :BaseFrame(parent, id, title, pos, size, style), Observer(), databaseM(db)
{
    loadingM = true;
    buildToolbar();

    panel_contents = new wxPanel(this, -1);
    splitter_window_1 = new wxSplitterWindow(panel_contents, -1);
    panel_splitter_bottom = new wxPanel(splitter_window_1, -1);
    notebook_1 = new wxNotebook(panel_splitter_bottom, -1, wxDefaultPosition, wxDefaultSize, 0);
    notebook_pane_1 = new wxPanel(notebook_1, -1);
    notebook_pane_2 = new wxPanel(notebook_1, -1);
    panel_splitter_top = new wxPanel(splitter_window_1, -1);
    statusbar_1 = CreateStatusBar(4);
    styled_text_ctrl_sql = new SqlEditor(panel_splitter_top, ID_stc_sql);
    styled_text_ctrl_stats = new wxStyledTextCtrl(notebook_pane_1, -1);
    styled_text_ctrl_stats->SetWrapMode(wxSTC_WRAP_WORD);
    styled_text_ctrl_stats->StyleSetForeground(1, *wxRED);
    styled_text_ctrl_stats->StyleSetForeground(2, *wxBLUE);

    set_properties();
    do_layout();
    setDatabase(db);    // must come after set_properties
    loadingM = false;
    buildMainMenu();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::buildToolbar()
{
    //toolBarM = CreateToolBar( wxTB_FLAT|wxTB_HORIZONTAL|wxTB_TEXT, wxID_ANY );
    toolBarM = CreateToolBar( wxTB_FLAT|wxTB_HORIZONTAL, wxID_ANY );

#ifdef __WXGTK20__
    wxSize bmpSize(24, 24);
#else
    wxSize bmpSize(16, 16);
#endif
	toolBarM->SetToolBitmapSize(bmpSize);

    toolBarM->AddTool( wxID_NEW, _("New"),
        wxArtProvider::GetBitmap(wxART_NEW, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL, _("New window"), _("New window") );
    toolBarM->AddTool( wxID_OPEN, _("Open"),
        wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL, _("Load a file"), _("Load a file") );
    toolBarM->AddTool( wxID_SAVE, _("Save"),
        wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL,  _("Save to file"), _("Save to file") );
    toolBarM->AddTool( wxID_SAVEAS, _("Save as"),
        wxArtProvider::GetBitmap(wxART_FILE_SAVE_AS, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL, _("Save under different name"), _("Save under different name") );
    toolBarM->AddSeparator();

    toolBarM->AddTool( wxID_BACKWARD, _("Back"),
        wxArtProvider::GetBitmap(wxART_GO_BACK, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL, _("Go to previous statement"), _("Go to previous statement") );
    toolBarM->AddTool( wxID_FORWARD, _("Next"),
        wxArtProvider::GetBitmap(wxART_GO_FORWARD, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL, _("Go to next statement"), _("Go to next statement") );
    toolBarM->AddTool( Cmds::History_Search, _("History"),
        wxArtProvider::GetBitmap(ART_History, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL, _("Browse and search statement history"), _("Browse and search statement history") );
    toolBarM->AddSeparator();

    toolBarM->AddTool( Cmds::Query_Execute, _("Execute"),
        wxArtProvider::GetBitmap(ART_ExecuteStatement, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL, _("F4 - Execute statement(s)"), _("F4 - Execute statement(s)") );
    toolBarM->AddTool( Cmds::Query_Show_plan, _("Show plan"),
        wxArtProvider::GetBitmap(ART_ShowExecutionPlan, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL, _("Show query execution plan"), _("Show query execution plan") );
    toolBarM->AddTool( Cmds::Query_Commit, _("Commit"),
        wxArtProvider::GetBitmap(ART_CommitTransaction, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL, _("F5 - Commit transaction"), _("F5 - Commit transaction") );
    toolBarM->AddTool( Cmds::Query_Rollback, _("Rollback"),
         wxArtProvider::GetBitmap(ART_RollbackTransaction, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL, _("F8 - Rollback transaction"), _("F8 - Rollback transaction") );
    toolBarM->AddSeparator();

    toolBarM->AddTool( Cmds::DataGrid_Insert_row, _("Insert row"),
        wxArtProvider::GetBitmap(ART_InsertRow, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL, _("Insert a new row in recordset"), _("Insert a new row in recordset") );
    toolBarM->AddTool( Cmds::DataGrid_Delete_row, _("Delete row"),
        wxArtProvider::GetBitmap(ART_DeleteRow, wxART_TOOLBAR, bmpSize), wxNullBitmap,
        wxITEM_NORMAL, _("Delete row(s) from recordset"), _("Delete row(s) from recordset") );

    toolBarM->Realize();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::buildMainMenu()
{
    menuBarM = new wxMenuBar();

    wxMenu* fileMenu = new wxMenu();                    // dynamic menus, created at runtime
    fileMenu->Append(wxID_NEW,      _("&New..."));
    fileMenu->Append(wxID_OPEN,     _("&Open..."));
    fileMenu->Append(wxID_SAVE,     _("&Save"));
    fileMenu->Append(wxID_SAVEAS,   _("Save &As..."));
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_CLOSE,    _("&Close"));
    menuBarM->Append(fileMenu, _("&File"));

    wxMenu* editMenu = new wxMenu();
    editMenu->Append(wxID_UNDO,         _("&Undo"));
    editMenu->Append(wxID_REDO,         _("&Redo"));
    editMenu->AppendSeparator();
    editMenu->Append(wxID_CUT,          _("Cu&t"));
    editMenu->Append(wxID_COPY,         _("&Copy"));
    editMenu->Append(wxID_PASTE,        _("&Paste"));
    editMenu->Append(wxID_DELETE,       _("&Delete"));
    editMenu->AppendSeparator();
    editMenu->Append(wxID_SELECTALL,    _("Select &all"));
    editMenu->AppendSeparator();
    editMenu->Append(wxID_REPLACE,      _("Fi&nd and Replace"));
    menuBarM->Append(editMenu, _("&Edit"));

    wxMenu* viewMenu = new wxMenu();
    viewMenu->AppendRadioItem(Cmds::View_Editor,          _("Edit&or"));
    viewMenu->AppendRadioItem(Cmds::View_Statistics,      _("&Statistics"));
    viewMenu->AppendRadioItem(Cmds::View_Data,            _("&Data"));
    viewMenu->AppendRadioItem(Cmds::View_Split_view,      _("Split &view"));
    viewMenu->AppendSeparator();
    viewMenu->Append(Cmds::View_Set_editor_font, _("Se&t editor font"));
    viewMenu->AppendSeparator();
    viewMenu->Append(Cmds::View_Focus_editor,    _("Focus &editor"));
    viewMenu->Append(Cmds::View_Focus_grid,      _("Focus &grid"));
    viewMenu->AppendSeparator();
    viewMenu->AppendCheckItem(Cmds::View_Wrap_long_lines,    _("&Wrap long lines"));
    menuBarM->Append(viewMenu, _("&View"));

    wxMenu* historyMenu = new wxMenu();
    historyMenu->Append(wxID_FORWARD,   _("&Next"));
    historyMenu->Append(wxID_BACKWARD,  _("&Previous"));
    historyMenu->Append(Cmds::History_Search,    _("&Search"));
    menuBarM->Append(historyMenu, _("&History"));

    wxMenu* queryMenu = new wxMenu();
    queryMenu->Append(Cmds::Query_Execute,             _("&Execute"));
    queryMenu->Append(Cmds::Query_Show_plan,           _("Show &plan"));
    queryMenu->Append(Cmds::Query_Execute_selection,   _("Execute &selection"));
    queryMenu->Append(Cmds::Query_Execute_from_cursor, _("Exec&ute from cursor"));
    queryMenu->AppendSeparator();
    queryMenu->Append(Cmds::Query_Commit,             _("&Commit"));
    queryMenu->Append(Cmds::Query_Rollback,           _("&Rollback"));
    menuBarM->Append(queryMenu, _("&Query"));

    wxMenu* gridMenu = new wxMenu();
    gridMenu->Append(Cmds::DataGrid_Insert_row,      _("I&nsert row"));
    gridMenu->Append(Cmds::DataGrid_Delete_row,      _("&Delete row"));
    gridMenu->AppendSeparator();
    gridMenu->Append(Cmds::DataGrid_Copy,            _("&Copy"));
    gridMenu->Append(Cmds::DataGrid_Copy_as_insert,  _("Copy &as insert statements"));
    gridMenu->Append(Cmds::DataGrid_Copy_as_update,  _("Copy as &update statements"));
    gridMenu->AppendSeparator();
    gridMenu->Append(Cmds::DataGrid_FetchAll,        _("&Fetch all records"));
    gridMenu->Append(Cmds::DataGrid_CancelFetchAll,  _("&Stop fetching all records"));
    gridMenu->AppendSeparator();
    gridMenu->Append(Cmds::DataGrid_Save_as_html,    _("Save as &html"));
    gridMenu->Append(Cmds::DataGrid_Save_as_csv,     _("Save as cs&v"));
    gridMenu->AppendSeparator();
    gridMenu->Append(Cmds::DataGrid_Set_header_font, _("Set h&eader font"));
    gridMenu->Append(Cmds::DataGrid_Set_cell_font,   _("Set cell f&ont"));
    menuBarM->Append(gridMenu, _("&Grid"));

    SetMenuBar(menuBarM);
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
    grid_data = new DataGrid(notebook_pane_2, ID_grid_data);
    grid_data->SetTable(new DataGridTable(statementM, databaseM), true);
    splitter_window_1->SplitHorizontally(panel_splitter_top, panel_splitter_bottom);
    splitter_window_1->Unsplit();

    SetIcon(wxArtProvider::GetIcon(ART_ExecuteSqlFrame, wxART_FRAME_ICON));

    keywordsM = wxT("");
    closeWhenTransactionDoneM = false;
    autoCommitM = config().get(wxT("autoCommitDDL"), false);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::do_layout()
{
    // begin wxGlade: ExecuteSqlFrame::do_layout
    wxBoxSizer* sizer_1 = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* sizer_2 = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* sizer_5 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizer_7 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizer_6 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizer_4 = new wxBoxSizer(wxHORIZONTAL);
    sizer_4->Add(styled_text_ctrl_sql, 1, wxEXPAND, 0);
    panel_splitter_top->SetAutoLayout(true);
    panel_splitter_top->SetSizer(sizer_4);
    sizer_4->Fit(panel_splitter_top);
    sizer_4->SetSizeHints(panel_splitter_top);
    sizer_6->Add(styled_text_ctrl_stats, 1, wxEXPAND, 0);
    notebook_pane_1->SetAutoLayout(true);
    notebook_pane_1->SetSizer(sizer_6);
    sizer_6->Fit(notebook_pane_1);
    sizer_6->SetSizeHints(notebook_pane_1);
    sizer_7->Add(grid_data, 1, wxEXPAND, 0);
    notebook_pane_2->SetAutoLayout(true);
    notebook_pane_2->SetSizer(sizer_7);
    sizer_7->Fit(notebook_pane_2);
    sizer_7->SetSizeHints(notebook_pane_2);
    notebook_1->AddPage(notebook_pane_1, _("Statistics"));
    notebook_1->AddPage(notebook_pane_2, _("Data"));
    sizer_5->Add(/* new wxNotebookSizer(notebook_1)*/ notebook_1, 1, wxEXPAND, 0);
    panel_splitter_bottom->SetAutoLayout(true);
    panel_splitter_bottom->SetSizer(sizer_5);
    sizer_5->Fit(panel_splitter_bottom);
    sizer_5->SetSizeHints(panel_splitter_bottom);
    sizer_2->Add(splitter_window_1, 1, wxEXPAND, 0);
    panel_contents->SetAutoLayout(true);
    panel_contents->SetSizer(sizer_2);
    sizer_2->Fit(panel_contents);
    sizer_2->SetSizeHints(panel_contents);
    sizer_1->Add(panel_contents, 1, wxEXPAND, 0);
    SetAutoLayout(true);
    SetSizer(sizer_1);
    Layout();

    styled_text_ctrl_sql->SetFocus();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::showProperties(wxString objectName)
{
    MetadataItem *m = databaseM->findByName(objectName);
    if (!m)
        m = databaseM->findByName(objectName.Upper());

    if (m)
    {
        frameManager().showMetadataPropertyFrame(GetParent(), m);
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
    EVT_STC_START_DRAG(ExecuteSqlFrame::ID_stc_sql, ExecuteSqlFrame::OnSqlEditStartDrag)
    EVT_CHAR_HOOK(ExecuteSqlFrame::OnKeyDown)
    EVT_CLOSE(ExecuteSqlFrame::OnClose)

    EVT_MENU(wxID_NEW,      ExecuteSqlFrame::OnMenuNew)
    EVT_MENU(wxID_OPEN,     ExecuteSqlFrame::OnMenuOpen)
    EVT_MENU(wxID_SAVE,     ExecuteSqlFrame::OnMenuSave)
    EVT_MENU(wxID_SAVEAS,   ExecuteSqlFrame::OnMenuSaveAs)
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

    EVT_MENU(Cmds::View_Editor,          ExecuteSqlFrame::OnMenuEditor)
    EVT_MENU(Cmds::View_Statistics,      ExecuteSqlFrame::OnMenuStatistics)
    EVT_MENU(Cmds::View_Data,            ExecuteSqlFrame::OnMenuData)
    EVT_MENU(Cmds::View_Split_view,      ExecuteSqlFrame::OnMenuSplitView)
    EVT_MENU(Cmds::View_Set_editor_font, ExecuteSqlFrame::OnMenuSetEditorFont)
    EVT_MENU(Cmds::View_Wrap_long_lines, ExecuteSqlFrame::OnMenuToggleWrap)
    EVT_MENU(Cmds::View_Focus_editor,    ExecuteSqlFrame::OnMenuFocusEditor)
    EVT_MENU(Cmds::View_Focus_grid,      ExecuteSqlFrame::OnMenuFocusGrid)

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
    EVT_MENU(Cmds::Query_Commit,              ExecuteSqlFrame::OnMenuCommit)
    EVT_MENU(Cmds::Query_Rollback,            ExecuteSqlFrame::OnMenuRollback)

    EVT_MENU(Cmds::DataGrid_Insert_row,      ExecuteSqlFrame::OnMenuGridInsertRow)
    EVT_MENU(Cmds::DataGrid_Delete_row,      ExecuteSqlFrame::OnMenuGridDeleteRow)
    EVT_MENU(Cmds::DataGrid_Copy,            ExecuteSqlFrame::OnMenuGridCopy)
    EVT_MENU(Cmds::DataGrid_Copy_as_insert,  ExecuteSqlFrame::OnMenuGridCopyAsInsert)
    EVT_MENU(Cmds::DataGrid_Copy_as_update,  ExecuteSqlFrame::OnMenuGridCopyAsUpdate)
    EVT_MENU(Cmds::DataGrid_Save_as_html,    ExecuteSqlFrame::OnMenuGridSaveAsHtml)
    EVT_MENU(Cmds::DataGrid_Save_as_csv,     ExecuteSqlFrame::OnMenuGridSaveAsCsv)
    EVT_MENU(Cmds::DataGrid_Set_header_font, ExecuteSqlFrame::OnMenuGridGridHeaderFont)
    EVT_MENU(Cmds::DataGrid_Set_cell_font,   ExecuteSqlFrame::OnMenuGridGridCellFont)
    EVT_MENU(Cmds::DataGrid_FetchAll,        ExecuteSqlFrame::OnMenuGridFetchAll)
    EVT_MENU(Cmds::DataGrid_CancelFetchAll,  ExecuteSqlFrame::OnMenuGridCancelFetchAll)

    EVT_UPDATE_UI(Cmds::DataGrid_Copy,           ExecuteSqlFrame::OnMenuUpdateGridHasSelection)
    EVT_UPDATE_UI(Cmds::DataGrid_Copy_as_insert, ExecuteSqlFrame::OnMenuUpdateGridHasSelection)
    EVT_UPDATE_UI(Cmds::DataGrid_Copy_as_update, ExecuteSqlFrame::OnMenuUpdateGridHasSelection)
    EVT_UPDATE_UI(Cmds::DataGrid_Save_as_html,   ExecuteSqlFrame::OnMenuUpdateGridHasSelection)
    EVT_UPDATE_UI(Cmds::DataGrid_Save_as_csv,    ExecuteSqlFrame::OnMenuUpdateGridHasSelection)
    EVT_UPDATE_UI(Cmds::DataGrid_FetchAll,       ExecuteSqlFrame::OnMenuUpdateGridFetchAll)
    EVT_UPDATE_UI(Cmds::DataGrid_CancelFetchAll, ExecuteSqlFrame::OnMenuUpdateGridCancelFetchAll)

    EVT_COMMAND(ExecuteSqlFrame::ID_grid_data, wxEVT_FRDG_ROWCOUNT_CHANGED, \
        ExecuteSqlFrame::OnGridRowCountChanged)
    EVT_COMMAND(ExecuteSqlFrame::ID_grid_data, wxEVT_FRDG_STATEMENT, \
        ExecuteSqlFrame::OnGridStatementExecuted)
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

    if (filenameM.IsEmpty())
    {
    if (styled_text_ctrl_sql->GetTextLength() > 0)
    {
        for (int i=0; i<styled_text_ctrl_sql->GetLineCount(); ++i)
        {
            wxString t = styled_text_ctrl_sql->GetLine(i).Trim();
            if (!t.IsEmpty())
            {
                SetTitle(t);
                break;
            }
        }
    }
    else
        SetTitle(_("Execute SQL statements"));
}
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
void ExecuteSqlFrame::autoCompleteColumns(int pos, int len)
{
    int start = styled_text_ctrl_sql->WordStartPosition(pos-1, true);
    if (start == -1)
        return;
    if (pos > 1 && styled_text_ctrl_sql->GetCharAt(pos-2) == '"')
    {   // quoted table name
        start = pos-3;
        while (start > -1 && styled_text_ctrl_sql->GetCharAt(start) != '"')
            --start;
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
//! handle function keys (F5, F8, F4, ...)
void ExecuteSqlFrame::OnKeyDown(wxKeyEvent &event)
{
    FR_TRY

    wxCommandEvent e;
    int key = event.GetKeyCode();
    if (!event.HasModifiers())
    {
        switch (key)
        {
            case WXK_F4:
                if (toolBarM->GetToolEnabled(Cmds::Query_Execute))
                    OnMenuExecute(e);
                return;         // needed on Linux
            case WXK_F5:
                if (toolBarM->GetToolEnabled(Cmds::Query_Commit))
                    OnMenuCommit(e);
                return;
            case WXK_F8:
                if (toolBarM->GetToolEnabled(Cmds::Query_Rollback))
                    OnMenuRollback(e);
                return;
            case WXK_F3:
                styled_text_ctrl_sql->find(false);
                return;
        };
    }

    // TODO: we might need Ctrl+N for new window, Ctrl+S for Save, etc. but it cannot be caught from here
    //       since OnKeyDown() doesn't seem to catch letters, only special keys

    if (wxWindow::FindFocus() == styled_text_ctrl_sql)
    {
        if (!styled_text_ctrl_sql->AutoCompActive())
        {
            enum { acSpace=0, acTab };
            int acc = acSpace;
            config().getValue(wxT("AutoCompleteKey"), acc);
            if (acc == acSpace && event.ControlDown() && key == WXK_SPACE)
                autoComplete(true);

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

    FR_CATCH
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnClose(wxCloseEvent& event)
{
    BaseFrame::OnClose(event);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuNew(wxCommandEvent& WXUNUSED(event))
{
    ExecuteSqlFrame *eff = new ExecuteSqlFrame(GetParent(), -1,
        _("Execute SQL statements"), databaseM);
    eff->styled_text_ctrl_sql->SetText(
        styled_text_ctrl_sql->GetSelectedText()
        );
    eff->Show();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuOpen(wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog fd(this, _("Select file to load"), wxT(""), wxT(""),
        _("SQL Scripts (*.sql)|*.sql|All files (*.*)|*.*"),
#if wxCHECK_VERSION(2, 8, 0)
        wxFD_OPEN | wxFD_CHANGE_DIR);
#else
        wxOPEN|wxCHANGE_DIR);
#endif

    if (wxID_OK == fd.ShowModal())
        loadSqlFile(fd.GetPath());
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuSave(wxCommandEvent& event)
{
    if (filenameM.IsEmpty())
        OnMenuSaveAs(event);
    else
    {
        styled_text_ctrl_sql->SaveFile(filenameM);
        statusbar_1->SetStatusText((_("File saved")), 2);
    }
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuSaveAs(wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog fd(this, _("Select file to save"), wxT(""), wxT(""),
        _("SQL Scripts (*.sql)|*.sql|All files (*.*)|*.*"),
#if wxCHECK_VERSION(2, 8, 0)
        wxFD_SAVE | wxFD_CHANGE_DIR | wxFD_OVERWRITE_PROMPT);
#else
        wxSAVE |wxCHANGE_DIR | wxOVERWRITE_PROMPT);
#endif

    if (wxID_OK != fd.ShowModal())
        return;

    filenameM = fd.GetPath();
    SetTitle(filenameM);
    styled_text_ctrl_sql->SaveFile(fd.GetPath());
    statusbar_1->SetStatusText((_("File saved")), 2);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuClose(wxCommandEvent& WXUNUSED(event))
{
    this->Close();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUndo(wxCommandEvent& WXUNUSED(event))
{
    styled_text_ctrl_sql->Undo();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuRedo(wxCommandEvent& WXUNUSED(event))
{
    styled_text_ctrl_sql->Redo();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuCut(wxCommandEvent& WXUNUSED(event))
{
    styled_text_ctrl_sql->Cut();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuCopy(wxCommandEvent& WXUNUSED(event))
{
    styled_text_ctrl_sql->Copy();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuPaste(wxCommandEvent& WXUNUSED(event))
{
    styled_text_ctrl_sql->Paste();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuDelete(wxCommandEvent& WXUNUSED(event))
{
    styled_text_ctrl_sql->Clear();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuSelectAll(wxCommandEvent& WXUNUSED(event))
{
    styled_text_ctrl_sql->SelectAll();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuReplace(wxCommandEvent &WXUNUSED(event))
{
    styled_text_ctrl_sql->find(true);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUpdateUndo(wxUpdateUIEvent& event)
{
    event.Enable(styled_text_ctrl_sql->CanUndo());
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUpdateRedo(wxUpdateUIEvent& event)
{
    event.Enable(styled_text_ctrl_sql->CanRedo());
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUpdateCopy(wxUpdateUIEvent& event)
{
    event.Enable((styled_text_ctrl_sql->GetSelectedText().Len()>0));
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUpdateCut(wxUpdateUIEvent& event)
{
    event.Enable((styled_text_ctrl_sql->GetSelectedText().Len()>0));
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUpdateDelete(wxUpdateUIEvent& event)
{
    event.Enable((styled_text_ctrl_sql->GetSelectedText().Len()>0));
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuUpdatePaste(wxUpdateUIEvent& event)
{
    event.Enable(styled_text_ctrl_sql->CanPaste());
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuEditor(wxCommandEvent& WXUNUSED(event))
{
    if (splitter_window_1->IsSplit())                    // screen is split -> show second
        splitter_window_1->Unsplit(panel_splitter_bottom);
    else if (!panel_splitter_top->IsShown()) // second is shown -> show first
    {
        panel_splitter_top->Show();
        panel_splitter_bottom->Show();
        splitter_window_1->SplitHorizontally(panel_splitter_top, panel_splitter_bottom);
        splitter_window_1->Unsplit(panel_splitter_bottom);
    }
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuData(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    if (splitter_window_1->IsSplit())                    // screen is split -> show second
        splitter_window_1->Unsplit(panel_splitter_top);
    else if (!panel_splitter_bottom->IsShown()) // second is shown -> show first
    {
        panel_splitter_top->Show();
        panel_splitter_bottom->Show();
        splitter_window_1->SplitHorizontally(panel_splitter_top, panel_splitter_bottom);
        splitter_window_1->Unsplit(panel_splitter_top);
    }
    notebook_1->SetSelection(1);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuStatistics(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    if (splitter_window_1->IsSplit())                    // screen is split -> show second
        splitter_window_1->Unsplit(panel_splitter_top);
    else if (!panel_splitter_bottom->IsShown()) // second is shown -> show first
    {
        panel_splitter_top->Show();
        panel_splitter_bottom->Show();
        splitter_window_1->SplitHorizontally(panel_splitter_top, panel_splitter_bottom);
        splitter_window_1->Unsplit(panel_splitter_top);
    }
    notebook_1->SetSelection(0);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuSplitView(wxCommandEvent& WXUNUSED(event))
{
    if(!splitter_window_1->IsSplit()){
        panel_splitter_top->Show();
        panel_splitter_bottom->Show();
        splitter_window_1->SplitHorizontally(panel_splitter_top, panel_splitter_bottom);
    }
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuSetEditorFont(wxCommandEvent& WXUNUSED(event))
{
    styled_text_ctrl_sql->setFont();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuToggleWrap(wxCommandEvent& WXUNUSED(event))
{
    styled_text_ctrl_sql->SetWrapMode(
        styled_text_ctrl_sql->GetWrapMode() == wxSTC_WRAP_WORD
        ? wxSTC_WRAP_NONE : wxSTC_WRAP_WORD
    );
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuFocusEditor(wxCommandEvent& WXUNUSED(event))
{
    if (!panel_splitter_top->IsShown())
    {
        panel_splitter_top->Show();
        panel_splitter_bottom->Show();
        splitter_window_1->SplitHorizontally(panel_splitter_top,
            panel_splitter_bottom);
    }
    styled_text_ctrl_sql->SetFocus();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuFocusGrid(wxCommandEvent& WXUNUSED(event))
{
    if (!panel_splitter_bottom->IsShown())
    {
        panel_splitter_top->Show();
        panel_splitter_bottom->Show();
        splitter_window_1->SplitHorizontally(panel_splitter_top, panel_splitter_bottom);
    }
    grid_data->SetFocus();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuHistoryNext(wxCommandEvent& WXUNUSED(event))
{
    StatementHistory& sh = StatementHistory::get(databaseM);
    if (historyPositionM != sh.size())  // we're already at the end?
    {
        historyPositionM++;
        if (historyPositionM == sh.size())
            styled_text_ctrl_sql->SetText(localBuffer);
        else
            styled_text_ctrl_sql->SetText(sh.get(historyPositionM));
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
        styled_text_ctrl_sql->SetText(sh.get(historyPositionM));
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
    FR_TRY

    clearStats();
    prepareAndExecute(false);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuShowPlan(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    prepareAndExecute(true);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuExecuteFromCursor(wxCommandEvent& WXUNUSED(event))
{
    if (config().get(wxT("SQLEditorExecuteClears"), false))
        clearStats();

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
    if (config().get(wxT("SQLEditorExecuteClears"), false))
        clearStats();

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
    FR_TRY

    grid_data->fetchAll();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuGridCancelFetchAll(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    grid_data->cancelFetchAll();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuGridInsertRow(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

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
        InsertDialog id(this, tab, tb, statementM, databaseM);
        id.ShowModal();
    }

    FR_CATCH
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuGridDeleteRow(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    if (!grid_data->getDataGridTable() || !grid_data->GetNumberRows())
        return;

    // M.B. when this is enabled the grid behaves strange on GTK2 (wx2.8.6)
    // when deleting multiple items. I didn't test other platforms
    // grid_data->BeginBatch();

    // M.B. This only works good when rows are selected by clicking on row
    // header and it is impossible to select a range which is off screen
    // - even using shift key (I only tried with GTK2 wx2.8.6).
    // wxArrayInt ai = grid_data->GetSelectedRows();
    // size_t count = ai.GetCount();

    // ...therefore, we iterate the rows ourselves
    wxArrayInt ai;
    size_t count = 0;
    for (int i = 0; i < grid_data->GetNumberRows(); i++)
    {
        for (int j = 0; j < grid_data->GetNumberCols(); j++)
        {
            if (grid_data->IsInSelection(i, j))
            {
                count++;
                ai.Add(i);
                break;
            }
        }
    }

    if (count > 1 && wxOK != showQuestionDialog(this,
        _("Multiple rows selected"),
        wxString::Format(_("Are you sure you wish to delete %d rows?"),
            count),
        AdvancedMessageDialogButtonsOkCancel(_("Delete"))))
    {
        return;
    }

    if (count == 0) // else - delete the row with cursor
        grid_data->DeleteRows(grid_data->GetGridCursorRow(), 1);
    else
    {
        while (count--)
            if (!grid_data->DeleteRows(ai.Item(count), 1))
                return;
    }

    // grid_data->EndBatch();   // see comment for BeginBatch above

    FR_CATCH
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuGridCopy(wxCommandEvent& WXUNUSED(event))
{
    grid_data->copyToCB();
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
    SetTitle(filenameM);
    return true;
}
//-----------------------------------------------------------------------------
//! enable/disable and show/hide controls depending of transaction status
void ExecuteSqlFrame::InTransaction(bool started)
{
    inTransactionM = started;
    SplitScreen();
    if (started)
        statusbar_1->SetStatusText(_("Transaction started"), 3);
    else
    {
        splitter_window_1->Unsplit(panel_splitter_bottom);        // show sql entry window
        grid_data->ClearGrid();
        statusbar_1->SetStatusText(wxEmptyString, 1);
    }
    toolBarM->EnableTool(Cmds::Query_Commit, started);
    toolBarM->EnableTool(Cmds::Query_Rollback, started);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::setSql(wxString sql)
{
    styled_text_ctrl_sql->SetText(sql);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::clearStats()
{
    if (config().get(wxT("SQLEditorExecuteClears"), false))
        styled_text_ctrl_stats->ClearAll();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::prepareAndExecute(bool prepareOnly)
{
    bool hasSelection = styled_text_ctrl_sql->GetSelectionStart() != styled_text_ctrl_sql->GetSelectionEnd();
    bool only = false;
    bool ok;
    config().getValue(wxT("OnlyExecuteSelected"), only);
    if (only && hasSelection)    // something is selected
    {
        bool single = false;
        config().getValue(wxT("TreatAsSingleStatement"), single);
        if (single)
            ok = execute(styled_text_ctrl_sql->GetSelectedText(), wxT(";"), prepareOnly);
        else
            ok = parseStatements(styled_text_ctrl_sql->GetSelectedText(), false, prepareOnly, styled_text_ctrl_sql->GetSelectionStart());
    }
    else
        ok = parseStatements(styled_text_ctrl_sql->GetText(), false, prepareOnly);

    if (ok || config().get(wxT("historyStoreUnsuccessful"), true))
    {
        // add to history
        StatementHistory& sh = StatementHistory::get(databaseM);
        sh.add(styled_text_ctrl_sql->GetText());
        historyPositionM = sh.size();
    }
}
//-----------------------------------------------------------------------------
//! adapted so we don't have to change all the other code that utilizes SQL editor
void ExecuteSqlFrame::executeAllStatements(bool closeWhenDone)
{
    clearStats();
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
        toolBarM->EnableTool(Cmds::Query_Execute, false);
        // TODO: HOWTO focus toolbar button? button_commit->SetFocus();
    }

    return true;
}
//-----------------------------------------------------------------------------
//! when autoexecute is TRUE, program just waits user to click Commit/Rollback and closes window
bool ExecuteSqlFrame::execute(wxString sql, const wxString& terminator,
    bool prepareOnly)
{
    // check if sql only contains comments
    wxString sqlclean(sql);
    sqlclean += wxT("\n");                                            // just in case -- comment is on last line
    SimpleParser::removeComments(sqlclean, wxT("/*"), wxT("*/"));
    SimpleParser::removeComments(sqlclean, wxT("--"), wxT("\n"));
    while (true)
    {
        wxString::size_type pos = sqlclean.find(wxT(";"));        // remove ;
        if (pos == wxString::npos)
            break;
        sqlclean.erase(pos, 1);
    }
    sqlclean.erase(sqlclean.find_last_not_of(wxT(" \n\t\r")) + 1);    // trim
    if (sqlclean.empty())
    {
        log(_("Parsed query: " + sql), ttSql);
        log(_("Empty statement detected, bailing out..."));
        return true;
    }

    if (styled_text_ctrl_sql->AutoCompActive())
        styled_text_ctrl_sql->AutoCompCancel();    // remove the list if needed
    wxDateTime start = wxDateTime::Now();
    notebook_1->SetSelection(0);
    bool retval = true;

    try
    {
        if (!inTransactionM)
        {
            log(_("Starting transaction..."));
            transactionM->Start();
            InTransaction(true);
        }

        int fetch1 = 0, mark1 = 0, read1 = 0, write1 = 0, ins1 = 0, upd1 = 0,
            del1 = 0, ridx1 = 0, rseq1 = 0, mem1 = 0;
        int fetch2, mark2, read2, write2, ins2, upd2, del2, ridx2, rseq2, mem2;
        bool doShowStats = config().get(wxT("SQLEditorShowStats"), true);
        if (!prepareOnly && doShowStats)
        {
            databaseM->getIBPPDatabase()->
                Statistics(&fetch1, &mark1, &read1, &write1, &mem1);
            databaseM->getIBPPDatabase()->
                Counts(&ins1, &upd1, &del1, &ridx1, &rseq1);
        }
        grid_data->ClearGrid(); // statement object will be invalidated, so clear the grid
        statementM = IBPP::StatementFactory(databaseM->getIBPPDatabase(), transactionM);
        log(_("Preparing query: " + sql), ttSql);
        statementM->Prepare(wx2std(sql, dbCharsetConversionM.getConverter()));

        wxTimeSpan dif = wxDateTime::Now().Subtract(start);
        log(wxString(_("Prepare time: ")) + dif.Format(wxT("%H:%M:%S.")));

        try
        {
            std::string plan;            // for some statements (DDL) it is never available
            statementM->Plan(plan);        // for INSERTs, it is available sometimes (insert into ... select ... )
            log(std2wx(plan));            // but if it not, IBPP throws the exception
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
        styled_text_ctrl_stats->Update();            // let GUI update the controls
        statementM->Execute();
        log(_("Done."));

        IBPP::STT type = statementM->Type();
        if (type == IBPP::stSelect)            // for select statements: show data
        {
            grid_data->fetchData(dbCharsetConversionM.getConverter());
            notebook_1->SetSelection(1);
            grid_data->SetFocus();
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
        }

        if (type != IBPP::stSelect) // for other statements: show rows affected
        {   // left trim
            wxString::size_type p = sql.find_first_not_of(wxT(" \n\t\r"));
            if (p != wxString::npos && p > 0)
                sql.erase(0, p);
            if (type == IBPP::stInsert || type == IBPP::stDelete
                || type == IBPP::stExecProcedure || type == IBPP::stUpdate)
            {
                wxString addon;
                if (statementM->AffectedRows() % 10 != 1)
                    addon = wxT("s");
                wxString s = wxString::Format(_("%d row%s affected directly."),
                    statementM->AffectedRows(), addon.c_str());
                log(wxT("") + s);
                statusbar_1->SetStatusText(s, 1);
            }
            SqlStatement stm(sql, databaseM, terminator);
            if (stm.isDDL())
                type = IBPP::stDDL;
            executedStatementsM.push_back(stm);
            styled_text_ctrl_sql->SetFocus();
            if (type == IBPP::stDDL && autoCommitM)
            {
                if (!commitTransaction())
                    retval = false;
            }
        }
    }
    catch (IBPP::Exception &e)
    {
        SplitScreen();
        log(std2wx(e.ErrorMessage()) + wxT("\n"), ttError);
        retval = false;
    }
    catch (...)
    {
        SplitScreen();
        log(_("SYSTEM ERROR!"), ttError);
        retval = false;
    }

    wxTimeSpan dif = wxDateTime::Now().Subtract(start);
    log(wxString(_("Execute time: ")) + dif.Format(wxT("%H:%M:%S.")));
    return retval;
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::SplitScreen()
{
    if (!splitter_window_1->IsSplit())                    // split screen if needed
    {
        panel_splitter_top->Show();
        panel_splitter_bottom->Show();
        splitter_window_1->SplitHorizontally(panel_splitter_top, panel_splitter_bottom);
    }
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuCommit(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    commitTransaction();

    FR_CATCH
}
//-----------------------------------------------------------------------------
bool ExecuteSqlFrame::commitTransaction()
{
    wxBusyCursor cr;

    // grid_data->stopFetching();
    if (!transactionM->Started())    // check
    {
        InTransaction(false);
        return true;    // nothing to commit, but it wasn't error
    }

    try
    {
        log(_("Commiting transaction..."));
        transactionM->Commit();
        log(_("Done."));
        statusbar_1->SetStatusText(_("Transaction commited"), 3);
        InTransaction(false);

        SubjectLocker locker(databaseM);
        // log statements, done before parsing in case parsing crashes FR
        for (std::vector<SqlStatement>::const_iterator it =
            executedStatementsM.begin(); it != executedStatementsM.end(); ++it)
        {
            if (!Logger::logStatement(*it, databaseM))
                break;
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
            Close();
            return true;
        }
    }
    catch (IBPP::Exception &e)
    {
        SplitScreen();
        log(std2wx(e.ErrorMessage()), ttError);
        return false;
    }
    catch (...)
    {
        SplitScreen();
        log(_("ERROR!\nA non-IBPP C++ runtime exception occured!"), ttError);
        return false;
    }

    notebook_1->SetSelection(0);

    // apparently is has to be at the end to have any effect
    styled_text_ctrl_sql->SetFocus();
    return true;
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnMenuRollback(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    wxBusyCursor cr;
    rollbackTransaction();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::rollbackTransaction()
{
    // grid_data->stopFetching();
    if (!transactionM->Started())    // check
    {
        executedStatementsM.clear();
        InTransaction(false);
        return;
    }

    try
    {
        log(_("Rolling back the transaction..."));
        transactionM->Rollback();
        log(_("Done."));
        statusbar_1->SetStatusText(_("Transaction rolled back."), 3);
        InTransaction(false);
        executedStatementsM.clear();

        if (closeWhenTransactionDoneM)
        {
            Close();
            return;
        }
    }
    catch (IBPP::Exception &e)
    {
        SplitScreen();
        log(std2wx(e.ErrorMessage()), ttError);
    }
    catch (...)
    {
        SplitScreen();
        log(_("ERROR!\nA non-IBPP C++ runtime exception occured !"), ttError);
    }

    notebook_1->SetSelection(0);
    styled_text_ctrl_sql->SetFocus();
}
//-----------------------------------------------------------------------------
//! toggle the views in the following order:
//! ... -> SQL_entry_box -> Split View -> Stats&Data -> ...
/*
void ExecuteSqlFrame::OnMenuButtonToggleClick(wxCommandEvent& WXUNUSED(event))
{
    if (splitter_window_1->IsSplit())                    // screen is split -> show second
        splitter_window_1->Unsplit(panel_splitter_top);
    else if (panel_splitter_top->IsShown())        // first is shown -> split again
        SplitScreen();
    else                                        // second is shown -> show first
    {
        panel_splitter_top->Show();
        panel_splitter_bottom->Show();
        splitter_window_1->SplitHorizontally(panel_splitter_top, panel_splitter_bottom);
        splitter_window_1->Unsplit(panel_splitter_bottom);
    }
}
*/
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnGridRowCountChanged(wxCommandEvent &event)
{
    wxString s;
    long rowsFetched = event.GetExtraLong();
    s.Printf(_("%d rows fetched"), rowsFetched);
    statusbar_1->SetStatusText(s, 1);

    toolBarM->EnableTool(Cmds::DataGrid_Delete_row, rowsFetched > 0);

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
            //SplitScreen();    // not needed atm, might be later (see TODO above)
            splitter_window_1->Unsplit(panel_splitter_top);        // show grid only
        }
    }
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnGridStatementExecuted(wxCommandEvent &event)
{
    log(event.GetString(), ttSql);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::update()
{
    if (!databaseM->isConnected())
        Close();
}
//-----------------------------------------------------------------------------
Database *ExecuteSqlFrame::getDatabase()
{
    return databaseM;
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::setDatabase(Database* db)
{
    databaseM = db;
    dbCharsetConversionM.setConnectionCharset(db->getConnectionCharset());

    wxString s = wxString::Format(wxT("%s@%s:%s"), db->getUsername().c_str(),
        db->getServer()->getName_().c_str(), db->getPath().c_str());
    statusbar_1->SetStatusText(s, 0);

    transactionM = IBPP::TransactionFactory(databaseM->getIBPPDatabase());
    db->attachObserver(this);    // observe database object

    executedStatementsM.clear();
    InTransaction(false);    // enable/disable controls
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

    wxArrayString as;
    as.Alloc(8192);             // pre-alloc 8k
    const Identifier::keywordContainer& k = Identifier::getKeywordSet();
    for (Identifier::keywordContainer::const_iterator ci = k.begin(); ci != k.end(); ++ci)
        as.Add((*ci).Upper());

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
    styled_text_ctrl_stats->AddText(s + wxT("\n"));
    int endpos = styled_text_ctrl_stats->GetLength();

    int style = 0;
    if (type == ttError)
        style = 1;
    if (type == ttSql)
        style = 2;

    styled_text_ctrl_stats->StartStyling(startpos, 255);
    styled_text_ctrl_stats->SetStyling(endpos-startpos-1, style);
    styled_text_ctrl_stats->GotoPos(endpos);
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
    execSql(w, _("Dropping field"), c->getDatabase(), sql, true);
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
        execSql(w, _("Dropping fields"), t->getDatabase(), sql, true);
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

    execSql(w, _("DROP"), m->getDatabase(), m->getDropSqlStatement(), true);
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

    ProgressDialog pd(w, _("Extracting DDL Definitions"), 2);
    CreateDDLVisitor cdv(&pd);
    m->acceptVisitor(&cdv);
    if (pd.isCanceled())
        return true;

    ExecuteSqlFrame* eff = new ExecuteSqlFrame(w->GetParent(), -1, wxT("DDL"),
        m->getDatabase());
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

    showSql(w->GetParent(), _("Editing stored procedure"), p->getDatabase(),
        p->getAlterSql());
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

    showSql(w->GetParent(), _("Altering view"), r->getDatabase(),
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

    showSql(w->GetParent(), _("Editing trigger"), t->getDatabase(),
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
    Database *db = g->getDatabase();
    if (!db)
    {
        wxMessageBox(_("No database assigned"), _("Warning"), wxOK | wxICON_ERROR);
        return true;
    }

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

    showSql(w->GetParent(), _("Editing exception"), e->getDatabase(),
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
        sql = wxT("DROP INDEX ") + i->getQuotedName();
    else if (type == wxT("RECOMPUTE"))
        sql = wxT("SET STATISTICS INDEX ") + i->getQuotedName();
    else if (type == wxT("TOGGLE_ACTIVE"))
        sql = wxT("ALTER INDEX ") + i->getQuotedName() + (i->isActive() ? wxT(" INACTIVE") : wxT(" ACTIVE"));

    execSql(w, wxEmptyString, i->getDatabase(), sql, true);
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
    if (uri.action != wxT("activate_triggers") && uri.action != wxT("deactivate_triggers"))
        return false;

    Table* t = (Table*)getObject(uri);
    wxWindow* w = getWindow(uri);
    if (!t || !w)
        return true;

    std::vector<Trigger*> list;
    t->getTriggers(list, Trigger::afterTrigger);
    t->getTriggers(list, Trigger::beforeTrigger);
    wxString sql;
    for (std::vector<Trigger*>::iterator it = list.begin(); it != list.end(); ++it)
    {
        sql += wxT("ALTER TRIGGER ") + (*it)->getQuotedName() + wxT(" ");
        if (uri.action == wxT("deactivate_triggers"))
            sql += wxT("IN");
        sql += wxT("ACTIVE;\n");
    }

    execSql(w, wxEmptyString, t->getDatabase(), sql, true);
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

    execSql(w, wxEmptyString, t->getDatabase(), sql, true);
    return true;
}
//-----------------------------------------------------------------------------
