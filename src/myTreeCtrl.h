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

#ifndef FR_MYTREECTRL_H
#define FR_MYTREECTRL_H

#include <wx/wx.h>
#include <wx/treectrl.h>

#include <map>
#include "metadata/metadataitem.h"

class Database;
class Server;
//-----------------------------------------------------------------------------
class myTreeCtrl: public wxTreeCtrl
{
private:
    std::map<int, int> imageMapM;
    void loadImages();
    // recursive function used by selectMetadataItem
    bool findMetadataItem(MetadataItem *item, wxTreeItemId parent);

protected:
    short m_spacing;    // fix wxWidgets bug (or lack of feature)

public:
    enum { ID_tree_ctrl = 101 };

    short GetSpacing() const { return m_spacing; }
    void SetSpacing(short spacing);

    enum {
        Menu_RegisterServer = 600, Menu_Manual, Menu_RelNotes, Menu_License,
        Menu_URLHomePage, Menu_URLProjectPage, Menu_URLFeatureRequest,
        Menu_URLBugReport,
        Menu_NewObject, Menu_DatabaseRegistrationInfo, Menu_RegisterDatabase,
        Menu_CreateDatabase, Menu_ManageUsers, Menu_UnRegisterServer,
        Menu_ServerProperties, Menu_Reconnect, Menu_ConnectAs,
        Menu_ExecuteProcedure, Menu_UnRegisterDatabase, Menu_Backup,
        Menu_Restore, Menu_Connect, Menu_Disconnect, Menu_Query,
        Menu_ShowConnectedUsers, Menu_CreateObject, Menu_DatabasePreferences,
        Menu_ShowAllGeneratorValues, Menu_Browse, Menu_BrowseColumns,
        Menu_Insert, Menu_LoadColumnsInfo, Menu_ObjectProperties,
        Menu_DropObject, Menu_ShowGeneratorValue, Menu_SetGeneratorValue,
        Menu_AddColumn, Menu_CreateTriggerForTable, Menu_RestoreIntoNew,
        Menu_MonitorEvents, Menu_GetServerVersion, Menu_AlterObject,
        Menu_DropDatabase, Menu_DatabaseProperties, Menu_GenerateData,

        // view menu
        Menu_ToggleStatusBar, Menu_ToggleSearchBar, Menu_ToggleDisconnected,

        // create new ... (stuff)
        Menu_CreateDomain, Menu_CreateException, Menu_CreateFunction,
        Menu_CreateGenerator, Menu_CreateProcedure, Menu_CreateRole,
        Menu_CreateTable, Menu_CreateTrigger, Menu_CreateView
    };

    void OnBeginDrag(wxTreeEvent &event);
    //void OnMouse(wxMouseEvent &event);
    void OnContextMenu(wxContextMenuEvent& event);

    // Returns observed metadata item based on specified tree item
    MetadataItem *getMetadataItem(wxTreeItemId item);

    // Returns observed metadata item based on currently selected tree item
    MetadataItem *getSelectedMetadataItem();
    Database *getSelectedDatabase();
    Server *getSelectedServer();

    // Selects the tree item represented by the metadata item
    bool selectMetadataItem(MetadataItem* item);
    int getItemImage(NodeType t);

    wxTreeItemId getLastItem(wxTreeItemId id);
    wxTreeItemId getNextItem(wxTreeItemId current);
    wxTreeItemId getPreviousItem(wxTreeItemId current);
    bool findText(const wxString& text, bool forward = true);

    myTreeCtrl(wxWindow* parent, const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize, long style = wxTR_HAS_BUTTONS);

    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif
