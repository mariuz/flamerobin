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

#ifndef MAINFRAME_H
#define MAINFRAME_H
//--------------------------------------------------------------------------------------
#include <wx/wx.h>
#include <wx/image.h>
#include <wx/treectrl.h>

#include <vector>

#include "gui/BaseFrame.h"
#include "metadata/generator.h"
#include "metadata/metadataitem.h"
#include "metadata/root.h"
#include "myTreeCtrl.h"
//-----------------------------------------------------------------------------
class MainFrame: public BaseFrame
{
public:
    // menu handling events
    void OnMenuRegisterServer(wxCommandEvent& event);
    void OnMenuQuit(wxCommandEvent& event);
    void OnMenuAbout(wxCommandEvent& event);
    void OnMenuManual(wxCommandEvent& event);
    void OnMenuRelNotes(wxCommandEvent& event);
    void OnMenuLicense(wxCommandEvent& event);
    void OnMenuConfigure(wxCommandEvent& event);
    void OnMenuRegisterDatabase(wxCommandEvent& event);
    void OnMenuDatabaseRegistrationInfo(wxCommandEvent& event);
    void OnMenuCreateDatabase(wxCommandEvent& event);
    void OnMenuDropDatabase(wxCommandEvent& event);
    void OnMenuRestoreIntoNewDatabase(wxCommandEvent& event);
    void OnMenuManageUsers(wxCommandEvent& event);
    void OnMenuRestartServer(wxCommandEvent& event);
    void OnMenuStopServer(wxCommandEvent& event);
    void OnMenuUnRegisterServer(wxCommandEvent& event);
    void OnMenuServerProperties(wxCommandEvent& event);
    void OnMenuUnRegisterDatabase(wxCommandEvent& event);
    void OnMenuShowConnectedUsers(wxCommandEvent& event);
    void OnMenuGetServerVersion(wxCommandEvent& event);
    void OnMenuMonitorEvents(wxCommandEvent& event);
    void OnMenuBackup(wxCommandEvent& event);
    void OnMenuQuery(wxCommandEvent& event);
    void OnMenuInsert(wxCommandEvent& event);
    void OnMenuBrowseColumns(wxCommandEvent& event);
    void OnMenuBrowse(wxCommandEvent& event);
    void OnMenuRestore(wxCommandEvent& event);
    void OnMenuShowAllGeneratorValues(wxCommandEvent& event);
    void OnMenuShowGeneratorValue(wxCommandEvent& event);
    void OnMenuSetGeneratorValue(wxCommandEvent& event);
    void OnMenuToggleStatusBar(wxCommandEvent& event);
    void OnMenuToggleSearchBar(wxCommandEvent& event);
    void OnMenuToggleDisconnected(wxCommandEvent& event);
    void OnMenuCreateObject(wxCommandEvent& event);
    void OnMenuLoadColumnsInfo(wxCommandEvent& event);
    void OnMenuAddColumn(wxCommandEvent& event);
    void OnMenuObjectProperties(wxCommandEvent& event);
    void OnMenuDropObject(wxCommandEvent& event);
    void OnMenuAlterObject(wxCommandEvent& event);
    void OnMenuCreateTriggerForTable(wxCommandEvent& event);
    void OnMenuExecuteProcedure(wxCommandEvent& event);
    void OnMenuDisconnect(wxCommandEvent& event);
    void OnMenuConnect(wxCommandEvent& event);
    void OnMenuConnectAs(wxCommandEvent& event);
    void OnMenuReconnect(wxCommandEvent& event);
    void OnMenuDatabasePreferences(wxCommandEvent& event);
    void OnMenuDatabaseProperties(wxCommandEvent& event);

    // create new object
    void showCreateTemplate(const MetadataItem *m);
    void OnMenuCreateDomain(wxCommandEvent& event);
    void OnMenuCreateException(wxCommandEvent& event);
    void OnMenuCreateFunction(wxCommandEvent& event);
    void OnMenuCreateGenerator(wxCommandEvent& event);
    void OnMenuCreateProcedure(wxCommandEvent& event);
    void OnMenuCreateRole(wxCommandEvent& event);
    void OnMenuCreateTable(wxCommandEvent& event);
    void OnMenuCreateTrigger(wxCommandEvent& event);
    void OnMenuCreateView(wxCommandEvent& event);

    // enabled menu items
    void OnMenuUpdateUnRegisterServer(wxUpdateUIEvent& event);
    void OnMenuUpdateIfServerSelected(wxUpdateUIEvent& event);
    void OnMenuUpdateIfDatabaseConnected(wxUpdateUIEvent& event);
    void OnMenuUpdateIfDatabaseNotConnected(wxUpdateUIEvent& event);
    void OnMenuUpdateIfDatabaseSelected(wxUpdateUIEvent& event);

    // other events
    void OnWindowMenuItem(wxCommandEvent& event);
    void OnMainMenuOpen(wxMenuEvent& event);
    void OnTreeSelectionChanged(wxTreeEvent& event);
    void OnTreeItemActivate(wxTreeEvent& event);
    void OnClose(wxCloseEvent& event);

    // search stuff (IDs 600+ are taken!)
    enum {
        ID_button_advanced = 401,
        ID_button_prev,
        ID_button_next,
        ID_search_box
    };
    void OnSearchTextChange(wxCommandEvent& event);
    void OnSearchBoxEnter(wxCommandEvent& event);
    void OnButtonSearchClick(wxCommandEvent &event);
    void OnButtonPrevClick(wxCommandEvent &event);
    void OnButtonNextClick(wxCommandEvent &event);

    myTreeCtrl* getTreeCtrl();
    MainFrame(wxWindow* parent, int id, const wxString& title, const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize, long style = wxDEFAULT_FRAME_STYLE);

private:
    bool connect();
    void showGeneratorValue(Generator* g);
    void updateStatusbarText();

    void showDocsHtmlFile(const wxString& fileName);

    void set_properties();
    void do_layout();
    void buildMainMenu();

protected:
    myTreeCtrl* tree_ctrl_1;
    wxMenuBar* menuBarM;
    wxMenu* windowMenuM;        // dynamic menu
    wxMenu* objectMenuM;
    wxPanel* mainPanelM;
    wxPanel* searchPanelM;
    wxBoxSizer* sizer_search;
    wxBoxSizer* inner_sizer;
    wxComboBox* searchBoxM;
    wxBitmapButton* button_prev;
    wxBitmapButton* button_next;
    wxBitmapButton* button_advanced;

    virtual const wxString getName() const;
    virtual const wxRect getDefaultRect() const;
    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif // MAINFRAME_H
