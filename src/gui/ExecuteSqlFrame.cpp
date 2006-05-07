/*
Copyright (c) 2004, 2005, 2006 The FlameRobin Development Team

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

#include <algorithm>
#include <map>
#include <sstream>
#include <vector>

#include <wx/datetime.h>
#include <wx/file.h>
#include <wx/fontdlg.h>
#include <wx/tokenzr.h>

#include "config/Config.h"
#include "dberror.h"
#include "framemanager.h"
#include "gui/AdvancedMessageDialog.h"
#include "gui/controls/DataGrid.h"
#include "gui/controls/DataGridTable.h"
#include "gui/ProgressDialog.h"
#include "gui/ExecuteSqlFrame.h"
#include "gui/StatementHistoryDialog.h"
#include "gui/StyleGuide.h"
#include "frutils.h"
#include "logger.h"
#include "metadata/CreateDDLVisitor.h"
#include "metadata/procedure.h"
#include "metadata/server.h"
#include "metadata/view.h"
#include "sql/Identifier.h"
#include "sql/SimpleParser.h"
#include "sql/SqlStatement.h"
#include "statementHistory.h"
#include "ugly.h"
#include "urihandler.h"

//-----------------------------------------------------------------------------
// TODO: This needs to be reworked to use the tokenizer
//       Perhaps we could have a SelectStatement class, that would be able to:
//       - load the select statement
//       - alter some of it (add new table, column, etc.)
//       - dump statement back to editor
bool DnDText::OnDropText(wxCoord, wxCoord, const wxString& text)
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
    if (m->getType() == ntTable)
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
    wxString sql = ownerM->GetText().Upper();

    // currently we don't support having comments and quotes (it's complicated)
    //if (!SimpleParser::stripSql(sql))
    //    return true;

    wxString::size_type psel, pfrom;
    psel = sql.find(wxT("SELECT"));
    if (psel == wxString::npos)                            // simple select statement
    {
        sql = wxT("SELECT ") + column_list + wxT("\nFROM ") + t->getQuotedName();
        ownerM->SetText(sql);
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

    ownerM->SetText(sql);
    return true;
}
//-----------------------------------------------------------------------------
//! included xpm files, so that icons are compiled into executable
namespace sql_icons {
#include "new.xpm"
#include "load.xpm"
#include "save.xpm"
#include "saveas.xpm"
#include "sqlicon.xpm"
#include "left.xpm"
#include "right.xpm"
#include "history.xpm"
};
//-----------------------------------------------------------------------------
// Setup the Scintilla editor
SqlEditor::SqlEditor(wxWindow *parent, wxWindowID id, ExecuteSqlFrame *frame)
    : SearchableEditor(parent, id)
{
    frameM = frame;
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
    EVT_MENU(SqlEditor::ID_MENU_UNDO,             SqlEditor::OnMenuUndo)
    EVT_MENU(SqlEditor::ID_MENU_REDO,             SqlEditor::OnMenuRedo)
    EVT_MENU(SqlEditor::ID_MENU_CUT,              SqlEditor::OnMenuCut)
    EVT_MENU(SqlEditor::ID_MENU_COPY,             SqlEditor::OnMenuCopy)
    EVT_MENU(SqlEditor::ID_MENU_PASTE,            SqlEditor::OnMenuPaste)
    EVT_MENU(SqlEditor::ID_MENU_DELETE,           SqlEditor::OnMenuDelete)
    EVT_MENU(SqlEditor::ID_MENU_SELECT_ALL,       SqlEditor::OnMenuSelectAll)
    EVT_MENU(SqlEditor::ID_MENU_EXECUTE_SELECTED, SqlEditor::OnMenuExecuteSelected)
    EVT_MENU(SqlEditor::ID_MENU_FIND_SELECTED,    SqlEditor::OnMenuFindSelected)
    EVT_MENU(SqlEditor::ID_MENU_FIND,             SqlEditor::OnMenuFind)
    EVT_MENU(SqlEditor::ID_MENU_WRAP,             SqlEditor::OnMenuWrap)
    EVT_MENU(SqlEditor::ID_MENU_SET_FONT,         SqlEditor::OnMenuSetFont)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void SqlEditor::OnContextMenu(wxContextMenuEvent& WXUNUSED(event))
{
    wxMenu m(0);
    m.Append(ID_MENU_UNDO, _("&Undo"));
    m.Append(ID_MENU_REDO, _("&Redo"));
    m.AppendSeparator();
    m.Append(ID_MENU_CUT,    _("Cu&t"));
    m.Append(ID_MENU_COPY,   _("&Copy"));
    m.Append(ID_MENU_PASTE,  _("&Paste"));
    m.Append(ID_MENU_DELETE, _("&Delete"));
    m.AppendSeparator();
    m.Append(ID_MENU_SELECT_ALL,       _("Select &All"));
    m.Append(ID_MENU_EXECUTE_SELECTED, _("E&xecute selected"));

    int slen = GetSelectionEnd() - GetSelectionStart();
    if (slen && slen < 50)     // something (small) is selected
    {
        wxString sel = GetSelectedText().Strip();
        size_t p = sel.find_first_of(wxT("\n\r\t"));
        if (p != wxString::npos)
            sel.Remove(p);
        m.Append(ID_MENU_FIND_SELECTED, _("S&how properties for ") + sel);
    }

    m.AppendSeparator();
    m.Append(ID_MENU_FIND,          _("&Find and replace"));
    m.Append(ID_MENU_SET_FONT,      _("Set F&ont"));
    m.AppendCheckItem(ID_MENU_WRAP, _("&Wrap"));
    if (wxSTC_WRAP_WORD == GetWrapMode())
        m.Check(ID_MENU_WRAP, true);

    // disable stuff
    m.Enable(ID_MENU_UNDO, CanUndo());
    m.Enable(ID_MENU_REDO, CanRedo());
    if (slen == 0)        // nothing is selected
    {
        m.Enable(ID_MENU_CUT,              false);
        m.Enable(ID_MENU_COPY,             false);
        m.Enable(ID_MENU_DELETE,           false);
        m.Enable(ID_MENU_EXECUTE_SELECTED, false);
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
void SqlEditor::OnMenuUndo(wxCommandEvent& WXUNUSED(event))
{
    Undo();
}
//-----------------------------------------------------------------------------
void SqlEditor::OnMenuRedo(wxCommandEvent& WXUNUSED(event))
{
    Redo();
}
//-----------------------------------------------------------------------------
void SqlEditor::OnMenuCut(wxCommandEvent& WXUNUSED(event))
{
    Cut();
}
//-----------------------------------------------------------------------------
void SqlEditor::OnMenuCopy(wxCommandEvent& WXUNUSED(event))
{
    Copy();
}
//-----------------------------------------------------------------------------
void SqlEditor::OnMenuPaste(wxCommandEvent& WXUNUSED(event))
{
    Paste();
}
//-----------------------------------------------------------------------------
void SqlEditor::OnMenuDelete(wxCommandEvent& WXUNUSED(event))
{
    Clear();
}
//-----------------------------------------------------------------------------
void SqlEditor::OnMenuSelectAll(wxCommandEvent& WXUNUSED(event))
{
    SelectAll();
}
//-----------------------------------------------------------------------------
void SqlEditor::OnMenuFindSelected(wxCommandEvent& WXUNUSED(event))
{
    wxString sel = GetSelectedText();
    int p = sel.Find(wxT(" "));
    if (p != -1)
        sel.Remove(p);
    frameM->showProperties(sel);
}
//-----------------------------------------------------------------------------
void SqlEditor::OnMenuExecuteSelected(wxCommandEvent& WXUNUSED(event))
{
    if (config().get(wxT("TreatAsSingleStatement"), false))
        frameM->execute(GetSelectedText());
    else
        frameM->parseStatements(GetSelectedText(), false, false, GetSelectionStart());
}
//-----------------------------------------------------------------------------
void SqlEditor::OnMenuFind(wxCommandEvent& WXUNUSED(event))
{
    find(true);        // calls SearchableEditor::find(), true means that new search is needed
}
//-----------------------------------------------------------------------------
void SqlEditor::OnMenuWrap(wxCommandEvent& WXUNUSED(event))
{
    SetWrapMode(GetWrapMode() == wxSTC_WRAP_WORD ? wxSTC_WRAP_NONE : wxSTC_WRAP_WORD);
}
//-----------------------------------------------------------------------------
void SqlEditor::OnMenuSetFont(wxCommandEvent& WXUNUSED(event))
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
#ifdef FR_NEWADVANCEDMESSAGEDIALOG
    showInformationDialog(wxGetTopLevelParent(this), _("The SQL editor font has been changed."),
        _("This setting affects only the SQL editor font. The font used in the result set data grid has to be changed separately."),
        AdvancedMessageDialogButtonsOk(), config(), wxT("DIALOG_WarnFont"));
#else
    AdvancedMessageBox(_("This setting only affects font in the editor.\nTo have characters displayed correctly in result grid,\nyou need to specify its font separately."),
        _("Font changed"), wxOK|wxICON_INFORMATION, 0, 0, wxT("DIALOG_WarnFont"));
#endif
}
//-----------------------------------------------------------------------------
ExecuteSqlFrame::ExecuteSqlFrame(wxWindow* parent, int id, wxString title, const wxPoint& pos, const wxSize& size, long style):
    BaseFrame(parent, id, title, pos, size, style), Observer()
{
    panel_contents = new wxPanel(this, -1);
    button_new = new wxBitmapButton(panel_contents, ID_button_new, wxBitmap(sql_icons::new_xpm));
    button_load = new wxBitmapButton(panel_contents, ID_button_load, wxBitmap(sql_icons::load_xpm));
    button_save = new wxBitmapButton(panel_contents, ID_button_save, wxBitmap(sql_icons::save_xpm));
    button_saveas = new wxBitmapButton(panel_contents, ID_button_saveas, wxBitmap(sql_icons::saveas_xpm));
    button_prev = new wxBitmapButton(panel_contents, ID_button_prev, wxBitmap(sql_icons::left_xpm));
    button_next = new wxBitmapButton(panel_contents, ID_button_next, wxBitmap(sql_icons::right_xpm));
    button_history = new wxBitmapButton(panel_contents, ID_button_history, wxBitmap(sql_icons::history_xpm));
    button_execute = new wxButton(panel_contents, ID_button_execute, _("Execute (F4)"));
    button_commit = new wxButton(panel_contents, ID_button_commit, _("Commit (F5)"));
    button_rollback = new wxButton(panel_contents, ID_button_rollback, _("Rollback (F8)"));
    button_plan = new wxButton(panel_contents, ID_button_plan, _("Show plan"));
    button_toggle = new wxButton(panel_contents, ID_button_toggle, _("Toggle view"));

    splitter_window_1 = new wxSplitterWindow(panel_contents, -1);
    panel_splitter_bottom = new wxPanel(splitter_window_1, -1);
    notebook_1 = new wxNotebook(panel_splitter_bottom, -1, wxDefaultPosition, wxDefaultSize, 0);
    notebook_pane_1 = new wxPanel(notebook_1, -1);
    notebook_pane_2 = new wxPanel(notebook_1, -1);
    panel_splitter_top = new wxPanel(splitter_window_1, -1);
    statusbar_1 = CreateStatusBar(4);
    styled_text_ctrl_sql = new SqlEditor(panel_splitter_top, ID_stc_sql, this);
    styled_text_ctrl_stats = new wxStyledTextCtrl(notebook_pane_1, -1);
    styled_text_ctrl_stats->SetWrapMode(wxSTC_WRAP_WORD);
    styled_text_ctrl_stats->StyleSetForeground(1, *wxRED);
    styled_text_ctrl_stats->StyleSetForeground(2, *wxBLUE);

    set_properties();
    do_layout();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::set_properties()
{
    SetTitle(_("Execute SQL statements"));
    SetSize(wxSize(628, 488));
    int statusbar_widths[] = { -2, 100, 60, -1 };
    statusbar_1->SetStatusWidths(4, statusbar_widths);
    const wxString statusbar_fields[] = {
        wxT("user @ database"),
        wxT("rows fetched"),
        wxT("cursor position"),
        wxT("Transaction status")
    };
    for(int i = 0; i < statusbar_1->GetFieldsCount(); ++i) {
        statusbar_1->SetStatusText(statusbar_fields[i], i);
    }
    grid_data = new DataGrid(notebook_pane_2, ID_grid_data);
    grid_data->SetTable(new GridTable(statementM), true);
    splitter_window_1->SplitHorizontally(panel_splitter_top, panel_splitter_bottom);

    button_new->SetToolTip(_("New window"));
    button_load->SetToolTip(_("Load"));
    button_save->SetToolTip(_("Save"));
    button_saveas->SetToolTip(_("Save As..."));
    button_prev->SetToolTip(_("Previous"));
    button_next->SetToolTip(_("Next"));
    button_history->SetToolTip(_("Statement History"));
    button_execute->SetToolTip(_("F4 - Execute SQL statement"));
    button_commit->SetToolTip(_("F5 - Commit transaction"));
    button_rollback->SetToolTip(_("F8 - Rollback transaction"));
    button_plan->SetToolTip(_("Show execution plan for query"));

    splitter_window_1->Unsplit();

    wxBitmap bmp = wxBitmap(sql_icons::sqlicon_xpm);
    wxIcon icon;
    icon.CopyFromBitmap(bmp);
    SetIcon(icon);

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
    wxBoxSizer* sizer_3 = new wxBoxSizer(wxHORIZONTAL);
    sizer_3->Add(button_new, 0, wxALL, 1);
    sizer_3->Add(button_load, 0, wxALL, 1);
    sizer_3->Add(button_save, 0, wxALL, 1);
    sizer_3->Add(button_saveas, 0, wxALL, 1);
    sizer_3->Add(button_prev, 0, wxALL, 1);
    sizer_3->Add(button_next, 0, wxALL, 1);
    sizer_3->Add(button_history, 0, wxALL, 1);
    sizer_3->Add(10, 5, 0, 0, 0);
    sizer_3->Add(button_execute, 0, wxALL, 3);
    sizer_3->Add(button_commit, 0, wxALL, 3);
    sizer_3->Add(button_rollback, 0, wxALL, 3);
    sizer_3->Add(button_plan, 0, wxALL, 3);
    sizer_3->Add(10, 5, 0, 0, 0);
    sizer_3->Add(button_toggle, 0, wxALL, 3);
    sizer_2->Add(sizer_3, 0, wxALL|wxEXPAND, 2);
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
// This function should probably go somewhere else, but we don't want SQL editor
// to know about database class
void ExecuteSqlFrame::showProperties(wxString objectName)
{
    MetadataItem *m = databaseM->findByName(objectName);
    if (!m)
    {
        wxMessageBox(
            wxString::Format(_("Object %s has not been found in this database."), objectName.c_str()),
            _("Search failed."),
            wxID_OK | wxICON_INFORMATION
        );
        return;
    }
    frameManager().showMetadataPropertyFrame(GetParent(), m);
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(ExecuteSqlFrame, wxFrame)
    EVT_STC_UPDATEUI(ExecuteSqlFrame::ID_stc_sql, ExecuteSqlFrame::OnSqlEditUpdateUI)
    EVT_STC_CHARADDED(ExecuteSqlFrame::ID_stc_sql, ExecuteSqlFrame::OnSqlEditCharAdded)
    EVT_STC_START_DRAG(ExecuteSqlFrame::ID_stc_sql, ExecuteSqlFrame::OnSqlEditStartDrag)
    EVT_CHAR_HOOK(ExecuteSqlFrame::OnKeyDown)
    EVT_CLOSE(ExecuteSqlFrame::OnClose)

    EVT_BUTTON(ExecuteSqlFrame::ID_button_new, ExecuteSqlFrame::OnButtonNewClick)
    EVT_BUTTON(ExecuteSqlFrame::ID_button_load, ExecuteSqlFrame::OnButtonLoadClick)
    EVT_BUTTON(ExecuteSqlFrame::ID_button_save, ExecuteSqlFrame::OnButtonSaveClick)
    EVT_BUTTON(ExecuteSqlFrame::ID_button_saveas, ExecuteSqlFrame::OnButtonSaveAsClick)
    EVT_BUTTON(ExecuteSqlFrame::ID_button_prev, ExecuteSqlFrame::OnButtonPrevClick)
    EVT_BUTTON(ExecuteSqlFrame::ID_button_next, ExecuteSqlFrame::OnButtonNextClick)
    EVT_BUTTON(ExecuteSqlFrame::ID_button_history, ExecuteSqlFrame::OnButtonHistoryClick)
    EVT_BUTTON(ExecuteSqlFrame::ID_button_execute, ExecuteSqlFrame::OnButtonExecuteClick)
    EVT_BUTTON(ExecuteSqlFrame::ID_button_commit, ExecuteSqlFrame::OnButtonCommitClick)
    EVT_BUTTON(ExecuteSqlFrame::ID_button_rollback, ExecuteSqlFrame::OnButtonRollbackClick)
    EVT_BUTTON(ExecuteSqlFrame::ID_button_plan, ExecuteSqlFrame::OnButtonPlanClick)
    EVT_BUTTON(ExecuteSqlFrame::ID_button_toggle, ExecuteSqlFrame::OnButtonToggleClick)
    EVT_COMMAND(ExecuteSqlFrame::ID_grid_data, wxEVT_FRDG_ROWCOUNT_CHANGED, \
        ExecuteSqlFrame::OnGridRowCountChanged)
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
    // print x:y coordinates in status bar
    int p = styled_text_ctrl_sql->GetCurrentPos();
    int row = styled_text_ctrl_sql->GetCurrentLine();
    int col = p - styled_text_ctrl_sql->PositionFromLine(row);
    statusbar_1->SetStatusText(wxString::Format(wxT("%d : %d"), col+1, row+1), 2);

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
//-----------------------------------------------------------------------------
//! returns true if there is a word in "wordlist" that starts with "word"
bool HasWord(wxString word, wxString& wordlist)
{
    // entire wordlist is uppercase
    word.MakeUpper();

    wxStringTokenizer tkz(wordlist, wxT(" "));
    while (tkz.HasMoreTokens())
    {
        wxString token = tkz.GetNextToken();
        if (token.StartsWith(word))
            return true;
    }
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
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::autoComplete(bool force)
{
    if (styled_text_ctrl_sql->AutoCompActive())
        return;

    // TODO: we can add support for . here
    // Like this: user types name of some table (ex. EMPLOYEE) and when he types
    // the dot (.), autocomplete shows list of all columns for Employee table
    // Similar can be done for Views and Output-params of SP

    // Returns the character byte at the position.
    // int GetCharAt(int pos);

    // TODO:
    // It should also work with aliases by parsing the FROM clause
    // FROM [object_name] [alias]
    // (left|right|outer...) JOIN [object_name] [alias]
    // Parser used for DnD could be used here once it supports table aliases

    int autoCompleteChars = 1;
    if (!force)
    {
        autoCompleteChars = config().get(wxT("AutocompleteChars"), 3);
        if (autoCompleteChars <= 0)
            return;
    }

    int pos = styled_text_ctrl_sql->GetCurrentPos();
    int start = styled_text_ctrl_sql->WordStartPosition(pos, true);
    if (start != -1 && pos - start >= autoCompleteChars)
    {
        // GTK version crashes if nothing matches, so this check must be made for GTK
        // For MSW, it doesn't crash but it flashes on the screen (also not very nice)
        if (HasWord(styled_text_ctrl_sql->GetTextRange(start, pos), keywordsM))
            styled_text_ctrl_sql->AutoCompShow(pos-start, keywordsM);
    }
}
//-----------------------------------------------------------------------------
//! handle function keys (F5, F8, F4, ...)
void ExecuteSqlFrame::OnKeyDown(wxKeyEvent &event)
{
    wxCommandEvent e;
    int key = event.GetKeyCode();
    if (!event.HasModifiers())
    {
        switch (key)
        {
            case WXK_F4:
                OnButtonExecuteClick(e);
                return;         // needed on Linux
            case WXK_F5:
                OnButtonCommitClick(e);
                return;
            case WXK_F8:
                OnButtonRollbackClick(e);
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
            bool acEnter = false;
            config().getValue(wxT("AutoCompleteWithEnter"), acEnter);
            if (!acEnter)
                styled_text_ctrl_sql->AutoCompCancel();
        }
    }
    event.Skip();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnClose(wxCloseEvent& event)
{
    BaseFrame::OnClose(event);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnButtonNewClick(wxCommandEvent& WXUNUSED(event))
{
    ExecuteSqlFrame *eff = new ExecuteSqlFrame(GetParent(), -1, _("Execute SQL statements"));
    eff->setDatabase(databaseM);
    eff->Show();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnButtonLoadClick(wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog fd(this, _("Select file to load"), wxT(""), wxT(""),
        _("SQL Scripts (*.sql)|*.sql|All files (*.*)|*.*"),
        wxOPEN|wxCHANGE_DIR);

    if (wxID_OK != fd.ShowModal())
        return;

    if (styled_text_ctrl_sql->LoadFile(fd.GetPath()))
        filenameM = fd.GetPath();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnButtonSaveAsClick(wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog fd(this, _("Select file to save"), wxT(""), wxT(""),
        _("SQL Scripts (*.sql)|*.sql|All files (*.*)|*.*"),
        wxSAVE |wxCHANGE_DIR | wxOVERWRITE_PROMPT);

    if (wxID_OK != fd.ShowModal())
        return;

    filenameM = fd.GetPath();
    styled_text_ctrl_sql->SaveFile(fd.GetPath());
    statusbar_1->SetStatusText((_("File saved")), 2);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnButtonSaveClick(wxCommandEvent& event)
{
    if (filenameM.IsEmpty())
        OnButtonSaveAsClick(event);
    else
    {
        styled_text_ctrl_sql->SaveFile(filenameM);
        statusbar_1->SetStatusText((_("File saved")), 2);
    }
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::updateHistoryButtons()
{
    StatementHistory& sh = StatementHistory::get(databaseM);
    button_prev->Enable(historyPositionM > 0 && sh.size() > 0);
    button_next->Enable(sh.size() > historyPositionM);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnButtonPrevClick(wxCommandEvent& WXUNUSED(event))
{
    StatementHistory& sh = StatementHistory::get(databaseM);
    if (historyPositionM > 0 && sh.size() > 0)
    {
        if (historyPositionM == sh.size())  // we're on local buffer => store it
            localBuffer = styled_text_ctrl_sql->GetText();
        historyPositionM--;
        styled_text_ctrl_sql->SetText(sh.get(historyPositionM));
    }
    updateHistoryButtons();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnButtonNextClick(wxCommandEvent& WXUNUSED(event))
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
    updateHistoryButtons();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnButtonHistoryClick(wxCommandEvent& WXUNUSED(event))
{
    StatementHistory& sh = StatementHistory::get(databaseM);
    StatementHistoryDialog *shf = new StatementHistoryDialog(this, &sh);
    shf->ShowModal();
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
    button_commit->Enable(started);
    button_rollback->Enable(started);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::setSql(wxString sql)
{
    styled_text_ctrl_sql->SetText(sql);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnButtonExecuteClick(wxCommandEvent& WXUNUSED(event))
{
    bool clearMessages = false;
    config().getValue(wxT("SQLEditorExecuteClears"), clearMessages);
    if (clearMessages)
        styled_text_ctrl_stats->ClearAll();

    prepareAndExecute(false);
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
            ok = execute(styled_text_ctrl_sql->GetSelectedText(), prepareOnly);
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
        updateHistoryButtons();
    }
}
//-----------------------------------------------------------------------------
//! adapted so we don't have to change all the other code that utilizes SQL editor
void ExecuteSqlFrame::executeAllStatements(bool closeWhenDone)
{
    bool ok = parseStatements(styled_text_ctrl_sql->GetText(), closeWhenDone);
    if (config().get(wxT("historyStoreGenerated"), true) &&
        (ok || config().get(wxT("historyStoreUnsuccessful"), true)))
    {
        // add buffer to history
        StatementHistory& sh = StatementHistory::get(databaseM);
        sh.add(styled_text_ctrl_sql->GetText());
        historyPositionM = sh.size();
        updateHistoryButtons();
    }
}
//-----------------------------------------------------------------------------
//! Parses all sql statements in STC
//! when autoexecute is TRUE, program just waits user to click Commit/Rollback and closes window
bool ExecuteSqlFrame::parseStatements(const wxString& statements,
    bool closeWhenDone, bool prepareOnly, int selectionOffset)
{
    wxBusyCursor cr;
    styled_text_ctrl_stats->Clear();

    using namespace std;
    terminatorM = wxT(";");
    wxString commands = statements;

    // find terminator, and execute the statement
    wxString::size_type oldpos = 0;
    wxString::size_type searchpos = 0;
    while (true)
    {
        wxString::size_type pos = commands.find(terminatorM, searchpos);
        wxString::size_type quote = commands.find(wxT("'"), searchpos);        // watch for quoted text
        wxString::size_type comment1 = commands.find(wxT("/*"), searchpos);    // watch for commented text
        wxString::size_type comment2 = commands.find(wxT("--"), searchpos);    // watch for commented text

        // check if terminator is maybe inside quotes or comments
        if (pos != wxString::npos)                                             // terminator found
        {
            // find the closest (check for quotes first)
            if (quote != wxString::npos && quote < pos &&                      // found & before terminator
                (comment1 == wxString::npos || quote < comment1) &&            // before comment1
                (comment2 == wxString::npos || quote < comment2))              // before comment2
            {
                searchpos = commands.find(wxT("'"), quote+1);              // end quote
                if (searchpos++ != wxString::npos)
                    continue;
                pos = wxString::npos;
            }

            // check for comment1
            if (pos != wxString::npos &&
                comment1 != wxString::npos && comment1 < pos &&                // found & before terminator
                (comment2 == wxString::npos || comment1 < comment2))           // before comment2
            {
                searchpos = commands.find(wxT("*/"), comment1 + 1);        // end comment
                if (searchpos++ != wxString::npos)
                    continue;
                pos = wxString::npos;
            }

            // check for comment2
            if (pos != wxString::npos &&
                comment2 != wxString::npos && comment2 < pos)                  // found & before terminator
            {
                searchpos = commands.find(wxT("\n"), comment2 + 1);        // end comment
                if (searchpos++ != wxString::npos)
                    continue;
                pos = wxString::npos;
            }
        }

        wxString::size_type lastpos = (pos == wxString::npos ? commands.length() : pos);
        wxString sql = commands.substr(oldpos, lastpos-oldpos);
        sql.erase(sql.find_last_not_of(wxT("\n\r\t ")) + 1);    // right-trim the statement

        stringstream strstrm;            // Search and intercept
        std::string first, second, third;    // SET TERM and COMMIT statements
        wxString strippedSql(sql);
        SimpleParser::removeComments(strippedSql, wxT("/*"), wxT("*/"));
        SimpleParser::removeComments(strippedSql, wxT("--"), wxT("\n"));
        strstrm << wx2std(strippedSql.Upper());
        strstrm >> first;
        strstrm >> second;
        strstrm >> third;
        if (first == "COMMIT")
            commitTransaction();
        else if (first == "ROLLBACK")
            rollbackTransaction();
        else if (first == "SET" && (second == "TERM" || second == "TERMINATOR"))
        {
            searchpos = oldpos = lastpos + terminatorM.length();    // has to be here since terminator length can change
            terminatorM = std2wx(third);
            if (terminatorM.empty())
            {
                ::wxMessageBox(_("SET TERM command found without terminator.\nStopping further execution."),
                    _("Warning."), wxOK | wxICON_WARNING);
                terminatorM = wxT(";");
                break;
            }
            continue;
        }
        else if (first == "SET" && (second == "AUTO" || second == "AUTODDL"))
        {
            if (third == "ON")
                autoCommitM = true;
            else if (third == "OFF")
                autoCommitM = false;
            else
                autoCommitM = !autoCommitM;
        }
        else
        {
            if (sql.length())
            {
                if (!execute(sql, prepareOnly))
                {
                    styled_text_ctrl_sql->centerCaret(true);
                    styled_text_ctrl_sql->GotoPos(selectionOffset+(int)lastpos);
                    styled_text_ctrl_sql->GotoPos(selectionOffset+(int)oldpos);
                    styled_text_ctrl_sql->SetSelectionStart(selectionOffset+(int)oldpos);        // select the text in STC
                    styled_text_ctrl_sql->SetSelectionEnd(selectionOffset+(int)lastpos);        // that failed to execute
                    styled_text_ctrl_sql->centerCaret(false);
                    styled_text_ctrl_sql->SetFocus();
                    return false;
                }
            }
        }

        if (pos == wxString::npos)    // last statement
            break;
        searchpos = oldpos = lastpos + terminatorM.length();
    }

    if (closeWhenDone)
    {
        closeWhenTransactionDoneM = true;
        button_execute->Disable();
        button_commit->SetFocus();
    }

    return true;
}
//-----------------------------------------------------------------------------
//! when autoexecute is TRUE, program just waits user to click Commit/Rollback and closes window
bool ExecuteSqlFrame::execute(wxString sql, bool prepareOnly)
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

        grid_data->ClearGrid(); // statement object will be invalidated, so clear the grid
        statementM = IBPP::StatementFactory(databaseM->getIBPPDatabase(), transactionM);
        log(_("Preparing query: " + sql), ttSql);
        statementM->Prepare(wx2std(sql));

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
            grid_data->fill(databaseM->getConnectionCharset());
            notebook_1->SetSelection(1);
            grid_data->SetFocus();
        }
        else                                // for other statements: show rows affected
        {
            wxString::size_type p = sql.find_first_not_of(wxT(" \n\t\r"));    // left trim
            if (p != wxString::npos && p > 0)
                sql.erase(0, p);
            if (type == IBPP::stInsert || type == IBPP::stDelete || type == IBPP::stExecProcedure
                || type == IBPP::stUpdate)
            {
                wxString s = wxString::Format(_("%d row(s) affected."), statementM->AffectedRows());
                log(wxT("") + s);
                statusbar_1->SetStatusText(s, 1);
            }
            SqlStatement stm(sql, databaseM);
            if (stm.isDDL())
                type = IBPP::stDDL;
            executedStatementsM.push_back(ExecutedStatement(sql, type, terminatorM));
            styled_text_ctrl_sql->SetFocus();
            if (type == IBPP::stDDL && autoCommitM)
                commitTransaction();
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
void ExecuteSqlFrame::OnButtonCommitClick(wxCommandEvent& WXUNUSED(event))
{
    commitTransaction();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnButtonPlanClick(wxCommandEvent& WXUNUSED(event))
{
    prepareAndExecute(true);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::commitTransaction()
{
    wxBusyCursor cr;

    // grid_data->stopFetching();
    if (!transactionM->Started())    // check
    {
        InTransaction(false);
        return;
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
        for (std::vector<ExecutedStatement>::const_iterator it = executedStatementsM.begin(); it != executedStatementsM.end(); ++it)
            if (!Logger::logStatement(*it, databaseM))
                break;

        // parse all successfully executed statements
        for (std::vector<ExecutedStatement>::const_iterator it = executedStatementsM.begin(); it != executedStatementsM.end(); ++it)
        {
            if ((*it).type == IBPP::stDDL)
                if (!databaseM->parseCommitedSql((*it).statement))
                    ::wxMessageBox(lastError().getMessage(), _("A non-fatal error occurred."), wxOK | wxICON_INFORMATION);
        }

        // possible future version (see database.cpp file for details: ONLY IF FIRST solution is used from database.cpp)
        //for (std::vector<wxString>::const_iterator it = executedStatementsM.begin(); it != executedStatementsM.end(); ++it)
        //    databaseM->addCommitedSql(*it);
        //databaseM->parseAll();

        executedStatementsM.clear();
        styled_text_ctrl_stats->ClearAll();

        // workaround for STC bug with 100% CPU load during Idle(),
        // it was supposed to be fixed in wxWidgets versions 2.5.4 and later,
        // but it looks like it is not (at least for gtk1)
        styled_text_ctrl_stats->SetWrapMode(wxSTC_WRAP_WORD);
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
        log(_("ERROR!\nA non-IBPP C++ runtime exception occured!"), ttError);
    }

    notebook_1->SetSelection(0);

    // apparently is has to be at the end to have any effect
    styled_text_ctrl_sql->SetFocus();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnButtonRollbackClick(wxCommandEvent& WXUNUSED(event))
{
    wxBusyCursor cr;
    rollbackTransaction();
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
void ExecuteSqlFrame::OnButtonToggleClick(wxCommandEvent& WXUNUSED(event))
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
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnGridRowCountChanged(wxCommandEvent &event)
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
            //SplitScreen();    // not needed atm, might be later (see TODO above)
            splitter_window_1->Unsplit(panel_splitter_top);        // show grid only
        }
    }
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::update()
{
    if (!databaseM->isConnected())
        Close();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::setDatabase(Database *db)
{
    databaseM = db;

    wxString s = wxString::Format(wxT("%s@%s:%s"), db->getUsername().c_str(),
        db->getServer()->getName_().c_str(), db->getPath().c_str());
    statusbar_1->SetStatusText(s, 0);

    transactionM = IBPP::TransactionFactory(databaseM->getIBPPDatabase());
    db->attachObserver(this);    // observe database object

    executedStatementsM.clear();
    InTransaction(false);    // enable/disable controls
    setKeywords();           // set words for autocomplete feature

    historyPositionM = StatementHistory::get(databaseM).size();
    updateHistoryButtons();

    // set drop target for DnD
    styled_text_ctrl_sql->SetDropTarget(new DnDText(styled_text_ctrl_sql, databaseM));
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
    const Identifier::keywordContainer& k = Identifier::getKeywordSet();
    for (Identifier::keywordContainer::const_iterator ci = k.begin(); ci != k.end(); ++ci)
        as.Add((*ci).Upper());

    // get list od database objects' names
    std::vector<Identifier> v;
    databaseM->getIdentifiers(v);
    for (std::vector<Identifier>::const_iterator it = v.begin(); it != v.end(); ++it)
        as.Add((*it).get());
    as.Sort();    // The list has to be sorted for autocomplete to work properly

    keywordsM.Empty();                          // create final wxString from array
    for (size_t i = 0; i < as.GetCount(); ++i)  // words are separated with spaces
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
    ExecuteSqlFrame *eff = new ExecuteSqlFrame(w, -1, _("Dropping field"));
    eff->setDatabase(c->getDatabase());
    eff->Show();
    eff->setSql(sql);
    eff->executeAllStatements(true);        // true = user must commit/rollback + frame is closed at once
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
        ExecuteSqlFrame *eff = new ExecuteSqlFrame(w, -1, _("Dropping fields"));
        eff->setDatabase(t->getDatabase());
        eff->Show();
        eff->setSql(sql);
        eff->executeAllStatements(true);        // true = user must commit/rollback + frame is closed at once
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

    ExecuteSqlFrame* eff = new ExecuteSqlFrame(w->GetParent(), -1, wxT("DROP"));
    eff->setDatabase(m->getDatabase());
    eff->Show();
    eff->setSql(m->getDropSqlStatement());
    eff->executeAllStatements(true);        // true = user must commit/rollback + frame is closed at once
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

    ExecuteSqlFrame* eff = new ExecuteSqlFrame(w->GetParent(), -1, wxT("DDL"));
    eff->setDatabase(m->getDatabase());
    eff->setSql(cdv.getSql());
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

    ExecuteSqlFrame *eff = new ExecuteSqlFrame(w->GetParent(), -1, _("Editing stored procedure"));
    eff->setDatabase(p->getDatabase());
    eff->Show();
    eff->setSql(p->getAlterSql());
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

    ExecuteSqlFrame *eff = new ExecuteSqlFrame(w->GetParent(), -1,
        _("Altering view"));
    eff->setDatabase(r->getDatabase());
    eff->Show();
    eff->setSql(r->getRebuildSql());
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

    ExecuteSqlFrame *eff = new ExecuteSqlFrame(w->GetParent(), -1, _("Editing trigger"));
    eff->setDatabase(t->getDatabase());
    eff->Show();
    eff->setSql(t->getAlterSql());
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

    g->loadValue(true);    // force reload of value from database
    int64_t oldvalue = g->getValue();
    Database *db = g->getDatabase();
    if (!db)
    {
        wxMessageBox(_("No database assigned"), _("Warning"), wxOK | wxICON_ERROR);
        return true;
    }

    wxString value = wxGetTextFromUser(_("Changing generator value"), _("Enter new value"),
// MH: I have no idea if this works on all systems... but it should be better
        wxLongLong(oldvalue).ToString(), w);
//        wxString::Format(wxT("%d"), oldvalue), w);
    if (value != wxT(""))
    {
        wxString sql = wxT("SET GENERATOR ") + g->getQuotedName() + wxT(" TO ") + value + wxT(";");
        ExecuteSqlFrame *esf = new ExecuteSqlFrame(w, -1, sql);
        esf->setDatabase(db);
        esf->Show();
        esf->setSql(sql);
        esf->executeAllStatements(true);        // true = user must commit/rollback + frame is closed at once
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

    ExecuteSqlFrame* eff = new ExecuteSqlFrame(w->GetParent(), -1, _("Editing exception"));
    eff->setDatabase(e->getDatabase());
    eff->Show();
    eff->setSql(e->getAlterSql());
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

    ExecuteSqlFrame *eff = new ExecuteSqlFrame(w, -1, wxEmptyString);
    eff->setDatabase(i->getDatabase());
    eff->Show();
    eff->setSql(sql);
    eff->executeAllStatements(true);        // true = user must commit/rollback + frame is closed at once
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

    ExecuteSqlFrame *eff = new ExecuteSqlFrame(w, -1, wxEmptyString);
    eff->setDatabase(t->getDatabase());
    eff->Show();
    eff->setSql(sql);
    eff->executeAllStatements(true);        // true = user must commit/rollback + frame is closed at once
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

    ExecuteSqlFrame *eff = new ExecuteSqlFrame(w, -1, wxEmptyString);
    eff->setDatabase(t->getDatabase());
    eff->Show();
    eff->setSql(sql);
    eff->executeAllStatements(true);        // true = user must commit/rollback + frame is closed at once
    return true;
}
//-----------------------------------------------------------------------------
