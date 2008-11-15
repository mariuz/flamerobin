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

#include <wx/clipbrd.h>
#include <wx/dnd.h>
#include <wx/tokenzr.h>

#include "config/Config.h"
#include "config/DatabaseConfig.h"
#include "core/ArtProvider.h"
#include "core/FRError.h"
#include "core/StringUtils.h"
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
#include "gui/MainFrame.h"
#include "gui/MetadataItemPropertiesFrame.h"
#include "gui/PreferencesDialog.h"
#include "gui/ProgressDialog.h"
#include "gui/RestoreFrame.h"
#include "gui/ServerRegistrationDialog.h"
#include "gui/SimpleHtmlFrame.h"
#include "frtypes.h"
#include "main.h"
#include "metadata/metadataitem.h"
#include "metadata/root.h"
#include "frutils.h"
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
//-----------------------------------------------------------------------------
MainFrame::MainFrame(wxWindow* parent, int id, const wxString& title,
        const wxPoint& pos, const wxSize& size, long style)
    : BaseFrame(parent, id, title, pos, size, style, wxT("FlameRobin_main"))
{
    wxArtProvider::Push(new ArtProvider);

    mainPanelM = new wxPanel(this);
    treeMainM = new DBHTreeControl(mainPanelM, wxDefaultPosition,
        wxDefaultSize,
#if defined __WXGTK20__ || defined __WXMAC__
        wxTR_NO_LINES |
#endif
        wxTR_HAS_BUTTONS | wxSUNKEN_BORDER);

    wxArrayString choices;  // load from config?

    searchPanelM = new wxPanel(mainPanelM, -1, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL | wxSUNKEN_BORDER);
    searchBoxM = new wxComboBox(searchPanelM, ID_search_box, wxEmptyString,
        wxDefaultPosition, wxDefaultSize, choices,
        wxCB_DROPDOWN | wxCB_SORT | wxTE_PROCESS_ENTER);
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
    if (!config().get(wxT("showSearchBar"), true))
    {
        searchPanelSizerM->Show(searchPanelM, false, true);    // recursive
        searchPanelSizerM->Layout();
    }
}
//-----------------------------------------------------------------------------
void MainFrame::buildMainMenu()
{
    menuBarM = new wxMenuBar();

    wxMenu* databaseMenu = new wxMenu();                    // dynamic menus, created at runtime
    databaseMenu->Append(Cmds::Menu_RegisterDatabase, _("R&egister existing database..."));
    databaseMenu->Append(Cmds::Menu_CreateDatabase, _("Create &new database..."));
    databaseMenu->Append(Cmds::Menu_RestoreIntoNew, _("Restore bac&kup into new database..."));
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
    serverMenu->Append(Cmds::Menu_ServerProperties, _("Server registration &info..."));
    serverMenu->AppendSeparator();
    serverMenu->Append(Cmds::Menu_GetServerVersion, _("Retrieve server &version"));
    serverMenu->Append(Cmds::Menu_ManageUsers, _("&Manage users..."));
    menuBarM->Append(serverMenu, _("&Server"));

    objectMenuM = new wxMenu();
    wxMenu* newMenu = new wxMenu();
    newMenu->Append(Cmds::Menu_CreateDomain,      _("&Domain"));
    newMenu->Append(Cmds::Menu_CreateException,   _("&Exception"));
    newMenu->Append(Cmds::Menu_CreateFunction,    _("&Function"));
    newMenu->Append(Cmds::Menu_CreateGenerator,   _("&Generator"));
    newMenu->Append(Cmds::Menu_CreateProcedure,   _("&Procedure"));
    newMenu->Append(Cmds::Menu_CreateRole,        _("&Role"));
    newMenu->Append(Cmds::Menu_CreateTable,       _("&Table"));
    newMenu->Append(Cmds::Menu_CreateTrigger,     _("Tr&igger"));
    newMenu->Append(Cmds::Menu_CreateView,        _("&View"));
    objectMenuM->Append(Cmds::Menu_NewObject, _("&New"), newMenu);
    menuBarM->Append(objectMenuM, _("&Object"));

    wxMenu* helpMenu = new wxMenu();
    helpMenu->Append(Cmds::Menu_Manual, _("&Manual"));
    helpMenu->Append(Cmds::Menu_RelNotes, _("&What's new"));
    helpMenu->Append(Cmds::Menu_License, _("&License"));
    helpMenu->AppendSeparator();
    helpMenu->Append(Cmds::Menu_URLHomePage, _("FlameRobin &home page"));
    helpMenu->Append(Cmds::Menu_URLProjectPage, _("SourceForge &project page"));
    helpMenu->Append(Cmds::Menu_URLFeatureRequest, _("SourceForge &feature requests"));
    helpMenu->Append(Cmds::Menu_URLBugReport, _("SourceForge &bug reports"));
#ifndef __WXMAC__
    helpMenu->AppendSeparator();
#endif
    helpMenu->Append(wxID_ABOUT, _("&About"));
    menuBarM->Append(helpMenu, _("&Help"));
    SetMenuBar(menuBarM);

    // update checkboxes
    config().setValue(wxT("HideDisconnectedDatabases"), false);
    viewMenu->Check(Cmds::Menu_ToggleDisconnected, true);
    if (config().get(wxT("showStatusBar"), true))
    {
        CreateStatusBar();
        viewMenu->Check(Cmds::Menu_ToggleStatusBar, true);
        GetStatusBar()->SetStatusText(_("[No database selected]"));
    }
    if (config().get(wxT("showSearchBar"), true))
        viewMenu->Check(Cmds::Menu_ToggleSearchBar, true);
}
//-----------------------------------------------------------------------------
void MainFrame::showDocsHtmlFile(const wxString& fileName)
{
    wxFileName fullFileName(config().getDocsPath(), fileName);
    showHtmlFile(this, fullFileName);
}
//-----------------------------------------------------------------------------
void MainFrame::showUrl(const wxString& url)
{
    if (!wxLaunchDefaultBrowser(url))
        wxLogError(_T("Failed to open URL \"%s\""), url.c_str());
}
//-----------------------------------------------------------------------------
void MainFrame::set_properties()
{
    SetTitle(_("FlameRobin Database Admin"));

    // Default (generic) tree looks pretty ugly on GTK 1
#if defined(__WXGTK__) && !defined(__WXGTK20__)
    treeMainM->SetIndent(12);
#endif

    wxTreeItemId rootNode = treeMainM->addRootNode(&getGlobalRoot());
    getGlobalRoot().load();
    if (treeMainM->GetCount() <= 1)
    {
        wxString confile = config().getDBHFileName();
        if (confile.Length() > 20)
            confile = wxT("\n") + confile + wxT("\n");  // break into several lines if path is long
        else
            confile = wxT(" ") + confile + wxT(" ");
        wxString msg;
        msg.Printf(_("The configuration file:%sdoes not exist or can not be opened.\n\nThis is normal for first time users.\n\nYou may now register new servers and databases."),
            confile.c_str());
        wxMessageBox(msg, _("Configuration file not found"), wxOK|wxICON_INFORMATION);

        Server s;
        s.setName_(wxT("Localhost"));
        s.setHostname(wxT("localhost"));
        getGlobalRoot().addServer(s);
    }
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
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
const wxRect MainFrame::getDefaultRect() const
{
    return wxRect(-1, -1, 360, 480);
}
//-----------------------------------------------------------------------------
DBHTreeControl* MainFrame::getTreeCtrl()
{
    return treeMainM;
}
//-----------------------------------------------------------------------------
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
    EVT_MENU(Cmds::Menu_ShowConnectedUsers, MainFrame::OnMenuShowConnectedUsers)
    EVT_UPDATE_UI(Cmds::Menu_ShowConnectedUsers, MainFrame::OnMenuUpdateIfDatabaseConnected)
    EVT_MENU(Cmds::Menu_GetServerVersion, MainFrame::OnMenuGetServerVersion)
    EVT_UPDATE_UI(Cmds::Menu_GetServerVersion, MainFrame::OnMenuUpdateIfServerSelected)
    EVT_MENU(Cmds::Menu_MonitorEvents, MainFrame::OnMenuMonitorEvents)
    EVT_UPDATE_UI(Cmds::Menu_MonitorEvents, MainFrame::OnMenuUpdateIfDatabaseConnected)
    EVT_MENU(Cmds::Menu_GenerateData, MainFrame::OnMenuGenerateData)
    EVT_UPDATE_UI(Cmds::Menu_GenerateData, MainFrame::OnMenuUpdateIfDatabaseConnected)
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
    EVT_MENU(Cmds::Menu_DropDatabase, MainFrame::OnMenuDropDatabase)
    EVT_UPDATE_UI(Cmds::Menu_DropDatabase, MainFrame::OnMenuUpdateIfDatabaseConnected)
    EVT_MENU(Cmds::Menu_Query, MainFrame::OnMenuQuery)
    EVT_UPDATE_UI(Cmds::Menu_Query, MainFrame::OnMenuUpdateIfDatabaseSelected)
    EVT_UPDATE_UI(Cmds::Menu_NewObject, MainFrame::OnMenuUpdateIfDatabaseConnected)
    EVT_MENU(Cmds::Menu_DatabasePreferences, MainFrame::OnMenuDatabasePreferences)
    EVT_UPDATE_UI(Cmds::Menu_DatabasePreferences, MainFrame::OnMenuUpdateIfDatabaseSelected)
    EVT_MENU(Cmds::Menu_DatabaseProperties, MainFrame::OnMenuDatabaseProperties)
    EVT_UPDATE_UI(Cmds::Menu_DatabaseProperties, MainFrame::OnMenuUpdateIfDatabaseConnected)
    EVT_MENU(Cmds::Menu_ExtractDatabaseDDL, MainFrame::OnMenuDatabaseExtractDDL)
    EVT_UPDATE_UI(Cmds::Menu_ExtractDatabaseDDL, MainFrame::OnMenuUpdateIfDatabaseConnected)

    EVT_MENU(Cmds::Menu_Insert, MainFrame::OnMenuInsert)
    EVT_MENU(Cmds::Menu_BrowseColumns, MainFrame::OnMenuBrowseColumns)
    EVT_MENU(Cmds::Menu_LoadColumnsInfo, MainFrame::OnMenuLoadColumnsInfo)
    EVT_MENU(Cmds::Menu_AddColumn, MainFrame::OnMenuAddColumn)
    EVT_MENU(Cmds::Menu_CreateTriggerForTable, MainFrame::OnMenuCreateTriggerForTable)
    EVT_MENU(Cmds::Menu_CreateProcedureForTable, MainFrame::OnMenuCreateProcedureForTable)
    EVT_MENU(Cmds::Menu_ExecuteProcedure, MainFrame::OnMenuExecuteProcedure)

    EVT_MENU(Cmds::Menu_ShowAllGeneratorValues, MainFrame::OnMenuShowAllGeneratorValues)
    EVT_UPDATE_UI(Cmds::Menu_ShowAllGeneratorValues, MainFrame::OnMenuUpdateIfMetadataItemHasChildren)
    EVT_MENU(Cmds::Menu_ShowGeneratorValue, MainFrame::OnMenuShowGeneratorValue)
    EVT_MENU(Cmds::Menu_SetGeneratorValue, MainFrame::OnMenuSetGeneratorValue)

    EVT_MENU(Cmds::Menu_CreateObject, MainFrame::OnMenuCreateObject)
    EVT_MENU(Cmds::Menu_AlterObject, MainFrame::OnMenuAlterObject)
    EVT_MENU(Cmds::Menu_DropObject, MainFrame::OnMenuDropObject)
    EVT_MENU(Cmds::Menu_ObjectProperties, MainFrame::OnMenuObjectProperties)

    EVT_MENU(Cmds::Menu_ToggleStatusBar, MainFrame::OnMenuToggleStatusBar)
    EVT_MENU(Cmds::Menu_ToggleSearchBar, MainFrame::OnMenuToggleSearchBar)
    EVT_MENU(Cmds::Menu_ToggleDisconnected, MainFrame::OnMenuToggleDisconnected)

    EVT_TEXT_ENTER(MainFrame::ID_search_box, MainFrame::OnSearchBoxEnter)
    EVT_TEXT(MainFrame::ID_search_box, MainFrame::OnSearchTextChange)
    EVT_BUTTON(MainFrame::ID_button_advanced, MainFrame::OnButtonSearchClick)
    EVT_BUTTON(MainFrame::ID_button_prev, MainFrame::OnButtonPrevClick)
    EVT_BUTTON(MainFrame::ID_button_next, MainFrame::OnButtonNextClick)

    EVT_MENU(Cmds::Menu_CreateDomain,     MainFrame::OnMenuCreateDomain)
    EVT_MENU(Cmds::Menu_CreateException,  MainFrame::OnMenuCreateException)
    EVT_MENU(Cmds::Menu_CreateFunction,   MainFrame::OnMenuCreateFunction)
    EVT_MENU(Cmds::Menu_CreateGenerator,  MainFrame::OnMenuCreateGenerator)
    EVT_MENU(Cmds::Menu_CreateProcedure,  MainFrame::OnMenuCreateProcedure)
    EVT_MENU(Cmds::Menu_CreateRole,       MainFrame::OnMenuCreateRole)
    EVT_MENU(Cmds::Menu_CreateTable,      MainFrame::OnMenuCreateTable)
    EVT_MENU(Cmds::Menu_CreateTrigger,    MainFrame::OnMenuCreateTrigger)
    EVT_MENU(Cmds::Menu_CreateView,       MainFrame::OnMenuCreateView)

    EVT_MENU_OPEN(MainFrame::OnMainMenuOpen)
    EVT_TREE_SEL_CHANGED(DBHTreeControl::ID_tree_ctrl, MainFrame::OnTreeSelectionChanged)
    EVT_TREE_ITEM_ACTIVATED(DBHTreeControl::ID_tree_ctrl, MainFrame::OnTreeItemActivate)

    EVT_CLOSE(MainFrame::OnClose)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
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

    if (m->findDatabase() != 0 && dynamic_cast<Database*>(m) == 0)  // has to be subitem of database
    {
        ContextMenuMetadataItemVisitor cmv(objectMenuM);
        m->acceptVisitor(&cmv);
    }
    if (objectMenuM->GetMenuItemCount() == 2)   // separator
        objectMenuM->Destroy(objectMenuM->FindItemByPosition(1));

    event.Skip();
}
//-----------------------------------------------------------------------------
void MainFrame::OnWindowMenuItem(wxCommandEvent& event)
{
    frameManager().bringOnTop(event.GetId());
}
//-----------------------------------------------------------------------------
void MainFrame::updateStatusbarText()
{
    wxStatusBar *sb = GetStatusBar();
    if (!sb)
        return;

    Database* d = treeMainM->getSelectedDatabase();
    if (d)
    {
        wxString s = d->getUsername();
        if (s.empty())
            s = _("[Trusted user]");
        s = s + wxT("@") + d->getConnectionString()
            + wxT(" (") + d->getConnectionCharset() + wxT(")");
        sb->SetStatusText(s);
    }
    else
        sb->SetStatusText(_("[No database selected]"));
}
//-----------------------------------------------------------------------------
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
            path = wxT(" > ") + path;
        path = treeMainM->GetItemText(t) + path;
        t = treeMainM->GetItemParent(t);
    }
    labelPanelM->setLabel(path);
    */
}
//-----------------------------------------------------------------------------
//! handle double-click on item (or press Enter)
void MainFrame::OnTreeItemActivate(wxTreeEvent& WXUNUSED(event))
{
    wxTreeItemId item = treeMainM->GetSelection();
    if (!item.IsOk())
        return;

    MetadataItem* m = treeMainM->getSelectedMetadataItem();
    if (!m)
        return;

    wxBusyCursor wait;
    NodeType nt = m->getType();

    enum { showProperties = 0, showColumnInfo, selectFromOrExecute };
    int treeActivateAction = showProperties;
    config().getValue(wxT("OnTreeActivate"), treeActivateAction);
    // if no columns in tree, then only Properties can be shown
    if (treeActivateAction == showColumnInfo)
    {
        if (!config().get(wxT("ShowColumnsInTree"), true))
            treeActivateAction = showProperties;
    }

    if (treeActivateAction == showColumnInfo && (nt == ntTable
        || nt == ntSysTable || nt == ntView || nt == ntProcedure))
    {
        if (!treeMainM->ItemHasChildren(item))
        {
            wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED,
                Cmds::Menu_LoadColumnsInfo);
            AddPendingEvent(event);
        }
    }
    else if (treeActivateAction == selectFromOrExecute
        && (nt == ntTable || nt == ntSysTable || nt == ntView))
    {
        wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED,
            Cmds::Menu_BrowseColumns);
        AddPendingEvent(event);
    }
    else if (treeActivateAction == selectFromOrExecute && (nt == ntProcedure))
    {
        wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED,
            Cmds::Menu_ExecuteProcedure);
        AddPendingEvent(event);
    }
    else
    {
        switch (nt)
        {
            case ntDatabase:
                if (!dynamic_cast<Database *>(m)->isConnected())
                {
                    wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED,
                        Cmds::Menu_Connect);
                    AddPendingEvent(event);
                    return;
                }
                break;
            case ntGenerator:
                showGeneratorValue(dynamic_cast<Generator*>(m));
                break;
            case ntColumn:
            case ntTable:
            case ntSysTable:
            case ntView:
            case ntProcedure:
            case ntDomain:
            case ntFunction:
            case ntTrigger:
            case ntException:
            case ntRole:
                {
                    wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED,
                        Cmds::Menu_ObjectProperties);
                    AddPendingEvent(event);
                }
                return;
            default:
                break;
        };
    }

    // Windows tree control automatically does it
#ifdef __WXGTK__
    bool toggle = true;
    config().getValue(wxT("ToggleNodeOnTreeActivate"), toggle);
    if (toggle)
    {
        if (treeMainM->IsExpanded(item))
            treeMainM->Collapse(item);
        else
            treeMainM->Expand(item);
    }
#endif
}
//-----------------------------------------------------------------------------
void MainFrame::OnClose(wxCloseEvent& event)
{
    Raise();
    if (event.CanVeto())
    {
        int res = showQuestionDialog(this, _("Do you really want to quit FlameRobin?"),
            _("All uncommitted transactions will be rolled back, and any uncommitted changes will be lost."),
            AdvancedMessageDialogButtonsOkCancel(_("&Quit")),
            config(), wxT("DIALOG_ConfirmQuit"), _("Always quit without asking"));
        if (res != wxOK)
        {
            event.Veto();
            return;
        }
    }
    //frameManager().setWindowMenu(0);    // tell it not to update menus anymore

    // the next few lines fix the (threading?) problem on some Linux distributions
    // which leave FlameRobin running if there were connected databases upon exit.
    // also, some other versions might crash (on Debian 64bit for example).
    // apparently, doing disconnect before exiting makes it work properly, and
    // on some distros, the wxSafeYield call is needed as well
    // as it doesn't hurt for others, we can leave it all here, at least until
    // Firebird packagers for various distros figure out how to properly use NPTL
    treeMainM->Freeze();
    getGlobalRoot().disconnectAllDatabases();
    wxSafeYield();
    treeMainM->Thaw();

    wxTheClipboard->Flush();
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
    showAboutBox(this);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuManual(wxCommandEvent& WXUNUSED(event))
{
    showDocsHtmlFile(wxT("fr_manual.html"));
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuRelNotes(wxCommandEvent& WXUNUSED(event))
{
    showDocsHtmlFile(wxT("fr_whatsnew.html"));
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuLicense(wxCommandEvent& WXUNUSED(event))
{
    showDocsHtmlFile(wxT("fr_license.html"));
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuURLHomePage(wxCommandEvent& WXUNUSED(event))
{
    showUrl(wxT("http://www.flamerobin.org"));
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuURLProjectPage(wxCommandEvent& WXUNUSED(event))
{
    showUrl(wxT("http://sourceforge.net/projects/flamerobin"));
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuURLFeatureRequest(wxCommandEvent& WXUNUSED(event))
{
    showUrl(wxT("http://sourceforge.net/tracker/?atid=699237&group_id=124340"));
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuURLBugReport(wxCommandEvent& WXUNUSED(event))
{
    showUrl(wxT("http://sourceforge.net/tracker/?atid=699234&group_id=124340"));
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
void MainFrame::OnMenuDatabaseExtractDDL(wxCommandEvent& WXUNUSED(event))
{
    MetadataItem* m = treeMainM->getSelectedDatabase();
    if (!m)
        return;

    URI uri = URI(wxT("fr://edit_ddl?parent_window=") +
        wxString::Format(wxT("%ld"), (uintptr_t)this) +
        wxT("&object_address=") + wxString::Format(wxT("%ld"), (uintptr_t)m));
    getURIProcessor().handleURI(uri);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuDatabaseProperties(wxCommandEvent& WXUNUSED(event))
{
    Database* d = treeMainM->getSelectedDatabase();
    if (!d)
        return;

    frameManager().showMetadataPropertyFrame(d);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuDatabasePreferences(wxCommandEvent& WXUNUSED(event))
{
    Database* d = treeMainM->getSelectedDatabase();
    if (!checkValidDatabase(d))
        return;
    DatabaseConfig dc(d);
    PreferencesDialog pd(this,
        wxString::Format(_("%s preferences"), d->getName_().c_str()),
        dc, wxT("db_settings.confdef"));
    if (pd.isOk() && pd.loadFromConfig())
    {
        pd.selectPage(0);
        pd.ShowModal();
    }
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuInsert(wxCommandEvent& WXUNUSED(event))
{
    Database* d = treeMainM->getSelectedDatabase();
    if (!checkValidDatabase(d))
        return;
    Table* t = dynamic_cast<Table*>(treeMainM->getSelectedMetadataItem());
    if (!t)
        return;

    showSql(this, wxString(_("Execute SQL statements")), d,
        t->getInsertStatement());
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateTriggerForTable(wxCommandEvent& WXUNUSED(event))
{
    MetadataItem* i = treeMainM->getSelectedMetadataItem();
    if (!i)
        return;
    URI uri = URI(wxT("fr://create_trigger?parent_window=") +
        wxString::Format(wxT("%ld"), (uintptr_t)this) +
        wxT("&object_address=") + wxString::Format(wxT("%ld"), (uintptr_t)i));
    getURIProcessor().handleURI(uri);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateProcedureForTable(wxCommandEvent& WXUNUSED(event))
{
    Table *t = dynamic_cast<Table*>(treeMainM->getSelectedMetadataItem());
    if (!t)
        return;
    showSql(this, wxString(_("Creating procedure")), t->findDatabase(),
        t->getProcedureTemplate());
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuExecuteProcedure(wxCommandEvent& WXUNUSED(event))
{
    Procedure* p = dynamic_cast<Procedure*>(treeMainM->getSelectedMetadataItem());
    if (!p)
        return;

    showSql(this, wxString(_("Executing procedure")), p->findDatabase(),
        p->getExecuteStatement());
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuBrowseColumns(wxCommandEvent& WXUNUSED(event))
{
    MetadataItem* i = treeMainM->getSelectedMetadataItem();
    if (!i)
        return;

    Relation* r = dynamic_cast<Relation*>(i);
    Database* d = i->findDatabase();
    if (!d || !r)
        return;

    r->checkAndLoadColumns();

    wxString sql(wxT("SELECT "));
    std::vector<MetadataItem*> temp;
    i->getChildren(temp);
    bool first = true;
    for (std::vector<MetadataItem*>::const_iterator it = temp.begin();
        it != temp.end(); ++it)
    {
        if (first)
            first = false;
        else
            sql += wxT(", ");
        sql += wxT("a.") + (*it)->getQuotedName();
    }
    // add DB_KEY only when table doesn't have a PK/UNQ constraint
    if (Table* t = dynamic_cast<Table*>(r))
    {
        if (!t->getPrimaryKey() && t->getUniqueConstraints()->size() == 0)
            sql += wxT(", a.RDB$DB_KEY");
    }
    sql += wxT("\nFROM ") + i->getQuotedName() + wxT(" a");
    execSql(this, wxString(_("Execute SQL statements")), d, sql, false);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuRegisterDatabase(wxCommandEvent& WXUNUSED(event))
{
    Server* s = treeMainM->getSelectedServer();
    if (!checkValidServer(s))
        return;

    Database db;
    DatabaseRegistrationDialog drd(this, _("Register Existing Database"));
    drd.setServer(s);
    drd.setDatabase(&db);

    if (drd.ShowModal() == wxID_OK)
        treeMainM->selectMetadataItem(s->addDatabase(db));
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuRestoreIntoNewDatabase(wxCommandEvent& WXUNUSED(event))
{
    Server* s = treeMainM->getSelectedServer();
    if (!checkValidServer(s))
        return;

    Database db;
    DatabaseRegistrationDialog drd(this, _("New database parameters"));
    drd.setServer(s);
    drd.setDatabase(&db);
    if (drd.ShowModal() != wxID_OK)
        return;

    Database* newDB = s->addDatabase(db);
    treeMainM->selectMetadataItem(newDB);
    RestoreFrame* f = new RestoreFrame(this, newDB);
    f->Show();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuDatabaseRegistrationInfo(wxCommandEvent& WXUNUSED(event))
{
    Database* d = treeMainM->getSelectedDatabase();
    if (!checkValidDatabase(d))
        return;

    DatabaseRegistrationDialog drd(this, _("Database Registration Info"));
    drd.setDatabase(d);
    if (drd.ShowModal())
        getGlobalRoot().save();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuCreateDatabase(wxCommandEvent& WXUNUSED(event))
{
    Server* s = treeMainM->getSelectedServer();
    if (!checkValidServer(s))
        return;

    Database db;
    DatabaseRegistrationDialog drd(this, _("Create New Database"), true);
    drd.setDatabase(&db);
    drd.setServer(s);

    if (drd.ShowModal() == wxID_OK)
        treeMainM->selectMetadataItem(s->addDatabase(db));
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuManageUsers(wxCommandEvent& WXUNUSED(event))
{
    Server* s = treeMainM->getSelectedServer();
    if (checkValidServer(s))
        frameManager().showMetadataPropertyFrame(s);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUnRegisterServer(wxCommandEvent& WXUNUSED(event))
{
    Server* s = treeMainM->getSelectedServer();
    if (!checkValidServer(s))
        return;

    int res = showQuestionDialog(this, _("Do you really want to unregister this server?"),
        _("The registration information for the server and all its registered databases will be deleted. This operation can not be undone."),
        AdvancedMessageDialogButtonsOkCancel(_("Unregister")));
    if (res == wxOK)
    {
    Root* r = s->getRoot();
    if (r)
        r->removeServer(s);
    }
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuServerProperties(wxCommandEvent& WXUNUSED(event))
{
    Server* s = treeMainM->getSelectedServer();
    if (!checkValidServer(s))
        return;

    ServerRegistrationDialog srd(this, _("Server Registration Info"));
    srd.setServer(s);
    if (srd.ShowModal())
        getGlobalRoot().save();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuRegisterServer(wxCommandEvent& WXUNUSED(event))
{
    Root* r = dynamic_cast<Root*>(treeMainM->getMetadataItem(treeMainM->GetRootItem()));
    if (!r)
        return;

    Server s;
    ServerRegistrationDialog srd(this, _("Register New Server"), true);
    srd.setServer(&s);
    if (wxID_OK == srd.ShowModal())
        treeMainM->selectMetadataItem(r->addServer(s));
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUnRegisterDatabase(wxCommandEvent& WXUNUSED(event))
{
    Database* d = treeMainM->getSelectedDatabase();
    if (!checkValidDatabase(d))
        return;
    // command should never be enabled when database is connected
    wxCHECK_RET(!d->isConnected(),
        wxT("Can not unregister connected database"));

    int res = showQuestionDialog(this, _("Do you really want to unregister this database?"),
        _("The registration information for the database will be deleted. This operation can not be undone."),
        AdvancedMessageDialogButtonsOkCancel(_("Unregister")));
    if (res == wxOK)
    {
    Server* s = d->getServer();
    if (s)
        s->removeDatabase(d);
    }
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuShowConnectedUsers(wxCommandEvent& WXUNUSED(event))
{
    Database* d = treeMainM->getSelectedDatabase();
    if (!checkValidDatabase(d))
        return;

    wxArrayString as;
    std::vector<std::string> users;
    d->getIBPPDatabase()->Users(users);
    for (std::vector<std::string>::const_iterator i = users.begin(); i != users.end(); ++i)
        as.Add(std2wx(*i));

    ::wxGetSingleChoice(_("Connected users"), d->getPath(), as);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuGetServerVersion(wxCommandEvent& WXUNUSED(event))
{
    Server* s = treeMainM->getSelectedServer();
    if (!s)
        return;

    std::string version;
    try
    {
        // progress dialog will get closed in case of fatal exception or when
        // retieving is complete
        ProgressDialog pd(this, _("Retrieving server version"), 1);
        IBPP::Service svc;
        if (!getService(s, svc, &pd, false))    // false = no need for sysdba
            return;
        svc->GetVersion(version);
    }
    catch (IBPP::Exception& e)
    {
        wxMessageBox(std2wx(e.ErrorMessage()), _("Error"));
        return;
    }

    wxMessageBox(std2wx(version), _("Server Version"),
        wxOK | wxICON_INFORMATION);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuGenerateData(wxCommandEvent& WXUNUSED(event))
{
    Database* db = treeMainM->getSelectedDatabase();
    if (!checkValidDatabase(db))
        return;

    DataGeneratorFrame* f = new DataGeneratorFrame(this, db);
    f->Show();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuMonitorEvents(wxCommandEvent& WXUNUSED(event))
{
    Database* db = treeMainM->getSelectedDatabase();
    if (!checkValidDatabase(db))
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
//-----------------------------------------------------------------------------
void MainFrame::OnMenuBackup(wxCommandEvent& WXUNUSED(event))
{
    Database* db = treeMainM->getSelectedDatabase();
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
//-----------------------------------------------------------------------------
void MainFrame::OnMenuRestore(wxCommandEvent& WXUNUSED(event))
{
    Database* db = treeMainM->getSelectedDatabase();
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
//-----------------------------------------------------------------------------
void MainFrame::OnMenuReconnect(wxCommandEvent& WXUNUSED(event))
{
    Database* d = treeMainM->getSelectedDatabase();
    if (!checkValidDatabase(d))
        return;

    wxBusyCursor cur;
    d->reconnect();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuConnectAs(wxCommandEvent& WXUNUSED(event))
{
    Database* d = treeMainM->getSelectedDatabase();
    if (!checkValidDatabase(d))
        return;
    // command should never be enabled when database is connected
    wxCHECK_RET(!d->isConnected(),
        wxT("Can not connect to already connected database"));

    DatabaseRegistrationDialog drd(this, _("Connect as..."), false, true);
    d->prepareTemporaryCredentials();
    drd.setDatabase(d);
    if (wxID_OK != drd.ShowModal() || !connect())
        d->resetCredentials();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuConnect(wxCommandEvent& WXUNUSED(event))
{
    connect();
}
//-----------------------------------------------------------------------------
bool MainFrame::connect()
{
    Database* db = treeMainM->getSelectedDatabase();
    if (!checkValidDatabase(db))
        return false;
    if (db->isConnected() || !connectDatabase(db, this))
        return false;
    if (db->isConnected())
    {
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
        treeMainM->Expand(treeMainM->GetSelection());
    }

    updateStatusbarText();
    Raise();
    Update();
    return true;
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuDisconnect(wxCommandEvent& WXUNUSED(event))
{
    Database* d = treeMainM->getSelectedDatabase();
    if (!checkValidDatabase(d))
        return;

    treeMainM->Freeze();
    try
    {
        d->disconnect();
    }
    catch (...)
    {
    }

    wxSafeYield();
    treeMainM->Thaw();
    updateStatusbarText();
}
//-----------------------------------------------------------------------------
void MainFrame::showGeneratorValue(Generator* g)
{
    if (g)
        g->loadValue();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuShowGeneratorValue(wxCommandEvent& WXUNUSED(event))
{
    showGeneratorValue(dynamic_cast<Generator*>(treeMainM->getSelectedMetadataItem()));
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuSetGeneratorValue(wxCommandEvent& WXUNUSED(event))
{
    Generator* g = dynamic_cast<Generator*>(treeMainM->getSelectedMetadataItem());
    if (!g)
        return;

    URI uri = URI(wxT("fr://edit_generator_value?parent_window=") + wxString::Format(wxT("%ld"), (uintptr_t)this)
        + wxT("&object_address=") + wxString::Format(wxT("%ld"), (uintptr_t)g));
    getURIProcessor().handleURI(uri);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuShowAllGeneratorValues(wxCommandEvent& WXUNUSED(event))
{
    Database* d = treeMainM->getSelectedDatabase();
    if (checkValidDatabase(d))
        d->loadGeneratorValues();
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
    MetadataItem* item = treeMainM->getSelectedMetadataItem();
    if (!item)
        return;
    showCreateTemplate(item);
}
//-----------------------------------------------------------------------------
void MainFrame::showCreateTemplate(const MetadataItem* m)
{
    // TODO: add a call for wizards. For example, we can have NewTableWizard which is a frame with grid in which
    // user can enter column data for new table (name, datatype, null option, collation, default, etc.) and also
    // enter a name for new table, etc. Wizard should return a bunch of DDL statements as a wxString which would we
    // pass to ExecSqlFrame.

    wxString sql = m->getCreateSqlTemplate();
    if (sql == wxT(""))
    {
        wxMessageBox(_("The feature is not yet available for this type of database objects."), _("Not yet implemented"), wxOK | wxICON_INFORMATION);
        return;
    }

    Database* db = treeMainM->getSelectedDatabase();
    if (!checkValidDatabase(db))
        return;

    showSql(this, wxEmptyString, db, sql);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuLoadColumnsInfo(wxCommandEvent& WXUNUSED(event))
{
    MetadataItem* t = treeMainM->getSelectedMetadataItem();
    if (!t)
        return;

    switch (t->getType())
    {
        case ntTable:
        case ntSysTable:
        case ntView:        ((Relation*)t)->checkAndLoadColumns();     break;
        case ntProcedure:   ((Procedure*)t)->checkAndLoadParameters(); break;
        default:            break;
    };

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

    wxTreeItemId id = treeMainM->GetSelection();
    if (id.IsOk() && treeMainM->ItemHasChildren(id))
        treeMainM->Expand(id);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuAddColumn(wxCommandEvent& WXUNUSED(event))
{
    Table* t = dynamic_cast<Table*>(treeMainM->getSelectedMetadataItem());
    if (!t)
        return;

    URI uri = URI(wxT("fr://add_field?parent_window=") + wxString::Format(wxT("%ld"), (uintptr_t)this)
        + wxT("&object_address=") + wxString::Format(wxT("%ld"), (uintptr_t)t));
    getURIProcessor().handleURI(uri);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuToggleDisconnected(wxCommandEvent& event)
{
    config().setValue(wxT("HideDisconnectedDatabases"), !event.IsChecked());
    getGlobalRoot().notifyAllServers();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuToggleStatusBar(wxCommandEvent& event)
{
    wxStatusBar* s = GetStatusBar();
    if (!s)
        s = CreateStatusBar();

    bool show = event.IsChecked();
    config().setValue(wxT("showStatusBar"), show);
    s->Show(show);
    SendSizeEvent();
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuToggleSearchBar(wxCommandEvent& event)
{
    bool show = event.IsChecked();
    config().setValue(wxT("showSearchBar"), show);
    searchPanelSizerM->Show(searchPanelM, show, true);    // recursive
    searchPanelSizerM->Layout();
}
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
void MainFrame::OnButtonSearchClick(wxCommandEvent& WXUNUSED(event))
{
    AdvancedSearchFrame *asf = new AdvancedSearchFrame(this);
    asf->Show();
}
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
void MainFrame::OnMenuObjectProperties(wxCommandEvent& WXUNUSED(event))
{
    MetadataItem* m = treeMainM->getSelectedMetadataItem();
    if (!m)
        return;

    Column* c = dynamic_cast<Column*>(m);
    if (c)
    {
        // Return when we're dealing with a system column
        if (c->isSystem() || !c->getTable())
            return;

        URI uri = URI(wxT("fr://edit_field?parent_window=")
            + wxString::Format(wxT("%ld"), (uintptr_t)this)
            + wxT("&object_address=") + wxString::Format(wxT("%ld"),
            (uintptr_t)c));
        getURIProcessor().handleURI(uri);
    }
    else
        frameManager().showMetadataPropertyFrame(m);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuAlterObject(wxCommandEvent& WXUNUSED(event))
{
    Database *db = treeMainM->getSelectedDatabase();
    Procedure* p = dynamic_cast<Procedure *>(
        treeMainM->getSelectedMetadataItem());
    if (p)
    {
        URI uri(wxT("fr://edit_procedure?parent_window=")
            + wxString::Format(wxT("%ld"), (uintptr_t)this)
            + wxT("&object_address=")
            + wxString::Format(wxT("%ld"), (uintptr_t)p));
        getURIProcessor().handleURI(uri);
        return;
    }
    View* v = dynamic_cast<View *>(treeMainM->getSelectedMetadataItem());
    Trigger* t = dynamic_cast<Trigger *>(treeMainM->getSelectedMetadataItem());
    Domain *dm = dynamic_cast<Domain *>(treeMainM->getSelectedMetadataItem());
    if (!db || !p && !v && !t && !dm)
        return;

    wxString sql;
    if (v)
        sql = v->getRebuildSql();
    else if (t)
        sql = t->getAlterSql();
    else if (dm)
        sql = dm->getAlterSqlTemplate();
    showSql(this, wxString(_("Alter object")), db, sql);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuRecreateDatabase(wxCommandEvent& WXUNUSED(event))
{
    Database* d = treeMainM->getSelectedDatabase();

    if (!checkValidDatabase(d))
        return;

    wxString msg(wxString::Format(
        _("Are you sure you wish to recreate the database \"%s\"?"),
        d->getName_().c_str()));
    wxString secondary;
    if (!d->isConnected())
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
            if (!d->isConnected())
                connect();
            if (d->isConnected())
            {
                d->drop();
                d->disconnect(true);    // true = just remove the child nodes
            }
        }
        catch(IBPP::Exception&) {}

        // use the dialog as some information (charset and page size) is
        // not necessarily available, and the user may want to change it too
        DatabaseRegistrationDialog drd(this, _("Recreate Database"), true);
        drd.setDatabase(d);
        drd.setServer(d->getServer());
        if (drd.ShowModal() == wxID_OK)
            treeMainM->selectMetadataItem(d);
    }
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuDropDatabase(wxCommandEvent& WXUNUSED(event))
{
    Database* d = treeMainM->getSelectedDatabase();

    if (!checkValidDatabase(d) || !confirmDropItem(d))
        return;

    int result = wxMessageBox(
        _("Do you wish to keep the registration info?"),
        _("Dropping database: ")+d->getName_(),
        wxYES_NO|wxCANCEL|wxICON_QUESTION);
    if (result == wxCANCEL)
        return;
    d->drop();
    d->disconnect(true);    // true = just remove the child nodes
    if (result == wxNO)
    {   // unregister
        Server* s = d->getServer();
        if (s)
            s->removeDatabase(d);
    }
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuDropObject(wxCommandEvent& WXUNUSED(event))
{
    Database* d = treeMainM->getSelectedDatabase();
    if (!checkValidDatabase(d))
        return;
    MetadataItem* m = treeMainM->getSelectedMetadataItem();
    if (!m)
        return;
    if (!confirmDropItem(m))
        return;

    // TODO: We could first check if there are some dependant objects, and offer the user to
    //       either drop dependencies, or drop those objects too. Then we should create a bunch of
    //       sql statements that do it.
    execSql(this, wxEmptyString, d, m->getDropSqlStatement(), true);
}
//-----------------------------------------------------------------------------
//! create new ExecSqlFrame and attach database object to it
void MainFrame::OnMenuQuery(wxCommandEvent& WXUNUSED(event))
{
    Database* d = treeMainM->getSelectedDatabase();
    if (!checkValidDatabase(d))
        return;
    if (!d->isConnected())
    {
        int res = showQuestionDialog(this, _("Do you want to connect to the database?"),
            _("The database is not connected. You first have to establish a database connection before you can execute SQL statements."),
            AdvancedMessageDialogButtonsOkCancel(_("C&onnect")),
            config(), wxT("DIALOG_ConfirmConnectForQuery"), _("Always connect without asking"));
        if (res == wxOK)
            connect();
    }
    if (!d->isConnected())
        return;

    wxBusyCursor bc;
    showSql(this, wxString(_("Execute SQL statements")), d, wxEmptyString);
}
//-----------------------------------------------------------------------------
const wxString MainFrame::getName() const
{
    return wxT("MainFrame");
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUpdateUnRegisterServer(wxUpdateUIEvent& event)
{
    Server* s = treeMainM->getSelectedServer();
    event.Enable(s != 0 && !s->hasConnectedDatabase());
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUpdateIfServerSelected(wxUpdateUIEvent& event)
{
    Server* s = treeMainM->getSelectedServer();
    event.Enable(s != 0);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUpdateIfDatabaseConnected(wxUpdateUIEvent& event)
{
    Database* d = treeMainM->getSelectedDatabase();
    event.Enable(d != 0 && d->isConnected());
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUpdateIfDatabaseNotConnected(wxUpdateUIEvent& event)
{
    Database* d = treeMainM->getSelectedDatabase();
    event.Enable(d != 0 && !d->isConnected());
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUpdateIfDatabaseSelected(wxUpdateUIEvent& event)
{
    Database* d = treeMainM->getSelectedDatabase();
    event.Enable(d != 0);
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUpdateIfMetadataItemHasChildren(wxUpdateUIEvent& event)
{
    MetadataItem* mi = treeMainM->getSelectedMetadataItem();
    event.Enable(mi != 0 && mi->getChildrenCount());
}
//-----------------------------------------------------------------------------
bool MainFrame::confirmDropItem(MetadataItem* item)
{
    wxString msg(wxString::Format(
        _("Are you sure you wish to drop the %s %s?"),
        item->getTypeName().Lower().c_str(),
        item->getName_().c_str()));
    return wxOK == showQuestionDialog(this, msg,
        _("Once you drop the object it is permanently removed from database."),
        AdvancedMessageDialogButtonsOkCancel(_("&Drop")),
        config(), wxT("DIALOG_ConfirmDrop"), _("Always drop without asking"));
}
//-----------------------------------------------------------------------------
bool MainFrame::openUnregisteredDatabase(const wxString& dbpath)
{
    Database tempDb;
    tempDb.setPath(dbpath);
    tempDb.setName_(tempDb.extractNameFromConnectionString());

    wxString iscUser, iscPassword;
    if (!wxGetEnv(wxT("ISC_USER"), &iscUser))
        iscUser = wxT("SYSDBA");
    tempDb.setUsername(iscUser);
    if (!wxGetEnv(wxT("ISC_PASSWORD"), &iscPassword))
        iscPassword = wxEmptyString;
    tempDb.setRawPassword(iscPassword);

    DatabaseRegistrationDialog drd(this, _("Database Connection Settings"));
    drd.setDatabase(&tempDb);
    if (drd.ShowModal() == wxID_OK)
    {
        Database* db = getGlobalRoot().addUnregisteredDatabase(tempDb);
        treeMainM->selectMetadataItem(db);
        if (db && connectDatabase(db, this))
            return true;
    }
    return false;
}
//-----------------------------------------------------------------------------
