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
#include <wx/file.h>
#include <wx/datetime.h>
#include <wx/tokenzr.h>
#include <wx/fontdlg.h>

#include <sstream>
#include <string>
#include <algorithm>
#include <vector>
#include <map>

#include "simpleparser.h"
#include "ExecuteSqlFrame.h"
#include "config.h"
#include "dberror.h"

// TODO: USE_MYDATAGRID
#ifndef USE_MYDATAGRID
#include "frDataGrid.h"
#include "frDataGridTable.h"
#endif

#include "metadata/server.h"
#include "metadata/procedure.h"
#include "metadata/view.h"
#include "styleguide.h"
#include "ugly.h"
#include "urihandler.h"
//-----------------------------------------------------------------------------
bool DnDText::OnDropText(wxCoord, wxCoord, const wxString& text)
{
	if (text.Mid(0, 7) != wxT("OBJECT:"))
		return false;

	long address;
	if (!text.Mid(7).ToLong(&address))
		return false;
	YxMetadataItem *m = (YxMetadataItem *)address;

	YDatabase *db = m->getDatabase();
	if (db != databaseM)
	{
		wxMessageBox(_("Cannot use objects from different databases."), _("Wrong database."), wxOK|wxICON_WARNING);
		return false;
	}

	YTable *t = 0;
	std::string column_list;
	if (m->getType() == ntColumn)
	{
		t = dynamic_cast<YTable *>(m->getParent());
		column_list = t->getName() + "." + m->getName();
	}
	if (m->getType() == ntTable)
	{
		t = dynamic_cast<YTable *>(m);
		column_list = t->getName() + ".*";
	}
	if (t == 0)
	{
		wxMessageBox(_("Only tables and table columns can be dropped."), _("Object type not supported."), wxOK|wxICON_WARNING);
		return false;
	}

	// setup complete. now the actual stuff:
	std::string sql = wx2std(ownerM->GetText().Upper());

	// currently we don't support having comments and quotes (it's complicated)
	//if (!Parser::stripSql(sql))
	//	return true;

	std::string::size_type psel, pfrom;
	psel = sql.find("SELECT");
	if (psel == std::string::npos)							// simple select statement
	{
		sql = "SELECT " + column_list + "\nFROM " + t->getName();
		ownerM->SetText(std2wx(sql));
		return true;
	}

	pfrom = sql.find("FROM", psel);
	if (pfrom == std::string::npos)
	{
		wxMessageBox(_("SELECT present, but FROM missing."), _("Unable to parse the statement"), wxOK|wxICON_WARNING);
		return true;
	}

	// read in the table names, and find position where FROM clause ends
	std::vector<std::string> tableNames;
	std::string::size_type from_end = pfrom + Parser::getTableNames(tableNames, sql.substr(pfrom));

	// if table is not there, add it
	if (std::find(tableNames.begin(), tableNames.end(), t->getName()) == tableNames.end())
	{
		std::vector<Join> relatedTables;
		if (YTable::tablesRelate(tableNames, t, relatedTables))	// foreign keys
		{
			std::string join_list;
			if (relatedTables.size() > 1)	// let the user decide
			{
				wxArrayString as;
				for (std::vector<Join>::iterator it = relatedTables.begin(); it != relatedTables.end(); ++it)
					as.Add(std2wx((*it).table));
				int selected = ::wxGetSingleChoiceIndex(_("Multiple foreign keys found"),
					_("Select the desired table"), as);
				if (selected == -1)
					return false;
				join_list = relatedTables[selected].fields;
			}
			else
				join_list = relatedTables[0].fields;

			// FIXME: dummy test value
			// can_be_null = (check if any of the FK fields can be null)
			bool can_be_null = true;

			std::string insert = (can_be_null?" LEFT":"");
			insert += " JOIN " + t->getName() + " ON " + join_list;
			insert = "\n" + insert + " ";
			sql.insert(from_end, insert);
		}
		else
		{
			sql.insert(pfrom + 5, " ");
			if (!tableNames.empty())
				sql.insert(pfrom+5, ",");
			sql.insert(pfrom + 5, t->getName());
		}
	}

	// add columns to SELECT. Possible solutions include either psel+8 or pfrom + 1. I picked pfrom + 1.
	sql.insert(pfrom, ",\n");
	sql.insert(pfrom+1, column_list);

	ownerM->SetText(std2wx(sql));
    return true;
}
//-----------------------------------------------------------------------------
//! included xpm files, so that icons are compiled into executable
namespace sql_icons {
#include "new.xpm"
#include "load.xpm"
#include "save.xpm"
#include "sqlicon.xpm"
};
//-----------------------------------------------------------------------------
// Setup the Scintilla editor
SqlEditor::SqlEditor(wxWindow *parent, wxWindowID id, ExecuteSqlFrame *frame)
	: wxStyledTextCtrl(parent, id)
{
	frameM = frame;
	std::string s;
	if (config().getValue("SqlEditorFont", s) && !s.empty())
	{
		wxFont f;
		f.SetNativeFontInfo(std2wx(s));
		if (f.Ok())
			StyleSetFont(wxSTC_STYLE_DEFAULT, f);
	}
	else
	{
		wxFont font(styleguide().getEditorFontSize(), wxMODERN, wxNORMAL, wxNORMAL);
		StyleSetFont(wxSTC_STYLE_DEFAULT, font);
	}

	int charset;
	if (config().getValue("SqlEditorCharset", charset))
		StyleSetCharacterSet(wxSTC_STYLE_DEFAULT, charset);

	setup();
}
//-----------------------------------------------------------------------------
//! This code has to be called each time the font has changed, so that the control updates
void SqlEditor::setup()
{
    StyleClearAll();
    StyleSetForeground(0,  wxColour(0x80, 0x00, 0x00));
    StyleSetForeground(1,  wxColour(0x00, 0xa0, 0x00));		// multiline comment
    StyleSetForeground(2,  wxColour(0x00, 0xa0, 0x00));		// one-line comment
    StyleSetForeground(3,  wxColour(0x00, 0xff, 0x00));
    StyleSetForeground(4,  wxColour(0x00, 0x00, 0xff));		// number
    StyleSetForeground(5,  wxColour(0x00, 0x00, 0x7f));		// keyword
    StyleSetForeground(6,  wxColour(0x00, 0x00, 0xff));		// 'single quotes'
    StyleSetForeground(7,  wxColour(0xff, 0x00, 0xff));
    StyleSetForeground(8,  wxColour(0x00, 0x7f, 0x7f));
    StyleSetForeground(9,  wxColour(0xff, 0x00, 0x00));
    StyleSetForeground(10, wxColour(0x00, 0x00, 0x00));		// ops
    StyleSetForeground(11, wxColour(0x00, 0x00, 0x00));
    StyleSetBackground(wxSTC_STYLE_BRACELIGHT, wxColour(0xff, 0xcc, 0x00));		// brace highlight
    StyleSetBackground(wxSTC_STYLE_BRACEBAD, wxColour(0xff, 0x33, 0x33));		// brace bad highlight
    StyleSetBold(5,  TRUE);
    StyleSetBold(10, TRUE);
    StyleSetBold(wxSTC_STYLE_BRACELIGHT, TRUE);
    StyleSetBold(wxSTC_STYLE_BRACEBAD, TRUE);
	StyleSetItalic(2, TRUE);
	StyleSetItalic(1, TRUE);
	SetLexer(wxSTC_LEX_SQL);
    SetKeyWords(0,
			wxT("abs action active add admin after all alter and any as asc ascending at auto autoddl ")
			wxT("avg based basename base_name before begin between bigint blob blobedit boolean both ")
			wxT("break buffer by cache cascade case cast char character character_length char_length ")
			wxT("check check_point_len check_point_length close coalesce collate collation column ")
			wxT("commit committed compiletime computed conditional connect constraint containing ")
			wxT("continue count create cstring current current_connection current_date current_role ")
			wxT("current_time current_timestamp current_transaction current_user cursor database date day ")
			wxT("db_key debug dec decimal declare default delete deleting desc descending describe ")
			wxT("descriptor disconnect display distinct do domain double drop echo edit else end entry_point ")
			wxT("escape event exception execute exists exit extern external extract false fetch file ")
			wxT("filter first float for foreign found free_it from full function gdscode generator gen_id ")
			wxT("global goto grant group group_commit_ group_commit_wait having help hour if iif ")
			wxT("immediate in inactive index indicator init inner input input_type insert inserting int ")
			wxT("integer into is isolation isql join key last lc_messages lc_type leading leave left length ")
			wxT("lev level like lock logfile log_buffer_size log_buf_size long manual max maximum ")
			wxT("maximum_segment max_segment merge message min minimum minute module_name month names national ")
			wxT("natural nchar no noauto not null nullif nulls numeric num_log_buffers num_log_bufs ")
			wxT("octet_length of on only open option or order outer output output_type overflow page pagelength ")
			wxT("pages page_size parameter password percent plan position post_event precision ")
			wxT("prepare preserve primary privileges procedure protected public quit raw_partitions ")
			wxT("rdb$db_key read real record_version recreate references release reserv reserving restrict ")
			wxT("retain return returning_values returns revoke right role rollback rows row_count ")
			wxT("runtime savepoint schema second segment select set shadow shared shell show singular size ")
			wxT("skip smallint snapshot some sort sqlcode sqlerror sqlwarning stability starting ")
			wxT("starts statement static statistics substring sub_type sum suspend table temporary ")
			wxT("terminator then ties time timestamp to trailing transaction translate translation trigger ")
			wxT("trim true type uncommitted union unique unknown update updating upper user using value ")
			wxT("values varchar variable varying version view wait wait_time weekday when whenever where ")
			wxT("while with work write year yearday" )
	);

	SetTabWidth(4);
	SetIndent(4);
	SetUseTabs(false);
	SetTabIndents(true);
	SetBackSpaceUnIndents(true);
	AutoCompSetIgnoreCase(true);
	AutoCompSetAutoHide(true);		// info in ScintillaDoc.html file (in scintilla source package)
	SetMarginWidth(0, 40);			// turn on the linenumbers margin, set width to 40pixels
	SetMarginWidth(1, 0);			// turn off the folding margin
	SetMarginType(0, 1);			// set margin type to linenumbers
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(SqlEditor, wxStyledTextCtrl)
    EVT_CONTEXT_MENU(SqlEditor::OnContextMenu)
    EVT_MENU(SqlEditor::ID_MENU_UNDO,             SqlEditor::OnMenuUndo)
    EVT_MENU(SqlEditor::ID_MENU_REDO,             SqlEditor::OnMenuRedo)
    EVT_MENU(SqlEditor::ID_MENU_CUT,              SqlEditor::OnMenuCut)
    EVT_MENU(SqlEditor::ID_MENU_COPY,             SqlEditor::OnMenuCopy)
    EVT_MENU(SqlEditor::ID_MENU_PASTE,            SqlEditor::OnMenuPaste)
    EVT_MENU(SqlEditor::ID_MENU_DELETE,           SqlEditor::OnMenuDelete)
    EVT_MENU(SqlEditor::ID_MENU_SELECT_ALL,       SqlEditor::OnMenuSelectAll)
    EVT_MENU(SqlEditor::ID_MENU_SELECT_STATEMENT, SqlEditor::OnMenuSelectStatement)
    EVT_MENU(SqlEditor::ID_MENU_EXECUTE_SELECTED, SqlEditor::OnMenuExecuteSelected)
    EVT_MENU(SqlEditor::ID_MENU_WRAP,             SqlEditor::OnMenuWrap)
    EVT_MENU(SqlEditor::ID_MENU_SET_FONT,         SqlEditor::OnMenuSetFont)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void SqlEditor::OnContextMenu(wxContextMenuEvent& WXUNUSED(event))
{
    wxMenu m(0);
    m.Append(ID_MENU_UNDO, _("Undo"));
    m.Append(ID_MENU_REDO, _("Redo"));
    m.AppendSeparator();
    m.Append(ID_MENU_CUT,    _("Cut"));
    m.Append(ID_MENU_COPY,   _("Copy"));
    m.Append(ID_MENU_PASTE,  _("Paste"));
    m.Append(ID_MENU_DELETE, _("Delete"));
    m.AppendSeparator();
    m.Append(ID_MENU_SELECT_ALL,       _("Select All"));
    m.Append(ID_MENU_SELECT_STATEMENT, _("Select statement"));
    m.Append(ID_MENU_EXECUTE_SELECTED, _("Execute selected"));
    m.AppendSeparator();
    m.Append(ID_MENU_SET_FONT,      _("Set Font"));
    m.AppendCheckItem(ID_MENU_WRAP, _("Wrap"));
	if (wxSTC_WRAP_WORD == GetWrapMode())
		m.Check(ID_MENU_WRAP, true);

	// disable stuff
	m.Enable(ID_MENU_UNDO, CanUndo());
	m.Enable(ID_MENU_REDO, CanRedo());
	if (GetSelectionStart() == GetSelectionEnd())		// nothing is selected
	{
		m.Enable(ID_MENU_CUT,              false);
		m.Enable(ID_MENU_COPY,             false);
		m.Enable(ID_MENU_DELETE,           false);
		m.Enable(ID_MENU_EXECUTE_SELECTED, false);
	}

    PopupMenu(&m, ScreenToClient(::wxGetMousePosition()));
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
	DeleteBackNotLine();
}
//-----------------------------------------------------------------------------
void SqlEditor::OnMenuSelectAll(wxCommandEvent& WXUNUSED(event))
{
	SelectAll();
}
//-----------------------------------------------------------------------------
void SqlEditor::OnMenuSelectStatement(wxCommandEvent& WXUNUSED(event))
{
}
//-----------------------------------------------------------------------------
void SqlEditor::OnMenuExecuteSelected(wxCommandEvent& WXUNUSED(event))
{
	bool single = false;
	config().getValue("TreatAsSingleStatement", single);
	if (single)
		frameM->execute(wx2std(GetSelectedText()));
	else
		frameM->parseStatements(GetSelectedText());
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
	std::string s;		// since we can't get the font from control we ask config() for it
	if (config().getValue("SqlEditorFont", s) && !s.empty())
	{
		f.SetNativeFontInfo(std2wx(s));
		f2 = ::wxGetFontFromUser(this, f);
	}
	else				// if config() doesn't have it, we'll use the default
	{
		wxFont font(styleguide().getEditorFontSize(), wxMODERN, wxNORMAL, wxNORMAL);
		f2 = ::wxGetFontFromUser(this, font);
	}

	if (!f2.Ok())	// user Canceled
		return;
	StyleSetFont(wxSTC_STYLE_DEFAULT, f2);

	// step 2 of 2: set charset
	std::map<std::string, int> sets;		// create human-readable names from wxSTC charsets
	sets["CHARSET_ANSI"] = 0;
	sets["CHARSET_EASTEUROPE"] = 238;
	sets["CHARSET_GB2312"] = 134;
	sets["CHARSET_HANGUL"] = 129;
	sets["CHARSET_HEBREW"] = 177;
	sets["CHARSET_SHIFTJIS"] = 128;
	#ifdef __WXMSW__
	sets["CHARSET_DEFAULT"] = 1;		// according to scintilla docs these only work on Windows
	sets["CHARSET_BALTIC"] = 186;		// so we won't offer them
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
	wxArrayString slist;					// copy to wxArrayString
	for (std::map<std::string, int>::iterator it = sets.begin(); it != sets.end(); ++it)
		slist.Add(std2wx((*it).first));

	wxString c = wxGetSingleChoice(_("Select charset to use"), _("Setting font for editor"), slist, this);
	if (c.IsEmpty())	// Canceled
		return;
	std::map<std::string, int>::iterator it = sets.find(wx2std(c));
	if (it == sets.end())
		return;		// should never happen

	StyleSetCharacterSet(wxSTC_STYLE_DEFAULT, (*it).second);
	if (wxYES == wxMessageBox(_("Would you like to keep these settings permanently?"), _("SQL Editor"), wxYES_NO|wxICON_QUESTION))
	{
		wxString fontdesc = f2.GetNativeFontInfoDesc();
		if (!fontdesc.IsEmpty())
			config().setValue("SqlEditorFont", wx2std(fontdesc));
		config().setValue("SqlEditorCharset", (*it).second);
	}
	setup();	// make control accept new settings
}
//-----------------------------------------------------------------------------
ExecuteSqlFrame::ExecuteSqlFrame(wxWindow* parent, int id, wxString title, const wxPoint& pos, const wxSize& size, long style):
    BaseFrame(parent, id, title, pos, size, style), YxObserver()
{
    panel_contents = new wxPanel(this, -1);
    button_new = new wxBitmapButton(panel_contents, ID_button_new, wxBitmap(sql_icons::new_xpm));
    button_load = new wxBitmapButton(panel_contents, ID_button_load, wxBitmap(sql_icons::load_xpm));
    button_save = new wxBitmapButton(panel_contents, ID_button_save, wxBitmap(sql_icons::save_xpm));
    button_execute = new wxButton(panel_contents, ID_button_execute, _("Execute (F9)"));
    button_commit = new wxButton(panel_contents, ID_button_commit, _("Commit (F5)"));
    button_rollback = new wxButton(panel_contents, ID_button_rollback, _("Rollback (F8)"));
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
// TODO: USE_MYDATAGRID
#ifdef USE_MYDATAGRID
    grid_data = new myDataGrid(notebook_pane_2, ID_grid_data, statementM, statusbar_1);
#else
    grid_data = new DataGrid(notebook_pane_2, ID_grid_data);
    grid_data->SetTable(new GridTable(statementM), true);
#endif
    splitter_window_1->SplitHorizontally(panel_splitter_top, panel_splitter_bottom);

	button_new->SetToolTip(_("New window"));
	button_load->SetToolTip(_("Load"));
	button_save->SetToolTip(_("Save"));
	button_execute->SetToolTip(_("F9 - Execute SQL statement"));
	button_commit->SetToolTip(_("F5 - Commit transaction"));
	button_rollback->SetToolTip(_("F8 - Rollback transaction"));

	splitter_window_1->Unsplit();

    wxBitmap bmp = wxBitmap(sql_icons::sqlicon_xpm);
    wxIcon icon;
    icon.CopyFromBitmap(bmp);
    SetIcon(icon);

	keywordsM = wxT("");
	closeWhenTransactionDoneM = false;
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
    sizer_3->Add(10, 5, 0, 0, 0);
    sizer_3->Add(button_execute, 0, wxALL, 3);
    sizer_3->Add(button_commit, 0, wxALL, 3);
    sizer_3->Add(button_rollback, 0, wxALL, 3);
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
BEGIN_EVENT_TABLE(ExecuteSqlFrame, wxFrame)
	EVT_STC_UPDATEUI(ExecuteSqlFrame::ID_stc_sql, ExecuteSqlFrame::OnSqlEditUpdateUI)
	EVT_STC_CHARADDED(ExecuteSqlFrame::ID_stc_sql, ExecuteSqlFrame::OnSqlEditCharAdded)
	EVT_CHAR_HOOK(ExecuteSqlFrame::OnKeyDown)
	EVT_CLOSE(ExecuteSqlFrame::OnClose)

	EVT_BUTTON(ExecuteSqlFrame::ID_button_new, ExecuteSqlFrame::OnButtonNewClick)
	EVT_BUTTON(ExecuteSqlFrame::ID_button_load, ExecuteSqlFrame::OnButtonLoadClick)
	EVT_BUTTON(ExecuteSqlFrame::ID_button_save, ExecuteSqlFrame::OnButtonSaveClick)
	EVT_BUTTON(ExecuteSqlFrame::ID_button_execute, ExecuteSqlFrame::OnButtonExecuteClick)
	EVT_BUTTON(ExecuteSqlFrame::ID_button_commit, ExecuteSqlFrame::OnButtonCommitClick)
	EVT_BUTTON(ExecuteSqlFrame::ID_button_rollback, ExecuteSqlFrame::OnButtonRollbackClick)
	EVT_BUTTON(ExecuteSqlFrame::ID_button_toggle, ExecuteSqlFrame::OnButtonToggleClick)
// TODO: USE_MYDATAGRID
#ifndef USE_MYDATAGRID
    EVT_COMMAND(ExecuteSqlFrame::ID_grid_data, wxEVT_FRDG_ROWCOUNT_CHANGED, \
        ExecuteSqlFrame::OnGridRowCountChanged)
#endif
END_EVENT_TABLE()
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
	}
	else
		styled_text_ctrl_sql->BraceBadLight(wxSTC_INVALID_POSITION);	// remove light

	if (styled_text_ctrl_sql->GetTextLength() > 5)
		SetTitle(styled_text_ctrl_sql->GetLine(0).Trim());
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
	autoComplete(false);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::autoComplete(bool force)
{
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

	int autoCompleteChars = 3;
	if (force)
		autoCompleteChars = 1;
	else
	{
		config().getValue("AutocompleteChars", autoCompleteChars);
		if (autoCompleteChars == 0)
			return;
	}

	int pos = styled_text_ctrl_sql->GetCurrentPos();
	int start = styled_text_ctrl_sql->WordStartPosition(pos, true);
	if (start != -1 && pos - start >= autoCompleteChars && !styled_text_ctrl_sql->AutoCompActive())
	{
		// GTK version crashes if nothing matches, so this check must be made for GTK
		// For MSW, it doesn't crash but it flashes on the screen (also not very nice)
		if (HasWord(styled_text_ctrl_sql->GetTextRange(start, pos), keywordsM))
			styled_text_ctrl_sql->AutoCompShow(pos-start, keywordsM);
	}
}
//-----------------------------------------------------------------------------
//! handle function keys (F5, F8, F9, ...)
void ExecuteSqlFrame::OnKeyDown(wxKeyEvent &event)
{
	wxCommandEvent e;
	int key = event.GetKeyCode();
	if (key == WXK_F9)
		executeAllStatements();
	if (key == WXK_F5)
		commitTransaction();
	if (key == WXK_F8)
		OnButtonRollbackClick(e);

	if (wxWindow::FindFocus() == styled_text_ctrl_sql)
	{
		if (event.ControlDown() && key == WXK_SPACE)
			autoComplete(true);
		if (key == WXK_RETURN && styled_text_ctrl_sql->AutoCompActive())
			styled_text_ctrl_sql->AutoCompCancel();
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

	styled_text_ctrl_sql->LoadFile(fd.GetPath());
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnButtonSaveClick(wxCommandEvent& WXUNUSED(event))
{
	wxFileDialog fd(this, _("Select file to save"), wxT(""), wxT(""),
		_("SQL Scripts (*.sql)|*.sql|All files (*.*)|*.*"),
		wxSAVE |wxCHANGE_DIR);

	if (wxID_OK != fd.ShowModal())
		return;

	styled_text_ctrl_sql->SaveFile(fd.GetPath());
	statusbar_1->SetStatusText((_("File saved")), 2);
}
//-----------------------------------------------------------------------------
//! enable/disable and show/hide controls depending of transaction status
void ExecuteSqlFrame::InTransaction(bool started)
{
	inTransactionM = started;

	if (!splitter_window_1->IsSplit())		// make sure window is split
	{								// needed in each case
		panel_splitter_top->Show();
		panel_splitter_bottom->Show();
		splitter_window_1->SplitHorizontally(panel_splitter_top, panel_splitter_bottom);
	}

	if (started)
		statusbar_1->SetStatusText(_("Transaction started"), 3);
	else
	{
		splitter_window_1->Unsplit(panel_splitter_bottom);		// show sql entry window
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
	bool hasSelection = styled_text_ctrl_sql->GetSelectionStart() != styled_text_ctrl_sql->GetSelectionEnd();
	bool only = false;
	config().getValue("OnlyExecuteSelected", only);
	if (only && hasSelection)	// something is selected
	{
		bool single = false;
		config().getValue("TreatAsSingleStatement", single);
		if (single)
			execute(wx2std(styled_text_ctrl_sql->GetSelectedText()));
		else
			parseStatements(styled_text_ctrl_sql->GetSelectedText());
	}
	else
		executeAllStatements();
}
//-----------------------------------------------------------------------------
//! adapted so we don't have to change all the other code that utilizes SQL editor
void ExecuteSqlFrame::executeAllStatements(bool closeWhenDone)
{
	parseStatements(styled_text_ctrl_sql->GetText(), closeWhenDone);
}
//-----------------------------------------------------------------------------
//! Parses all sql statements in STC
//! when autoexecute is TRUE, program just waits user to click Commit/Rollback and closes window
void ExecuteSqlFrame::parseStatements(const wxString& statements, bool closeWhenDone)
{
	wxBusyCursor cr;
	styled_text_ctrl_stats->Clear();

	using namespace std;
	string terminator = ";";
	string commands = wx2std(statements);

	// find terminator, and execute the statement
	string::size_type oldpos = 0;
	string::size_type searchpos = 0;
	while (true)
	{
		string::size_type pos = commands.find(terminator, searchpos);
		string::size_type quote = commands.find("'", searchpos);			// watch for quoted text
		string::size_type comment1 = commands.find("/*", searchpos);		// watch for commented text
		string::size_type comment2 = commands.find("--", searchpos);		// watch for commented text

		// check if terminator is maybe inside quotes or comments
		if (pos != string::npos)										// terminator found
		{
			// find the closest (check for quotes first)
			if (quote != string::npos && quote < pos && 				// found & before terminator
				(comment1 == string::npos || quote < comment1) &&		// before comment1
				(comment2 == string::npos || quote < comment2))			// before comment2
			{
				searchpos = 1 + commands.find("'", quote+1);			// end quote
				continue;
			}

			// check for comment1
			if (comment1 != string::npos && comment1 < pos &&			// found & before terminator
				(comment2 == string::npos || comment1 < comment2))		// before comment2
			{
				searchpos = 1 + commands.find("*/", comment1+1);			// end comment
				continue;
			}

			// check for comment2
			if (comment2 != string::npos && comment2 < pos)				// found & before terminator
			{
				searchpos = 1 + commands.find("\n", comment2+1);			// end comment
				continue;
			}
		}

		std::string::size_type lastpos = (pos == std::string::npos ? commands.length() : pos);
		string sql = commands.substr(oldpos, lastpos-oldpos);
		sql.erase(sql.find_last_not_of("\n\r\t ")+1);	// right-trim the statement

		stringstream strstrm;			// Search and intercept
		string first, second, third;	// SET TERM and COMMIT statements
		strstrm << upcase(sql);
		strstrm >> first;
		strstrm >> second;
		strstrm >> third;

		if (first == "COMMIT")
			commitTransaction();
		else if (first == "SET" && (second == "TERM" || second == "TERMINATOR"))
		{
			searchpos = oldpos = lastpos + terminator.length();	// has to be here since terminator length can change
			terminator = third;
			if (terminator.empty())
			{
				::wxMessageBox(_("SET TERM command found without terminator.\nStopping further execution."),
					_("Warning."), wxOK|wxICON_WARNING);
				terminator = ";";
				break;
			}
			continue;
		}
		else if (sql.length() && !execute(sql))
		{
			styled_text_ctrl_sql->SetSelectionStart((int)oldpos);		// select the text in STC
			styled_text_ctrl_sql->SetSelectionEnd((int)lastpos);		// that failed to execute
			return;
		}

		if (pos == string::npos)	// last statement
			break;

		searchpos = oldpos = lastpos + terminator.length();
	}

	if (closeWhenDone)
	{
		closeWhenTransactionDoneM = true;
		button_execute->Disable();
		button_commit->SetFocus();
	}
}
//-----------------------------------------------------------------------------
//! when autoexecute is TRUE, program just waits user to click Commit/Rollback and closes window
bool ExecuteSqlFrame::execute(std::string sql)
{
	if (styled_text_ctrl_sql->AutoCompActive())
		styled_text_ctrl_sql->AutoCompCancel();	// remove the list if needed
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
		statementM = IBPP::StatementFactory(databaseM->getDatabase(), transactionM);
		log(_("Preparing query: " + std2wx(sql)), ttSql);
		statementM->Prepare(sql);

		wxTimeSpan dif = wxDateTime::Now().Subtract(start);
		log(wxString(_("Prepare time: ")) + dif.Format(wxT("%H:%M:%S.")));

		try
		{
			std::string plan;			// for some statements (DDL) it is never available
			statementM->Plan(plan);		// for INSERTs, it is available sometimes (insert into ... select ... )
			log(std2wx(plan));			// but if it not, IBPP throws the exception
		}
		catch(IBPP::Exception&)
		{
			log(_("Plan not available."));
		}

		log(wxT(""));
  		log(wxT(""));
  		log(_("Executing..."));
		statementM->Execute();
		log(_("Done."));

		IBPP::STT type = statementM->Type();
		if (type == IBPP::stSelect)			// for select statements: show data
		{
			grid_data->fill();
			//data_grid->AdjustScrollbars();	// sometimes scrollbars get lost (this should be a solution)
												// if someone can make a reproducible test that always makes scrollbars
												// vanish, then we can test if this is the solution
			notebook_1->SetSelection(1);
			grid_data->SetFocus();
		}
		else								// for other statements: show rows affected
		{
			std::string::size_type p = sql.find_first_not_of(" \n\t\r");	// left trim
			if (p != std::string::npos && p > 0)
				sql.erase(0, p);
			bool changeNull = (sql.substr(0,44) == "UPDATE RDB$RELATION_FIELDS SET RDB$NULL_FLAG");
			bool setGenerator = (sql.substr(0,13) == "SET GENERATOR");

			if (type == IBPP::stInsert || type == IBPP::stDelete || type == IBPP::stExecProcedure
				|| type == IBPP::stUpdate && !changeNull && !setGenerator)
			{
				wxString s = wxString::Format(_("%d row(s) affected."), statementM->AffectedRows());
				log(wxT("") + s);
				statusbar_1->SetStatusText(s, 1);
			}
			else if (type == IBPP::stDDL || changeNull || setGenerator)
				executedStatementsM.push_back(sql);
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
	if (!splitter_window_1->IsSplit())					// split screen if needed
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
void ExecuteSqlFrame::commitTransaction()
{
	wxBusyCursor cr;

    // grid_data->stopFetching();
	if (!transactionM->Started())	// check
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

		// parse all successfully executed statements
		for (std::vector<std::string>::const_iterator it = executedStatementsM.begin(); it != executedStatementsM.end(); ++it)
			if (!databaseM->parseCommitedSql(*it))
				::wxMessageBox(std2wx(lastError().getMessage()), _("A non-fatal error occurred."), wxOK|wxICON_INFORMATION);

		// possible future version (see database.cpp file for details: ONLY IF FIRST solution is used from database.cpp)
		//for (std::vector<std::string>::const_iterator it = executedStatementsM.begin(); it != executedStatementsM.end(); ++it)
		//	databaseM->addCommitedSql(*it);
		//databaseM->parseAll();

		executedStatementsM.clear();
		styled_text_ctrl_stats->ClearAll();
        // workaround for STC bug with 100% CPU load during Idle(),
        // can be removed for wxWidgets versions 2.5.4 and later
        styled_text_ctrl_stats->SetWrapMode(wxSTC_WRAP_WORD);
		styled_text_ctrl_sql->SetFocus();

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
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnButtonRollbackClick(wxCommandEvent& WXUNUSED(event))
{
	wxBusyCursor cr;

    // grid_data->stopFetching();
	if (!transactionM->Started())	// check
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
		styled_text_ctrl_sql->SetFocus();

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
}
//-----------------------------------------------------------------------------
//! toggle the views in the following order:
//! ... -> SQL_entry_box -> Split View -> Stats&Data -> ...
void ExecuteSqlFrame::OnButtonToggleClick(wxCommandEvent& WXUNUSED(event))
{
	if (splitter_window_1->IsSplit())					// screen is split -> show second
		splitter_window_1->Unsplit(panel_splitter_top);
	else if (panel_splitter_top->IsShown())		// first is shown -> split again
		SplitScreen();
	else										// second is shown -> show first
	{
		panel_splitter_top->Show();
		panel_splitter_bottom->Show();
		splitter_window_1->SplitHorizontally(panel_splitter_top, panel_splitter_bottom);
		splitter_window_1->Unsplit(panel_splitter_bottom);
	}
}
// TODO: USE_MYDATAGRID
#ifndef USE_MYDATAGRID
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnGridRowCountChanged(wxCommandEvent &event)
{
    wxString s;
    s.Printf(_("%d rows fetched"), event.GetExtraLong());
    statusbar_1->SetStatusText(s, 1);
}
#endif
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::update()
{
	if (!databaseM->isConnected())
		Close();
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::setDatabase(YDatabase *db)
{
	databaseM = db;

	std::string s = db->getUsername() + "@" + db->getPath();
	statusbar_1->SetStatusText(std2wx(s), 0);

	transactionM = IBPP::TransactionFactory(databaseM->getDatabase());
	db->attach(this);	// observe database object

	executedStatementsM.clear();
	InTransaction(false);	// enable/disable controls
	setKeywords();			// set words for autocomplete feature

	// set drop target for DnD
    styled_text_ctrl_sql->SetDropTarget(new DnDText(styled_text_ctrl_sql, databaseM));
}
//-----------------------------------------------------------------------------
//! closes window if database is removed (unregistered)
void ExecuteSqlFrame::removeObservedObject(YxSubject *object)
{
	YxObserver::removeObservedObject(object);
	if (object == databaseM)
		Close();
}
//-----------------------------------------------------------------------------
//! Creates a list for autocomplete feature

//! The list consists of:
//! - sql keywords (longer that 4 characters)
//! - names of database objects (tables, views, etc.)
//
void ExecuteSqlFrame::setKeywords()
{
	wxArrayString as;

	// a bunch of as.Add("something") statements, placed in separate file
	#include "autocomplete-sql_keywords.txt"

	// get list od database objects' names
	std::vector<std::string> v;
	databaseM->getIdentifiers(v);
	for (std::vector<std::string>::const_iterator it = v.begin(); it != v.end(); ++it)
		as.Add(std2wx(*it));

	// TODO:
	// we can also make ExecuteSqlFrame observer of YTables/YViews/... objects
	// so it can reload this list if something changes

	as.Sort();	// The list has to be sorted for autocomplete to work properly

	keywordsM = wxT("");						// create final string from array
	for (size_t i = 0; i<as.GetCount(); ++i)	// words are separated with spaces
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
const std::string ExecuteSqlFrame::getName() const
{
	return "ExecuteSqlFrame";
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::doReadConfigSettings(const std::string& prefix)
{
	BaseFrame::doReadConfigSettings(prefix);
	int zoom;
	if (config().getValue(prefix + "::zoom", zoom))
		styled_text_ctrl_sql->SetZoom(zoom);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::doWriteConfigSettings(const std::string& prefix) const
{
	BaseFrame::doWriteConfigSettings(prefix);
	config().setValue(prefix + "::zoom", styled_text_ctrl_sql->GetZoom());
}
//-----------------------------------------------------------------------------
const wxRect ExecuteSqlFrame::getDefaultRect() const
{
    return wxRect(-1, -1, 528, 486);
}
//-----------------------------------------------------------------------------
//! also used to drop constraints
class DropColumnHandler: public YxURIHandler
{
public:
	bool handleURI(std::string& uriStr);
private:
    // singleton; registers itself on creation.
    static const DropColumnHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const DropColumnHandler DropColumnHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool DropColumnHandler::handleURI(std::string& uriStr)
{
    YURI uriObj(uriStr);
	if (uriObj.action != "drop_field" && uriObj.action != "drop_constraint")
		return false;

	std::string ms = uriObj.getParam("object_address");		// object
	unsigned long mo;
	if (!std2wx(ms).ToULong(&mo))
		return true;
	YxMetadataItem *c = (YxMetadataItem *)mo;

	ms = uriObj.getParam("parent_window");		// window
	if (!std2wx(ms).ToULong(&mo))
		return true;
	wxWindow *w = (wxWindow *)mo;

	if (c)
	{
		std::string sql = "ALTER TABLE " + c->getParent()->getName() + " DROP ";
		if (uriObj.action == "drop_constraint")
			sql += "CONSTRAINT ";
		sql += c->getName();
		ExecuteSqlFrame *eff = new ExecuteSqlFrame(w, -1, _("Dropping field"));
		eff->setDatabase(c->getDatabase());
		eff->Show();
		eff->setSql(std2wx(sql));
		eff->executeAllStatements(true);		// true = user must commit/rollback + frame is closed at once
	}
	return true;
}
//-----------------------------------------------------------------------------
class EditProcedureHandler: public YxURIHandler
{
public:
	bool handleURI(std::string& uriStr);
private:
    // singleton; registers itself on creation.
    static const EditProcedureHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const EditProcedureHandler EditProcedureHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool EditProcedureHandler::handleURI(std::string& uriStr)
{
    YURI uriObj(uriStr);
	if (uriObj.action != "edit_procedure")
		return false;

	std::string ms = uriObj.getParam("object_address");		// object
	unsigned long mo;
	if (!std2wx(ms).ToULong(&mo))
		return true;
	YProcedure *p = (YProcedure *)mo;

	ms = uriObj.getParam("parent_window");		// window
	if (!std2wx(ms).ToULong(&mo))
		return true;
	wxWindow *w = (wxWindow *)mo;

	if (p)
	{
		ExecuteSqlFrame *eff = new ExecuteSqlFrame(w->GetParent(), -1, _("Editing stored procedure"));
		eff->setDatabase(p->getDatabase());
		eff->Show();
		eff->setSql(std2wx(p->getAlterSql()));
	}
	return true;
}
//-----------------------------------------------------------------------------
class EditViewHandler: public YxURIHandler
{
public:
	bool handleURI(std::string& uriStr);
private:
    // singleton; registers itself on creation.
    static const EditViewHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const EditViewHandler EditViewHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool EditViewHandler::handleURI(std::string& uriStr)
{
    YURI uriObj(uriStr);
	if (uriObj.action != "edit_view")
		return false;

	std::string ms = uriObj.getParam("object_address");		// object
	unsigned long mo;
	if (!std2wx(ms).ToULong(&mo))
		return true;
	YView *v = (YView *)mo;

	ms = uriObj.getParam("parent_window");		// window
	if (!std2wx(ms).ToULong(&mo))
		return true;
	wxWindow *w = (wxWindow *)mo;

	if (v)
	{
		ExecuteSqlFrame *eff = new ExecuteSqlFrame(w->GetParent(), -1, _("Editing view"));
		eff->setDatabase(v->getDatabase());
		eff->Show();
		eff->setSql(std2wx(v->getAlterSql()));
	}
	return true;
}
//-----------------------------------------------------------------------------
class EditTriggerHandler: public YxURIHandler
{
public:
	bool handleURI(std::string& uriStr);
private:
    // singleton; registers itself on creation.
    static const EditTriggerHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const EditTriggerHandler EditTriggerHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool EditTriggerHandler::handleURI(std::string& uriStr)
{
    YURI uriObj(uriStr);
	if (uriObj.action != "edit_trigger")
		return false;

	std::string ms = uriObj.getParam("object_address");		// object
	unsigned long mo;
	if (!std2wx(ms).ToULong(&mo))
		return true;
	YTrigger *t = (YTrigger *)mo;

	ms = uriObj.getParam("parent_window");		// window
	if (!std2wx(ms).ToULong(&mo))
		return true;
	wxWindow *w = (wxWindow *)mo;

	if (t)
	{
		ExecuteSqlFrame *eff = new ExecuteSqlFrame(w->GetParent(), -1, _("Editing trigger"));
		eff->setDatabase(t->getDatabase());
		eff->Show();
		eff->setSql(std2wx(t->getAlterSql()));
	}
	return true;
}
//-----------------------------------------------------------------------------
class EditGeneratorValueHandler: public YxURIHandler
{
public:
	bool handleURI(std::string& uriStr);
private:
    // singleton; registers itself on creation.
    static const EditGeneratorValueHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const EditGeneratorValueHandler EditGeneratorValueHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool EditGeneratorValueHandler::handleURI(std::string& uriStr)
{
    YURI uriObj(uriStr);
	if (uriObj.action != "edit_generator_value")
		return false;

	std::string ms = uriObj.getParam("object_address");		// object
	unsigned long mo;
	if (!std2wx(ms).ToULong(&mo))
		return true;
	YGenerator *g = (YGenerator *)mo;

	ms = uriObj.getParam("parent_window");		// window
	if (!std2wx(ms).ToULong(&mo))
		return true;
	wxWindow *w = (wxWindow *)mo;

	if (g)
	{
    	g->loadValue(true);	// force reload of value from database
	    int oldvalue = g->getValue();
	    YDatabase *db = g->getDatabase();
	    if (!db)
	    {
		    wxMessageBox(_("No database assigned"), _("Warning"), wxOK | wxICON_ERROR);
		    return true;
	    }

	    wxString value = wxGetTextFromUser(_("Changing generator value"), _("Enter new value"),
		    wxString::Format(wxT("%d"), oldvalue), w);
	    if (value != wxT(""))
	    {
		    std::string sql = "SET GENERATOR " + g->getName() + " TO " + wx2std(value) + ";";
		    ExecuteSqlFrame *esf = new ExecuteSqlFrame(w, -1, wxString(std2wx(sql)));
		    esf->setDatabase(db);
		    esf->Show();
		    esf->setSql(std2wx(sql));
		    esf->executeAllStatements(true);		// true = user must commit/rollback + frame is closed at once
	    }
	}
	return true;
}
//-----------------------------------------------------------------------------
class EditExceptionHandler: public YxURIHandler
{
public:
	bool handleURI(std::string& uriStr);
private:
    // singleton; registers itself on creation.
    static const EditExceptionHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const EditExceptionHandler EditExceptionHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool EditExceptionHandler::handleURI(std::string& uriStr)
{
    YURI uriObj(uriStr);
	if (uriObj.action != "edit_exception")
		return false;

	std::string ms = uriObj.getParam("object_address");		// object
	unsigned long mo;
	if (!std2wx(ms).ToULong(&mo))
		return true;
	YException *e = (YException *)mo;

	ms = uriObj.getParam("parent_window");		// window
	if (!std2wx(ms).ToULong(&mo))
		return true;
	wxWindow *w = (wxWindow *)mo;

	if (e)
	{
		ExecuteSqlFrame *eff = new ExecuteSqlFrame(w->GetParent(), -1, _("Editing exception"));
		eff->setDatabase(e->getDatabase());
		eff->Show();
		eff->setSql(std2wx(e->getAlterSql()));
	}
	return true;
}
//-----------------------------------------------------------------------------
