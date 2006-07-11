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

#include <wx/clipbrd.h>
#include <wx/tokenzr.h>

#include "config/Config.h"
#include "config/DatabaseConfig.h"
#include "core/FRError.h"
#include "core/StringUtils.h"
#include "dberror.h"
#include "gui/AdvancedMessageDialog.h"
#include "gui/AdvancedSearchFrame.h"
#include "gui/BackupFrame.h"
#include "gui/ContextMenuMetadataItemVisitor.h"
#include "gui/DatabaseRegistrationDialog.h"
#include "gui/EventWatcherFrame.h"
#include "gui/ExecuteSqlFrame.h"
#include "gui/MainFrame.h"
#include "gui/PreferencesDialog.h"
#include "gui/ProgressDialog.h"
#include "gui/RestoreFrame.h"
#include "gui/ServerRegistrationDialog.h"
#include "gui/SimpleHtmlFrame.h"
#include "framemanager.h"
#include "frtypes.h"
#include "frversion.h"
#include "main.h"
#include "metadata/metadataitem.h"
#include "metadata/root.h"
#include "myTreeCtrl.h"
#include "treeitem.h"
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
//! included xpm files, so that icons are compiled into executable
namespace sql_icons {
#include "left.xpm"
#include "right.xpm"
#include "search.xpm"
};
//-----------------------------------------------------------------------------
MainFrame::MainFrame(wxWindow* parent, int id, const wxString& title, const wxPoint& pos, const wxSize& size, long style):
    BaseFrame(parent, id, title, pos, size, style, wxT("FlameRobin_main"))
{
    mainPanelM = new wxPanel(this);
    tree_ctrl_1 = new myTreeCtrl(mainPanelM, wxDefaultPosition, wxDefaultSize,
#if defined __WXGTK20__ || defined __WXMAC__
        wxTR_NO_LINES |
#endif
        wxTR_HAS_BUTTONS | wxSUNKEN_BORDER);

    wxArrayString choices;  // load from config?

    searchPanelM = new wxPanel(mainPanelM, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL|wxSUNKEN_BORDER);
    searchBoxM = new wxComboBox(searchPanelM, ID_search_box, wxEmptyString, wxDefaultPosition, wxDefaultSize,
        choices, wxCB_DROPDOWN|wxCB_SORT);
    button_prev = new wxBitmapButton(searchPanelM, ID_button_prev, wxBitmap(sql_icons::left_xpm));
    button_next = new wxBitmapButton(searchPanelM, ID_button_next, wxBitmap(sql_icons::right_xpm));
    button_advanced = new wxBitmapButton(searchPanelM, ID_button_advanced, wxBitmap(sql_icons::search_xpm));
    button_advanced->SetToolTip(_("Advanced metadata search"));
    button_prev->SetToolTip(_("Previous match"));
    button_next->SetToolTip(_("Next match"));

    buildMainMenu();
    SetStatusBarPane(-1);   // disable automatic fill
    set_properties();
    do_layout();
    tree_ctrl_1->SetFocus();

    if (!config().get(wxT("showSearchBar"), true))
    {
        inner_sizer->Show(searchPanelM, false, true);    // recursive
        inner_sizer->Layout();
    }
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
    viewMenu->AppendCheckItem(myTreeCtrl::Menu_ToggleSearchBar, _("S&earch bar"));
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
    serverMenu->Append(myTreeCtrl::Menu_GetServerVersion, _("Retrieve server &version"));
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
    if (config().get(wxT("showSearchBar"), true))
        viewMenu->Check(myTreeCtrl::Menu_ToggleSearchBar, true);
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

    // Default (generic) tree looks pretty ugly on GTK 1
#if defined(__WXGTK__) && !defined(__WXGTK20__)
    tree_ctrl_1->SetIndent(12);
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
        wxMessageBox(msg, _("Configuration file not found"), wxOK|wxICON_INFORMATION);

        Server s;
        s.setName_(wxT("Localhost"));
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
    wxBoxSizer* outer_sizer = new wxBoxSizer(wxVERTICAL);
    outer_sizer->Add(mainPanelM, 1, wxEXPAND, 0);
    inner_sizer = new wxBoxSizer(wxVERTICAL);
    inner_sizer->Add(tree_ctrl_1, 1, wxEXPAND, 0);

    sizer_search = new wxBoxSizer(wxHORIZONTAL);
    sizer_search->Add(searchBoxM, 1, wxEXPAND, 0);
    sizer_search->Add(button_prev);
    sizer_search->Add(button_next);
    sizer_search->Add(button_advanced);
    searchPanelM->SetSizer(sizer_search);

    inner_sizer->Add(searchPanelM, 0, wxEXPAND);
    mainPanelM->SetSizer(inner_sizer);
    SetAutoLayout(true);
    SetSizer(outer_sizer);
    Layout();
}
//-----------------------------------------------------------------------------
const wxRect MainFrame::getDefaultRect() const
{
    return wxRect(-1, -1, 257, 367);
}
//-----------------------------------------------------------------------------
myTreeCtrl* MainFrame::getTreeCtrl()
{
    return tree_ctrl_1;
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
    EVT_MENU(myTreeCtrl::Menu_GetServerVersion, MainFrame::OnMenuGetServerVersion)
    EVT_UPDATE_UI(myTreeCtrl::Menu_GetServerVersion, MainFrame::OnMenuUpdateIfServerSelected)
    EVT_MENU(myTreeCtrl::Menu_MonitorEvents, MainFrame::OnMenuMonitorEvents)
    EVT_UPDATE_UI(myTreeCtrl::Menu_MonitorEvents, MainFrame::OnMenuUpdateIfDatabaseConnected)
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
    EVT_MENU(myTreeCtrl::Menu_DropDatabase, MainFrame::OnMenuDropDatabase)
    EVT_UPDATE_UI(myTreeCtrl::Menu_DropDatabase, MainFrame::OnMenuUpdateIfDatabaseConnected)
    EVT_MENU(myTreeCtrl::Menu_Query, MainFrame::OnMenuQuery)
    EVT_UPDATE_UI(myTreeCtrl::Menu_Query, MainFrame::OnMenuUpdateIfDatabaseSelected)
    EVT_UPDATE_UI(myTreeCtrl::Menu_NewObject, MainFrame::OnMenuUpdateIfDatabaseConnected)
    EVT_MENU(myTreeCtrl::Menu_DatabasePreferences, MainFrame::OnMenuDatabasePreferences)
    EVT_UPDATE_UI(myTreeCtrl::Menu_DatabasePreferences, MainFrame::OnMenuUpdateIfDatabaseSelected)
    EVT_MENU(myTreeCtrl::Menu_DatabaseProperties, MainFrame::OnMenuDatabaseProperties)
    EVT_UPDATE_UI(myTreeCtrl::Menu_DatabaseProperties, MainFrame::OnMenuUpdateIfDatabaseConnected)

    EVT_MENU(myTreeCtrl::Menu_Insert, MainFrame::OnMenuInsert)
    EVT_MENU(myTreeCtrl::Menu_Browse, MainFrame::OnMenuBrowse)
    EVT_MENU(myTreeCtrl::Menu_BrowseColumns, MainFrame::OnMenuBrowseColumns)
    EVT_MENU(myTreeCtrl::Menu_LoadColumnsInfo, MainFrame::OnMenuLoadColumnsInfo)
    EVT_MENU(myTreeCtrl::Menu_AddColumn, MainFrame::OnMenuAddColumn)
    EVT_MENU(myTreeCtrl::Menu_CreateTriggerForTable, MainFrame::OnMenuCreateTriggerForTable)
    EVT_MENU(myTreeCtrl::Menu_ExecuteProcedure, MainFrame::OnMenuExecuteProcedure)

    EVT_MENU(myTreeCtrl::Menu_ShowAllGeneratorValues, MainFrame::OnMenuShowAllGeneratorValues)
    EVT_UPDATE_UI(myTreeCtrl::Menu_ShowAllGeneratorValues, MainFrame::OnMenuUpdateIfMetadataItemHasChildren)
    EVT_MENU(myTreeCtrl::Menu_ShowGeneratorValue, MainFrame::OnMenuShowGeneratorValue)
    EVT_MENU(myTreeCtrl::Menu_SetGeneratorValue, MainFrame::OnMenuSetGeneratorValue)

    EVT_MENU(myTreeCtrl::Menu_CreateObject, MainFrame::OnMenuCreateObject)
    EVT_MENU(myTreeCtrl::Menu_AlterObject, MainFrame::OnMenuAlterObject)
    EVT_MENU(myTreeCtrl::Menu_DropObject, MainFrame::OnMenuDropObject)
    EVT_MENU(myTreeCtrl::Menu_ObjectProperties, MainFrame::OnMenuObjectProperties)

    EVT_MENU(myTreeCtrl::Menu_ToggleStatusBar, MainFrame::OnMenuToggleStatusBar)
    EVT_MENU(myTreeCtrl::Menu_ToggleSearchBar, MainFrame::OnMenuToggleSearchBar)
    EVT_MENU(myTreeCtrl::Menu_ToggleDisconnected, MainFrame::OnMenuToggleDisconnected)

    EVT_TEXT_ENTER(MainFrame::ID_search_box, MainFrame::OnSearchBoxEnter)
    EVT_TEXT(MainFrame::ID_search_box, MainFrame::OnSearchTextChange)
    EVT_BUTTON(MainFrame::ID_button_advanced, MainFrame::OnButtonSearchClick)
    EVT_BUTTON(MainFrame::ID_button_prev, MainFrame::OnButtonPrevClick)
    EVT_BUTTON(MainFrame::ID_button_next, MainFrame::OnButtonNextClick)

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
void MainFrame::updateStatusbarText()
{
    wxStatusBar *sb = GetStatusBar();
    if (!sb)
        return;

    Database* d = tree_ctrl_1->getSelectedDatabase();
    if (d)
    {
        wxString s = d->getUsername() + wxT("@") + d->getConnectionString()
            + wxT(" (") + d->getConnectionCharset() + wxT(")");
        sb->SetStatusText(s);
    }
    else
        sb->SetStatusText(_("[No database selected]"));
}
//-----------------------------------------------------------------------------
void MainFrame::OnTreeSelectionChanged(wxTreeEvent& WXUNUSED(event))
{
    FR_TRY

    updateStatusbarText();

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

    NodeType nt = m->getType();

    enum { showProperties = 0, showColumnInfo };
    int treeActivateAction = showProperties;
    config().getValue(wxT("OnTreeActivate"), treeActivateAction);
    if (!config().get(wxT("ShowColumnsInTree"), true))   // if no columns in tree, then only Properties can be shown
        treeActivateAction = showProperties;

    if (treeActivateAction == showColumnInfo && (nt == ntTable || nt == ntSysTable || nt == ntView || nt == ntProcedure))
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
                connect();
                break;
            case ntGenerator:
                showGeneratorValue(dynamic_cast<Generator*>(m));
                break;
            case ntColumn:
                OnMenuObjectProperties(event);
                break;
            case ntTable:
            case ntSysTable:
            case ntView:
            case ntProcedure:
            case ntDomain:
            case ntFunction:
            case ntTrigger:
            case ntException:
            case ntRole:
                frameManager().showMetadataPropertyFrame(this, m, true);
                break;
            default:
                break;
        };
    }

#ifdef __WXGTK__
    bool toggle = true;
    config().getValue(wxT("ToggleNodeOnTreeActivate"), toggle);
    if (toggle)
    {
        if (tree_ctrl_1->IsExpanded(item))
            tree_ctrl_1->Collapse(item);
        else
            tree_ctrl_1->Expand(item);
    }
#endif

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnClose(wxCloseEvent& event)
{
    FR_TRY

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
    frameManager().setWindowMenu(0);    // tell it not to update menus anymore

    // the next few lines fix the (threading?) problem on some Linux distributions
    // which leave FlameRobin running if there were connected databases upon exit.
    // also, some other versions might crash (on Debian 64bit for example).
    // apparently, doing disconnect before exiting makes it work properly, and
    // on some distros, the wxSafeYield call is needed as well
    // as it doesn't hurt for others, we can leave it all here, at least until
    // Firebird packagers for various distros figure out how to properly use NPTL
    tree_ctrl_1->Freeze();
    getGlobalRoot().disconnectAllDatabases();
    wxSafeYield();
    tree_ctrl_1->Thaw();

    wxTheClipboard->Flush();
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
        FR_VERSION_MAJOR, FR_VERSION_MINOR, FR_VERSION_RLS);

/*  Commented out for the time being, because the revision string in
    frversion.h is only updated when the file is modifed, or when a full
    checkout or export happens :-(

    // extract revision number from string "$Rev: NNNN "
    wxStringTokenizer tknzr(wxT(FR_VERSION_SVN));
    tknzr.NextToken();
    unsigned long revision;
    if (wxString(tknzr.GetNextToken()).ToULong(&revision))
        msg += wxString::Format(wxT(".%u"), revision);
*/

#if wxUSE_UNICODE
    msg += wxT(" Unicode");
#endif
    msg += wxT("\n");
    msg += _("Database administration tool for Firebird RDBMS");
    msg += wxT("\n\n");
    msg += ib;
    msg += wxT("\n\n");
    msg += _("Copyright (c) 2004-2006  FlameRobin Development Team");
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
void MainFrame::OnMenuDatabaseProperties(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Database* d = tree_ctrl_1->getSelectedDatabase();
    if (!d)
        return;

    frameManager().showMetadataPropertyFrame(this, d);

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
        wxString::Format(_("%s preferences"), d->getName_().c_str()),
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
    URI uri = URI(wxT("fr://create_trigger?parent_window=") +
        wxString::Format(wxT("%ld"), (uintptr_t)this) +
        wxT("&object_address=") + wxString::Format(wxT("%ld"), (uintptr_t)i));
    getURIProcessor().handleURI(uri);

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
    sql += i->getQuotedName();

    if (t == ntProcedure)
    {
        Procedure* p = dynamic_cast<Procedure*>(i);
        if (!p)
            return;

        if (!p->isSelectable())
        {
            ::wxMessageBox(_("This procedure is not selectable"),
                _("Cannot create statement"), wxOK|wxICON_INFORMATION);
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
            ::wxMessageBox(_("This procedure is not selectable"),
                _("Cannot create statement"), wxOK|wxICON_INFORMATION);
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
            sql += (*it)->getQuotedName();
        }
        sql += wxT("\nFROM ") + i->getQuotedName();
    }

    ExecuteSqlFrame* eff = new ExecuteSqlFrame(this, -1, wxString(_("Execute SQL statements")));
    eff->setDatabase(d);
    eff->Show();
    eff->setSql(sql);
    eff->Update();
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
    DatabaseRegistrationDialog drd(this, _("Register Existing Database"));
    drd.setServer(s);
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
    DatabaseRegistrationDialog drd(this, _("New database parameters"));
    drd.setServer(s);
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

    DatabaseRegistrationDialog drd(this, _("Database Registration Info"));
    drd.setDatabase(d);
    if (drd.ShowModal())
        getGlobalRoot().save();

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
    DatabaseRegistrationDialog drd(this, _("Create New Database"), true);
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

    Server* s = tree_ctrl_1->getSelectedServer();
    if (checkValidServer(s))
        frameManager().showMetadataPropertyFrame(this, s);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuUnRegisterServer(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Server* s = tree_ctrl_1->getSelectedServer();
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

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuServerProperties(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Server* s = tree_ctrl_1->getSelectedServer();
    if (!checkValidServer(s))
        return;

    ServerRegistrationDialog srd(this, _("Server Registration Info"));
    srd.setServer(s);
    if (srd.ShowModal())
        getGlobalRoot().save();

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
    ServerRegistrationDialog srd(this, _("Register New Server"), true);
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

    int res = showQuestionDialog(this, _("Do you really want to unregister this database?"),
        _("The registration information for the database will be deleted. This operation can not be undone."),
        AdvancedMessageDialogButtonsOkCancel(_("Unregister")));
    if (res == wxOK)
    {
    Server* s = d->getServer();
    if (s)
        s->removeDatabase(d);
    }

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
void MainFrame::OnMenuGetServerVersion(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Server* s = tree_ctrl_1->getSelectedServer();
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

    wxMessageBox(std2wx(version), _("Server version"),
        wxOK|wxICON_INFORMATION);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuMonitorEvents(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Database* db = tree_ctrl_1->getSelectedDatabase();
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

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuBackup(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Database* db = tree_ctrl_1->getSelectedDatabase();
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

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuRestore(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Database* db = tree_ctrl_1->getSelectedDatabase();
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

    DatabaseRegistrationDialog drd(this, _("Connect as..."), false, true);
    d->prepareTemporaryCredentials();
    drd.setDatabase(d);
    if (wxID_OK != drd.ShowModal() || !connect())
        d->resetCredentials();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuConnect(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    connect();

    FR_CATCH
}
//-----------------------------------------------------------------------------
bool MainFrame::connect()
{
    Database* db = tree_ctrl_1->getSelectedDatabase();
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
    }

    updateStatusbarText();
    Raise();
    Update();
    return true;
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuDisconnect(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Database* d = tree_ctrl_1->getSelectedDatabase();
    if (!checkValidDatabase(d))
        return;

    tree_ctrl_1->Freeze();
    if (!d->disconnect())
        reportLastError(_("Error Disconnecting Database"));

    FR_CATCH

    wxSafeYield();
    tree_ctrl_1->Thaw();
    updateStatusbarText();
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
        case ntSysTable:
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

    Table* t = dynamic_cast<Table*>(tree_ctrl_1->getSelectedMetadataItem());
    if (!t)
        return;

    URI uri = URI(wxT("fr://add_field?parent_window=") + wxString::Format(wxT("%ld"), (uintptr_t)this)
        + wxT("&object_address=") + wxString::Format(wxT("%ld"), (uintptr_t)t));
    getURIProcessor().handleURI(uri);

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
void MainFrame::OnMenuToggleSearchBar(wxCommandEvent& event)
{
    FR_TRY

    bool show = event.IsChecked();
    config().setValue(wxT("showSearchBar"), show);
    inner_sizer->Show(searchPanelM, show, true);    // recursive
    inner_sizer->Layout();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnSearchTextChange(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    if (tree_ctrl_1->findText(searchBoxM->GetValue()))
        searchBoxM->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    else
        searchBoxM->SetForegroundColour(*wxRED);
    wxStatusBar *sb = GetStatusBar();
    if (sb)
        sb->SetStatusText(_("Hit ENTER to focus the tree."));

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnSearchBoxEnter(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

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
            searchBoxM->Append(tree_ctrl_1->GetItemText(tree_ctrl_1->GetSelection()));
    }
    searchBoxM->SetValue(wxEmptyString);
    tree_ctrl_1->SetFocus();
    wxStatusBar *sb = GetStatusBar();
    if (sb)
        sb->SetStatusText(_("Item added to the list."));

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnButtonSearchClick(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    AdvancedSearchFrame *asf = new AdvancedSearchFrame(this);
    asf->Show();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnButtonPrevClick(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    // move backward and search then
    wxTreeItemId id = tree_ctrl_1->GetSelection();
    if (id.IsOk())
    {
        tree_ctrl_1->SelectItem(tree_ctrl_1->getPreviousItem(id));
        tree_ctrl_1->findText(searchBoxM->GetValue(),false);
        if (id == tree_ctrl_1->GetSelection())
        {
            wxStatusBar *sb = GetStatusBar();
            if (sb)
                sb->SetStatusText(_("No more matches."));
        }
    }

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnButtonNextClick(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    // move forward and search then
    wxTreeItemId id = tree_ctrl_1->GetSelection();
    if (id.IsOk())
    {
        tree_ctrl_1->SelectItem(tree_ctrl_1->getNextItem(id));
        tree_ctrl_1->findText(searchBoxM->GetValue(), true);
        if (id == tree_ctrl_1->GetSelection())
        {
            wxStatusBar *sb = GetStatusBar();
            if (sb)
                sb->SetStatusText(_("No more matches."));
        }
    }

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
        // Return when we're dealing with a system column
        if (c->isSystem())
            return;

        URI uri = URI(wxT("fr://edit_field?parent_window=") + wxString::Format(wxT("%ld"), (uintptr_t)this)
            + wxT("&object_address=") + wxString::Format(wxT("%ld"), (uintptr_t)c));
        getURIProcessor().handleURI(uri);
    }
    else
        frameManager().showMetadataPropertyFrame(this, m);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuAlterObject(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Database *db = tree_ctrl_1->getSelectedDatabase();
    Procedure* p = dynamic_cast<Procedure *>(tree_ctrl_1->getSelectedMetadataItem());
    View* v = dynamic_cast<View *>(tree_ctrl_1->getSelectedMetadataItem());
    Trigger* t = dynamic_cast<Trigger *>(tree_ctrl_1->getSelectedMetadataItem());
    if (!db || !p && !v && !t)
        return;

    ExecuteSqlFrame* eff = new ExecuteSqlFrame(this, -1, wxString(_("Alter object")));
    eff->setDatabase(db);
    if (p)
        eff->setSql(p->getAlterSql());
    else if (v)
        eff->setSql(v->getRebuildSql());
    else if (t)
        eff->setSql(t->getAlterSql());
    eff->Show();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void MainFrame::OnMenuDropDatabase(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    Database* d = tree_ctrl_1->getSelectedDatabase();

    if (!checkValidDatabase(d) || !confirmDropItem(d))
        return;

    int result = wxMessageBox(
        _("Do you wish to keep the registration info?"),
        _("Dropping database: ")+d->getName_(),
        wxYES_NO|wxCANCEL|wxICON_QUESTION);
    if (result == wxCANCEL)
        return;
    if (!d->drop())
    {
        reportLastError(_("Error dropping database."));
        return;
    }
    d->disconnect(true);    // true = just remove the child nodes
    if (result == wxNO)
    {   // unregister
        Server* s = d->getServer();
        if (s)
            s->removeDatabase(d);
    }

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
    if (!confirmDropItem(m))
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
void MainFrame::OnMenuUpdateIfMetadataItemHasChildren(wxUpdateUIEvent& event)
{
    FR_TRY

    MetadataItem* mi = tree_ctrl_1->getSelectedMetadataItem();
    event.Enable(mi != 0 && mi->getChildrenCount());

    FR_CATCH
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
