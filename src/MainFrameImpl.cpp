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

  Contributor(s): Nando Dessena, Michael Hieke
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

#include <wx/progdlg.h>
#include <wx/version.h>
#include "ibpp.h"

#include "MainFrame.h"
#include "gui/BackupFrame.h"
#include "gui/ExecuteSqlFrame.h"
#include "gui/DatabaseRegistrationDialog.h"
#include "gui/MetadataItemPropertiesFrame.h"
#include "gui/RestoreFrame.h"
#include "gui/ServerRegistrationDialog.h"
#include "gui/OptionsDialog.h"
#include "treeitem.h"
#include "ugly.h"
#include "dberror.h"
#include "urihandler.h"

//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(MainFrame, wxFrame)
	EVT_MENU(myTreeCtrl::Menu_RegisterServer, MainFrame::OnMenuRegisterServer)
	EVT_MENU(myTreeCtrl::Menu_Quit, MainFrame::OnMenuQuit)
	EVT_MENU(myTreeCtrl::Menu_About, MainFrame::OnMenuAbout)
	EVT_MENU(myTreeCtrl::Menu_Configure, MainFrame::OnMenuConfigure)
	EVT_MENU(myTreeCtrl::Menu_RegisterDatabase, MainFrame::OnMenuRegisterDatabase)
	EVT_MENU(myTreeCtrl::Menu_DatabaseRegistrationInfo, MainFrame::OnMenuDatabaseRegistrationInfo)
	EVT_MENU(myTreeCtrl::Menu_CreateDatabase, MainFrame::OnMenuCreateDatabase)
	EVT_MENU(myTreeCtrl::Menu_ManageUsers, MainFrame::OnMenuManageUsers)
	EVT_MENU(myTreeCtrl::Menu_RestartServer, MainFrame::OnMenuRestartServer)
	EVT_MENU(myTreeCtrl::Menu_StopServer, MainFrame::OnMenuStopServer)
	EVT_MENU(myTreeCtrl::Menu_UnRegisterServer, MainFrame::OnMenuUnRegisterServer)
	EVT_UPDATE_UI(myTreeCtrl::Menu_UnRegisterServer, MainFrame::OnMenuUpdateUnRegisterServer)
	EVT_MENU(myTreeCtrl::Menu_ServerProperties, MainFrame::OnMenuServerProperties)
	EVT_MENU(myTreeCtrl::Menu_UnRegisterDatabase, MainFrame::OnMenuUnRegisterDatabase)
	EVT_UPDATE_UI(myTreeCtrl::Menu_UnRegisterDatabase, MainFrame::OnMenuUpdateIfDatabaseNotConnected)
	EVT_MENU(myTreeCtrl::Menu_ShowConnectedUsers, MainFrame::OnMenuShowConnectedUsers)
	EVT_UPDATE_UI(myTreeCtrl::Menu_ShowConnectedUsers, MainFrame::OnMenuUpdateIfDatabaseConnected)
	EVT_MENU(myTreeCtrl::Menu_Backup, MainFrame::OnMenuBackup)
	EVT_MENU(myTreeCtrl::Menu_Restore, MainFrame::OnMenuRestore)
	EVT_UPDATE_UI(myTreeCtrl::Menu_Restore, MainFrame::OnMenuUpdateIfDatabaseNotConnected)
	EVT_MENU(myTreeCtrl::Menu_Connect, MainFrame::OnMenuConnect)
	EVT_UPDATE_UI(myTreeCtrl::Menu_Connect, MainFrame::OnMenuUpdateIfDatabaseNotConnected)
	EVT_MENU(myTreeCtrl::Menu_Disconnect, MainFrame::OnMenuDisconnect)
	EVT_UPDATE_UI(myTreeCtrl::Menu_Disconnect, MainFrame::OnMenuUpdateIfDatabaseConnected)
	EVT_MENU(myTreeCtrl::Menu_Reconnect, MainFrame::OnMenuReconnect)
	EVT_UPDATE_UI(myTreeCtrl::Menu_Reconnect, MainFrame::OnMenuUpdateIfDatabaseConnected)
	EVT_MENU(myTreeCtrl::Menu_Query, MainFrame::OnMenuQuery)
	EVT_MENU(myTreeCtrl::Menu_Insert, MainFrame::OnMenuInsert)
	EVT_MENU(myTreeCtrl::Menu_Browse, MainFrame::OnMenuBrowse)
	EVT_MENU(myTreeCtrl::Menu_BrowseColumns, MainFrame::OnMenuBrowseColumns)
	EVT_MENU(myTreeCtrl::Menu_ShowAllGeneratorValues, MainFrame::OnMenuShowAllGeneratorValues)
	EVT_MENU(myTreeCtrl::Menu_ShowGeneratorValue, MainFrame::OnMenuShowGeneratorValue)
	EVT_MENU(myTreeCtrl::Menu_SetGeneratorValue, MainFrame::OnMenuSetGeneratorValue)
	EVT_MENU(myTreeCtrl::Menu_CreateObject, MainFrame::OnMenuCreateObject)
	EVT_MENU(myTreeCtrl::Menu_LoadColumnsInfo, MainFrame::OnMenuLoadColumnsInfo)
	EVT_MENU(myTreeCtrl::Menu_ObjectProperties, MainFrame::OnMenuObjectProperties)
	EVT_MENU(myTreeCtrl::Menu_DropObject, MainFrame::OnMenuDropObject)

	EVT_TREE_ITEM_ACTIVATED(myTreeCtrl::ID_tree_ctrl, MainFrame::OnTreeItemActivate)
	EVT_CLOSE(MainFrame::OnClose)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
//! handle double-click on item (or press Enter)
void MainFrame::OnTreeItemActivate(wxTreeEvent& WXUNUSED(event))
{
	wxTreeItemId item = tree_ctrl_1->GetSelection();
	if (!item.IsOk())
		return;

	YxMetadataItem *m = tree_ctrl_1->getSelectedMetadataItem();
	if (!m)
		return;

	bool expanded = tree_ctrl_1->IsExpanded(item);

	// has to be declared outside, since local cannot be initialized inside switch...case
	MetadataItemPropertiesFrame *mipf;

	// do default action for the node, depending on the type
	NodeType nt = m->getType();
	switch (nt)
	{
		//case ntServer:	// pehaps it is better to leave default double-click action: "collapse the tree branch"
		//	ServerProperties(dynamic_cast<YServer *>(m));
		//	break;
		case ntDatabase:
			connect(false);						// false = don't warn if already connected
			break;
		case ntTable:
		case ntView:
			if (!((YxMetadataItemWithColumns *)m)->checkAndLoadColumns())
				::wxMessageBox(std2wx(lastError().getMessage()), _("An error occurred"), wxICON_ERROR);
			break;
		case ntProcedure:
			if (!((YProcedure *)m)->checkAndLoadParameters())
				::wxMessageBox(std2wx(lastError().getMessage()), _("An error occurred"), wxICON_ERROR);
			break;
		case ntGenerator:
      		showGeneratorValue(dynamic_cast<YGenerator*>(m));
			break;
		case ntDomain:
		case ntFunction:
		case ntTrigger:
			mipf = new MetadataItemPropertiesFrame(this, m);
			mipf->Show();
			break;
		default:
			return;
	};

	if (!expanded)						// tree's event will happen later and contract it
		tree_ctrl_1->Collapse(item);	// so an ugly hack to fix it.
}
//-----------------------------------------------------------------------------
void MainFrame::OnClose(wxCloseEvent& event)
{
	// OPTION: Are you sure you want to close FlameRobin?
	BaseFrame::OnClose(event);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuQuit(wxCommandEvent& WXUNUSED(event))
{
	Close();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuAbout(wxCommandEvent& WXUNUSED(event))
{
	wxString ib;
	ib.Printf(_("This tool uses IBPP library version %d.%d.%d.%d\nand wxWidgets library version %d.%d.%d"),
		(IBPP::Version & 0xFF000000) >> 24,
		(IBPP::Version & 0x00FF0000) >> 16,
		(IBPP::Version & 0x0000FF00) >> 8,
		(IBPP::Version & 0x000000FF),
		wxMAJOR_VERSION,
		wxMINOR_VERSION,
		wxRELEASE_NUMBER
	);

	wxString msg(_("FlameRobin v0.2.3"));
	msg += wxT("\n");
 	msg += _("Database administration tool for Firebird RDBMS");
	msg += wxT("\n\n");
	msg += ib;
	msg += wxT("\n\n");
 	msg += _("Copyright © 2004,2005  FlameRobin development team");
	msg += wxT("\n");
	msg += _("http://www.flamerobin.org");

	::wxMessageBox(msg, _("About FlameRobin"), wxOK | wxICON_INFORMATION);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuConfigure(wxCommandEvent& WXUNUSED(event))
{
    OptionsDialog *d = new OptionsDialog(this);
	d->ShowModal();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuInsert(wxCommandEvent& WXUNUSED(event))
{
	YTable *t = dynamic_cast<YTable *>(tree_ctrl_1->getSelectedMetadataItem());
	if (!t)
		return;

	YDatabase *d = dynamic_cast<YDatabase *>(t->getParent());
	if (!d)
		return;

	wxString sql = std2wx(t->getInsertStatement());

    ExecuteSqlFrame *eff = new ExecuteSqlFrame(this, -1, wxString(_("Execute SQL statements")));
	eff->setDatabase(d);
	eff->setSql(sql);
	eff->Show();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuBrowse(wxCommandEvent& WXUNUSED(event))
{
	YxMetadataItem *i = tree_ctrl_1->getSelectedMetadataItem();
	if (!i)
		return;

	NodeType t = i->getType();
	if (t != ntTable && t != ntView && t != ntSysTable && t != ntProcedure)
		return;

	YDatabase *d = dynamic_cast<YDatabase *>(i->getParent());
	if (!d)
		return;

	wxString sql(wxT("select * from "));
	sql += std2wx(i->getName());

	if (t == ntProcedure)
	{
		YProcedure *p = dynamic_cast<YProcedure *>(i);
		if (!p)
			return;

		if (!p->isSelectable())
		{
			::wxMessageBox(_("This procedure is not selectable"), _("Cannot create statement"), wxICON_INFORMATION);
			return;
		}
		sql = std2wx(p->getSelectStatement(false));	// false = without columns info (just *)
	}

    ExecuteSqlFrame *eff = new ExecuteSqlFrame(this, -1, wxString(_("Execute SQL statements")));
	eff->setDatabase(d);
	eff->Show();
	eff->setSql(sql);
	if (t != ntProcedure)
		eff->executeAllStatements();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuBrowseColumns(wxCommandEvent& WXUNUSED(event))
{
	YxMetadataItem *i = tree_ctrl_1->getSelectedMetadataItem();
	if (!i)
		return;

	NodeType t = i->getType();
	if (t != ntTable && t != ntView && t != ntSysTable && t != ntProcedure)
		return;

	YDatabase *d = dynamic_cast<YDatabase *>(i->getParent());
	if (!d)
		return;

	std::string sql;
	if (t == ntProcedure)
	{
		YProcedure *p = dynamic_cast<YProcedure *>(i);
		if (!p)
			return;

		if (!p->isSelectable())
		{
			::wxMessageBox(_("This procedure is not selectable"), _("Cannot create statement"), wxICON_INFORMATION);
			return;
		}
		sql = p->getSelectStatement(true);	// true = with columns info
	}
	else
	{
		if (t == ntTable)
			((YTable *)i)->checkAndLoadColumns();
		else
			((YView *)i)->checkAndLoadColumns();
		sql = "SELECT ";
		std::vector<YxMetadataItem *> temp;
		i->getChildren(temp);
		bool first = true;
		for (std::vector<YxMetadataItem *>::const_iterator it = temp.begin(); it != temp.end(); ++it)
		{
			if (first)
				first = false;
			else
				sql += ", ";
			sql += (*it)->getName();
		}
		sql += "\nFROM " + i->getName();
	}

    ExecuteSqlFrame *eff = new ExecuteSqlFrame(this, -1, wxString(_("Execute SQL statements")));
	eff->setDatabase(d);
	eff->Show();
	eff->setSql(std2wx(sql));
	if (t != ntProcedure)
		eff->executeAllStatements();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuRegisterDatabase(wxCommandEvent& WXUNUSED(event))
{
	YServer *s = dynamic_cast<YServer *>(tree_ctrl_1->getSelectedMetadataItem());
	if (!s)
		return;

	YDatabase db;
    DatabaseRegistrationDialog drd(this, -1, _("Register Existing Database"));
	drd.setDatabase(&db);

    if (drd.ShowModal() == wxID_OK)
		tree_ctrl_1->selectMetadataItem(s->addDatabase(db));
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuDatabaseRegistrationInfo(wxCommandEvent& WXUNUSED(event))
{
	YDatabase *d = dynamic_cast<YDatabase *>(tree_ctrl_1->getSelectedMetadataItem());
	if (!d)
		return;

    DatabaseRegistrationDialog drd(this, -1, _("Database Registration Info"));
	drd.setDatabase(d);
	drd.ShowModal();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateDatabase(wxCommandEvent& WXUNUSED(event))
{
	YServer *s = dynamic_cast<YServer *>(tree_ctrl_1->getSelectedMetadataItem());
	if (!s)
		return;

	YDatabase db;
    DatabaseRegistrationDialog drd(this, -1, _("Create New Database"), true);
	drd.setDatabase(&db);
	drd.setServer(s);

    if (drd.ShowModal() == wxID_OK)
        tree_ctrl_1->selectMetadataItem(s->addDatabase(db));
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuManageUsers(wxCommandEvent& WXUNUSED(event))
{
	// TODO:
	::wxMessageBox(_("This feature is not yet implemented"), _("Not in this version"), wxICON_INFORMATION);
}
//-----------------------------------------------------------------------------
// FIXME: this doesn't work for server, but on database basis
void MainFrame::OnMenuRestartServer(wxCommandEvent& WXUNUSED(event))
{
	::wxMessageBox(_("This feature is not yet implemented"), _("Not in this version"), wxICON_INFORMATION);
}
//-----------------------------------------------------------------------------
// FIXME: this doesn't work for server, but on database basis
void MainFrame::OnMenuStopServer(wxCommandEvent& WXUNUSED(event))
{
	::wxMessageBox(_("This feature is not yet implemented"), _("Not in this version"), wxICON_INFORMATION);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUnRegisterServer(wxCommandEvent& WXUNUSED(event))
{
	wxTreeItemId item = tree_ctrl_1->GetSelection();
	if (!item.IsOk())
		return;

	YServer *s = dynamic_cast<YServer *>(tree_ctrl_1->getSelectedMetadataItem());
	if (!s)
		return;

	// FIXME: check if some of the databases is connected... and prevent the action

	if (wxCANCEL == wxMessageBox(_("Are you sure?"), _("Unregister server"), wxOK | wxCANCEL | wxICON_QUESTION))
		return;

	YRoot *r = dynamic_cast<YRoot *>(s->getParent());
	if (r)
		r->removeServer(s);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuServerProperties(wxCommandEvent& WXUNUSED(event))
{
	YServer *s = dynamic_cast<YServer *>(tree_ctrl_1->getSelectedMetadataItem());
	if (!s)
		return;
    ServerRegistrationDialog srd(this, -1, _("Server Registration Info"));
	srd.setServer(s);
	srd.ShowModal();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuRegisterServer(wxCommandEvent& WXUNUSED(event))
{
	YRoot *r = dynamic_cast<YRoot *>(tree_ctrl_1->getSelectedMetadataItem());
	if (!r)
		return;
	YServer s;
    ServerRegistrationDialog srd(this, -1, _("Register New Server"));
	srd.setServer(&s);
	if (wxID_OK == srd.ShowModal())
		tree_ctrl_1->selectMetadataItem(r->addServer(s));
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUnRegisterDatabase(wxCommandEvent& WXUNUSED(event))
{
	YDatabase *d = dynamic_cast<YDatabase *>(tree_ctrl_1->getSelectedMetadataItem());
	if (!d)
		return;

	if (d->isConnected())
	{
		::wxMessageBox(_("Cannot remove connected database.\nPlease disconnect first"), _("Database connected."), wxOK | wxICON_EXCLAMATION );
		return;
	}

	if (wxCANCEL == wxMessageBox(_("Are you sure?"), _("Unregister database"), wxOK | wxCANCEL | wxICON_QUESTION))
		return;

	YServer *s = dynamic_cast<YServer *>(d->getParent());
	if (s)
		s->removeDatabase(d);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuShowConnectedUsers(wxCommandEvent& WXUNUSED(event))
{
	YDatabase *d = dynamic_cast<YDatabase *>(tree_ctrl_1->getSelectedMetadataItem());
	if (!d)
		return;
/*
	if (!d->isConnected())
	{
		if (wxYES == ::wxMessageBox(_("Do you want to connect?"), _("Not connected."), wxYES_NO|wxICON_QUESTION ))
		{
			connect(false);
			if (!d->isConnected())
				return;
		}
		else
			return;
	}
*/
	wxArrayString as;
	std::vector<std::string> users;
	d->getDatabase()->Users(users);
	for (std::vector<std::string>::const_iterator i = users.begin(); i != users.end(); ++i)
		as.Add(std2wx(*i));

	::wxGetSingleChoice(_("Connected users"), std2wx(d->getPath()), as);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuBackup(wxCommandEvent& WXUNUSED(event))
{
	YDatabase *db = dynamic_cast<YDatabase *>(tree_ctrl_1->getSelectedMetadataItem());
	if (!db)
		return;

    BackupFrame* f = new BackupFrame(this, db);
    f->Show();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuRestore(wxCommandEvent& WXUNUSED(event))
{
	YDatabase *db = dynamic_cast<YDatabase *>(tree_ctrl_1->getSelectedMetadataItem());
	if (!db)
		return;

    RestoreFrame* f = new RestoreFrame(this, db);
    f->Show();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuReconnect(wxCommandEvent& WXUNUSED(event))
{
	YDatabase *db = dynamic_cast<YDatabase *>(tree_ctrl_1->getSelectedMetadataItem());
	if (!db)
		return;
/*
	if (!db->isConnected())
	{
		if (wxYES == ::wxMessageBox(_("Do you want to connect?"), _("Not connected."), wxYES_NO|wxICON_QUESTION ))
			connect(false);
		return;
	}
*/
	::wxBeginBusyCursor();
	bool ok = db->reconnect();
	::wxEndBusyCursor();
	if (!ok)
		wxMessageBox(std2wx(lastError().getMessage()), _("Error!"), wxOK | wxICON_ERROR);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuConnect(wxCommandEvent& WXUNUSED(event))
{
	connect(true);	// true = warn if already connected
}
//-----------------------------------------------------------------------------
void MainFrame::connect(bool warn)
{
	YDatabase *db = dynamic_cast<YDatabase *>(tree_ctrl_1->getSelectedMetadataItem());
	if (!db)
		return;

	if (db->isConnected())
	{
		if (warn)
			wxMessageBox(_("Already connected"), _("Warning"), wxOK | wxICON_WARNING);
		return;
	}

	wxString pass;
	if (db->getPassword().empty())
	{
		wxString message(_("Enter password for user: "));
		message += std2wx(db->getUsername());
		pass = ::wxGetPasswordFromUser(message, _("Connecting to database"));
		if (pass.IsEmpty())
			return;
	}
	else
		pass = std2wx(db->getPassword());

	wxProgressDialog progress_dialog(_("Connecting..."), std2wx(db->getPath()), 9, NULL, wxPD_AUTO_HIDE | wxPD_APP_MODAL );
	if (!db->connect(wx2std(pass)))
	{
		wxMessageBox(std2wx(lastError().getMessage()), _("Error!"), wxOK | wxICON_ERROR);
		return;
	}

	NodeType types[9] = { ntTable, ntView, ntProcedure, ntTrigger, ntRole, ntDomain,
        ntFunction, ntGenerator, ntException };
	wxString names[9] = { _("tables"), _("views"), _("procedures"), _("triggers"), _("roles"), _("domains"),
		_("functions"), _("generators"), _("exceptions")
	};
	for (int i = 0; i < 9; i++)
	{
		progress_dialog.Update(i + 1, wxString::Format(_("Loading %s."), names[i].c_str()));
		if (!db->loadObjects(types[i]))
		{
			wxMessageBox(std2wx(lastError().getMessage()),
   				wxString::Format(_("Error loading %s."), names[i].c_str()), wxOK | wxICON_ERROR);
			return;
		}
	}

	wxTreeItemId id = tree_ctrl_1->GetSelection();
	tree_ctrl_1->Expand(id);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuDisconnect(wxCommandEvent& WXUNUSED(event))
{
	YDatabase *db = dynamic_cast<YDatabase *>(tree_ctrl_1->getSelectedMetadataItem());
	if (!db)
		return;
/*
	if (!db->isConnected())
	{
		wxMessageBox(_("Not connected"), _("Warning"), wxOK | wxICON_WARNING);
		return;
	}
*/
	if (!db->disconnect())
		wxMessageBox(std2wx(lastError().getMessage()), _("Error!"), wxOK | wxICON_ERROR);
}
//-----------------------------------------------------------------------------
void MainFrame::showGeneratorValue(YGenerator* g)
{
	if (!g)
		return;
	if (!g->loadValue(true))
		wxMessageBox(std2wx(lastError().getMessage()), _("Error!"), wxOK | wxICON_ERROR);
	else
		g->notify();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuShowGeneratorValue(wxCommandEvent& WXUNUSED(event))
{
	showGeneratorValue(dynamic_cast<YGenerator *>(tree_ctrl_1->getSelectedMetadataItem()));
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuSetGeneratorValue(wxCommandEvent& WXUNUSED(event))
{
    YGenerator *g = dynamic_cast<YGenerator *>(tree_ctrl_1->getSelectedMetadataItem());
	if (!g)
		return;

    std::ostringstream uriStream;
    std::string uriStr = "fr://edit_generator_value?parent_window=" + wx2std(wxString::Format(wxT("%d"), (int)this))
        + "&object_address=" + wx2std(wxString::Format(wxT("%d"), (int)g));
    getURIProcessor().handleURI(uriStr);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuShowAllGeneratorValues(wxCommandEvent& WXUNUSED(event))
{
	wxTreeItemId item = tree_ctrl_1->GetItemParent(tree_ctrl_1->GetSelection());
	YDatabase *db = dynamic_cast<YDatabase *>(tree_ctrl_1->getMetadataItem(item));
	if (!db)
	{
		wxMessageBox(_("No database assigned"), _("Warning"), wxOK | wxICON_ERROR);
		return;
	}

	for (YMetadataCollection<YGenerator>::const_iterator it = db->generatorsBegin(); it != db->generatorsEnd(); ++it)
	{
		YGenerator *g = const_cast<YGenerator *>(&(*it));
		if (!g->loadValue())
			wxMessageBox(std2wx(lastError().getMessage()), _("Error!"), wxOK|wxICON_ERROR);
	}
	db->refreshByType(ntGenerator);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateObject(wxCommandEvent& WXUNUSED(event))
{
	YxMetadataItem *t = tree_ctrl_1->getSelectedMetadataItem();
	if (!t)
		return;

	wxTreeItemId item = tree_ctrl_1->GetItemParent(tree_ctrl_1->GetSelection());
	YDatabase *db = dynamic_cast<YDatabase *>(tree_ctrl_1->getMetadataItem(item));
	if (!db)
	{
		wxMessageBox(_("No database assigned"), _("Warning"), wxOK | wxICON_ERROR);
		return;
	}

	// TODO: add a call for wizards. For example, we can have NewTableWizard which is a frame with grid in which
	// user can enter column data for new table (name, datatype, null option, collation, default, etc.) and also
	// enter a name for new table, etc. Wizard should return a bunch of DDL statements as a string which would we
	// pass to ExecuteSqlFrame.

	std::string sql = t->getCreateSqlTemplate();
	if (sql == "")
	{
		wxMessageBox(_("The feature is not yet available for this type of database objects."), _("Not yet implemented"), wxOK | wxICON_INFORMATION);
		return;
	}

    ExecuteSqlFrame *eff = new ExecuteSqlFrame(this, -1, wxEmptyString);
	eff->setDatabase(db);
	eff->setSql(std2wx(sql));
	eff->Show();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuLoadColumnsInfo(wxCommandEvent& WXUNUSED(event))
{
	YxMetadataItem *t = tree_ctrl_1->getSelectedMetadataItem();
	if (!t)
		return;

	bool success = true;
	switch (t->getType())
	{
		case ntTable:
		case ntView:		success = ((YxMetadataItemWithColumns *)t)->checkAndLoadColumns();	break;
		case ntProcedure:	success = ((YProcedure *)t)->checkAndLoadParameters();				break;
		default:			break;
	};

	if (!success)
	{
		::wxMessageBox(std2wx(lastError().getMessage()), _("Error!"), wxOK | wxICON_ERROR);
		return;
	}

	if (t->getType() == ntProcedure)
	{
		std::vector<YxMetadataItem *> temp;
		((YProcedure *)t)->getChildren(temp);
		if (temp.empty())
			::wxMessageBox(_("This procedure doesn't have any input or output parameters."), _("No parameters."), wxOK | wxICON_INFORMATION);
	}
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuObjectProperties(wxCommandEvent& WXUNUSED(event))
{
	YxMetadataItem *m = tree_ctrl_1->getSelectedMetadataItem();
	if (!m)
		return;

    MetadataItemPropertiesFrame *mipf = new MetadataItemPropertiesFrame(this, m);
	mipf->Show();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuDropObject(wxCommandEvent& WXUNUSED(event))
{
	YxMetadataItem *m = tree_ctrl_1->getSelectedMetadataItem();
	if (!m)
		return;

	YDatabase *d = dynamic_cast<YDatabase *>(m->getParent());
	if (!d)
		return;

	// TODO: We could first check if there are some dependant objects, and offer the user to
	//       either drop dependencies, or drop those objects too. Then we should create a bunch of
	//       sql statements that do it.

    std::string sql = m->getDropSqlStatement();
    ExecuteSqlFrame *eff = new ExecuteSqlFrame(this, -1, wxString(std2wx(sql)));
	eff->setDatabase(d);
	eff->Show();
	eff->setSql(std2wx(sql));
	eff->executeAllStatements(true);		// true = user must commit/rollback + frame is closed at once
}
//-----------------------------------------------------------------------------
//! create new ExecuteSqlFrame and attach database object to it
void MainFrame::OnMenuQuery(wxCommandEvent& WXUNUSED(event))
{
	YDatabase *d = dynamic_cast<YDatabase *>(tree_ctrl_1->getSelectedMetadataItem());
	if (!d)
		return;

	if (!d->isConnected())
	{
		if (wxYES == ::wxMessageBox(_("Do you want to connect?"), _("Not connected."), wxYES_NO | wxICON_QUESTION ))
		{
			connect(false);
			if (!d->isConnected())
				return;
		}
		else
			return;
	}

    ExecuteSqlFrame *eff = new ExecuteSqlFrame(this, -1, wxString(_("Execute SQL statements")));
	eff->setDatabase(d);
	eff->Show();
}
//-----------------------------------------------------------------------------
const std::string MainFrame::getName() const
{
	return "MainFrame";
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUpdateUnRegisterServer(wxUpdateUIEvent& event)
{
	YServer *s = dynamic_cast<YServer *>(tree_ctrl_1->getSelectedMetadataItem());
	event.Enable(s != 0 && !s->hasConnectedDatabase());
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUpdateIfDatabaseConnected(wxUpdateUIEvent& event)
{
	YDatabase *db = dynamic_cast<YDatabase *>(tree_ctrl_1->getSelectedMetadataItem());
	event.Enable(db != 0 && db->isConnected());
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUpdateIfDatabaseNotConnected(wxUpdateUIEvent& event)
{
	YDatabase *db = dynamic_cast<YDatabase *>(tree_ctrl_1->getSelectedMetadataItem());
	event.Enable(db != 0 && !db->isConnected());
}
//-----------------------------------------------------------------------------
