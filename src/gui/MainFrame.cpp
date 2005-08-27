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

#include <wx/progdlg.h>

#include "BackupFrame.h"
#include "config/Config.h"
#include "contextmenuvisitor.h"
#include "DatabaseRegistrationDialog.h"
#include "dberror.h"
#include "ExecuteSqlFrame.h"
#include "FieldPropertiesFrame.h"
#include "framemanager.h"
#include "frversion.h"
#include "main.h"
#include "MainFrame.h"
#include "metadata/metadataitem.h"
#include "metadata/root.h"
#include "myTreeCtrl.h"
#include "PreferencesDialog.h"
#include "RestoreFrame.h"
#include "ServerRegistrationDialog.h"
#include "SimpleHtmlFrame.h"
#include "treeitem.h"
#include "TriggerWizardDialog.h"
#include "ugly.h"
#include "urihandler.h"
//-----------------------------------------------------------------------------
MainFrame::MainFrame(wxWindow* parent, int id, const wxString& title, const wxPoint& pos, const wxSize& size, long style):
    BaseFrame(parent, id, title, pos, size, style, wxT("FlameRobin_main"))
{
    tree_ctrl_1 = new myTreeCtrl(this, wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS|wxSUNKEN_BORDER);
	buildMainMenu();
	SetStatusBarPane(-1);	// disable automatic fill
    set_properties();
    do_layout();
}
//-----------------------------------------------------------------------------
void MainFrame::buildMainMenu()
{
	menuBarM = new wxMenuBar();

    wxMenu *databaseMenu = new wxMenu();					// dynamic menus, created at runtime
	databaseMenu->Append(myTreeCtrl::Menu_RegisterDatabase, _("R&egister existing database..."));
	databaseMenu->Append(myTreeCtrl::Menu_CreateDatabase, _("Create &new database..."));
    databaseMenu->AppendSeparator();
	ContextMenuVisitor cmvd(databaseMenu);
	Database dummy;
	dummy.accept(&cmvd);
    databaseMenu->AppendSeparator();
    databaseMenu->Append(wxID_EXIT, _("&Quit"));
    menuBarM->Append(databaseMenu, _("&Database"));

    #ifdef __WXMAC__
	wxMenu *editMenu = new wxMenu();
    editMenu->Append(myTreeCtrl::Menu_Cut, _("Cu&t"));
    editMenu->Append(myTreeCtrl::Menu_Copy, _("&Copy"));
    editMenu->Append(myTreeCtrl::Menu_Paste, _("&Paste"));
    editMenu->Append(myTreeCtrl::Menu_Delete, _("&Delete"));
	menuBarM->Append(editMenu, _("&Edit"));
    #endif

	wxMenu *viewMenu = new wxMenu();
    viewMenu->AppendCheckItem(myTreeCtrl::Menu_ToggleStatusBar, _("&Status bar"));
    viewMenu->AppendCheckItem(myTreeCtrl::Menu_ToggleDisconnected, _("&Disconnected databases"));
    viewMenu->AppendSeparator();
    viewMenu->Append(myTreeCtrl::Menu_Configure, _("P&references..."));
	menuBarM->Append(viewMenu, _("&View"));
	frameManager().setWindowMenu(viewMenu);

	wxMenu *serverMenu = new wxMenu();
	serverMenu->Append(myTreeCtrl::Menu_RegisterServer, _("&Register server..."));
	serverMenu->Append(myTreeCtrl::Menu_UnRegisterServer, _("&Unregister server"));
	serverMenu->Append(myTreeCtrl::Menu_ServerProperties, _("Server registration &info..."));
	serverMenu->AppendSeparator();
	serverMenu->Append(myTreeCtrl::Menu_ManageUsers, _("&Manage users..."));
	menuBarM->Append(serverMenu, _("&Server"));

	objectMenuM = new wxMenu();
	wxMenu *newMenu = new wxMenu();
	newMenu->Append(myTreeCtrl::Menu_CreateDomain, 		_("&Domain"));
	newMenu->Append(myTreeCtrl::Menu_CreateException, 	_("&Exception"));
	newMenu->Append(myTreeCtrl::Menu_CreateFunction, 	_("&Function"));
	newMenu->Append(myTreeCtrl::Menu_CreateGenerator, 	_("&Generator"));
	newMenu->Append(myTreeCtrl::Menu_CreateProcedure, 	_("&Procedure"));
	newMenu->Append(myTreeCtrl::Menu_CreateRole, 		_("&Role"));
	newMenu->Append(myTreeCtrl::Menu_CreateTable, 		_("&Table"));
	newMenu->Append(myTreeCtrl::Menu_CreateTrigger, 	_("Tr&igger"));
	newMenu->Append(myTreeCtrl::Menu_CreateView,		_("&View"));
	objectMenuM->Append(myTreeCtrl::Menu_NewObject, _("&New"), newMenu);
	menuBarM->Append(objectMenuM, _("&Object"));

    wxMenu* helpMenu = new wxMenu();
    helpMenu->Append(myTreeCtrl::Menu_Manual, _("&Manual"));
    helpMenu->Append(myTreeCtrl::Menu_RelNotes, _("&What's new"));
    helpMenu->Append(myTreeCtrl::Menu_License, _("&License"));
    helpMenu->AppendSeparator();
    helpMenu->Append(wxID_ABOUT, _("&About"));
    menuBarM->Append(helpMenu, _("&Help"));
	SetMenuBar(menuBarM);

	// update checkboxes
	config().setValue("HideDisconnectedDatabases", false);
	viewMenu->Check(myTreeCtrl::Menu_ToggleDisconnected, true);
    if (config().get("showStatusBar", true))
    {
        CreateStatusBar();
        viewMenu->Check(myTreeCtrl::Menu_ToggleStatusBar, true);
		GetStatusBar()->SetStatusText(_("[No database selected]"));
	}
}
//-----------------------------------------------------------------------------
void MainFrame::set_properties()
{
    SetTitle(_("FlameRobin Database Admin"));

	// Default tree looks pretty ugly on non-Windows platforms(GTK)
	#ifndef __WXMSW__
	tree_ctrl_1->SetIndent(10);
	#endif

	TreeItem *rootdata = new TreeItem(tree_ctrl_1);
	wxTreeItemId root = tree_ctrl_1->AddRoot(_("Firebird Servers"), tree_ctrl_1->getItemImage(ntRoot), -1, rootdata);
	getGlobalRoot().attach(rootdata);									// link wxTree root node with rootNodeM

	getGlobalRoot().load();
	if (tree_ctrl_1->GetCount() <= 1)
	{
        wxString confile = std2wx(config().getDBHFileName());
        if (confile.Length() > 20)
            confile = wxT("\n") + confile + wxT("\n");  // break into several lines if path is long
        else
            confile = wxT(" ") + confile + wxT(" ");
        wxString msg;
        msg.Printf(_("The configuration file:%sdoes not exist or cannot be opened.\n\nThis is normal for first time users.\n\nYou may now register new servers and databases."),
            confile.c_str());
        wxMessageBox(msg, _("Configuration file not found"), wxICON_INFORMATION);

		Server s;
		s.setName("Localhost");
		s.setHostname("localhost");
		getGlobalRoot().addServer(s);
	}
	tree_ctrl_1->Expand(root);

	// make the first server active
	wxTreeItemIdValue cookie;
	wxTreeItemId firstServer = tree_ctrl_1->GetFirstChild(root, cookie);
	if (firstServer.IsOk())
		tree_ctrl_1->SelectItem(firstServer);

	#include "fricon.xpm"
    wxBitmap bmp(fricon_xpm);
    wxIcon icon;
    icon.CopyFromBitmap(bmp);
    SetIcon(icon);
}
//-----------------------------------------------------------------------------
void MainFrame::do_layout()
{
    // begin wxGlade: MainFrame::do_layout
    wxBoxSizer* sizer_1 = new wxBoxSizer(wxVERTICAL);
    sizer_1->Add(tree_ctrl_1, 1, wxEXPAND, 0);
    SetAutoLayout(true);
    SetSizer(sizer_1);
    Layout();
    // end wxGlade
}
//-----------------------------------------------------------------------------
const wxRect MainFrame::getDefaultRect() const
{
	return wxRect(-1, -1, 257, 367);
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(MainFrame, wxFrame)
	EVT_MENU(myTreeCtrl::Menu_RegisterServer, MainFrame::OnMenuRegisterServer)
	EVT_MENU(wxID_EXIT, MainFrame::OnMenuQuit)
	EVT_MENU(wxID_ABOUT, MainFrame::OnMenuAbout)
	EVT_MENU(myTreeCtrl::Menu_Manual, MainFrame::OnMenuManual)
	EVT_MENU(myTreeCtrl::Menu_RelNotes, MainFrame::OnMenuRelNotes)
	EVT_MENU(myTreeCtrl::Menu_License, MainFrame::OnMenuLicense)
	EVT_MENU(myTreeCtrl::Menu_Configure, MainFrame::OnMenuConfigure)

	EVT_MENU(myTreeCtrl::Menu_RegisterDatabase, MainFrame::OnMenuRegisterDatabase)
	EVT_UPDATE_UI(myTreeCtrl::Menu_RegisterDatabase, MainFrame::OnMenuUpdateIfServerSelected)
	EVT_MENU(myTreeCtrl::Menu_CreateDatabase, MainFrame::OnMenuCreateDatabase)
	EVT_UPDATE_UI(myTreeCtrl::Menu_CreateDatabase, MainFrame::OnMenuUpdateIfServerSelected)
	EVT_MENU(myTreeCtrl::Menu_ManageUsers, MainFrame::OnMenuManageUsers)
	EVT_UPDATE_UI(myTreeCtrl::Menu_ManageUsers, MainFrame::OnMenuUpdateIfServerSelected)
	EVT_MENU(myTreeCtrl::Menu_UnRegisterServer, MainFrame::OnMenuUnRegisterServer)
	EVT_UPDATE_UI(myTreeCtrl::Menu_UnRegisterServer, MainFrame::OnMenuUpdateUnRegisterServer)
	EVT_MENU(myTreeCtrl::Menu_ServerProperties, MainFrame::OnMenuServerProperties)
	EVT_UPDATE_UI(myTreeCtrl::Menu_ServerProperties, MainFrame::OnMenuUpdateIfServerSelected)

	EVT_MENU(myTreeCtrl::Menu_UnRegisterDatabase, MainFrame::OnMenuUnRegisterDatabase)
	EVT_UPDATE_UI(myTreeCtrl::Menu_UnRegisterDatabase, MainFrame::OnMenuUpdateIfDatabaseNotConnected)
	EVT_MENU(myTreeCtrl::Menu_ShowConnectedUsers, MainFrame::OnMenuShowConnectedUsers)
	EVT_UPDATE_UI(myTreeCtrl::Menu_ShowConnectedUsers, MainFrame::OnMenuUpdateIfDatabaseConnected)
	EVT_MENU(myTreeCtrl::Menu_DatabaseRegistrationInfo, MainFrame::OnMenuDatabaseRegistrationInfo)
	EVT_UPDATE_UI(myTreeCtrl::Menu_DatabaseRegistrationInfo, MainFrame::OnMenuUpdateIfDatabaseSelected)
	EVT_MENU(myTreeCtrl::Menu_Backup, MainFrame::OnMenuBackup)
	EVT_UPDATE_UI(myTreeCtrl::Menu_Backup, MainFrame::OnMenuUpdateIfDatabaseSelected)
	EVT_MENU(myTreeCtrl::Menu_Restore, MainFrame::OnMenuRestore)
	EVT_UPDATE_UI(myTreeCtrl::Menu_Restore, MainFrame::OnMenuUpdateIfDatabaseNotConnected)
	EVT_MENU(myTreeCtrl::Menu_Connect, MainFrame::OnMenuConnect)
	EVT_UPDATE_UI(myTreeCtrl::Menu_Connect, MainFrame::OnMenuUpdateIfDatabaseNotConnected)
	EVT_MENU(myTreeCtrl::Menu_ConnectAs, MainFrame::OnMenuConnectAs)
	EVT_UPDATE_UI(myTreeCtrl::Menu_ConnectAs, MainFrame::OnMenuUpdateIfDatabaseNotConnected)
	EVT_MENU(myTreeCtrl::Menu_Disconnect, MainFrame::OnMenuDisconnect)
	EVT_UPDATE_UI(myTreeCtrl::Menu_Disconnect, MainFrame::OnMenuUpdateIfDatabaseConnected)
	EVT_MENU(myTreeCtrl::Menu_Reconnect, MainFrame::OnMenuReconnect)
	EVT_UPDATE_UI(myTreeCtrl::Menu_Reconnect, MainFrame::OnMenuUpdateIfDatabaseConnected)
	EVT_MENU(myTreeCtrl::Menu_Query, MainFrame::OnMenuQuery)
	EVT_UPDATE_UI(myTreeCtrl::Menu_Query, MainFrame::OnMenuUpdateIfDatabaseSelected)
	EVT_UPDATE_UI(myTreeCtrl::Menu_NewObject, MainFrame::OnMenuUpdateIfDatabaseConnected)

	EVT_MENU(myTreeCtrl::Menu_Insert, MainFrame::OnMenuInsert)
	EVT_MENU(myTreeCtrl::Menu_Browse, MainFrame::OnMenuBrowse)
	EVT_MENU(myTreeCtrl::Menu_BrowseColumns, MainFrame::OnMenuBrowseColumns)
	EVT_MENU(myTreeCtrl::Menu_LoadColumnsInfo, MainFrame::OnMenuLoadColumnsInfo)
	EVT_MENU(myTreeCtrl::Menu_AddColumn, MainFrame::OnMenuAddColumn)
	EVT_MENU(myTreeCtrl::Menu_CreateTriggerForTable, MainFrame::OnMenuCreateTriggerForTable)

	EVT_MENU(myTreeCtrl::Menu_ShowAllGeneratorValues, MainFrame::OnMenuShowAllGeneratorValues)
	EVT_MENU(myTreeCtrl::Menu_ShowGeneratorValue, MainFrame::OnMenuShowGeneratorValue)
	EVT_MENU(myTreeCtrl::Menu_SetGeneratorValue, MainFrame::OnMenuSetGeneratorValue)

	EVT_MENU(myTreeCtrl::Menu_CreateObject, MainFrame::OnMenuCreateObject)
	EVT_MENU(myTreeCtrl::Menu_ObjectProperties, MainFrame::OnMenuObjectProperties)
	EVT_MENU(myTreeCtrl::Menu_DropObject, MainFrame::OnMenuDropObject)

	EVT_MENU(myTreeCtrl::Menu_ToggleStatusBar, MainFrame::OnMenuToggleStatusBar)
	EVT_MENU(myTreeCtrl::Menu_ToggleDisconnected, MainFrame::OnMenuToggleDisconnected)

	EVT_MENU(myTreeCtrl::Menu_CreateDomain, 	MainFrame::OnMenuCreateDomain)
	EVT_MENU(myTreeCtrl::Menu_CreateException, 	MainFrame::OnMenuCreateException)
	EVT_MENU(myTreeCtrl::Menu_CreateFunction, 	MainFrame::OnMenuCreateFunction)
	EVT_MENU(myTreeCtrl::Menu_CreateGenerator, 	MainFrame::OnMenuCreateGenerator)
	EVT_MENU(myTreeCtrl::Menu_CreateProcedure, 	MainFrame::OnMenuCreateProcedure)
	EVT_MENU(myTreeCtrl::Menu_CreateRole, 		MainFrame::OnMenuCreateRole)
	EVT_MENU(myTreeCtrl::Menu_CreateTable, 		MainFrame::OnMenuCreateTable)
	EVT_MENU(myTreeCtrl::Menu_CreateTrigger, 	MainFrame::OnMenuCreateTrigger)
	EVT_MENU(myTreeCtrl::Menu_CreateView, 		MainFrame::OnMenuCreateView)

	EVT_MENU_OPEN(MainFrame::OnMainMenuOpen)
	EVT_MENU_RANGE(5000, 6000, MainFrame::OnWindowMenuItem)
	EVT_TREE_SEL_CHANGED(myTreeCtrl::ID_tree_ctrl, MainFrame::OnTreeSelectionChanged)
	EVT_TREE_ITEM_ACTIVATED(myTreeCtrl::ID_tree_ctrl, MainFrame::OnTreeItemActivate)
	EVT_CLOSE(MainFrame::OnClose)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void MainFrame::OnMainMenuOpen(wxMenuEvent& event)
{
	#ifndef __WXGTK__
	if (event.IsPopup())	// on gtk all menus are treated as popup apparently
	{
		event.Skip();
		return;
	}
	#endif

	MetadataItem *m = tree_ctrl_1->getSelectedMetadataItem();
	if (!m)
	{
		event.Skip();
		return;
	}

	static NodeType nt = ntUnknown;
	if (m->getType() == nt)
	{
		event.Skip();
		return;
	}
	nt = m->getType();

	// rebuild object menu
	while (objectMenuM->GetMenuItemCount() > 2)
		objectMenuM->Destroy(objectMenuM->FindItemByPosition(2));
	if (objectMenuM->GetMenuItemCount() == 1)
		objectMenuM->AppendSeparator();

	if (m->getDatabase() != 0 && dynamic_cast<Database *>(m) == 0)	// has to be subitem of database
	{
		ContextMenuVisitor cmv(objectMenuM);
		m->accept(&cmv);
	}
	if (objectMenuM->GetMenuItemCount() == 2)	// separator
		objectMenuM->Destroy(objectMenuM->FindItemByPosition(1));

	event.Skip();
}
//-----------------------------------------------------------------------------
void MainFrame::OnWindowMenuItem(wxCommandEvent& event)
{
	frameManager().bringOnTop(event.GetId());
}
//-----------------------------------------------------------------------------
void MainFrame::OnTreeSelectionChanged(wxTreeEvent& WXUNUSED(event))
{
	if (!GetStatusBar())
		return;

	static Database *lastDatabase = 0;		// remember the last database/node type, so menus don't
	Database *d = tree_ctrl_1->getSelectedDatabase();
	if (d != lastDatabase)
	{
		if (d)
		{
			std::string s = d->getUsername() + "@" + d->getConnectionString() + " (" + d->getCharset() + ")";
			GetStatusBar()->SetStatusText(std2wx(s));
		}
		else
			GetStatusBar()->SetStatusText(_("[No database selected]"));
		lastDatabase = d;
	}
}
//-----------------------------------------------------------------------------
//! handle double-click on item (or press Enter)
void MainFrame::OnTreeItemActivate(wxTreeEvent& event)
{
	wxTreeItemId item = tree_ctrl_1->GetSelection();
	if (!item.IsOk())
		return;

	MetadataItem *m = tree_ctrl_1->getSelectedMetadataItem();
	if (!m)
		return;

	bool expanded = tree_ctrl_1->IsExpanded(item);
	NodeType nt = m->getType();

	enum { showProperties = 0, showColumnInfo };
	int treeActivateAction = showProperties;
	config().getValue("OnTreeActivate", treeActivateAction);
	if (!config().get("ShowColumnsInTree", true))	// if no columns in tree, then only Properties can be shown
		treeActivateAction = showProperties;

	if (treeActivateAction == showColumnInfo && (nt == ntTable || nt == ntView || nt == ntProcedure))
	{
		if (nt == ntProcedure)
		{
			if (!((Procedure *)m)->checkAndLoadParameters())
				::wxMessageBox(std2wx(lastError().getMessage()), _("An error occurred"), wxICON_ERROR);
		}
		else
		{
			if (!((Relation *)m)->checkAndLoadColumns())
				::wxMessageBox(std2wx(lastError().getMessage()), _("An error occurred"), wxICON_ERROR);
		}
	}
	else
	{
		switch (nt)
		{
			case ntDatabase:
				connect(false);						// false = don't warn if already connected
				break;
			case ntGenerator:
				showGeneratorValue(dynamic_cast<Generator*>(m));
				break;
			case ntColumn:
				OnMenuObjectProperties(event);
				break;
			case ntTable:
			case ntView:
			case ntProcedure:
			case ntDomain:
			case ntFunction:
			case ntTrigger:
            case ntException:
            case ntRole:
                frameManager().showMetadataPropertyFrame(this, m, true);
				break;
			#ifdef __WXMSW__
			default:
				return;
			#endif
		};
	}

	#ifdef __WXGTK__
	if (expanded)						// on GTK the tree control does nothing by itself, so we have to tell it
		tree_ctrl_1->Collapse(item);
	else
		tree_ctrl_1->Expand(item);
	#endif

	#ifdef __WXMSW__
	if (!expanded)						// on MSW the tree control toggles the branch automatically
		tree_ctrl_1->Collapse(item);	// so an ugly hack to trick it.
	#endif
}
//-----------------------------------------------------------------------------
void MainFrame::OnClose(wxCloseEvent& event)
{
	bool confirm = false;
	config().getValue("ConfirmQuit", confirm);
	if (confirm && event.CanVeto() && wxNO ==
		wxMessageBox(_("Are you sure you wish to exit?"), wxT("FlameRobin"), wxYES_NO|wxICON_QUESTION))
	{
		event.Veto();
		return;
	}
	frameManager().setWindowMenu(0);	// tell it not to update menus anymore
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

	wxString msg;
    msg.Printf(_("FlameRobin %d.%d.%d"),
        FR_VERSION_MAJOR, FR_VERSION_MINOR, FR_VERSION_RELEASE);
#if wxUSE_UNICODE
	msg += wxT(" Unicode");
#endif
	msg += wxT("\n");
 	msg += _("Database administration tool for Firebird RDBMS");
	msg += wxT("\n\n");
	msg += ib;
	msg += wxT("\n\n");
 	msg += _("Copyright (c) 2004,2005  FlameRobin development team");
	msg += wxT("\n");
	msg += _("http://www.flamerobin.org");

	::wxMessageBox(msg, _("About FlameRobin"), wxOK | wxICON_INFORMATION);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuManual(wxCommandEvent& WXUNUSED(event))
{
    showDocsHtmlFile(this, wxT("fr_manual.html"));
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuRelNotes(wxCommandEvent& WXUNUSED(event))
{
    showDocsHtmlFile(this, wxT("fr_whatsnew.html"));
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuLicense(wxCommandEvent& WXUNUSED(event))
{
    showDocsHtmlFile(this, wxT("fr_license.html"));
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuConfigure(wxCommandEvent& WXUNUSED(event))
{
    PreferencesDialog pd(this, _("Preferences"), config(),
        wxT("fr_settings.confdef"));
    if (pd.isOk() && pd.loadFromConfig())
    {
        static int pdSelection = 0;
        pd.selectPage(pdSelection);
        pd.ShowModal();
        pdSelection = pd.getSelectedPage();
    }
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuInsert(wxCommandEvent& WXUNUSED(event))
{
	Table *t = dynamic_cast<Table *>(tree_ctrl_1->getSelectedMetadataItem());
	Database *d = tree_ctrl_1->getSelectedDatabase();
	if (!t || !d)
		return;

	wxString sql = std2wx(t->getInsertStatement());
    ExecuteSqlFrame *eff = new ExecuteSqlFrame(this, -1, wxString(_("Execute SQL statements")));
	eff->setDatabase(d);
	eff->setSql(sql);
	eff->Show();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateTriggerForTable(wxCommandEvent& WXUNUSED(event))
{
	MetadataItem *i = tree_ctrl_1->getSelectedMetadataItem();
	if (!i)
		return;
	TriggerWizardDialog *t = new TriggerWizardDialog(this, i);
	t->Show();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuBrowse(wxCommandEvent& WXUNUSED(event))
{
	MetadataItem *i = tree_ctrl_1->getSelectedMetadataItem();
	if (!i)
		return;

	NodeType t = i->getType();
	if (t != ntTable && t != ntView && t != ntSysTable && t != ntProcedure)
		return;

	Database *d = i->getDatabase();
	if (!d)
		return;

	wxString sql(wxT("select * from "));
	sql += std2wx(i->getName());

	if (t == ntProcedure)
	{
		Procedure *p = dynamic_cast<Procedure *>(i);
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
	MetadataItem *i = tree_ctrl_1->getSelectedMetadataItem();
	if (!i)
		return;

	NodeType t = i->getType();
	if (t != ntTable && t != ntView && t != ntSysTable && t != ntProcedure)
		return;

	Database *d = i->getDatabase();
	if (!d)
		return;

	std::string sql;
	if (t == ntProcedure)
	{
		Procedure *p = dynamic_cast<Procedure *>(i);
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
			((Table *)i)->checkAndLoadColumns();
		else
			((View *)i)->checkAndLoadColumns();
		sql = "SELECT ";
		std::vector<MetadataItem *> temp;
		i->getChildren(temp);
		bool first = true;
		for (std::vector<MetadataItem *>::const_iterator it = temp.begin(); it != temp.end(); ++it)
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
	Server *s = tree_ctrl_1->getSelectedServer();
	if (!s)
		return;

	Database db;
    DatabaseRegistrationDialog drd(this, -1, _("Register Existing Database"));
	drd.setDatabase(&db);

    if (drd.ShowModal() == wxID_OK)
		tree_ctrl_1->selectMetadataItem(s->addDatabase(db));
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuDatabaseRegistrationInfo(wxCommandEvent& WXUNUSED(event))
{
	Database *d = tree_ctrl_1->getSelectedDatabase();
	if (!d)
		return;

    DatabaseRegistrationDialog drd(this, -1, _("Database Registration Info"));
	drd.setDatabase(d);
	drd.ShowModal();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateDatabase(wxCommandEvent& WXUNUSED(event))
{
    Server *s = tree_ctrl_1->getSelectedServer();
	if (!s)
		return;

	Database db;
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
void MainFrame::OnMenuUnRegisterServer(wxCommandEvent& WXUNUSED(event))
{
	Server *s = tree_ctrl_1->getSelectedServer();
	if (!s)
		return;

	// FIXME: check if some of the databases is connected... and prevent the action

	if (wxCANCEL == wxMessageBox(_("Are you sure?"), _("Unregister server"), wxOK | wxCANCEL | wxICON_QUESTION))
		return;

	Root *r = dynamic_cast<Root *>(s->getParent());
	if (r)
		r->removeServer(s);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuServerProperties(wxCommandEvent& WXUNUSED(event))
{
	Server *s = tree_ctrl_1->getSelectedServer();
	if (!s)
		return;
    ServerRegistrationDialog srd(this, -1, _("Server Registration Info"));
	srd.setServer(s);
	srd.ShowModal();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuRegisterServer(wxCommandEvent& WXUNUSED(event))
{
	Root *r = dynamic_cast<Root *>(tree_ctrl_1->getMetadataItem(tree_ctrl_1->GetRootItem()));
	if (!r)
		return;
	Server s;
    ServerRegistrationDialog srd(this, -1, _("Register New Server"));
	srd.setServer(&s);
	if (wxID_OK == srd.ShowModal())
		tree_ctrl_1->selectMetadataItem(r->addServer(s));
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUnRegisterDatabase(wxCommandEvent& WXUNUSED(event))
{
	Database *d = tree_ctrl_1->getSelectedDatabase();
	if (!d)
		return;
	if (d->isConnected())
	{
		::wxMessageBox(_("Cannot remove connected database.\nPlease disconnect first"), _("Database connected."), wxOK | wxICON_EXCLAMATION );
		return;
	}

	if (wxCANCEL == wxMessageBox(_("Are you sure?"), _("Unregister database"), wxOK | wxCANCEL | wxICON_QUESTION))
		return;

	Server *s = d->getServer();
	if (s)
		s->removeDatabase(d);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuShowConnectedUsers(wxCommandEvent& WXUNUSED(event))
{
	Database *d = tree_ctrl_1->getSelectedDatabase();
	if (!d)
		return;
	wxArrayString as;
	std::vector<std::string> users;
	d->getIBPPDatabase()->Users(users);
	for (std::vector<std::string>::const_iterator i = users.begin(); i != users.end(); ++i)
		as.Add(std2wx(*i));

	::wxGetSingleChoice(_("Connected users"), std2wx(d->getPath()), as);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuBackup(wxCommandEvent& WXUNUSED(event))
{
	Database *d = tree_ctrl_1->getSelectedDatabase();
	if (!d)
		return;
    BackupFrame* f = new BackupFrame(this, d);
    f->Show();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuRestore(wxCommandEvent& WXUNUSED(event))
{
	Database *d = tree_ctrl_1->getSelectedDatabase();
	if (!d)
		return;
    RestoreFrame* f = new RestoreFrame(this, d);
    f->Show();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuReconnect(wxCommandEvent& WXUNUSED(event))
{
	Database *d = tree_ctrl_1->getSelectedDatabase();
	if (!d)
		return;
	::wxBeginBusyCursor();
	bool ok = d->reconnect();
	::wxEndBusyCursor();
	if (!ok)
		wxMessageBox(std2wx(lastError().getMessage()), _("Error!"), wxOK | wxICON_ERROR);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuConnectAs(wxCommandEvent& WXUNUSED(event))
{
	Database *d = tree_ctrl_1->getSelectedDatabase();
	if (!d)
		return;
	if (d->isConnected())
	{
		wxMessageBox(_("Please disconnect first."), _("Already connected"), wxOK | wxICON_WARNING);
		return;
	}

    DatabaseRegistrationDialog drd(this, -1, _("Connect as..."), false, true);
	d->prepareTemporaryCredentials();
	drd.setDatabase(d);
	if (wxID_OK != drd.ShowModal() || !connect(false))
		d->resetCredentials();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuConnect(wxCommandEvent& WXUNUSED(event))
{
	connect(true);	// true = warn if already connected
}
//-----------------------------------------------------------------------------
bool MainFrame::connect(bool warn)
{
	Database *db = tree_ctrl_1->getSelectedDatabase();
	if (!db)
		return false;
	if (db->isConnected())
	{
		if (warn)
			wxMessageBox(_("Already connected"), _("Warning"), wxOK | wxICON_WARNING);
		return false;
	}

	wxString pass;
	if (db->getPassword().empty())
	{
		wxString message(_("Enter password for user: "));
		message += std2wx(db->getUsername());
		pass = ::wxGetPasswordFromUser(message, _("Connecting to database"));
		if (pass.IsEmpty())
			return false;
	}
	else
		pass = std2wx(db->getPassword());

	wxProgressDialog progress_dialog(_("Connecting..."), std2wx(db->getPath()), 9, NULL, wxPD_AUTO_HIDE | wxPD_APP_MODAL );
	if (!db->connect(wx2std(pass)))
	{
		wxMessageBox(std2wx(lastError().getMessage()), _("Error!"), wxOK | wxICON_ERROR);
		return false;
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
			break;
		}
	}

	wxTreeItemId id = tree_ctrl_1->GetSelection();
	tree_ctrl_1->Expand(id);
	return true;
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuDisconnect(wxCommandEvent& WXUNUSED(event))
{
	Database *d = tree_ctrl_1->getSelectedDatabase();
	if (!d)
		return;
	if (!d->disconnect())
		wxMessageBox(std2wx(lastError().getMessage()), _("Error!"), wxOK | wxICON_ERROR);
}
//-----------------------------------------------------------------------------
void MainFrame::showGeneratorValue(Generator* g)
{
	if (!g)
		return;
	if (!g->loadValue(true))
		wxMessageBox(std2wx(lastError().getMessage()), _("Error!"), wxOK | wxICON_ERROR);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuShowGeneratorValue(wxCommandEvent& WXUNUSED(event))
{
	showGeneratorValue(dynamic_cast<Generator *>(tree_ctrl_1->getSelectedMetadataItem()));
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuSetGeneratorValue(wxCommandEvent& WXUNUSED(event))
{
    Generator *g = dynamic_cast<Generator *>(tree_ctrl_1->getSelectedMetadataItem());
	if (!g)
		return;

    URI uri = URI("fr://edit_generator_value?parent_window=" + wx2std(wxString::Format(wxT("%d"), (int)this))
        + "&object_address=" + wx2std(wxString::Format(wxT("%d"), (int)g)));
    getURIProcessor().handleURI(uri);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuShowAllGeneratorValues(wxCommandEvent& WXUNUSED(event))
{
	Database *db = tree_ctrl_1->getSelectedDatabase();
	if (!db)
	{
		wxMessageBox(_("No database assigned"), _("Warning"), wxOK | wxICON_ERROR);
		return;
	}

	for (MetadataCollection<Generator>::const_iterator it = db->generatorsBegin(); it != db->generatorsEnd(); ++it)
	{
		Generator *g = const_cast<Generator *>(&(*it));
		if (!g->loadValue())
			wxMessageBox(std2wx(lastError().getMessage()), _("Error!"), wxOK|wxICON_ERROR);
	}
	db->refreshByType(ntGenerator);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateDomain(wxCommandEvent& WXUNUSED(event))
{
	Domain d;
	showCreateTemplate(&d);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateException(wxCommandEvent& WXUNUSED(event))
{
	Exception e;
	showCreateTemplate(&e);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateFunction(wxCommandEvent& WXUNUSED(event))
{
	Function x;
	showCreateTemplate(&x);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateGenerator(wxCommandEvent& WXUNUSED(event))
{
	Generator x;
	showCreateTemplate(&x);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateProcedure(wxCommandEvent& WXUNUSED(event))
{
	Procedure x;
	showCreateTemplate(&x);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateRole(wxCommandEvent& WXUNUSED(event))
{
	Role x;
	showCreateTemplate(&x);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateTable(wxCommandEvent& WXUNUSED(event))
{
	Table x;
	showCreateTemplate(&x);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateTrigger(wxCommandEvent& WXUNUSED(event))
{
	Trigger x;
	showCreateTemplate(&x);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateView(wxCommandEvent& WXUNUSED(event))
{
	View x;
	showCreateTemplate(&x);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateObject(wxCommandEvent& WXUNUSED(event))
{
	MetadataItem *m = tree_ctrl_1->getSelectedMetadataItem();
	if (!m)
		return;
	showCreateTemplate(m);
}
//-----------------------------------------------------------------------------
void MainFrame::showCreateTemplate(const MetadataItem *m)
{
	// TODO: add a call for wizards. For example, we can have NewTableWizard which is a frame with grid in which
	// user can enter column data for new table (name, datatype, null option, collation, default, etc.) and also
	// enter a name for new table, etc. Wizard should return a bunch of DDL statements as a string which would we
	// pass to ExecuteSqlFrame.

	std::string sql = m->getCreateSqlTemplate();
	if (sql == "")
	{
		wxMessageBox(_("The feature is not yet available for this type of database objects."), _("Not yet implemented"), wxOK | wxICON_INFORMATION);
		return;
	}

	Database *db = tree_ctrl_1->getSelectedDatabase();
	if (!db)
	{
		wxMessageBox(_("No database assigned"), _("Warning"), wxOK | wxICON_ERROR);
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
	MetadataItem *t = tree_ctrl_1->getSelectedMetadataItem();
	if (!t)
		return;

	bool success = true;
	switch (t->getType())
	{
		case ntTable:
		case ntView:		success = ((Relation *)t)->checkAndLoadColumns();	break;
		case ntProcedure:	success = ((Procedure *)t)->checkAndLoadParameters();				break;
		default:			break;
	};

	if (!success)
	{
		::wxMessageBox(std2wx(lastError().getMessage()), _("Error!"), wxOK | wxICON_ERROR);
		return;
	}

	if (t->getType() == ntProcedure)
	{
		std::vector<MetadataItem *> temp;
		((Procedure *)t)->getChildren(temp);
		if (temp.empty())
			::wxMessageBox(_("This procedure doesn't have any input or output parameters."), _("No parameters."), wxOK | wxICON_INFORMATION);
	}
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuAddColumn(wxCommandEvent& WXUNUSED(event))
{
	MetadataItem *i = tree_ctrl_1->getSelectedMetadataItem();
	if (!i)
		return;
	Table *t = dynamic_cast<Table *>(i);
	if (!t)
		return;

	FieldPropertiesFrame *f = new FieldPropertiesFrame(this, -1,
		wxString::Format(_("TABLE: %s"), std2wx(t->getName()).c_str()),
		t);
	f->setProperties();
	f->Show();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuToggleDisconnected(wxCommandEvent& event)
{
	config().setValue("HideDisconnectedDatabases", !event.IsChecked());
	getGlobalRoot().notifyAllServers();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuToggleStatusBar(wxCommandEvent& event)
{
    wxStatusBar *s = GetStatusBar();
    if (!s)
        s = CreateStatusBar();

    bool show = event.IsChecked();
    config().setValue("showStatusBar", show);
    s->Show(show);
    SendSizeEvent();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuObjectProperties(wxCommandEvent& WXUNUSED(event))
{
	MetadataItem *m = tree_ctrl_1->getSelectedMetadataItem();
	if (!m)
		return;

	Column *c = dynamic_cast<Column *>(m);
	if (c)
	{
		Table *t = dynamic_cast<Table *>(c->getParent());
		if (!t)		// dummy check
			return;
		FieldPropertiesFrame *f = new FieldPropertiesFrame(this, -1,
			wxString::Format(_("TABLE: %s"), std2wx(t->getName()).c_str()),
			t);
		f->setField(c);
		f->Show();
	}
	else
        frameManager().showMetadataPropertyFrame(this, m);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuDropObject(wxCommandEvent& WXUNUSED(event))
{
	MetadataItem *m = tree_ctrl_1->getSelectedMetadataItem();
	Database *d = tree_ctrl_1->getSelectedDatabase();
	if (!m || !d)
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
	Database *d = tree_ctrl_1->getSelectedDatabase();
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
	Server *s = tree_ctrl_1->getSelectedServer();
	event.Enable(s != 0 && !s->hasConnectedDatabase());
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUpdateIfServerSelected(wxUpdateUIEvent& event)
{
	Server *s = tree_ctrl_1->getSelectedServer();
	event.Enable(s != 0);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUpdateIfDatabaseConnected(wxUpdateUIEvent& event)
{
	Database *db = tree_ctrl_1->getSelectedDatabase();
	event.Enable(db != 0 && db->isConnected());
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUpdateIfDatabaseNotConnected(wxUpdateUIEvent& event)
{
	Database *db = tree_ctrl_1->getSelectedDatabase();
	event.Enable(db != 0 && !db->isConnected());
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUpdateIfDatabaseSelected(wxUpdateUIEvent& event)
{
	Database *db = tree_ctrl_1->getSelectedDatabase();
	event.Enable(db != 0);
}
//-----------------------------------------------------------------------------
