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

#include <wx/clipbrd.h>
#include <wx/dir.h>
#include <wx/dnd.h>
#include <wx/arrstr.h>

#include "config/Config.h"
#include "config/DatabaseConfig.h"
#include "core/ArtProvider.h"
#include "core/CodeTemplateProcessor.h"
#include "core/FRError.h"
#include "core/URIProcessor.h"
#include "frutils.h"
#include "gui/AboutBox.h"
#include "gui/AdvancedMessageDialog.h"
#include "gui/AdvancedSearchFrame.h"
#include "gui/BackupFrame.h"
#include "gui/CommandIds.h"
#include "gui/ContextMenuMetadataItemVisitor.h"
#include "gui/controls/DBHTreeControl.h"
#include "gui/DataGeneratorFrame.h"
#include "gui/DatabaseRegistrationDialog.h"
#include "gui/EventWatcherFrame.h"
#include "gui/ExecuteSql.h"
#include "gui/ExecuteSqlFrame.h"
#include "gui/MainFrame.h"
#include "gui/MetadataItemPropertiesFrame.h"
#include "gui/PreferencesDialog.h"
#include "gui/ProgressDialog.h"
#include "gui/RestoreFrame.h"
#include "gui/ServerRegistrationDialog.h"
#include "gui/SimpleHtmlFrame.h"
#include "gui/ShutdownFrame.h"
#include "gui/StartupFrame.h"
#include "main.h"
#include "metadata/column.h"
#include "metadata/domain.h"
#include "metadata/generator.h"
#include "metadata/function.h"
#include "metadata/MetadataItemCreateStatementVisitor.h"
#include "metadata/MetadataTemplateManager.h"
#include "metadata/package.h"
#include "metadata/procedure.h"
#include "metadata/root.h"
#include "metadata/server.h"
#include "metadata/table.h"
#include "metadata/trigger.h"
#include "metadata/view.h"


bool checkValidDatabase(DatabasePtr database)
{
    if (database)
        return true;
    wxMessageBox(_("Operation can not be performed - no database assigned"),
        _("Internal Error"), wxOK | wxICON_ERROR);
    return false;
}

bool checkValidServer(ServerPtr server)
{
    if (server)
        return true;
    wxMessageBox(_("Operation can not be performed - no server assigned"),
        _("Internal Error"), wxOK | wxICON_ERROR);
    return false;
}

DatabasePtr getDatabase(MetadataItem* mi)
{
    if (mi)
        return mi->getDatabase();
    return DatabasePtr();
}

ServerPtr getServer(MetadataItem* mi)
{
    if (mi)
    {
        if (Server* s = dynamic_cast<Server*>(mi))
            return s->shared_from_this();
        if (DatabasePtr db = mi->getDatabase())
            return db->getServer();
    }
    return ServerPtr();
}

//! helper class to enable drag and drop of database files to the tree ctrl
#if wxUSE_DRAG_AND_DROP
class DnDDatabaseFile : public wxFileDropTarget
{
private:
    MainFrame* frameM;
public:
    DnDDatabaseFile(MainFrame* frame) { frameM = frame; }
    virtual bool OnDropFiles(wxCoord, wxCoord, const wxArrayString& filenames)
    {
        for (size_t i = 0; i < filenames.GetCount(); i++)
            frameM->openUnregisteredDatabase(filenames[i]);
        return true;
    }
};
#endif

MainFrame::MainFrame(wxWindow* parent, int id, const wxString& title,
        const wxPoint& pos, const wxSize& size, long style)
    : BaseFrame(parent, id, title, pos, size, style, "FlameRobin_main"),
        rootM(new Root())
{
    wxArtProvider::Push(new ArtProvider);

    mainPanelM = new wxPanel(this);
    treeMainM = new DBHTreeControl(mainPanelM, wxDefaultPosition,
        wxDefaultSize,
#if defined __WXGTK20__ || defined __WXMAC__
        wxTR_NO_LINES |
#endif
        wxTR_HAS_BUTTONS | wxBORDER_THEME);

    wxArrayString choices;  // load from config?

    searchPanelM = new wxPanel(mainPanelM, -1, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL | wxBORDER_THEME);
    long comboStyle =  wxCB_DROPDOWN | wxTE_PROCESS_ENTER;
#ifndef __WXMAC__ 
    // Not supported on OSX/Cocoa presently 
    comboStyle |= wxCB_SORT;
#endif 
    searchBoxM = new wxComboBox(searchPanelM, ID_search_box, wxEmptyString,
        wxDefaultPosition, wxDefaultSize, choices, comboStyle);
        wxSize btnBmpSize(16, 16);
    button_prev = new wxBitmapButton(searchPanelM, ID_button_prev,
        wxArtProvider::GetBitmap(wxART_GO_BACK, wxART_TOOLBAR, btnBmpSize));
    button_next = new wxBitmapButton(searchPanelM, ID_button_next,
        wxArtProvider::GetBitmap(wxART_GO_FORWARD, wxART_TOOLBAR, btnBmpSize));
    button_advanced = new wxBitmapButton(searchPanelM, ID_button_advanced,
        wxArtProvider::GetBitmap(wxART_FIND, wxART_TOOLBAR, btnBmpSize));
    button_advanced->SetToolTip(_("Advanced metadata search"));
    button_prev->SetToolTip(_("Previous match"));
    button_next->SetToolTip(_("Next match"));

    buildMainMenu();
    SetStatusBarPane(-1);   // disable automatic fill
    set_properties();
    do_layout();
    treeMainM->SetFocus();
#if wxUSE_DRAG_AND_DROP
    treeMainM->SetDropTarget(new DnDDatabaseFile(this));
#endif
    if (!config().get("showSearchBar", true))
    {
        searchPanelSizerM->Show(searchPanelM, false, true);    // recursive
        searchPanelSizerM->Layout();
    }
}

void MainFrame::buildMainMenu()
{
    menuBarM = new wxMenuBar();

    databaseMenuM = new wxMenu();
    databaseMenuM->Append(Cmds::Menu_NewVolatileSQLEditor, _("Open new Volatile &SQL Editor..."));
    databaseMenuM->Append(Cmds::Menu_RegisterDatabase, _("R&egister existing database..."));
    databaseMenuM->Append(Cmds::Menu_CreateDatabase, _("Create &new database..."));
    databaseMenuM->Append(Cmds::Menu_RestoreIntoNew, _("Restore bac&kup into new database..."));
    databaseMenuM->AppendSeparator();
    menuBarM->Append(databaseMenuM, _("&Database"));

#ifdef __WXMAC__
    wxMenu* editMenu = new wxMenu();
    editMenu->Append(wxID_CUT, _("Cu&t"));
    editMenu->Append(wxID_COPY, _("&Copy"));
    editMenu->Append(wxID_PASTE, _("&Paste"));
    editMenu->Append(wxID_CLEAR, _("&Delete"));
    menuBarM->Append(editMenu, _("&Edit"));
#endif

    wxMenu* viewMenu = new wxMenu();
    viewMenu->AppendCheckItem(Cmds::Menu_ToggleStatusBar, _("&Status bar"));
    viewMenu->AppendCheckItem(Cmds::Menu_ToggleSearchBar, _("S&earch bar"));
    viewMenu->AppendCheckItem(Cmds::Menu_ToggleDisconnected, _("&Disconnected databases"));
    viewMenu->AppendSeparator();
    viewMenu->Append(wxID_PREFERENCES, _("P&references..."));
    menuBarM->Append(viewMenu, _("&View"));
//    frameManager().setWindowMenu(viewMenu);

    wxMenu* serverMenu = new wxMenu();
    serverMenu->Append(Cmds::Menu_RegisterServer, _("&Register server..."));
    serverMenu->Append(Cmds::Menu_UnRegisterServer, _("&Unregister server"));
    serverMenu->Append(Cmds::Menu_ServerProperties, _("Server registration &info"));
    serverMenu->AppendSeparator();
    serverMenu->Append(Cmds::Menu_GetServerVersion, _("Retrieve server &version"));
    serverMenu->Append(Cmds::Menu_ManageUsers, _("&Manage users"));
    menuBarM->Append(serverMenu, _("&Server"));


    objectMenuM = new wxMenu();
    wxMenu* newMenu = new wxMenu();
    newMenu->Append(Cmds::Menu_CreateCollation, _("&Collation"));
    newMenu->Append(Cmds::Menu_CreateDBTrigger, _("D&B Trigger"));
    newMenu->Append(Cmds::Menu_CreateDDLTrigger, _("DD&L Trigger"));
    newMenu->Append(Cmds::Menu_CreateDMLTrigger, _("DML Tr&igger"));
    newMenu->Append(Cmds::Menu_CreateDomain, _("&Domain"));
    newMenu->Append(Cmds::Menu_CreateException, _("&Exception"));
    newMenu->Append(Cmds::Menu_CreateFunction, _("&Function"));
    newMenu->Append(Cmds::Menu_CreateGenerator, _("&Generator"));
    newMenu->Append(Cmds::Menu_CreateGTTTable, _("Global &Temporary"));
    newMenu->Append(Cmds::Menu_CreateIndex, _("&Index"));
    newMenu->Append(Cmds::Menu_CreatePackage, _("P&ackage"));
    newMenu->Append(Cmds::Menu_CreateProcedure, _("&Procedure"));
    newMenu->Append(Cmds::Menu_CreateRole, _("&Role"));
    newMenu->Append(Cmds::Menu_CreateTable, _("&Table"));
    newMenu->Append(Cmds::Menu_CreateUDF, _("&UDF"));
    newMenu->Append(Cmds::Menu_CreateUser, _("U&ser"));
    newMenu->Append(Cmds::Menu_CreateView, _("&View"));
    // removed accelerator from "New", any of them potentially conflicts
    // with one of the commands in the object menu
    objectMenuM->Append(Cmds::Menu_NewObject, _("New"), newMenu);
    menuBarM->Append(objectMenuM, _("&Object"));

    wxMenu* helpMenu = new wxMenu();
    helpMenu->Append(Cmds::Menu_Manual, _("&Manual"));
    helpMenu->Append(Cmds::Menu_RelNotes, _("&What's new"));
    helpMenu->Append(Cmds::Menu_License, _("&License"));
    helpMenu->AppendSeparator();
    helpMenu->Append(Cmds::Menu_URLHomePage, _("FlameRobin &home page"));
    helpMenu->Append(Cmds::Menu_URLProjectPage, _("Github &project page"));
    helpMenu->Append(Cmds::Menu_URLFeatureRequest, _("Github &feature requests"));
    helpMenu->Append(Cmds::Menu_URLBugReport, _("Github &bug reports"));
#ifndef __WXMAC__
    helpMenu->AppendSeparator();
#endif
    helpMenu->Append(wxID_ABOUT, _("&About"));
    menuBarM->Append(helpMenu, _("&Help"));
    SetMenuBar(menuBarM);

    // update checkboxes
    config().setValue("HideDisconnectedDatabases", false);
    viewMenu->Check(Cmds::Menu_ToggleDisconnected, true);
    if (config().get("showStatusBar", true))
    {
        CreateStatusBar();
        viewMenu->Check(Cmds::Menu_ToggleStatusBar, true);
        GetStatusBar()->SetStatusText(_("[No database selected]"));
    }
    if (config().get("showSearchBar", true))
        viewMenu->Check(Cmds::Menu_ToggleSearchBar, true);
}

void MainFrame::showDocsHtmlFile(const wxString& fileName)
{
    wxFileName fullFileName(config().getDocsPath(), fileName);
    showHtmlFile(this, fullFileName);
}

void MainFrame::showUrl(const wxString& url)
{
    if (!wxLaunchDefaultBrowser(url))
        wxLogError(_T("Failed to open URL \"%s\""), url.c_str());
}

void MainFrame::set_properties()
{
    SetTitle(_("FlameRobin Database Admin"));

    if (!rootM->load())
    {
        wxString confile = config().getDBHFileName();
        if (confile.Length() > 20)
            confile = "\n" + confile + "\n";  // break into several lines if path is long
        else
            confile = " " + confile + " ";
        wxString msg;
        msg.Printf(_("The configuration file:%sdoes not exist or can not be opened.\n\nThis is normal for first time users.\n\nYou may now register new servers and databases."),
            confile.c_str());
        wxMessageBox(msg, _("Configuration file not found"), wxOK|wxICON_INFORMATION);

        ServerPtr s(new Server());
        s->setName_("Localhost");
        s->setHostname("localhost");
        rootM->addServer(s);
        rootM->save();
    }
    wxTreeItemId rootNode = treeMainM->addRootNode(rootM.get());
    treeMainM->Expand(rootNode);

    // make the first server active
    wxTreeItemIdValue cookie;
    wxTreeItemId firstServer = treeMainM->GetFirstChild(rootNode, cookie);
    if (firstServer.IsOk())
    {
        treeMainM->SelectItem(firstServer);
    }

    SetIcon(wxArtProvider::GetIcon(ART_FlameRobin, wxART_FRAME_ICON));
}

void MainFrame::do_layout()
{
    wxSizer* sizerCB = new wxBoxSizer(wxVERTICAL);
    sizerCB->AddStretchSpacer(1);
    sizerCB->Add(searchBoxM, 0, wxEXPAND);
    sizerCB->AddStretchSpacer(1);

    wxSizer* sizerSearch = new wxBoxSizer(wxHORIZONTAL);
    sizerSearch->Add(sizerCB, 1, wxEXPAND);
    sizerSearch->Add(button_prev);
    sizerSearch->Add(button_next);
    sizerSearch->Add(button_advanced);
    searchPanelM->SetSizer(sizerSearch);

    searchPanelSizerM = new wxBoxSizer(wxVERTICAL);
    searchPanelSizerM->Add(treeMainM, 1, wxEXPAND);
    searchPanelSizerM->Add(searchPanelM, 0, wxEXPAND);
    mainPanelM->SetSizer(searchPanelSizerM);

    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(mainPanelM, 1, wxEXPAND, 0);
    SetAutoLayout(true);
    SetSizer(sizerAll);
    Layout();
}

const wxRect MainFrame::getDefaultRect() const
{
    return wxRect(-1, -1, 360, 480);
}

DBHTreeControl* MainFrame::getTreeCtrl()
{
    return treeMainM;
}

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_MENU(Cmds::Menu_RegisterServer, MainFrame::OnMenuRegisterServer)
EVT_MENU(wxID_EXIT, MainFrame::OnMenuQuit)
EVT_MENU(wxID_ABOUT, MainFrame::OnMenuAbout)
EVT_MENU(Cmds::Menu_Manual, MainFrame::OnMenuManual)
EVT_MENU(Cmds::Menu_RelNotes, MainFrame::OnMenuRelNotes)
EVT_MENU(Cmds::Menu_License, MainFrame::OnMenuLicense)
EVT_MENU(Cmds::Menu_URLHomePage, MainFrame::OnMenuURLHomePage)
EVT_MENU(Cmds::Menu_URLProjectPage, MainFrame::OnMenuURLProjectPage)
EVT_MENU(Cmds::Menu_URLFeatureRequest, MainFrame::OnMenuURLFeatureRequest)
EVT_MENU(Cmds::Menu_URLBugReport, MainFrame::OnMenuURLBugReport)
EVT_MENU(wxID_PREFERENCES, MainFrame::OnMenuConfigure)

EVT_MENU(Cmds::Menu_NewVolatileSQLEditor, MainFrame::OnMenuNewVolatileSQLEditor)
EVT_MENU(Cmds::Menu_RegisterDatabase, MainFrame::OnMenuRegisterDatabase)
EVT_UPDATE_UI(Cmds::Menu_RegisterDatabase, MainFrame::OnMenuUpdateIfServerSelected)
EVT_MENU(Cmds::Menu_CreateDatabase, MainFrame::OnMenuCreateDatabase)
EVT_UPDATE_UI(Cmds::Menu_CreateDatabase, MainFrame::OnMenuUpdateIfServerSelected)
EVT_MENU(Cmds::Menu_RestoreIntoNew, MainFrame::OnMenuRestoreIntoNewDatabase)
EVT_UPDATE_UI(Cmds::Menu_RestoreIntoNew, MainFrame::OnMenuUpdateIfServerSelected)
EVT_MENU(Cmds::Menu_ManageUsers, MainFrame::OnMenuManageUsers)
EVT_UPDATE_UI(Cmds::Menu_ManageUsers, MainFrame::OnMenuUpdateIfServerSelected)
EVT_MENU(Cmds::Menu_UnRegisterServer, MainFrame::OnMenuUnRegisterServer)
EVT_UPDATE_UI(Cmds::Menu_UnRegisterServer, MainFrame::OnMenuUpdateUnRegisterServer)
EVT_MENU(Cmds::Menu_ServerProperties, MainFrame::OnMenuServerProperties)
EVT_UPDATE_UI(Cmds::Menu_ServerProperties, MainFrame::OnMenuUpdateIfServerSelected)

EVT_MENU(Cmds::Menu_UnRegisterDatabase, MainFrame::OnMenuUnRegisterDatabase)
EVT_UPDATE_UI(Cmds::Menu_UnRegisterDatabase, MainFrame::OnMenuUpdateIfDatabaseNotConnected)
EVT_MENU(Cmds::Menu_GetServerVersion, MainFrame::OnMenuGetServerVersion)
EVT_UPDATE_UI(Cmds::Menu_GetServerVersion, MainFrame::OnMenuUpdateIfServerSelected)
EVT_MENU(Cmds::Menu_MonitorEvents, MainFrame::OnMenuMonitorEvents)
EVT_UPDATE_UI(Cmds::Menu_MonitorEvents, MainFrame::OnMenuUpdateIfDatabaseConnectedOrAutoConnect)
EVT_MENU(Cmds::Menu_GenerateData, MainFrame::OnMenuGenerateData)
EVT_UPDATE_UI(Cmds::Menu_GenerateData, MainFrame::OnMenuUpdateIfDatabaseConnectedOrAutoConnect)
EVT_MENU(Cmds::Menu_CloneDatabase, MainFrame::OnMenuCloneDatabase)
EVT_UPDATE_UI(Cmds::Menu_CloneDatabase, MainFrame::OnMenuUpdateIfDatabaseSelected)
EVT_MENU(Cmds::Menu_DatabaseRegistrationInfo, MainFrame::OnMenuDatabaseRegistrationInfo)
EVT_UPDATE_UI(Cmds::Menu_DatabaseRegistrationInfo, MainFrame::OnMenuUpdateIfDatabaseSelected)
EVT_MENU(Cmds::Menu_Backup, MainFrame::OnMenuBackup)
EVT_UPDATE_UI(Cmds::Menu_Backup, MainFrame::OnMenuUpdateIfDatabaseSelected)
EVT_MENU(Cmds::Menu_Restore, MainFrame::OnMenuRestore)
EVT_UPDATE_UI(Cmds::Menu_Restore, MainFrame::OnMenuUpdateIfDatabaseNotConnected)
EVT_MENU(Cmds::Menu_Connect, MainFrame::OnMenuConnect)
EVT_UPDATE_UI(Cmds::Menu_Connect, MainFrame::OnMenuUpdateIfDatabaseNotConnected)
EVT_MENU(Cmds::Menu_ConnectAs, MainFrame::OnMenuConnectAs)
EVT_UPDATE_UI(Cmds::Menu_ConnectAs, MainFrame::OnMenuUpdateIfDatabaseNotConnected)
EVT_MENU(Cmds::Menu_Disconnect, MainFrame::OnMenuDisconnect)
EVT_UPDATE_UI(Cmds::Menu_Disconnect, MainFrame::OnMenuUpdateIfDatabaseConnected)
EVT_MENU(Cmds::Menu_Reconnect, MainFrame::OnMenuReconnect)
EVT_UPDATE_UI(Cmds::Menu_Reconnect, MainFrame::OnMenuUpdateIfDatabaseConnected)
EVT_MENU(Cmds::Menu_RecreateDatabase, MainFrame::OnMenuRecreateDatabase)
EVT_UPDATE_UI(Cmds::Menu_RecreateDatabase, MainFrame::OnMenuUpdateIfDatabaseConnectedOrAutoConnect)
EVT_MENU(Cmds::Menu_DropDatabase, MainFrame::OnMenuDropDatabase)
EVT_UPDATE_UI(Cmds::Menu_DropDatabase, MainFrame::OnMenuUpdateIfDatabaseConnectedOrAutoConnect)
EVT_MENU(Cmds::Menu_ExecuteStatements, MainFrame::OnMenuExecuteStatements)
EVT_UPDATE_UI(Cmds::Menu_ExecuteStatements, MainFrame::OnMenuUpdateIfDatabaseConnectedOrAutoConnect)
EVT_UPDATE_UI(Cmds::Menu_NewObject, MainFrame::OnMenuUpdateIfDatabaseConnectedOrAutoConnect)
EVT_MENU(Cmds::Menu_DatabasePreferences, MainFrame::OnMenuDatabasePreferences)
EVT_UPDATE_UI(Cmds::Menu_DatabasePreferences, MainFrame::OnMenuUpdateIfDatabaseSelected)
EVT_MENU(Cmds::Menu_DatabaseProperties, MainFrame::OnMenuDatabaseProperties)
EVT_UPDATE_UI(Cmds::Menu_DatabaseProperties, MainFrame::OnMenuUpdateIfDatabaseConnectedOrAutoConnect)

EVT_MENU(Cmds::Menu_ShutdownDatabase, MainFrame::OnMenuShutdownDatabase)
EVT_UPDATE_UI(Cmds::Menu_ShutdownDatabase, MainFrame::OnMenuUpdateIfDatabaseNotConnected)

EVT_MENU(Cmds::Menu_StartupDatabase, MainFrame::OnMenuStartupDatabase)
EVT_UPDATE_UI(Cmds::Menu_StartupDatabase, MainFrame::OnMenuUpdateIfDatabaseNotConnected)

    EVT_MENU(Cmds::Menu_BrowseData, MainFrame::OnMenuBrowseData)
    EVT_MENU(Cmds::Menu_AddColumn, MainFrame::OnMenuAddColumn)
    EVT_MENU(Cmds::Menu_ExecuteProcedure, MainFrame::OnMenuExecuteProcedure)
    EVT_MENU(Cmds::Menu_ExecuteFunction, MainFrame::OnMenuExecuteFunction)

    EVT_MENU(Cmds::Menu_ShowAllGeneratorValues, MainFrame::OnMenuShowAllGeneratorValues)
    EVT_UPDATE_UI(Cmds::Menu_ShowAllGeneratorValues, MainFrame::OnMenuUpdateIfMetadataItemHasChildren)
    EVT_MENU(Cmds::Menu_ShowGeneratorValue, MainFrame::OnMenuShowGeneratorValue)
    EVT_MENU(Cmds::Menu_SetGeneratorValue, MainFrame::OnMenuSetGeneratorValue)

    EVT_MENU(Cmds::Menu_ShowAllStatisticsValue, MainFrame::OnMenuShowAllStatisticsValues)
    EVT_UPDATE_UI(Cmds::Menu_ShowAllStatisticsValue, MainFrame::OnMenuUpdateIfMetadataItemHasChildren)
    EVT_MENU(Cmds::Menu_ShowStatisticsValue, MainFrame::OnMenuShowStatisticsValue)
    EVT_MENU(Cmds::Menu_SetStatisticsValue, MainFrame::OnMenuSetStatisticsValue)



    EVT_MENU(Cmds::Menu_CreateObject, MainFrame::OnMenuCreateObject)
    EVT_MENU(Cmds::Menu_AlterObject, MainFrame::OnMenuAlterObject)
    EVT_MENU(Cmds::Menu_DropObject, MainFrame::OnMenuDropObject)
    EVT_MENU(Cmds::Menu_ObjectProperties, MainFrame::OnMenuObjectProperties)
    EVT_UPDATE_UI(Cmds::Menu_ObjectProperties, MainFrame::OnMenuUpdateIfDatabaseConnectedOrAutoConnect)
    EVT_MENU(Cmds::Menu_ObjectRefresh, MainFrame::OnMenuObjectRefresh)
    EVT_UPDATE_UI(Cmds::Menu_ObjectRefresh, MainFrame::OnMenuUpdateIfDatabaseConnected)
    EVT_MENU(Cmds::Menu_RebuildObject, MainFrame::OnMenRebuildObject)
    EVT_MENU(Cmds::Menu_ActiveObject, MainFrame::OnMenActiveObject)
    EVT_MENU(Cmds::Menu_InactiveObject, MainFrame::OnMenInactiveObject)

    EVT_MENU(Cmds::Menu_ToggleStatusBar, MainFrame::OnMenuToggleStatusBar)
    EVT_MENU(Cmds::Menu_ToggleSearchBar, MainFrame::OnMenuToggleSearchBar)
    EVT_MENU(Cmds::Menu_ToggleDisconnected, MainFrame::OnMenuToggleDisconnected)

    EVT_TEXT_ENTER(MainFrame::ID_search_box, MainFrame::OnSearchBoxEnter)
    EVT_TEXT(MainFrame::ID_search_box, MainFrame::OnSearchTextChange)
    EVT_BUTTON(MainFrame::ID_button_advanced, MainFrame::OnButtonSearchClick)
    EVT_BUTTON(MainFrame::ID_button_prev, MainFrame::OnButtonPrevClick)
    EVT_BUTTON(MainFrame::ID_button_next, MainFrame::OnButtonNextClick)

    EVT_MENU(Cmds::Menu_CreateCollation,  MainFrame::OnMenuCreateCollation)
    EVT_MENU(Cmds::Menu_CreateDBTrigger,  MainFrame::OnMenuCreateDBTrigger)
    EVT_MENU(Cmds::Menu_CreateDDLTrigger, MainFrame::OnMenuCreateDDLTrigger)
    EVT_MENU(Cmds::Menu_CreateDMLTrigger, MainFrame::OnMenuCreateDMLTrigger)
    EVT_MENU(Cmds::Menu_CreateDomain,     MainFrame::OnMenuCreateDomain)
    EVT_MENU(Cmds::Menu_CreateException,  MainFrame::OnMenuCreateException)
    EVT_MENU(Cmds::Menu_CreateFunction,   MainFrame::OnMenuCreateFunction)
    EVT_MENU(Cmds::Menu_CreateGenerator,  MainFrame::OnMenuCreateGenerator)
    EVT_MENU(Cmds::Menu_CreateGTTTable,   MainFrame::OnMenuCreateGTTTable)
    EVT_MENU(Cmds::Menu_CreateIndex,      MainFrame::OnMenuCreateIndex)
    EVT_MENU(Cmds::Menu_CreatePackage,    MainFrame::OnMenuCreatePackage)
    EVT_MENU(Cmds::Menu_CreateProcedure,  MainFrame::OnMenuCreateProcedure)
    EVT_MENU(Cmds::Menu_CreateRole,       MainFrame::OnMenuCreateRole)
    EVT_MENU(Cmds::Menu_CreateTable,      MainFrame::OnMenuCreateTable)
    EVT_MENU(Cmds::Menu_CreateUDF,        MainFrame::OnMenuCreateUDF)
    EVT_MENU(Cmds::Menu_CreateUser,       MainFrame::OnMenuCreateUser)
    EVT_MENU(Cmds::Menu_CreateView,       MainFrame::OnMenuCreateView)

    EVT_MENU_RANGE(Cmds::Menu_TemplateFirst, Cmds::Menu_TemplateLast,
        MainFrame::OnMenuGenerateCode)

    EVT_MENU_OPEN(MainFrame::OnMainMenuOpen)
    EVT_TREE_SEL_CHANGED(DBHTreeControl::ID_tree_ctrl, MainFrame::OnTreeSelectionChanged)
    EVT_TREE_ITEM_ACTIVATED(DBHTreeControl::ID_tree_ctrl, MainFrame::OnTreeItemActivate)

    EVT_SET_FOCUS(MainFrame::OnSetFocus)
END_EVENT_TABLE()

void MainFrame::OnMainMenuOpen(wxMenuEvent& event)
{
    #ifndef __WXGTK__
    if (event.IsPopup())    // on gtk all menus are treated as popup apparently
    {
        event.Skip();
        return;
    }
    #endif

    MetadataItem* m = treeMainM->getSelectedMetadataItem();
    if (!m)
    {
        event.Skip();
        return;
    }

    if (m->getType() == ntUnknown)
    {
        event.Skip();
        return;
    }

    if (event.GetMenu() == objectMenuM)
    {
        // rebuild object menu
        while (objectMenuM->GetMenuItemCount() > 2)
            objectMenuM->Destroy(objectMenuM->FindItemByPosition(2));
        if (objectMenuM->GetMenuItemCount() == 1)
            objectMenuM->AppendSeparator();

        // object has to be subitem of database
        DatabasePtr db = m->getDatabase();
        if (db && db.get() != m)
        {
            MainObjectMenuMetadataItemVisitor visitor(objectMenuM);
            m->acceptVisitor(&visitor);
        }
        if (objectMenuM->GetMenuItemCount() == 2)   // separator
            objectMenuM->Destroy(objectMenuM->FindItemByPosition(1));
    }
    else if (event.GetMenu() == databaseMenuM)
    {
        // rebuild database menu
        while (databaseMenuM->GetMenuItemCount() > 4)
            databaseMenuM->Destroy(databaseMenuM->FindItemByPosition(4));
        // current object has to be subitem of database
        DatabasePtr db = m->getDatabase();
        if (db && db.get())
        {
            ContextMenuMetadataItemVisitor cmvd(databaseMenuM);
            db.get()->acceptVisitor(&cmvd);
            databaseMenuM->AppendSeparator();
        }
        databaseMenuM->Append(wxID_EXIT, _("&Quit"));
    }

    event.Skip();
}

void MainFrame::updateStatusbarText()
{
    if (wxStatusBar* sb = GetStatusBar())
    {
        if (DatabasePtr db = getDatabase(treeMainM->getSelectedMetadataItem()))
            sb->SetStatusText(db->getConnectionInfoString());
        else
            sb->SetStatusText(_("[No database selected]"));
    }
}

void MainFrame::OnTreeSelectionChanged(wxTreeEvent& WXUNUSED(event))
{
    updateStatusbarText();

    /* currently disabled until we decide on the new AUI interface for it
    // switch notebook to show the "Items" page
    int pg = notebookM->GetPageIndex(labelPanelM);
    if (pg == wxNOT_FOUND)  // Create it?
    {
        return;
    }
    notebookM->SetSelection(pg);

    // TODO: listctrl should show what tree shows, but it should also contain
    //       info about metadata items, provide context menu and double-click
    //       action for each of them and Observe the items for removal,
    //       changes and adding new ones.
    //       i.e. we need a special, separate class for this
    wxListCtrl *lc = labelPanelM->getListCtrl();
    lc->SetImageList(treeMainM->GetImageList(), wxIMAGE_LIST_SMALL);
    lc->ClearAll();
    wxTreeItemId t = treeMainM->GetSelection();
    if (!t.IsOk())
        return;
    for (wxTreeItemId id = treeMainM->GetLastChild(t); id.IsOk();
        id = treeMainM->GetPrevSibling(id))
    {
        lc->InsertItem(0, treeMainM->GetItemText(id),
            treeMainM->GetItemImage(id));
    }

    wxString path;
    while (t.IsOk())
    {
        if (!path.IsEmpty())
            path = " > ") + path;
        path = treeMainM->GetItemText(t) + path;
        t = treeMainM->GetItemParent(t);
    }
    labelPanelM->setLabel(path);
    */
}

//! handle double-click on item (or press Enter)
void MainFrame::OnTreeItemActivate(wxTreeEvent& event)
{
#ifndef __WXGTK__
    event.Skip();
#endif

    wxTreeItemId item = treeMainM->GetSelection();
    if (!item.IsOk())
        return;

    MetadataItem* m = treeMainM->getSelectedMetadataItem();
    if (!m)
        return;

    NodeType nt = m->getType();

    enum { showProperties = 0, showColumnInfo, selectFromOrExecute };
    int treeActivateAction = showProperties;
    config().getValue("OnTreeActivate", treeActivateAction);

    if (treeActivateAction == showColumnInfo && (nt == ntTable
        || nt == ntSysTable || nt == ntView || nt == ntProcedure))
    {
        m->ensureChildrenLoaded();
    }
    else if (treeActivateAction == selectFromOrExecute
        && (nt == ntTable || nt == ntSysTable || nt == ntView))
    {
        wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED,
            Cmds::Menu_BrowseData);
        AddPendingEvent(evt);
    }
    else if (treeActivateAction == selectFromOrExecute && (nt == ntProcedure))
    {
        wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED,
            Cmds::Menu_ExecuteProcedure);
        AddPendingEvent(evt);
    }
    else
    {
        switch (nt)
        {
            case ntDatabase:
                if (!dynamic_cast<Database *>(m)->isConnected())
                {
                    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED,
                        Cmds::Menu_Connect);
                    AddPendingEvent(evt);
                    return;
                }
                break;
            case ntGenerator:
                showGeneratorValue(dynamic_cast<Generator*>(m));
                break;
            case ntCollation:
            case ntColumn:
            case ntTable:
            case ntSysTable:
            case ntView:
            case ntPackage:
            case ntSysPackage:
            case ntProcedure:
            case ntDomain:
            case ntFunction:
            case ntFunctionSQL:
            case ntUDF:
            case ntDBTrigger:
            case ntDDLTrigger:
            case ntDMLTrigger:
            case ntException:
            case ntRole:
            case ntSysRole:
            case ntIndex:
            case ntSysIndices:
                {
                    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED,
                        Cmds::Menu_ObjectProperties);
                    AddPendingEvent(evt);
                }
                return;
            default:
                break;
        };
    }

    // Windows tree control automatically does it
#ifdef __WXGTK__
    bool toggle = true;
    config().getValue("ToggleNodeOnTreeActivate", toggle);
    if (toggle)
    {
        if (treeMainM->IsExpanded(item))
            treeMainM->Collapse(item);
        else
            treeMainM->Expand(item);
    }
#endif
}

bool MainFrame::doCanClose()
{
    std::vector<BaseFrame*> frames(BaseFrame::getFrames());
    for (std::vector<BaseFrame*>::iterator it = frames.begin();
        it != frames.end(); it++)
    {
        if ((*it) != this && !(*it)->Close())
            return false;
    }
    return true;
}

void MainFrame::doBeforeDestroy()
{
    Raise();
    //frameManager().setWindowMenu(0);    // tell it not to update menus anymore

    // the next few lines fix the (threading?) problem on some Linux distributions
    // which leave FlameRobin running if there were connected databases upon exit.
    // also, some other versions might crash (on Debian 64bit for example).
    // apparently, doing disconnect before exiting makes it work properly, and
    // on some distros, the wxSafeYield call is needed as well
    // as it doesn't hurt for others, we can leave it all here, at least until
    // Firebird packagers for various distros figure out how to properly use NPTL
    treeMainM->Freeze();
    rootM->disconnectAllDatabases();
    wxSafeYield();
    treeMainM->Thaw();

    wxTheClipboard->Flush();
}

void MainFrame::OnMenuQuit(wxCommandEvent& WXUNUSED(event))
{
    Close();
}

void MainFrame::OnMenuAbout(wxCommandEvent& WXUNUSED(event))
{
    showAboutBox(this);
}

void MainFrame::OnMenuManual(wxCommandEvent& WXUNUSED(event))
{
    showUrl("http://flamerobin.org/dokuwiki/wiki/manual");
}

void MainFrame::OnMenuRelNotes(wxCommandEvent& WXUNUSED(event))
{
    showDocsHtmlFile("fr_whatsnew.html");
}

void MainFrame::OnMenuLicense(wxCommandEvent& WXUNUSED(event))
{
    showDocsHtmlFile("fr_license.html");
}

void MainFrame::OnMenuURLHomePage(wxCommandEvent& WXUNUSED(event))
{
    showUrl("http://www.flamerobin.org");
}

void MainFrame::OnMenuURLProjectPage(wxCommandEvent& WXUNUSED(event))
{
    showUrl("https://github.com/mariuz/flamerobin");
}

void MainFrame::OnMenuURLFeatureRequest(wxCommandEvent& WXUNUSED(event))
{
    showUrl("https://github.com/mariuz/flamerobin/issues");
}

void MainFrame::OnMenuURLBugReport(wxCommandEvent& WXUNUSED(event))
{
    showUrl("https://github.com/mariuz/flamerobin/issues");
}

void MainFrame::OnMenuConfigure(wxCommandEvent& WXUNUSED(event))
{
    PreferencesDialog pd(this, _("Preferences"), config(),
        wxFileName(config().getConfDefsPath(), "fr_settings.confdef"));
    if (pd.isOk() && pd.loadFromTargetConfig())
    {
        static int pdSelection = 0;
        pd.selectPage(pdSelection);
        pd.ShowModal();
        pdSelection = pd.getSelectedPage();
    }
}

void MainFrame::OnMenuDatabaseProperties(wxCommandEvent& WXUNUSED(event))
{
    DatabasePtr db = getDatabase(treeMainM->getSelectedMetadataItem());
    if (!checkValidDatabase(db))
        return;
    if (!tryAutoConnectDatabase())
        return;

    MetadataItemPropertiesFrame::showPropertyPage(db.get());
}

void MainFrame::OnMenuExecuteFunction(wxCommandEvent& WXUNUSED(event))
{
    executeSysTemplate("execute_function",
        treeMainM->getSelectedMetadataItem(), this);

}

void MainFrame::OnMenActiveObject(wxCommandEvent& WXUNUSED(event))
{
    MetadataItem* mi = treeMainM->getSelectedMetadataItem();
    URI uri;
    if (dynamic_cast<Trigger*>(mi))
    {
        uri.parseURI("fr://activate_trigger");
    }
    else
    if (dynamic_cast<Index*>(mi))
    {
        uri.parseURI("fr://index_action");
        uri.addParam("type=TOGGLE_ACTIVE");
    }
    uri.addParam(wxString::Format("parent_window=%p", this));
    uri.addParam(wxString::Format("object_handle=%lu", mi->getHandle()));
    getURIProcessor().handleURI(uri);
    return;
}

void MainFrame::OnMenInactiveObject(wxCommandEvent& WXUNUSED(event))
{
    MetadataItem* mi = treeMainM->getSelectedMetadataItem();
    URI uri;
    if (dynamic_cast<Trigger*>(mi))
    {
        uri.parseURI("fr://deactivate_trigger");
    }else
    if (dynamic_cast<Index*>(mi))
    {
        uri.parseURI("fr://index_action");
        uri.addParam("type=TOGGLE_ACTIVE");
    }
    uri.addParam(wxString::Format("parent_window=%p", this));
    uri.addParam(wxString::Format("object_handle=%lu", mi->getHandle()));
    getURIProcessor().handleURI(uri);
    return;
}

void MainFrame::OnMenuShowAllStatisticsValues(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnMenuShowStatisticsValue(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnMenuSetStatisticsValue(wxCommandEvent& WXUNUSED(event))
{
    MetadataItem* mi = treeMainM->getSelectedMetadataItem();
    if (dynamic_cast<Index*>(mi))
    {
        URI uri("fr://index_action");
        uri.addParam(wxString::Format("parent_window=%p", this));
        uri.addParam(wxString::Format("object_handle=%lu", mi->getHandle()));
        uri.addParam("type=RECOMPUTE");
        getURIProcessor().handleURI(uri);
        return;
    }
}

void MainFrame::OnMenuShutdownDatabase(wxCommandEvent& WXUNUSED(event))
{
    DatabasePtr db = getDatabase(treeMainM->getSelectedMetadataItem());
    if (!checkValidDatabase(db))
        return;
    
    ShutdownFrame* rf = ShutdownFrame::findFrameFor(db);
    if (rf)
    {
        rf->Raise();
        return;
    }
    rf = new ShutdownFrame(this, db);
    rf->Show();
}

void MainFrame::OnMenuStartupDatabase(wxCommandEvent& WXUNUSED(event))
{
    DatabasePtr db = getDatabase(treeMainM->getSelectedMetadataItem());
    if (!checkValidDatabase(db))
        return;

    StartupFrame* rf = StartupFrame::findFrameFor(db);
    if (rf)
    {
        rf->Raise();
        return;
    }
    rf = new StartupFrame(this, db);
    rf->Show();

}


void MainFrame::OnMenuDatabasePreferences(wxCommandEvent& WXUNUSED(event))
{
    DatabasePtr d = getDatabase(treeMainM->getSelectedMetadataItem());
    if (!checkValidDatabase(d))
        return;
    DatabaseConfig dc(d.get(), config());
    PreferencesDialog pd(this,
        wxString::Format(_("%s Preferences"), d->getName_().c_str()), dc,
        wxFileName(config().getConfDefsPath(), "db_settings.confdef"));
    if (pd.isOk() && pd.loadFromTargetConfig())
    {
        pd.selectPage(0);
        pd.ShowModal();
    }
}

void MainFrame::OnMenuGenerateCode(wxCommandEvent& event)
{
    MetadataItem* mi = treeMainM->getSelectedMetadataItem();
    if (!mi)
        return;
    DatabasePtr database = getDatabase(mi);
    if (!checkValidDatabase(database))
        return;
    if (!tryAutoConnectDatabase(database))
        return;

    MetadataTemplateManager tm(mi);

    int i = (int)Cmds::Menu_TemplateFirst;
    for (TemplateDescriptorList::const_iterator it = tm.descriptorsBegin();
        it != tm.descriptorsEnd(); ++it, ++i)
    {
        if (i == event.GetId())
        {
            executeCodeTemplate((*it)->getTemplateFileName(), mi, database);
            break;
        }
    }
}

void MainFrame::OnMenuExecuteProcedure(wxCommandEvent& WXUNUSED(event))
{
    executeSysTemplate("execute_procedure",
        treeMainM->getSelectedMetadataItem(), this);
}

void MainFrame::OnMenuBrowseData(wxCommandEvent& WXUNUSED(event))
{
    executeSysTemplate("browse_data",
        treeMainM->getSelectedMetadataItem(), this);
}

void MainFrame::OnMenuNewVolatileSQLEditor(wxCommandEvent& WXUNUSED(event))
{
    DatabasePtr db;
    ServerPtr serverPtrM;

    db = std::make_shared<Database>();
    serverPtrM = std::make_shared<Server>();
    db->setServer(serverPtrM);
    db->setId(UINT_MAX-30);
    db->setIsVolatile(true);

    ExecuteSqlFrame* eff = new ExecuteSqlFrame(this, -1, _("Omni SQL Editor"), db);
    eff->Show();
}
void MainFrame::OnMenuRegisterDatabase(wxCommandEvent& WXUNUSED(event))
{
    ServerPtr s = getServer(treeMainM->getSelectedMetadataItem());
    if (!checkValidServer(s))
        return;

    DatabasePtr db(new Database());
    db->setServer(s);

    DatabaseRegistrationDialog drd(this, _("Register Existing Database"));
    drd.setDatabase(db);
    if (drd.ShowModal() == wxID_OK)
    {
        s->addDatabase(db);
        rootM->save();
        treeMainM->selectMetadataItem(db.get());
    }
}

void MainFrame::OnMenuRestoreIntoNewDatabase(wxCommandEvent& WXUNUSED(event))
{
    ServerPtr s = getServer(treeMainM->getSelectedMetadataItem());
    if (!checkValidServer(s))
        return;

    DatabasePtr db(new Database());
    db->setServer(s);

    DatabaseRegistrationDialog drd(this, _("New database parameters"));
    drd.setDatabase(db);
    if (drd.ShowModal() != wxID_OK)
        return;

    s->addDatabase(db);
    rootM->save();
    treeMainM->selectMetadataItem(db.get());
    RestoreFrame* f = new RestoreFrame(this, db);
    f->Show();
}

void MainFrame::OnMenuCloneDatabase(wxCommandEvent& WXUNUSED(event))
{
    ServerPtr s = getServer(treeMainM->getSelectedMetadataItem());
    if (!checkValidServer(s))
        return;

    DatabasePtr d = getDatabase(treeMainM->getSelectedMetadataItem());
    if (!checkValidDatabase(d))
        return;

    DatabasePtr db(new Database());
    db->setName_(d->getName_()+_(" clone"));
    db->getAuthenticationMode().setMode(db->getAuthenticationMode().getMode());
    db->setPath(d->getPath());
    db->setUsername(d->getUsername());
    db->setEncryptedPassword(d->getDecryptedPassword());
    db->setConnectionCharset(d->getConnectionCharset());
    db->setRole(d->getRole());

    DatabaseRegistrationDialog drd(this, _("Clone Registration Info"));
    drd.setDatabase(db);
    if (drd.ShowModal() == wxID_OK)
    {
        s->addDatabase(db);
        rootM->save();
        treeMainM->selectMetadataItem(db.get());
    }
}

void MainFrame::OnMenuDatabaseRegistrationInfo(wxCommandEvent& WXUNUSED(event))
{
    DatabasePtr d = getDatabase(treeMainM->getSelectedMetadataItem());
    if (!checkValidDatabase(d))
        return;

    DatabaseRegistrationDialog drd(this, _("Database Registration Info"));
    drd.setDatabase(d);
    if (drd.ShowModal() == wxID_OK)
    {
        rootM->save();
        updateStatusbarText();
    }
}

void MainFrame::OnMenuCreateDatabase(wxCommandEvent& WXUNUSED(event))
{
    ServerPtr s = getServer(treeMainM->getSelectedMetadataItem());
    if (!checkValidServer(s))
        return;

    DatabasePtr db(new Database());
    db->setServer(s);

    DatabaseRegistrationDialog drd(this, _("Create New Database"), true);
    drd.setDatabase(db);
    if (drd.ShowModal() == wxID_OK)
    {
        s->addDatabase(db);
        rootM->save();
        treeMainM->selectMetadataItem(db.get());
    }
}

void MainFrame::OnMenuManageUsers(wxCommandEvent& WXUNUSED(event))
{
    ServerPtr s = getServer(treeMainM->getSelectedMetadataItem());
    if (checkValidServer(s))
        MetadataItemPropertiesFrame::showPropertyPage(s.get());
}

void MainFrame::OnMenuUnRegisterServer(wxCommandEvent& WXUNUSED(event))
{
    ServerPtr s = getServer(treeMainM->getSelectedMetadataItem());
    if (!checkValidServer(s))
        return;

    int res = showQuestionDialog(this, _("Do you really want to unregister this server?"),
        _("The registration information for the server and all its registered databases will be deleted. This operation can not be undone."),
        AdvancedMessageDialogButtonsOkCancel(_("Unregister")));
    if (res == wxOK)
    {
        rootM->removeServer(s);
        rootM->save();
    }
}

void MainFrame::OnMenuServerProperties(wxCommandEvent& WXUNUSED(event))
{
    ServerPtr s = getServer(treeMainM->getSelectedMetadataItem());
    if (!checkValidServer(s))
        return;

    ServerRegistrationDialog srd(this, _("Server Registration Info"), s);
    if (srd.ShowModal() == wxID_OK)
        rootM->save();
}

void MainFrame::OnMenuRegisterServer(wxCommandEvent& WXUNUSED(event))
{
    ServerRegistrationDialog srd(this, _("Register New Server"));
    if (srd.ShowModal() == wxID_OK)
    {
        ServerPtr s = srd.getServer();
        rootM->addServer(s);
        rootM->save();
        treeMainM->selectMetadataItem(s.get());
    }
}

void MainFrame::OnMenuUnRegisterDatabase(wxCommandEvent& WXUNUSED(event))
{
    DatabasePtr d = getDatabase(treeMainM->getSelectedMetadataItem());
    if (!checkValidDatabase(d))
        return;
    // command should never be enabled when database is connected
    wxCHECK_RET(!d->isConnected(),
        "Can not unregister connected database");

    int res = showQuestionDialog(this, _("Do you really want to unregister this database?"),
        _("The registration information for the database will be deleted. This operation can not be undone."),
        AdvancedMessageDialogButtonsOkCancel(_("Unregister")));
    if (res == wxOK)
        unregisterDatabase(d);
}

void MainFrame::unregisterDatabase(DatabasePtr database)
{
    wxCHECK_RET(database,
        "Cannot unregister unassigned database");

    ServerPtr server = database->getServer();
    wxCHECK_RET(server,
        "Cannot unregister database without server");

    server->removeDatabase(database);
    rootM->save();
}

void MainFrame::OnMenuGetServerVersion(wxCommandEvent& WXUNUSED(event))
{
    ServerPtr s = getServer(treeMainM->getSelectedMetadataItem());
    if (!checkValidServer(s))
        return;

    std::string version;
    try
    {
        // progress dialog will get closed in case of fatal exception or when
        // retieving is complete
        ProgressDialog pd(this, _("Retrieving server version"), 1);
        pd.doShow();
        IBPP::Service svc;
        if (!getService(s.get(), svc, &pd, false))    // false = no need for sysdba
            return;
        svc->GetVersion(version);
    }
    catch (IBPP::Exception& e)
    {
        wxMessageBox(e.what(), _("Error"));
        return;
    }

    wxMessageBox(version, _("Server Version"),
        wxOK | wxICON_INFORMATION);
}

void MainFrame::OnMenuGenerateData(wxCommandEvent& WXUNUSED(event))
{
    DatabasePtr db = getDatabase(treeMainM->getSelectedMetadataItem());
    if (!checkValidDatabase(db))
        return;
    if (!tryAutoConnectDatabase(db))
        return;

    DataGeneratorFrame* f = new DataGeneratorFrame(this, db.get());
    f->Show();
}

void MainFrame::OnMenuMonitorEvents(wxCommandEvent& WXUNUSED(event))
{
    DatabasePtr db = getDatabase(treeMainM->getSelectedMetadataItem());
    if (!checkValidDatabase(db))
        return;
    if (!tryAutoConnectDatabase(db))
        return;

    EventWatcherFrame* ewf = EventWatcherFrame::findFrameFor(db);
    if (ewf)
    {
        ewf->Raise();
        return;
    }
    ewf = new EventWatcherFrame(this, db);
    ewf->Show();
}

void MainFrame::OnMenuBackup(wxCommandEvent& WXUNUSED(event))
{
    DatabasePtr db = getDatabase(treeMainM->getSelectedMetadataItem());
    if (!checkValidDatabase(db))
        return;

    BackupFrame* bf = BackupFrame::findFrameFor(db);
    if (bf)
    {
        bf->Raise();
        return;
    }
    bf = new BackupFrame(this, db);
    bf->Show();
}

void MainFrame::OnMenuRestore(wxCommandEvent& WXUNUSED(event))
{
    DatabasePtr db = getDatabase(treeMainM->getSelectedMetadataItem());
    if (!checkValidDatabase(db))
        return;

    RestoreFrame* rf = RestoreFrame::findFrameFor(db);
    if (rf)
    {
        rf->Raise();
        return;
    }
    rf = new RestoreFrame(this, db);
    rf->Show();
}

void MainFrame::OnMenuReconnect(wxCommandEvent& WXUNUSED(event))
{
    DatabasePtr db = getDatabase(treeMainM->getSelectedMetadataItem());
    if (!checkValidDatabase(db))
        return;

    wxBusyCursor bc;
    db->reconnect();
}

void MainFrame::OnMenuConnectAs(wxCommandEvent& WXUNUSED(event))
{
    DatabasePtr db = getDatabase(treeMainM->getSelectedMetadataItem());
    if (!checkValidDatabase(db))
        return;
    // command should never be enabled when database is connected
    wxCHECK_RET(!db->isConnected(),
        "Can not connect to already connected database");

    DatabaseRegistrationDialog drd(this, _("Connect as..."), false, true);
    db->prepareTemporaryCredentials();
    drd.setDatabase(db);
    if (wxID_OK != drd.ShowModal() || !connect())
        db->resetCredentials();
}

void MainFrame::OnMenuConnect(wxCommandEvent& WXUNUSED(event))
{
    connect();
}

bool MainFrame::getAutoConnectDatabase()
{
    int value;
    if (config().getValue("DIALOG_ConfirmAutoConnect", value))
        return value == wxYES;
    // enable all commands to show the dialog when connection is needed
    return true;
}

bool MainFrame::tryAutoConnectDatabase()
{
    DatabasePtr db = getDatabase(treeMainM->getSelectedMetadataItem());
    return checkValidDatabase(db) && tryAutoConnectDatabase(db);
}

bool MainFrame::tryAutoConnectDatabase(DatabasePtr database)
{
    if (database->isConnected())
        return true;

    int res = showQuestionDialog(this, _("Do you want to connect to the database?"),
        _("The database is not connected. You first have to establish a connection before you can execute SQL statements or otherwise work with the database."),
        AdvancedMessageDialogButtonsYesNoCancel(_("C&onnect"), _("Do&n't connect")),
        config(), "DIALOG_ConfirmAutoConnect", _("Don't ask again, &always (don't) connect"));
    if (res == wxYES)
    {
        connect();
        updateStatusbarText();
    }
    return database->isConnected();
}

bool MainFrame::connect()
{
    DatabasePtr db = getDatabase(treeMainM->getSelectedMetadataItem());
    if (!checkValidDatabase(db))
        return false;
    if (db->isConnected() || !connectDatabase(db.get(), this))
        return false;
    if (db->isConnected())
    {
        if (db->usesDifferentConnectionCharset())
        {
            DatabaseConfig dc(db.get(), config());
            if (dc.get("differentCharsetWarning", true))
            {
                if (wxNO == wxMessageBox(wxString::Format(
                    _("Database charset: %s\nis different from connection charset: %s.\n\nWould you like to be reminded next time?"),
                    db->getDatabaseCharset().c_str(),
                    db->getConnectionCharset().c_str()),
                    _("Warning"),
                    wxICON_QUESTION | wxYES_NO))
                {
                    dc.setValue("differentCharsetWarning", false);
                }
            }
        }
        treeMainM->Expand(treeMainM->GetSelection());
    }

    updateStatusbarText();
    Raise();
    Update();
    return true;
}

void MainFrame::OnMenuDisconnect(wxCommandEvent& WXUNUSED(event))
{
    DatabasePtr db = getDatabase(treeMainM->getSelectedMetadataItem());
    if (!checkValidDatabase(db))
        return;

    // give SQL editor windows with active transactions a chance to commit
    // or even cancel this action
    std::vector<BaseFrame*> frames(BaseFrame::getFrames());
    for (std::vector<BaseFrame*>::iterator it = frames.begin();
        it != frames.end(); it++)
    {
        ExecuteSqlFrame* esf = dynamic_cast<ExecuteSqlFrame*>(*it);
        if (esf && esf->getDatabase() == db.get() && !esf->canClose())
            return;
    }

    treeMainM->Freeze();
    try
    {
        db->disconnect();
    }
    catch (...)
    {
    }

    wxSafeYield();
    treeMainM->Thaw();
    updateStatusbarText();
}

void MainFrame::showGeneratorValue(Generator* g)
{
    if (g)
    {
        // make sure value is reloaded from database
        g->invalidate();
        g->ensurePropertiesLoaded();
    }
}

void MainFrame::OnMenuShowGeneratorValue(wxCommandEvent& WXUNUSED(event))
{
    showGeneratorValue(dynamic_cast<Generator*>(treeMainM->getSelectedMetadataItem()));
}

void MainFrame::OnMenuSetGeneratorValue(wxCommandEvent& WXUNUSED(event))
{
    Generator* g = dynamic_cast<Generator*>(treeMainM->getSelectedMetadataItem());
    if (!g)
        return;

    URI uri("fr://edit_generator_value");
    uri.addParam(wxString::Format("parent_window=%p", this));
    uri.addParam(wxString::Format("object_handle=%lu", g->getHandle()));
    getURIProcessor().handleURI(uri);
}

void MainFrame::OnMenuShowAllGeneratorValues(wxCommandEvent& WXUNUSED(event))
{
    DatabasePtr db = getDatabase(treeMainM->getSelectedMetadataItem());
    if (checkValidDatabase(db))
        db->loadGeneratorValues();
}

void MainFrame::OnMenuCreateDomain(wxCommandEvent& WXUNUSED(event))
{
    showCreateTemplate(
        MetadataItemCreateStatementVisitor::getCreateDomainStatement());
}

void MainFrame::OnMenuCreateException(wxCommandEvent& WXUNUSED(event))
{
    showCreateTemplate(
        MetadataItemCreateStatementVisitor::getCreateExceptionStatement());
}

void MainFrame::OnMenuCreateFunction(wxCommandEvent& WXUNUSED(event))
{
    showCreateTemplate(
        MetadataItemCreateStatementVisitor::getCreateFunctionSQLStatement());
}

void MainFrame::OnMenuCreateIndex(wxCommandEvent& WXUNUSED(event))
{
    showCreateTemplate(
        MetadataItemCreateStatementVisitor::getCreateIndexStatement());
}

void MainFrame::OnMenuCreateGenerator(wxCommandEvent& WXUNUSED(event))
{
    showCreateTemplate(
        MetadataItemCreateStatementVisitor::getCreateGeneratorStatement());
}

void MainFrame::OnMenuCreatePackage(wxCommandEvent& WXUNUSED(event))
{
    showCreateTemplate(
        MetadataItemCreateStatementVisitor::getCreatePackageStatement());
}

void MainFrame::OnMenuCreateProcedure(wxCommandEvent& WXUNUSED(event))
{
    showCreateTemplate(
        MetadataItemCreateStatementVisitor::getCreateProcedureStatement());
}

void MainFrame::OnMenuCreateRole(wxCommandEvent& WXUNUSED(event))
{
    showCreateTemplate(
        MetadataItemCreateStatementVisitor::getCreateRoleStatement());
}

void MainFrame::OnMenuCreateTable(wxCommandEvent& WXUNUSED(event))
{
    showCreateTemplate(
        MetadataItemCreateStatementVisitor::getCreateTableStatement());
}

void MainFrame::OnMenuCreateGTTTable(wxCommandEvent& WXUNUSED(event))
{
    showCreateTemplate(
        MetadataItemCreateStatementVisitor::getCreateGTTTableStatement());
}

void MainFrame::OnMenuCreateDMLTrigger(wxCommandEvent& WXUNUSED(event))
{
    showCreateTemplate(
        MetadataItemCreateStatementVisitor::getCreateDMLTriggerStatement());
}

void MainFrame::OnMenuCreateDBTrigger(wxCommandEvent& WXUNUSED(event))
{
    showCreateTemplate(
        MetadataItemCreateStatementVisitor::getCreateDBTriggerStatement());
}

void MainFrame::OnMenuCreateDDLTrigger(wxCommandEvent& WXUNUSED(event))
{
    showCreateTemplate(
        MetadataItemCreateStatementVisitor::getCreateDDLTriggerStatement());
}

void MainFrame::OnMenuCreateUDF(wxCommandEvent& WXUNUSED(event))
{
    showCreateTemplate(
        MetadataItemCreateStatementVisitor::getCreateUDFStatement());
}

void MainFrame::OnMenuCreateUser(wxCommandEvent& WXUNUSED(event))
{
    showCreateTemplate(
        MetadataItemCreateStatementVisitor::getCreateUserStatement());
}

void MainFrame::OnMenuCreateView(wxCommandEvent& WXUNUSED(event))
{
    showCreateTemplate(
        MetadataItemCreateStatementVisitor::getCreateViewStatement());
}

void MainFrame::OnMenuCreateObject(wxCommandEvent& WXUNUSED(event))
{
    MetadataItem* item = treeMainM->getSelectedMetadataItem();
    if (!item)
        return;

    MetadataItemCreateStatementVisitor csv;
    item->acceptVisitor(&csv);
    showCreateTemplate(csv.getStatement());
}

void MainFrame::showCreateTemplate(const wxString& statement)
{
    // TODO: add a call for wizards. For example, we can have NewTableWizard which is a frame with grid in which
    // user can enter column data for new table (name, datatype, null option, collation, default, etc.) and also
    // enter a name for new table, etc. Wizard should return a bunch of DDL statements as a wxString which would we
    // pass to ExecSqlFrame.

    if (statement == wxEmptyString)
    {
        wxMessageBox(_("The feature is not yet available for this type of database objects."),
            _("Not yet implemented"), wxOK | wxICON_INFORMATION);
        return;
    }

    DatabasePtr db = getDatabase(treeMainM->getSelectedMetadataItem());
    if (!checkValidDatabase(db))
        return;
    if (!tryAutoConnectDatabase(db))
        return;

    showSql(this, wxEmptyString, db, statement);
}

void MainFrame::OnMenuCreateCollation(wxCommandEvent& WXUNUSED(event))
{
    showCreateTemplate(
        MetadataItemCreateStatementVisitor::getCreateCollationStatment());
}

void MainFrame::OnMenuAddColumn(wxCommandEvent& WXUNUSED(event))
{
    Table* t = dynamic_cast<Table*>(treeMainM->getSelectedMetadataItem());
    if (!t)
        return;

    URI uri("fr://add_field");
    uri.addParam(wxString::Format("parent_window=%p",this));
    uri.addParam(wxString::Format("object_handle=%lu", t->getHandle()));
    getURIProcessor().handleURI(uri);
}

void MainFrame::OnMenuToggleDisconnected(wxCommandEvent& event)
{
    config().setValue("HideDisconnectedDatabases", !event.IsChecked());
    // no need to call notifyAllServers() - DBH tree nodes observe the global
    // config objects themselves
}

void MainFrame::OnMenuToggleStatusBar(wxCommandEvent& event)
{
    wxStatusBar* s = GetStatusBar();
    if (!s)
        s = CreateStatusBar();

    bool show = event.IsChecked();
    config().setValue("showStatusBar", show);
    s->Show(show);
    SendSizeEvent();
}

void MainFrame::OnMenuToggleSearchBar(wxCommandEvent& event)
{
    bool show = event.IsChecked();
    config().setValue("showSearchBar", show);
    searchPanelSizerM->Show(searchPanelM, show, true);    // recursive
    searchPanelSizerM->Layout();
}

void MainFrame::OnSearchTextChange(wxCommandEvent& WXUNUSED(event))
{
    if (treeMainM->findText(searchBoxM->GetValue()))
        searchBoxM->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    else
        searchBoxM->SetForegroundColour(*wxRED);
    wxStatusBar *sb = GetStatusBar();
    if (sb)
        sb->SetStatusText(_("Hit ENTER to focus the tree."));
}

void MainFrame::OnSearchBoxEnter(wxCommandEvent& WXUNUSED(event))
{
    wxString text = searchBoxM->GetValue();
    if (text.IsEmpty())
        return;
    // if it's a wildcard, add wildcard to the list...
    if (searchBoxM->GetForegroundColour() == *wxRED)
        searchBoxM->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    else
    {
        if (text.Find(wxChar('*')) != -1 || text.Find(wxChar('?')) != -1)
            searchBoxM->Append(text);
        else    // ...otherwise, add item's name to the list
            searchBoxM->Append(treeMainM->GetItemText(treeMainM->GetSelection()));
    }
    searchBoxM->SetValue(wxEmptyString);
    treeMainM->SetFocus();
    wxStatusBar *sb = GetStatusBar();
    if (sb)
        sb->SetStatusText(_("Item added to the list."));
}

void MainFrame::OnButtonSearchClick(wxCommandEvent& WXUNUSED(event))
{
    AdvancedSearchFrame *asf = new AdvancedSearchFrame(this, rootM);
    asf->Show();
}

void MainFrame::OnButtonPrevClick(wxCommandEvent& WXUNUSED(event))
{
    // move backward and search then
    wxTreeItemId id = treeMainM->GetSelection();
    if (id.IsOk())
    {
        treeMainM->SelectItem(treeMainM->getPreviousItem(id));
        treeMainM->findText(searchBoxM->GetValue(),false);
        if (id == treeMainM->GetSelection())
        {
            wxStatusBar *sb = GetStatusBar();
            if (sb)
                sb->SetStatusText(_("No more matches."));
        }
    }
}

void MainFrame::OnButtonNextClick(wxCommandEvent& WXUNUSED(event))
{
    // move forward and search then
    wxTreeItemId id = treeMainM->GetSelection();
    if (id.IsOk())
    {
        treeMainM->SelectItem(treeMainM->getNextItem(id));
        treeMainM->findText(searchBoxM->GetValue(), true);
        if (id == treeMainM->GetSelection())
        {
            wxStatusBar *sb = GetStatusBar();
            if (sb)
                sb->SetStatusText(_("No more matches."));
        }
    }
}

void MainFrame::OnMenuObjectProperties(wxCommandEvent& WXUNUSED(event))
{
    MetadataItem* m = treeMainM->getSelectedMetadataItem();
    if (!m)
        return;
    if (!tryAutoConnectDatabase())
        return;

    Column* c = dynamic_cast<Column*>(m);
    if (c)
    {
        // Return when we're dealing with a system column
        if (c->isSystem() || !c->getTable())
            return;

        URI uri("fr://edit_field");
        uri.addParam(wxString::Format("parent_window=%p", this));
        uri.addParam(wxString::Format("object_handle=%lu", c->getHandle()));

        getURIProcessor().handleURI(uri);
    }
    else
        MetadataItemPropertiesFrame::showPropertyPage(m);
}

void MainFrame::OnMenuObjectRefresh(wxCommandEvent& WXUNUSED(event))
{
    if (MetadataItem* mi = treeMainM->getSelectedMetadataItem())
    {
        if (!tryAutoConnectDatabase())
            return;

        // make sure notifyObservers() is called only once
        SubjectLocker lock(mi);

        mi->invalidate();
        mi->invalidateDescription();
        mi->notifyObservers();
    }
}

void MainFrame::OnMenuAlterObject(wxCommandEvent& WXUNUSED(event))
{
    MetadataItem* mi = treeMainM->getSelectedMetadataItem();
    Procedure* p = dynamic_cast<Procedure*>(mi);
    if (p)
    {
        URI uri("fr://edit_procedure");
        uri.addParam(wxString::Format("parent_window=%p", this));
        uri.addParam(wxString::Format("object_handle=%lu", p->getHandle()));
        getURIProcessor().handleURI(uri);
        return;
    }

    DatabasePtr db = getDatabase(mi);
    if (!db)
        return;

    wxString sql;
    if (Collation* c = dynamic_cast<Collation*>(mi))
        sql = c->getAlterSql();
    else if (View* v = dynamic_cast<View*>(mi))
        sql = v->getAlterSql();
        //        sql = v->getRebuildSql();
    else if (Trigger* t = dynamic_cast<Trigger*>(mi))
        sql = t->getAlterSql();
    else if (Domain* dm = dynamic_cast<Domain*>(mi))
        sql = dm->getAlterSqlTemplate();
    else if (Package* pk = dynamic_cast<Package*>(mi))
        sql = pk->getAlterSql();
    else if (FunctionSQL* fn = dynamic_cast<FunctionSQL*>(mi))
        sql = fn->getAlterSql();

    if (!sql.empty())
        showSql(this, wxString(_("Alter object")), db, sql);
}

void MainFrame::OnMenRebuildObject(wxCommandEvent& WXUNUSED(event))
{
    MetadataItem* mi = treeMainM->getSelectedMetadataItem();
    DatabasePtr db = getDatabase(mi);
    if (!db)
        return;

    wxString sql;
    if (View* v = dynamic_cast<View*>(mi))
        sql = v->getRebuildSql();
    if (!sql.empty())
        showSql(this, wxString(_("Rebuild object")), db, sql);
}

void MainFrame::OnMenuRecreateDatabase(wxCommandEvent& WXUNUSED(event))
{
    DatabasePtr db = getDatabase(treeMainM->getSelectedMetadataItem());
    if (!checkValidDatabase(db))
        return;

    wxString msg(wxString::Format(
        _("Are you sure you wish to recreate the database \"%s\"?"),
        db->getName_().c_str()));
    wxString secondary;
    if (!db->isConnected())
        secondary = _("First a connection to the database will be established. You may need to enter the password.\n");
    secondary += _("The database will be dropped, and a new empty database will be created. All the data will be deleted, this is an irreversible action!");
    if (wxOK == showQuestionDialog(this, msg, secondary,
        AdvancedMessageDialogButtonsOkCancel(_("Recreate"))))
    {
        // it's unclear at this point whether the database does still exist
        // try to connect to it, and if that succeeds then drop it
        // ignore all errors along the way...
        try
        {
            if (!db->isConnected())
                connect();
            if (db->isConnected())
                db->drop();
        }
        catch(IBPP::Exception&) {}

        // use the dialog as some information (charset and page size) is
        // not necessarily available, and the user may want to change it too
        DatabaseRegistrationDialog drd(this, _("Recreate Database"), true);
        drd.setDatabase(db);
        if (drd.ShowModal() == wxID_OK)
            treeMainM->selectMetadataItem(db.get());
    }
}

void MainFrame::OnMenuDropDatabase(wxCommandEvent& WXUNUSED(event))
{
    DatabasePtr db = getDatabase(treeMainM->getSelectedMetadataItem());
    if (!checkValidDatabase(db) || !tryAutoConnectDatabase(db))
        return;
    if (!confirmDropDatabase(db.get()))
        return;

    int result = wxMessageBox(
        _("Do you wish to keep the registration info?"),
        _("Dropping database: ") + db->getName_(),
        wxYES_NO | wxCANCEL | wxICON_ASTERISK);
    if (result == wxCANCEL)
        return;
    db->drop();
    if (result == wxNO)
        unregisterDatabase(db);
}

void MainFrame::OnMenuDropObject(wxCommandEvent& WXUNUSED(event))
{
    MetadataItem* mi = treeMainM->getSelectedMetadataItem();
    if (!mi)
        return;
    DatabasePtr db = getDatabase(mi);
    if (!checkValidDatabase(db))
        return;
    if (!confirmDropItem(mi))
        return;

    // TODO: We could first check if there are some dependant objects,
    //       and offer the user to either drop dependencies, or drop those
    //       objects too.
    //       Then we should create a bunch of sql statements that do it.
    wxString stmt(mi->getDropSqlStatement());
    if (!stmt.empty())
        execSql(this, wxEmptyString, db, stmt, true);
}

//! create new ExecSqlFrame and attach database object to it
void MainFrame::OnMenuExecuteStatements(wxCommandEvent& WXUNUSED(event))
{
    DatabasePtr db = getDatabase(treeMainM->getSelectedMetadataItem());
    if (!checkValidDatabase(db))
        return;
    if (!tryAutoConnectDatabase(db))
        return;

    showSql(this, wxString(_("Execute SQL statements")), db, wxEmptyString);
}

const wxString MainFrame::getName() const
{
    return "MainFrame";
}

void MainFrame::OnMenuUpdateUnRegisterServer(wxUpdateUIEvent& event)
{
    ServerPtr s = getServer(treeMainM->getSelectedMetadataItem());
    event.Enable(s != 0 && !s->hasConnectedDatabase());
}

void MainFrame::OnMenuUpdateIfServerSelected(wxUpdateUIEvent& event)
{
    ServerPtr s = getServer(treeMainM->getSelectedMetadataItem());
    event.Enable(s != 0);
}

void MainFrame::OnMenuUpdateIfDatabaseConnected(wxUpdateUIEvent& event)
{
    DatabasePtr d = getDatabase(treeMainM->getSelectedMetadataItem());
    event.Enable(d != 0 && d->isConnected());
}

void MainFrame::OnMenuUpdateIfDatabaseConnectedOrAutoConnect(
    wxUpdateUIEvent& event)
{
    DatabasePtr d = getDatabase(treeMainM->getSelectedMetadataItem());
    event.Enable(d != 0 && (d->isConnected() || getAutoConnectDatabase()));
}

void MainFrame::OnMenuUpdateIfDatabaseNotConnected(wxUpdateUIEvent& event)
{
    DatabasePtr d = getDatabase(treeMainM->getSelectedMetadataItem());
    event.Enable(d != 0 && !d->isConnected());
}

void MainFrame::OnMenuUpdateIfDatabaseSelected(wxUpdateUIEvent& event)
{
    DatabasePtr d = getDatabase(treeMainM->getSelectedMetadataItem());
    event.Enable(d != 0);
}

void MainFrame::OnMenuUpdateIfMetadataItemHasChildren(wxUpdateUIEvent& event)
{
    MetadataItem* mi = treeMainM->getSelectedMetadataItem();
    event.Enable(mi != 0 && mi->getChildrenCount());
}

bool MainFrame::confirmDropItem(MetadataItem* item)
{
    wxString msg(wxString::Format(
        _("Are you sure you wish to drop the %s %s?"),
        item->getTypeName().Lower().c_str(),
        item->getName_().c_str()));
    return wxOK == showQuestionDialog(this, msg,
        _("Once you drop the object it is permanently removed from database."),
        AdvancedMessageDialogButtonsOkCancel(_("&Drop")),
        config(), "DIALOG_ConfirmDrop", _("Always drop without asking"));
}

bool MainFrame::confirmDropDatabase(Database* db)
{
    wxString msg(wxString::Format(
        _("Are you sure you wish to drop database %s?"),
        db->getName_().c_str()));
    return wxOK == showQuestionDialog(this, msg,
        _("Once you drop the database, all data is lost."),
        AdvancedMessageDialogButtonsOkCancel(_("&Drop")));
}

bool MainFrame::openUnregisteredDatabase(const wxString& dbpath)
{
    DatabasePtr database(new Database());
    database->setPath(dbpath);
    database->setName_(Database::extractNameFromConnectionString(dbpath));

    wxString iscUser, iscPassword;
    if (!wxGetEnv("ISC_USER", &iscUser))
        iscUser = "SYSDBA";
    database->setUsername(iscUser);
    if (!wxGetEnv("ISC_PASSWORD", &iscPassword))
        iscPassword = wxEmptyString;
    database->setRawPassword(iscPassword);

    DatabaseRegistrationDialog drd(this, _("Database Connection Settings"));
    drd.setDatabase(database);
    if (drd.ShowModal() == wxID_OK)
    {
        rootM->addUnregisteredDatabase(database);
        treeMainM->selectMetadataItem(database.get());
        if (connectDatabase(database.get(), this))
            return true;
    }
    return false;
}

void MainFrame::OnSetFocus(wxFocusEvent& event)
{
    // fix an annoying bug where closing a MetadataItemPropertyFrame does
    // focus the main frame instead of its previously focused control
    mainPanelM->SetFocus();
    event.Skip();
}

void MainFrame::executeSysTemplate(const wxString& name, MetadataItem* item,
    wxWindow* parentWindow)
{
    DatabasePtr database = getDatabase(item);
    if (!checkValidDatabase(database))
        return;
    if (!tryAutoConnectDatabase(database))
        return;

    wxString code;
    ProgressDialog pd(parentWindow, _("Processing template..."));
    CodeTemplateProcessor tp(item, parentWindow);
    tp.processTemplateFile(code, config().getSysTemplateFileName(name),
        item, &pd);
    handleTemplateOutput(tp, database, code);
}

void MainFrame::executeCodeTemplate(const wxFileName& fileName,
    MetadataItem* item, DatabasePtr database)
{
    wxString code;
    ProgressDialog pd(this, _("Processing template..."));
    CodeTemplateProcessor tp(item, this);
    tp.processTemplateFile(code, fileName, item, &pd);
    handleTemplateOutput(tp, database, code);
}

void MainFrame::handleTemplateOutput(TemplateProcessor& tp,
    DatabasePtr database, const wxString& code)
{
    if (getStringAsBoolean(tp.getVar("output.autoexec")))
        execSql(this, wxString(_("Execute SQL statements")),
            database, code, false);
    else
        showSql(this, wxString(_("Execute SQL statements")),
            database, code);
}

bool MainFrame::handleURI(URI& uri)
{
    if (uri.action == "create_trigger")
    {
        Relation* r = extractMetadataItemFromURI<Relation>(uri);
        wxWindow* w = getParentWindow(uri);
        if (!r || !w)
            return true;
        executeSysTemplate("create_trigger", r, w);
        return true;
    }
    else
        return false;
}

