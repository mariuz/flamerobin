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

#ifndef MAINFRAME_H
#define MAINFRAME_H

#include <wx/wx.h>
#include <wx/image.h>
#include <wx/treectrl.h>
#include <wx/aui/aui.h>

#include <vector>

#include "gui/BaseFrame.h"
#include "gui/GUIURIHandlerHelper.h"
#include "metadata/MetadataClasses.h"
#include "metadata/MetadataItemURIHandlerHelper.h"

class DBHTreeControl;
class LabelPanel;
class TemplateProcessor;
class wxFileName;

class MainFrame: public BaseFrame, private URIHandler,
    private MetadataItemURIHandlerHelper, private GUIURIHandlerHelper
{
public:
    // menu handling events
    void OnMenuRegisterServer(wxCommandEvent& event);
    void OnMenuQuit(wxCommandEvent& event);
    void OnMenuAbout(wxCommandEvent& event);
    void OnMenuManual(wxCommandEvent& event);
    void OnMenuRelNotes(wxCommandEvent& event);
    void OnMenuLicense(wxCommandEvent& event);
    void OnMenuURLHomePage(wxCommandEvent& event);
    void OnMenuURLProjectPage(wxCommandEvent& event);
    void OnMenuURLFeatureRequest(wxCommandEvent& event);
    void OnMenuURLBugReport(wxCommandEvent& event);
    void OnMenuConfigure(wxCommandEvent& event);
    void OnMenuNewVolatileSQLEditor(wxCommandEvent& event);
    void OnMenuRegisterDatabase(wxCommandEvent& event);
    void OnMenuCloneDatabase(wxCommandEvent& event);
    void OnMenuDatabaseRegistrationInfo(wxCommandEvent& event);
    void OnMenuCreateDatabase(wxCommandEvent& event);
    void OnMenuRecreateDatabase(wxCommandEvent& event);
    void OnMenuDropDatabase(wxCommandEvent& event);
    void OnMenuRestoreIntoNewDatabase(wxCommandEvent& event);
    void OnMenuManageUsers(wxCommandEvent& event);
    void OnMenuUnRegisterServer(wxCommandEvent& event);
    void OnMenuServerProperties(wxCommandEvent& event);
    void OnMenuUnRegisterDatabase(wxCommandEvent& event);
    void OnMenuGetServerVersion(wxCommandEvent& event);
    void OnMenuMonitorEvents(wxCommandEvent& event);
    void OnMenuGenerateData(wxCommandEvent& event);
    void OnMenuBackup(wxCommandEvent& event);
    void OnMenuExecuteStatements(wxCommandEvent& event);
    void OnMenuInsert(wxCommandEvent& event);
    void OnMenuBrowseData(wxCommandEvent& event);
    void OnMenuRestore(wxCommandEvent& event);
    void OnMenuShowAllGeneratorValues(wxCommandEvent& event);
    void OnMenuShowGeneratorValue(wxCommandEvent& event);
    void OnMenuSetGeneratorValue(wxCommandEvent& event);
    void OnMenuToggleStatusBar(wxCommandEvent& event);
    void OnMenuToggleSearchBar(wxCommandEvent& event);
    void OnMenuToggleDisconnected(wxCommandEvent& event);
    void OnMenuCreateObject(wxCommandEvent& event);
    void OnMenuAddColumn(wxCommandEvent& event);
    void OnMenuObjectProperties(wxCommandEvent& event);
    void OnMenuObjectRefresh(wxCommandEvent& event);
    void OnMenuDropObject(wxCommandEvent& event);
    void OnMenuAlterObject(wxCommandEvent& event);
    void OnMenRebuildObject(wxCommandEvent& event);
    void OnMenuCreateTriggerForTable(wxCommandEvent& event);
    void OnMenuGenerateCode(wxCommandEvent& event);
    void OnMenuExecuteProcedure(wxCommandEvent& event);
    void OnMenuDisconnect(wxCommandEvent& event);
    void OnMenuConnect(wxCommandEvent& event);
    void OnMenuConnectAs(wxCommandEvent& event);
    void OnMenuReconnect(wxCommandEvent& event);
    void OnMenuDatabasePreferences(wxCommandEvent& event);
    void OnMenuDatabaseProperties(wxCommandEvent& event);
    void OnMenuExecuteFunction(wxCommandEvent& event);
    void OnMenActiveObject(wxCommandEvent& event);
    void OnMenInactiveObject(wxCommandEvent& event);
    void OnMenuShowAllStatisticsValues(wxCommandEvent& event);
    void OnMenuShowStatisticsValue(wxCommandEvent& event);
    void OnMenuSetStatisticsValue(wxCommandEvent& event);
    void OnMenuShutdownDatabase(wxCommandEvent& event);
    void OnMenuStartupDatabase(wxCommandEvent& event);

    // create new object
    void showCreateTemplate(const wxString& statement);
    void OnMenuCreateCollation(wxCommandEvent& event);
    void OnMenuCreateDBTrigger(wxCommandEvent& event);
    void OnMenuCreateDDLTrigger(wxCommandEvent& event);
    void OnMenuCreateDMLTrigger(wxCommandEvent& event);
    void OnMenuCreateDomain(wxCommandEvent& event);
    void OnMenuCreateException(wxCommandEvent& event);
    void OnMenuCreateFunction(wxCommandEvent& event);
    void OnMenuCreateIndex(wxCommandEvent& event);
    void OnMenuCreateGenerator(wxCommandEvent& event);
    void OnMenuCreateGTTTable(wxCommandEvent& event);
    void OnMenuCreatePackage(wxCommandEvent& event);
    void OnMenuCreateProcedure(wxCommandEvent& event);
    void OnMenuCreateRole(wxCommandEvent& event);
    void OnMenuCreateTable(wxCommandEvent& event);
    void OnMenuCreateUDF(wxCommandEvent& event);
    void OnMenuCreateUser(wxCommandEvent& event);
    void OnMenuCreateView(wxCommandEvent& event);

    // enabled menu items
    void OnMenuUpdateUnRegisterServer(wxUpdateUIEvent& event);
    void OnMenuUpdateIfServerSelected(wxUpdateUIEvent& event);
    void OnMenuUpdateIfDatabaseConnected(wxUpdateUIEvent& event);
    void OnMenuUpdateIfDatabaseConnectedOrAutoConnect(wxUpdateUIEvent& event);
    void OnMenuUpdateIfDatabaseNotConnected(wxUpdateUIEvent& event);
    void OnMenuUpdateIfDatabaseSelected(wxUpdateUIEvent& event);
    void OnMenuUpdateIfMetadataItemHasChildren(wxUpdateUIEvent& event);

    // other events
    void OnMainMenuOpen(wxMenuEvent& event);
    void OnTreeSelectionChanged(wxTreeEvent& event);
    void OnTreeItemActivate(wxTreeEvent& event);
    void OnSetFocus(wxFocusEvent& event);

    // search stuff (IDs 600+ are taken!)
    enum {
        ID_button_advanced = 401,
        ID_button_prev,
        ID_button_next,
        ID_search_box,
        ID_notebook
    };
    void OnSearchTextChange(wxCommandEvent& event);
    void OnSearchBoxEnter(wxCommandEvent& event);
    void OnButtonSearchClick(wxCommandEvent &event);
    void OnButtonPrevClick(wxCommandEvent &event);
    void OnButtonNextClick(wxCommandEvent &event);

    DBHTreeControl* getTreeCtrl();
    MainFrame(wxWindow* parent, int id, const wxString& title, const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize, long style = wxDEFAULT_FRAME_STYLE);

    bool openUnregisteredDatabase(const wxString& dbpath);
    bool handleURI(URI& uri);
private:
    RootPtr rootM;

    virtual bool doCanClose();
    virtual void doBeforeDestroy();

    bool getAutoConnectDatabase();
    bool tryAutoConnectDatabase();
    bool tryAutoConnectDatabase(DatabasePtr database);

    void unregisterDatabase(DatabasePtr database);

    bool connect();
    void showGeneratorValue(Generator* g);
    void updateStatusbarText();

    void showDocsHtmlFile(const wxString& fileName);
    void showUrl(const wxString& url);

    void set_properties();
    void do_layout();
    void buildMainMenu();

    bool confirmDropItem(MetadataItem* item);
    bool confirmDropDatabase(Database* db);

    void executeSysTemplate(const wxString& name, MetadataItem* item,
        wxWindow* parentWindow);
    void handleTemplateOutput(TemplateProcessor& tp,
        DatabasePtr database, const wxString& code);
    void executeCodeTemplate(const wxFileName& fileName,
        MetadataItem* item, DatabasePtr database);
protected:
    DBHTreeControl* treeMainM;
    wxMenuBar* menuBarM;
    wxMenu* windowMenuM;        // dynamic menu
    wxMenu* objectMenuM;
    wxMenu* databaseMenuM;
    wxPanel* mainPanelM;
    wxPanel* searchPanelM;
    wxBoxSizer* searchPanelSizerM;
    wxComboBox* searchBoxM;
    wxBitmapButton* button_prev;
    wxBitmapButton* button_next;
    wxBitmapButton* button_advanced;

    virtual const wxString getName() const;
    virtual const wxRect getDefaultRect() const;
    DECLARE_EVENT_TABLE()
};

#endif // MAINFRAME_H
