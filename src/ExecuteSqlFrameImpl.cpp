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
#include <string>
#include <sstream>
#include "ExecuteSqlFrame.h"
#include "ugly.h"
#include "config.h"
#include "dberror.h"
#include "metadata/server.h"
#include "metadata/procedure.h"
#include "metadata/view.h"
#include "urihandler.h"

//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(ExecuteSqlFrame, wxFrame)
	EVT_STC_UPDATEUI(ExecuteSqlFrame::ID_sql_edit, ExecuteSqlFrame::OnSqlEditUpdateUI)
	EVT_STC_CHARADDED(ExecuteSqlFrame::ID_sql_edit, ExecuteSqlFrame::OnSqlEditCharAdded)
	EVT_CHAR_HOOK(ExecuteSqlFrame::OnKeyDown)
	EVT_CLOSE(ExecuteSqlFrame::OnClose)

	EVT_BUTTON(ExecuteSqlFrame::ID_button_new, ExecuteSqlFrame::OnButtonNewClick)
	EVT_BUTTON(ExecuteSqlFrame::ID_button_load, ExecuteSqlFrame::OnButtonLoadClick)
	EVT_BUTTON(ExecuteSqlFrame::ID_button_save, ExecuteSqlFrame::OnButtonSaveClick)
	EVT_BUTTON(ExecuteSqlFrame::ID_button_execute, ExecuteSqlFrame::OnButtonExecuteClick)
	EVT_BUTTON(ExecuteSqlFrame::ID_button_commit, ExecuteSqlFrame::OnButtonCommitClick)
	EVT_BUTTON(ExecuteSqlFrame::ID_button_rollback, ExecuteSqlFrame::OnButtonRollbackClick)
	EVT_BUTTON(ExecuteSqlFrame::ID_button_toggle, ExecuteSqlFrame::OnButtonToggleClick)
	EVT_BUTTON(ExecuteSqlFrame::ID_button_wrap, ExecuteSqlFrame::OnButtonWrapClick)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
//! display editor col:row in StatusBar and do highlighting of braces ()
void ExecuteSqlFrame::OnSqlEditUpdateUI(wxStyledTextEvent& WXUNUSED(event))
{
	// print x:y coordinates in status bar
	int p = sql_edit->GetCurrentPos();
	int row = sql_edit->GetCurrentLine();
	int col = p - sql_edit->PositionFromLine(row);
	execute_sql_frame_statusbar->SetStatusText(wxString::Format(wxT("%d : %d"), col+1, row+1), 2);

	// check for braces, and highlight
	int c1 = sql_edit->GetCharAt(p);
	int c2 = (p > 1 ? sql_edit->GetCharAt(p-1) : 0);

	if (c2=='(' || c2==')' || c1=='(' || c1==')')
	{
		int sp = (c2=='(' || c2==')') ? p-1 : p;

		int q = sql_edit->BraceMatch(sp);
		if (q == wxSTC_INVALID_POSITION)
			sql_edit->BraceBadLight(sp);
		else
			sql_edit->BraceHighlight(sp, q);
	}
	else
		sql_edit->BraceBadLight(wxSTC_INVALID_POSITION);	// remove light

	if (sql_edit->GetTextLength() > 5)
		SetTitle(sql_edit->GetLine(0).Trim());
}
//-----------------------------------------------------------------------------
//! returns true if there is a word in "wordlist" that starts with "word"
bool HasWord(wxString word, wxString &wordlist)
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

	int pos = sql_edit->GetCurrentPos();
	int start = sql_edit->WordStartPosition(pos, true);
	if (start != -1 && pos - start > 2 && !sql_edit->AutoCompActive())	// require 3 characters to show auto-complete
	{
		// GTK version crashes if nothing matches, so this check must be made for GTK
		// For MSW, it doesn't crash but it flashes on the screen (also not very nice)
		if (HasWord(sql_edit->GetTextRange(start, pos), keywordsM))
			sql_edit->AutoCompShow(pos-start, keywordsM);
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
		wxOPEN|wxCHANGE_DIR
	);

	if (wxID_CANCEL == fd.ShowModal())
		return;

	sql_edit->LoadFile(fd.GetPath());
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnButtonSaveClick(wxCommandEvent& WXUNUSED(event))
{
	wxFileDialog fd(this, _("Select file to save"), wxT(""), wxT(""),
		_("SQL Scripts (*.sql)|*.sql|All files (*.*)|*.*"),
		wxSAVE |wxCHANGE_DIR
	);

	if (wxID_CANCEL == fd.ShowModal())
		return;

	sql_edit->SaveFile(fd.GetPath());
	execute_sql_frame_statusbar->SetStatusText((_("File saved.")), 2);
}
//-----------------------------------------------------------------------------
//! enable/disable and show/hide controls depending of transaction status
void ExecuteSqlFrame::InTransaction(bool started)
{
	inTransactionM = started;

	if (!window_1->IsSplit())		// make sure window is split
	{								// needed in each case
		window_1_pane_1->Show();
		window_1_pane_2->Show();
		window_1->SplitHorizontally(window_1_pane_1, window_1_pane_2);
	}

	if (started)
		execute_sql_frame_statusbar->SetStatusText(_("Transaction started"), 3);
	else
	{
		window_1->Unsplit(window_1_pane_2);		// show sql entry window
		data_grid->ClearGrid();
		execute_sql_frame_statusbar->SetStatusText(wxEmptyString, 1);
	}

	button_commit->Enable(started);
	button_rollback->Enable(started);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::setSql(wxString sql)
{
	sql_edit->SetText(sql);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::OnButtonExecuteClick(wxCommandEvent& WXUNUSED(event))
{
	executeAllStatements();
}
//-----------------------------------------------------------------------------
//! Parses all sql statements in STC
//! when autoexecute is TRUE, program just waits user to click Commit/Rollback and closes window
void ExecuteSqlFrame::executeAllStatements(bool autoExecute)
{
	wxBusyCursor cr;
	stats_text_ctrl->Clear();

	using namespace std;
	string terminator = ";";
	string commands = wx2std(sql_edit->GetText());

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

		string sql = commands.substr(oldpos, pos-oldpos);
		sql.erase(sql.find_last_not_of("\n\r\t ")+1);	// right-trim the statement

		stringstream strstrm;			// Search and intercept
		string first, second, third;	// SET TERM and COMMIT statements
		strstrm << upcase(sql);
		strstrm >> first;
		strstrm >> second;
		strstrm >> third;

		if (first == "COMMIT")
			commitTransaction();
		else if (first == "SET" && second == "TERM")
		{
			searchpos = oldpos = pos + terminator.length();	// has to be here since terminator length can change
			terminator = third;
			continue;
		}
		else if (sql.length() && !execute(sql))
		{
			sql_edit->SetSelectionStart((int)oldpos);	// select the text in STC
			sql_edit->SetSelectionEnd((int)pos);		// that failed to execute
			return;
		}

		if (pos == string::npos)
			break;

		searchpos = oldpos = pos + terminator.length();
	}

	if (autoExecute)
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

		data_grid->ClearGrid(); // statement object will be invalidated, so clear the grid
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
			data_grid->fill();
			//data_grid->AdjustScrollbars();	// sometimes scrollbars get lost (this should be a solution)
												// if someone can make a reproducible test that always makes scrollbars
												// vanish, then we can test if this is the solution
			notebook_1->SetSelection(1);
			data_grid->SetFocus();
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
				execute_sql_frame_statusbar->SetStatusText(s, 1);
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
	if (!window_1->IsSplit())					// split screen if needed
	{
		window_1_pane_1->Show();
		window_1_pane_2->Show();
		window_1->SplitHorizontally(window_1_pane_1, window_1_pane_2);
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
		execute_sql_frame_statusbar->SetStatusText(_("Transaction commited"), 3);
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
		stats_text_ctrl->ClearAll();
		sql_edit->SetFocus();

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
		execute_sql_frame_statusbar->SetStatusText(_("Transaction rolled back."), 3);
		InTransaction(false);
		executedStatementsM.clear();
		sql_edit->SetFocus();

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
//! wraps/unwraps text in editor
void ExecuteSqlFrame::OnButtonWrapClick(wxCommandEvent& WXUNUSED(event))
{
	int mode = sql_edit->GetWrapMode();
	sql_edit->SetWrapMode(mode == wxSTC_WRAP_WORD ? wxSTC_WRAP_NONE : wxSTC_WRAP_WORD);
}
//-----------------------------------------------------------------------------
//! toggle the views in the following order:
//! ... -> SQL_entry_box -> Split View -> Stats&Data -> ...
void ExecuteSqlFrame::OnButtonToggleClick(wxCommandEvent& WXUNUSED(event))
{
	if (window_1->IsSplit())					// screen is split -> show second
		window_1->Unsplit(window_1_pane_1);
	else if (window_1_pane_1->IsShown())		// first is shown -> split again
		SplitScreen();
	else										// second is shown -> show first
	{
		window_1_pane_1->Show();
		window_1_pane_2->Show();
		window_1->SplitHorizontally(window_1_pane_1, window_1_pane_2);
		window_1->Unsplit(window_1_pane_2);
	}
}
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
	execute_sql_frame_statusbar->SetStatusText(std2wx(s), 0);

	transactionM = IBPP::TransactionFactory(databaseM->getDatabase());
	db->attach(this);	// observe database object

	executedStatementsM.clear();
	InTransaction(false);	// enable/disable controls
	setKeywords();			// set words for autocomplete feature
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
	int startpos = stats_text_ctrl->GetLength();
	stats_text_ctrl->AddText(s + wxT("\n"));
	int endpos = stats_text_ctrl->GetLength();

	int style = 0;
	if (type == ttError)
		style = 1;
	if (type == ttSql)
		style = 2;

	stats_text_ctrl->StartStyling(startpos, 255);
	stats_text_ctrl->SetStyling(endpos-startpos-1, style);
	stats_text_ctrl->GotoPos(endpos);
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
		sql_edit->SetZoom(zoom);
}
//-----------------------------------------------------------------------------
void ExecuteSqlFrame::doWriteConfigSettings(const std::string& prefix) const
{
	BaseFrame::doWriteConfigSettings(prefix);
	config().setValue(prefix + "::zoom", sql_edit->GetZoom());
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
