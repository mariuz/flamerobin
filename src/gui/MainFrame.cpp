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
#include "config/DatabaseConfig.h"
#include "ContextMenuMetadataItemVisitor.h"
#include "core/FRError.h"
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
bool checkValidDatabase(Database* database)
{
    if (database)
        return true;
    wxMessageBox(_("Operation can not be performed - no database assigned"),
        _("Internal Error"), wxOK|wxICON_ERROR);
    return false;
}
//-----------------------------------------------------------------------------
bool checkValidServer(Server* server)
{
    if (server)
        return true;
    wxMessageBox(_("Operation can not be performed - no server assigned"),
        _("Internal Error"), wxOK|wxICON_ERROR);
    return false;
}
//-----------------------------------------------------------------------------
void reportLastError(const wxString& actionMsg)
{
    wxMessageBox(lastError().getMessage(), actionMsg, wxOK | wxICON_ERROR);
}
//-----------------------------------------------------------------------------
MainFrame::MainFrame(wxWindow* parent, int id, const wxString& title, const wxPoint& pos, const wxSize& size, long style):
    BaseFrame(parent, id, title, pos, size, style, wxT("FlameRobin_main"))
{
    tree_ctrl_1 = new myTreeCtrl(this, wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS|wxSUNKEN_BORDER);
    buildMainMenu();
    SetStatusBarPane(-1);   // disable automatic fill
    set_properties();
    do_layout();
}
//-----------------------------------------------------------------------------
void MainFrame::buildMainMenu()
{
    menuBarM = new wxMenuBar();

    wxMenu* databaseMenu = new wxMenu();                    // dynamic menus, created at runtime
    databaseMenu->Append(myTreeCtrl::Menu_RegisterDatabase, _("R&egister existing database..."));
    databaseMenu->Append(myTreeCtrl::Menu_CreateDatabase, _("Create &new database..."));
    databaseMenu->Append(myTreeCtrl::Menu_RestoreIntoNew, _("Restore bac&kup into new database..."));
    databaseMenu->AppendSeparator();
    ContextMenuMetadataItemVisitor cmvd(databaseMenu);
    Database dummy;
    dummy.acceptVisitor(&cmvd);
    databaseMenu->AppendSeparator();
    databaseMenu->Append(wxID_EXIT, _("&Quit"));
    menuBarM->Append(databaseMenu, _("&Database"));

#ifdef __WXMAC__
    wxMenu* editMenu = new wxMenu();
    editMenu->Append(wxID_CUT, _("Cu&t"));
    editMenu->Append(wxID_COPY, _("&Copy"));
    editMenu->Append(wxID_PASTE, _("&Paste"));
    editMenu->Append(wxID_CLEAR, _("&Delete"));
    menuBarM->Append(editMenu, _("&Edit"));
#endif

    wxMenu* viewMenu = new wxMenu();
    viewMenu->AppendCheckItem(myTreeCtrl::Menu_ToggleStatusBar, _("&Status bar"));
    viewMenu->AppendCheckItem(myTreeCtrl::Menu_ToggleDisconnected, _("&Disconnected databases"));
    viewMenu->AppendSeparator();
    viewMenu->Append(wxID_PREFERENCES, _("P&references..."));
    menuBarM->Append(viewMenu, _("&View"));
    frameManager().setWindowMenu(viewMenu);

    wxMenu* serverMenu = new wxMenu();
    serverMenu->Append(myTreeCtrl::Menu_RegisterServer, _("&Register server..."));
    serverMenu->Append(myTreeCtrl::Menu_UnRegisterServer, _("&Unregister server"));
    serverMenu->Append(myTreeCtrl::Menu_ServerProperties, _("Server registration &info..."));
    serverMenu->AppendSeparator();
    serverMenu->Append(myTreeCtrl::Menu_ManageUsers, _("&Manage users..."));
    menuBarM->Append(serverMenu, _("&Server"));

    objectMenuM = new wxMenu();
    wxMenu* newMenu = new wxMenu();
    newMenu->Append(myTreeCtrl::Menu_CreateDomain,      _("&Domain"));
    newMenu->Append(myTreeCtrl::Menu_CreateException,   _("&Exception"));
    newMenu->Append(myTreeCtrl::Menu_CreateFunction,    _("&Function"));
    newMenu->Append(myTreeCtrl::Menu_CreateGenerator,   _("&Generator"));
    newMenu->Append(myTreeCtrl::Menu_CreateProcedure,   _("&Procedure"));
    newMenu->Append(myTreeCtrl::Menu_CreateRole,        _("&Role"));
    newMenu->Append(myTreeCtrl::Menu_CreateTable,       _("&Table"));
    newMenu->Append(myTreeCtrl::Menu_CreateTrigger,     _("Tr&igger"));
    newMenu->Append(myTreeCtrl::Menu_CreateView,        _("&View"));
    objectMenuM->Append(myTreeCtrl::Menu_NewObject, _("&New"), newMenu);
    menuBarM->Append(objectMenuM, _("&Object"));

    wxMenu* helpMenu = new wxMenu();
    helpMenu->Append(myTreeCtrl::Menu_Manual, _("&Manual"));
    helpMenu->Append(myTreeCtrl::Menu_RelNotes, _("&What's new"));
    helpMenu->Append(myTreeCtrl::Menu_License, _("&License"));
#ifndef __WXMAC__
    helpMenu->AppendSeparator();
#endif
    helpMenu->Append(wxID_ABOUT, _("&About"));
    menuBarM->Append(helpMenu, _("&Help"));
    SetMenuBar(menuBarM);

    // update checkboxes
    config().setValue(wxT("HideDisconnectedDatabases"), false);
    viewMenu->Check(myTreeCtrl::Menu_ToggleDisconnected, true);
    if (config().get(wxT("showStatusBar"), true))
    {
        CreateStatusBar();
        viewMenu->Check(myTreeCtrl::Menu_ToggleStatusBar, true);
        GetStatusBar()->SetStatusText(_("[No database selected]"));
    }
}
//-----------------------------------------------------------------------------
void MainFrame::showDocsHtmlFile(const wxString& fileName)
{
    wxFileName fullFileName(config().getDocsPath(), fileName);
    showHtmlFile(this, fullFileName);
}
//-----------------------------------------------------------------------------
void MainFrame::set_properties()
{
    SetTitle(_("FlameRobin Database Admin"));

    // Default tree looks pretty ugly on non-Windows platforms(GTK)
    #ifndef __WXMSW__
    tree_ctrl_1->SetIndent(10);
    #endif

    TreeItem* rootdata = new TreeItem(tree_ctrl_1);
    wxTreeItemId root = tree_ctrl_1->AddRoot(_("Firebird Servers"), tree_ctrl_1->getItemImage(ntRoot), -1, rootdata);
    // link wxTree root node with rootNodeM
    getGlobalRoot().attachObserver(rootdata);

    getGlobalRoot().load();
    if (tree_ctrl_1->GetCount() <= 1)
    {
        wxString confile = config().getDBHFileName();
        if (confile.Length() > 20)
            confile = wxT("\n") + confile + wxT("\n");  // break into several lines if path is long
        else
            confile = wxT(" ") + confile + wxT(" ");
        wxString msg;
        msg.Printf(_("The configuration file:%sdoes not exist or can not be opened.\n\nThis is normal for first time users.\n\nYou may now register new servers and databases."),
            confile.c_str());
        wxMessageBox(msg, _("Configuration file not found"), wxICON_INFORMATION);

        Server s;
        s.setName(wxT("Localhost"));
        s.setHostname(wxT("localhost"));
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
    EVT_MENU(wxID_PREFERENCES, MainFrame::OnMenuConfigure)

    EVT_MENU(myTreeCtrl::Menu_RegisterDatabase, MainFrame::OnMenuRegisterDatabase)
    EVT_UPDATE_UI(myTreeCtrl::Menu_RegisterDatabase, MainFrame::OnMenuUpdateIfServerSelected)
    EVT_MENU(myTreeCtrl::Menu_CreateDatabase, MainFrame::OnMenuCreateDatabase)
    EVT_UPDATE_UI(myTreeCtrl::Menu_CreateDatabase, MainFrame::OnMenuUpdateIfServerSelected)
    EVT_MENU(myTreeCtrl::Menu_RestoreIntoNew, MainFrame::OnMenuRestoreIntoNewDatabase)
    EVT_UPDATE_UI(myTreeCtrl::Menu_RestoreIntoNew, MainFrame::OnMenuUpdateIfServerSelected)
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
    EVT_MENU(myTreeCtrl::Menu_DatabasePreferences, MainFrame::OnMenuDatabasePreferences)
    EVT_UPDATE_UI(myTreeCtrl::Menu_DatabasePreferences, MainFrame::OnMenuUpdateIfDatabaseSelected)

    EVT_MENU(myTreeCtrl::Menu_Insert, MainFrame::OnMenuInsert)
    EVT_MENU(myTreeCtrl::Menu_Browse, MainFrame::OnMenuBrowse)
    EVT_MENU(myTreeCtrl::Menu_BrowseColumns, MainFrame::OnMenuBrowseColumns)
    EVT_MENU(myTreeCtrl::Menu_LoadColumnsInfo, MainFrame::OnMenuLoadColumnsInfo)
    EVT_MENU(myTreeCtrl::Menu_AddColumn, MainFrame::OnMenuAddColumn)
    EVT_MENU(myTreeCtrl::Menu_CreateTriggerForTable, MainFrame::OnMenuCreateTriggerForTable)
    EVT_MENU(myTreeCtrl::Menu_ExecuteProcedure, MainFrame::OnMenuExecuteProcedure)

    EVT_MENU(myTreeCtrl::Menu_ShowAllGeneratorValues, MainFrame::OnMenuShowAllGeneratorValues)
    EVT_MENU(myTreeCtrl::Menu_ShowGeneratorValue, MainFrame::OnMenuShowGeneratorValue)
    EVT_MENU(myTreeCtrl::Menu_SetGeneratorValue, MainFrame::OnMenuSetGeneratorValue)

    EVT_MENU(myTreeCtrl::Menu_CreateObject, MainFrame::OnMenuCreateObject)
    EVT_MENU(myTreeCtrl::Menu_ObjectProperties, MainFrame::OnMenuObjectProperties)
    EVT_MENU(myTreeCtrl::Menu_DropObject, MainFrame::OnMenuDropObject)

    EVT_MENU(myTreeCtrl::Menu_ToggleStatusBar, MainFrame::OnMenuToggleStatusBar)
    EVT_MENU(myTreeCtrl::Menu_ToggleDisconnected, MainFrame::OnMenuToggleDisconnected)

    EVT_MENU(myTreeCtrl::Menu_CreateDomain,     MainFrame::OnMenuCreateDomain)
    EVT_MENU(myTreeCtrl::Menu_CreateException,  MainFrame::OnMenuCreateException)
    EVT_MENU(myTreeCtrl::Menu_CreateFunction,   MainFrame::OnMenuCreateFunction)
    EVT_MENU(myTreeCtrl::Menu_CreateGenerator,  MainFrame::OnMenuCreateGenerator)
    EVT_MENU(myTreeCtrl::Menu_CreateProcedure,  MainFrame::OnMenuCreateProcedure)
    EVT_MENU(myTreeCtrl::Menu_CreateRole,       MainFrame::OnMenuCreateRole)
    EVT_MENU(myTreeCtrl::Menu_CreateTable,      MainFrame::OnMenuCreateTable)
    EVT_MENU(myTreeCtrl::Menu_CreateTrigger,    MainFrame::OnMenuCreateTrigger)
    EVT_MENU(myTreeCtrl::Menu_CreateView,       MainFrame::OnMenuCreateView)

    EVT_MENU_OPEN(MainFrame::OnMainMenuOpen)
    EVT_MENU_RANGE(5000, 6000, MainFrame::OnWindowMenuItem)
    EVT_TREE_SEL_CHANGED(myTreeCtrl::ID_tree_ctrl, MainFrame::OnTreeSelectionChanged)
    EVT_TREE_ITEM_ACTIVATED(myTreeCtrl::ID_tree_ctrl, MainFrame::OnTreeItemActivate)
    EVT_CLOSE(MainFrame::OnClose)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void MainFrame::OnMainMenuOpen(wxMenuEvent& event)
{
    FR_TRY

    #ifndef __WXGTK__
    if (event.IsPopup())    // on gtk all menus are treated as popup apparently
    {
        event.Skip();
        return;
    }
    #endif

    MetadataItem* m = tree_ctrl_1->getSelectedMetadataItem();
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

    if (m->getDatabase() != 0 && dynamic_cast<Database*>(m) == 0)  // has to be subitem of database
    {
        ContextMenuMetadataItemVisitor cmv(objectMenuM);
        m->acceptVisitor(&cmv);
    }
    if (objectMenuM->GetMenuItemCount() == 2)   // separator
        objectMenuM->Destroy(objectMenuM->FindItemByPosition(1));

    event.Skip();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnWindowMenuItem(wxCommandEvent& event)
{
    FR_TRY

    frameManager().bringOnTop(event.GetId());

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnTreeSelectionChanged(wxTreeEvent& WXUNUSED(event))
{
    FR_TRY

    if (!GetStatusBar())
        return;

    static Database* lastDatabase = 0;      // remember the last database/node type, so menus don't
    Database* d = tree_ctrl_1->getSelectedDatabase();
    if (d != lastDatabase)
    {
        if (d)
        {
            wxString s = d->getUsername() + wxT("@") + d->getConnectionString()
                + wxT(" (") + d->getConnectionCharset() + wxT(")");
            GetStatusBar()->SetStatusText(s);
        }
        else
            GetStatusBar()->SetStatusText(_("[No database selected]"));
        lastDatabase = d;
    }

    FR_CATCH
}
//-----------------------------------------------------------------------------
//! handle double-click on item (or press Enter)
void MainFrame::OnTreeItemActivate(wxTreeEvent& event)
{
    FR_TRY

    wxTreeItemId item = tree_ctrl_1->GetSelection();
    if (!item.IsOk())
        return;

    MetadataItem* m = tree_ctrl_1->getSelectedMetadataItem();
    if (!m)
        return;

    bool expanded = tree_ctrl_1->IsExpanded(item);
    NodeType nt = m->getType();

    enum { showProperties = 0, showColumnInfo };
    int treeActivateAction = showProperties;
    config().getValue(wxT("OnTreeActivate"), treeActivateAction);
    if (!config().get(wxT("ShowColumnsInTree"), true))   // if no columns in tree, then only Properties can be shown
        treeActivateAction = showProperties;

    if (treeActivateAction == showColumnInfo && (nt == ntTable || nt == ntView || nt == ntProcedure))
    {
        bool success;
        if (nt == ntProcedure)
            success = (static_cast<Procedure*>(m))->checkAndLoadParameters();
        else
            success = (static_cast<Relation*>(m))->checkAndLoadColumns();
        if (!success)
            reportLastError(_("Error Loading Information"));
    }
    else
    {
        switch (nt)
        {
            case ntDatabase:
                connect(false);                     // false = don't warn if already connected
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
    if (expanded)                       // on GTK the tree control does nothing by itself, so we have to tell it
        tree_ctrl_1->Collapse(item);
    else
        tree_ctrl_1->Expand(item);
    #endif

    #ifdef __WXMSW__
    if (!expanded)                      // on MSW the tree control toggles the branch automatically
        tree_ctrl_1->Collapse(item);    // so an ugly hack to trick it.
    #endif

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnClose(wxCloseEvent& event)
{
    FR_TRY

    Raise();
    bool confirm = false;
    config().getValue(wxT("ConfirmQuit"), confirm);
    if (confirm && event.CanVeto() && wxNO ==
        wxMessageBox(_("Are you sure you wish to exit?"), wxT("FlameRobin"), wxYES_NO | wxICON_QUESTION))
    {
        event.Veto();
        return;
    }
    frameManager().setWindowMenu(0);    // tell it not to update menus anymore
    BaseFrame::OnClose(event);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuQuit(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Close();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuAbout(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

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

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuManual(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    showDocsHtmlFile(wxT("fr_manual.html"));

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuRelNotes(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    showDocsHtmlFile(wxT("fr_whatsnew.html"));

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuLicense(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    showDocsHtmlFile(wxT("fr_license.html"));

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuConfigure(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    PreferencesDialog pd(this, _("Preferences"), config(),
        wxT("fr_settings.confdef"));
    if (pd.isOk() && pd.loadFromConfig())
    {
        static int pdSelection = 0;
        pd.selectPage(pdSelection);
        pd.ShowModal();
        pdSelection = pd.getSelectedPage();
    }

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuDatabasePreferences(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Database* d = tree_ctrl_1->getSelectedDatabase();
    if (!checkValidDatabase(d))
        return;
    DatabaseConfig dc(d);
    PreferencesDialog pd(this,
        wxString::Format(_("%s preferences"), d->getName().c_str()),
        dc, wxT("db_settings.confdef"));
    if (pd.isOk() && pd.loadFromConfig())
        pd.ShowModal();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuInsert(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Database* d = tree_ctrl_1->getSelectedDatabase();
    if (!checkValidDatabase(d))
        return;
    Table* t = dynamic_cast<Table*>(tree_ctrl_1->getSelectedMetadataItem());
    if (!t)
        return;

    wxString sql = t->getInsertStatement();
    ExecuteSqlFrame* eff = new ExecuteSqlFrame(this, -1, wxString(_("Execute SQL statements")));
    eff->setDatabase(d);
    eff->setSql(sql);
    eff->Show();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateTriggerForTable(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    MetadataItem* i = tree_ctrl_1->getSelectedMetadataItem();
    if (!i)
        return;
    TriggerWizardDialog* t = new TriggerWizardDialog(this, i);
    t->Show();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuExecuteProcedure(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Procedure* p = dynamic_cast<Procedure*>(tree_ctrl_1->getSelectedMetadataItem());
    if (!p)
        return;

    ExecuteSqlFrame* eff = new ExecuteSqlFrame(this, -1, wxString(_("Executing procedure")));
    eff->setDatabase(p->getDatabase());
    eff->setSql(p->getExecuteStatement());
    eff->Show();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuBrowse(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    MetadataItem* i = tree_ctrl_1->getSelectedMetadataItem();
    if (!i)
        return;

    NodeType t = i->getType();
    if (t != ntTable && t != ntView && t != ntSysTable && t != ntProcedure)
        return;

    Database* d = i->getDatabase();
    if (!d)
        return;

    wxString sql(wxT("select * from "));
    sql += i->getName();

    if (t == ntProcedure)
    {
        Procedure* p = dynamic_cast<Procedure*>(i);
        if (!p)
            return;

        if (!p->isSelectable())
        {
            ::wxMessageBox(_("This procedure is not selectable"), _("Cannot create statement"), wxICON_INFORMATION);
            return;
        }
        sql = p->getSelectStatement(false); // false = without columns info (just *)
    }

    ExecuteSqlFrame* eff = new ExecuteSqlFrame(this, -1, wxString(_("Execute SQL statements")));
    eff->setDatabase(d);
    eff->Show();
    eff->setSql(sql);
    if (t != ntProcedure)
        eff->executeAllStatements();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuBrowseColumns(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    MetadataItem* i = tree_ctrl_1->getSelectedMetadataItem();
    if (!i)
        return;

    NodeType t = i->getType();
    if (t != ntTable && t != ntView && t != ntSysTable && t != ntProcedure)
        return;

    Database* d = i->getDatabase();
    if (!d)
        return;

    wxString sql;
    if (t == ntProcedure)
    {
        Procedure* p = dynamic_cast<Procedure*>(i);
        if (!p)
            return;

        if (!p->isSelectable())
        {
            ::wxMessageBox(_("This procedure is not selectable"), _("Cannot create statement"), wxICON_INFORMATION);
            return;
        }
        sql = p->getSelectStatement(true);  // true = with columns info
    }
    else
    {
        if (t == ntTable)
            ((Table*)i)->checkAndLoadColumns();
        else
            ((View*)i)->checkAndLoadColumns();
        sql = wxT("SELECT ");
        std::vector<MetadataItem*> temp;
        i->getChildren(temp);
        bool first = true;
        for (std::vector<MetadataItem*>::const_iterator it = temp.begin(); it != temp.end(); ++it)
        {
            if (first)
                first = false;
            else
                sql += wxT(", ");
            sql += (*it)->getName();
        }
        sql += wxT("\nFROM ") + i->getName();
    }

    ExecuteSqlFrame* eff = new ExecuteSqlFrame(this, -1, wxString(_("Execute SQL statements")));
    eff->setDatabase(d);
    eff->Show();
    eff->setSql(sql);
    if (t != ntProcedure)
        eff->executeAllStatements();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuRegisterDatabase(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Server* s = tree_ctrl_1->getSelectedServer();
    if (!checkValidServer(s))
        return;

    Database db;
    DatabaseRegistrationDialog drd(this, -1, _("Register Existing Database"));
    drd.setDatabase(&db);

    if (drd.ShowModal() == wxID_OK)
        tree_ctrl_1->selectMetadataItem(s->addDatabase(db));

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuRestoreIntoNewDatabase(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Server* s = tree_ctrl_1->getSelectedServer();
    if (!checkValidServer(s))
        return;

    Database db;
    DatabaseRegistrationDialog drd(this, -1, _("New database parameters"));
    drd.setDatabase(&db);
    if (drd.ShowModal() != wxID_OK)
        return;

    Database* newDB = s->addDatabase(db);
    tree_ctrl_1->selectMetadataItem(newDB);
    RestoreFrame* f = new RestoreFrame(this, newDB);
    f->Show();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuDatabaseRegistrationInfo(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Database* d = tree_ctrl_1->getSelectedDatabase();
    if (!checkValidDatabase(d))
        return;

    DatabaseRegistrationDialog drd(this, -1, _("Database Registration Info"));
    drd.setDatabase(d);
    drd.ShowModal();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateDatabase(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Server* s = tree_ctrl_1->getSelectedServer();
    if (!checkValidServer(s))
        return;

    Database db;
    DatabaseRegistrationDialog drd(this, -1, _("Create New Database"), true);
    drd.setDatabase(&db);
    drd.setServer(s);

    if (drd.ShowModal() == wxID_OK)
        tree_ctrl_1->selectMetadataItem(s->addDatabase(db));

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuManageUsers(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    // TODO:
    ::wxMessageBox(_("This feature is not yet implemented"), _("Not in this version"), wxICON_INFORMATION);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUnRegisterServer(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Server* s = tree_ctrl_1->getSelectedServer();
    if (!checkValidServer(s))
        return;

    if (wxCANCEL == wxMessageBox(_("Are you sure?"), _("Unregister server"), wxOK | wxCANCEL | wxICON_QUESTION))
        return;

    Root* r = dynamic_cast<Root*>(s->getParent());
    if (r)
        r->removeServer(s);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuServerProperties(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Server* s = tree_ctrl_1->getSelectedServer();
    if (!checkValidServer(s))
        return;

    ServerRegistrationDialog srd(this, -1, _("Server Registration Info"));
    srd.setServer(s);
    srd.ShowModal();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuRegisterServer(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Root* r = dynamic_cast<Root*>(tree_ctrl_1->getMetadataItem(tree_ctrl_1->GetRootItem()));
    if (!r)
        return;

    Server s;
    ServerRegistrationDialog srd(this, -1, _("Register New Server"));
    srd.setServer(&s);
    if (wxID_OK == srd.ShowModal())
        tree_ctrl_1->selectMetadataItem(r->addServer(s));

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUnRegisterDatabase(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Database* d = tree_ctrl_1->getSelectedDatabase();
    if (!checkValidDatabase(d))
        return;
    // command should never be enabled when database is connected
    wxCHECK_RET(!d->isConnected(),
        wxT("Can not unregister connected database"));

    if (wxCANCEL == wxMessageBox(_("Are you sure?"), _("Unregister database"), wxOK | wxCANCEL | wxICON_QUESTION))
        return;

    Server* s = d->getServer();
    if (s)
        s->removeDatabase(d);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuShowConnectedUsers(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Database* d = tree_ctrl_1->getSelectedDatabase();
    if (!checkValidDatabase(d))
        return;

    wxArrayString as;
    std::vector<std::string> users;
    d->getIBPPDatabase()->Users(users);
    for (std::vector<std::string>::const_iterator i = users.begin(); i != users.end(); ++i)
        as.Add(std2wx(*i));

    ::wxGetSingleChoice(_("Connected users"), d->getPath(), as);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuBackup(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Database* d = tree_ctrl_1->getSelectedDatabase();
    if (!checkValidDatabase(d))
        return;

    BackupFrame* f = new BackupFrame(this, d);
    f->Show();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuRestore(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Database* d = tree_ctrl_1->getSelectedDatabase();
    if (!checkValidDatabase(d))
        return;

    RestoreFrame* f = new RestoreFrame(this, d);
    f->Show();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuReconnect(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Database* d = tree_ctrl_1->getSelectedDatabase();
    if (!checkValidDatabase(d))
        return;

    ::wxBeginBusyCursor();
    bool ok = d->reconnect();
    ::wxEndBusyCursor();
    if (!ok)
        reportLastError(_("Error Reconnecting Database"));

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuConnectAs(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Database* d = tree_ctrl_1->getSelectedDatabase();
    if (!checkValidDatabase(d))
        return;
    // command should never be enabled when database is connected
    wxCHECK_RET(!d->isConnected(),
        wxT("Can not connect to already connected database"));

    DatabaseRegistrationDialog drd(this, -1, _("Connect as..."), false, true);
    d->prepareTemporaryCredentials();
    drd.setDatabase(d);
    if (wxID_OK != drd.ShowModal() || !connect(false))
        d->resetCredentials();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuConnect(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    connect(true);  // true = warn if already connected

    FR_CATCH
}
//-----------------------------------------------------------------------------
bool MainFrame::connect(bool warn)
{
    Database* db = tree_ctrl_1->getSelectedDatabase();
    if (!checkValidDatabase(db))
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
        message += db->getUsername();
        pass = ::wxGetPasswordFromUser(message, _("Connecting to database"));
        if (pass.IsEmpty())
            return false;
    }
    else
        pass = db->getPassword();

    wxProgressDialog progress_dialog(_("Connecting..."), db->getPath(), 9, NULL,
        wxPD_AUTO_HIDE | wxPD_APP_MODAL);
    if (!db->connect(pass))
    {
        reportLastError(_("Error Connecting to Database"));
        return false;
    }

    NodeType types[9] = { ntTable, ntView, ntProcedure, ntTrigger, ntRole, ntDomain,
        ntFunction, ntGenerator, ntException };
    wxString names[9] = { _("Tables"), _("Views"), _("Procedures"), _("Triggers"), _("Roles"), _("Domains"),
        _("Functions"), _("Generators"), _("Exceptions")
    };
    for (int i = 0; i < 9; i++)
    {
        progress_dialog.Update(i + 1, wxString::Format(_("Loading %s."), names[i].c_str()));
        if (!db->loadObjects(types[i]))
        {
            reportLastError(wxString::Format(_("Error Loading %s"),
                names[i].c_str()));
            break;
        }
    }

    wxTreeItemId id = tree_ctrl_1->GetSelection();
    tree_ctrl_1->Expand(id);

    if (db->usesDifferentConnectionCharset())
    {
        DatabaseConfig dc(db);
        if (dc.get(wxT("differentCharsetWarning"), true))
        {
            if (wxNO == wxMessageBox(wxString::Format(
                _("Database charset: %s\nis different from connection charset: %s.\n\nWould you like to be reminded next time?"),
                db->getDatabaseCharset().c_str(),
                db->getConnectionCharset().c_str()),
                _("Warning"),
                wxICON_QUESTION | wxYES_NO))
            {
                dc.setValue(wxT("differentCharsetWarning"), false);
            }
        }
    }
    return true;
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuDisconnect(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Database* d = tree_ctrl_1->getSelectedDatabase();
    if (!checkValidDatabase(d))
        return;
    if (!d->disconnect())
        reportLastError(_("Error Disconnecting Database"));

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::showGeneratorValue(Generator* g)
{
    if (!g)
        return;
    if (!g->loadValue(true))
        reportLastError(_("Error Loading Generator Value"));
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuShowGeneratorValue(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    showGeneratorValue(dynamic_cast<Generator*>(tree_ctrl_1->getSelectedMetadataItem()));

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuSetGeneratorValue(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Generator* g = dynamic_cast<Generator*>(tree_ctrl_1->getSelectedMetadataItem());
    if (!g)
        return;

    URI uri = URI(wxT("fr://edit_generator_value?parent_window=") + wxString::Format(wxT("%ld"), (uintptr_t)this)
        + wxT("&object_address=") + wxString::Format(wxT("%ld"), (uintptr_t)g));
    getURIProcessor().handleURI(uri);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuShowAllGeneratorValues(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Database* d = tree_ctrl_1->getSelectedDatabase();
    if (checkValidDatabase(d))
    {
        if (!d->loadGeneratorValues())
            reportLastError(_("Error Loading Generator Value"));
    }

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateDomain(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Domain d;
    showCreateTemplate(&d);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateException(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Exception e;
    showCreateTemplate(&e);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateFunction(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Function x;
    showCreateTemplate(&x);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateGenerator(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Generator x;
    showCreateTemplate(&x);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateProcedure(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Procedure x;
    showCreateTemplate(&x);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateRole(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Role x;
    showCreateTemplate(&x);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateTable(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Table x;
    showCreateTemplate(&x);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateTrigger(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Trigger x;
    showCreateTemplate(&x);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateView(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    View x;
    showCreateTemplate(&x);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateObject(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    MetadataItem* item = tree_ctrl_1->getSelectedMetadataItem();
    if (!item)
        return;
    showCreateTemplate(item);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::showCreateTemplate(const MetadataItem* m)
{
    // TODO: add a call for wizards. For example, we can have NewTableWizard which is a frame with grid in which
    // user can enter column data for new table (name, datatype, null option, collation, default, etc.) and also
    // enter a name for new table, etc. Wizard should return a bunch of DDL statements as a wxString which would we
    // pass to ExecuteSqlFrame.

    wxString sql = m->getCreateSqlTemplate();
    if (sql == wxT(""))
    {
        wxMessageBox(_("The feature is not yet available for this type of database objects."), _("Not yet implemented"), wxOK | wxICON_INFORMATION);
        return;
    }

    Database* db = tree_ctrl_1->getSelectedDatabase();
    if (!checkValidDatabase(db))
        return;

    ExecuteSqlFrame* eff = new ExecuteSqlFrame(this, -1, wxEmptyString);
    eff->setDatabase(db);
    eff->setSql(sql);
    eff->Show();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuLoadColumnsInfo(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    MetadataItem* t = tree_ctrl_1->getSelectedMetadataItem();
    if (!t)
        return;

    bool success = true;
    switch (t->getType())
    {
        case ntTable:
        case ntView:        success = ((Relation*)t)->checkAndLoadColumns();   break;
        case ntProcedure:   success = ((Procedure*)t)->checkAndLoadParameters();               break;
        default:            break;
    };

    if (!success)
    {
        reportLastError(_("Error Loading Information"));
        return;
    }

    if (t->getType() == ntProcedure)
    {
        std::vector<MetadataItem*> temp;
        ((Procedure*)t)->getChildren(temp);
        if (temp.empty())
        {
            ::wxMessageBox(_("This procedure doesn't have any input or output parameters."),
               _("Information"), wxOK | wxICON_INFORMATION);
        }
    }

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuAddColumn(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    MetadataItem* i = tree_ctrl_1->getSelectedMetadataItem();
    if (!i)
        return;
    Table* t = dynamic_cast<Table*>(i);
    if (!t)
        return;

    FieldPropertiesFrame* f = new FieldPropertiesFrame(this, -1,
        wxString::Format(_("TABLE: %s"), t->getName().c_str()),
        t);
    f->setProperties();
    f->Show();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuToggleDisconnected(wxCommandEvent& event)
{
    FR_TRY

    config().setValue(wxT("HideDisconnectedDatabases"), !event.IsChecked());
    getGlobalRoot().notifyAllServers();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuToggleStatusBar(wxCommandEvent& event)
{
    FR_TRY

    wxStatusBar* s = GetStatusBar();
    if (!s)
        s = CreateStatusBar();

    bool show = event.IsChecked();
    config().setValue(wxT("showStatusBar"), show);
    s->Show(show);
    SendSizeEvent();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuObjectProperties(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    MetadataItem* m = tree_ctrl_1->getSelectedMetadataItem();
    if (!m)
        return;

    Column* c = dynamic_cast<Column*>(m);
    if (c)
    {
        Table* t = dynamic_cast<Table*>(c->getParent());
        if (!t)     // dummy check
            return;
        FieldPropertiesFrame* f = new FieldPropertiesFrame(this, -1,
            wxEmptyString, t);
        f->setField(c);
        f->Show();
    }
    else
        frameManager().showMetadataPropertyFrame(this, m);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuDropObject(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Database* d = tree_ctrl_1->getSelectedDatabase();
    if (!checkValidDatabase(d))
        return;
    MetadataItem* m = tree_ctrl_1->getSelectedMetadataItem();
    if (!m)
        return;

    // TODO: We could first check if there are some dependant objects, and offer the user to
    //       either drop dependencies, or drop those objects too. Then we should create a bunch of
    //       sql statements that do it.

    wxString sql = m->getDropSqlStatement();
    ExecuteSqlFrame* eff = new ExecuteSqlFrame(this, -1, sql);
    eff->setDatabase(d);
    eff->Show();
    eff->setSql(sql);
    eff->executeAllStatements(true);        // true = user must commit/rollback + frame is closed at once

    FR_CATCH
}
//-----------------------------------------------------------------------------
//! create new ExecuteSqlFrame and attach database object to it
void MainFrame::OnMenuQuery(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Database* d = tree_ctrl_1->getSelectedDatabase();
    if (!checkValidDatabase(d))
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
    ExecuteSqlFrame* eff = new ExecuteSqlFrame(this, -1, wxString(_("Execute SQL statements")));
    eff->setDatabase(d);
    eff->Show();

    FR_CATCH
}
//-----------------------------------------------------------------------------
const wxString MainFrame::getName() const
{
    return wxT("MainFrame");
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUpdateUnRegisterServer(wxUpdateUIEvent& event)
{
    FR_TRY

    Server* s = tree_ctrl_1->getSelectedServer();
    event.Enable(s != 0 && !s->hasConnectedDatabase());

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUpdateIfServerSelected(wxUpdateUIEvent& event)
{
    FR_TRY

    Server* s = tree_ctrl_1->getSelectedServer();
    event.Enable(s != 0);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUpdateIfDatabaseConnected(wxUpdateUIEvent& event)
{
    FR_TRY

    Database* d = tree_ctrl_1->getSelectedDatabase();
    event.Enable(d != 0 && d->isConnected());

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUpdateIfDatabaseNotConnected(wxUpdateUIEvent& event)
{
    FR_TRY

    Database* d = tree_ctrl_1->getSelectedDatabase();
    event.Enable(d != 0 && !d->isConnected());

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUpdateIfDatabaseSelected(wxUpdateUIEvent& event)
{
    FR_TRY

    Database* d = tree_ctrl_1->getSelectedDatabase();
    event.Enable(d != 0);

    FR_CATCH
}
//-----------------------------------------------------------------------------
