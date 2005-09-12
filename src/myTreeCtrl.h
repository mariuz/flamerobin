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

  Contributor(s):
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

protected:
    short m_spacing;    // fix wxWidgets bug (or lack of feature)

public:
    enum { ID_tree_ctrl = 101 };

    short GetSpacing() const { return m_spacing; }
    void SetSpacing(short spacing);

    enum {
        Menu_RegisterServer = 600, Menu_Manual, Menu_RelNotes, Menu_License,
        Menu_NewObject, Menu_ToggleStatusBar, Menu_ToggleDisconnected,
        Menu_DatabaseRegistrationInfo, Menu_RegisterDatabase,
        Menu_CreateDatabase, Menu_ManageUsers, Menu_UnRegisterServer,
        Menu_ServerProperties, Menu_Reconnect, Menu_ConnectAs,
        Menu_ExecuteProcedure, Menu_UnRegisterDatabase, Menu_Backup,
        Menu_Restore, Menu_Connect, Menu_Disconnect, Menu_Query,
        Menu_ShowConnectedUsers, Menu_CreateObject, Menu_DatabasePreferences,
        Menu_ShowAllGeneratorValues, Menu_Browse, Menu_BrowseColumns,
        Menu_Insert, Menu_LoadColumnsInfo, Menu_ObjectProperties,
        Menu_DropObject, Menu_ShowGeneratorValue, Menu_SetGeneratorValue,
        Menu_AddColumn, Menu_CreateTriggerForTable, Menu_RestoreIntoNew,

        // create new ... (stuff)
        Menu_CreateDomain, Menu_CreateException, Menu_CreateFunction,
        Menu_CreateGenerator, Menu_CreateProcedure, Menu_CreateRole,
        Menu_CreateTable, Menu_CreateTrigger, Menu_CreateView
    };

    void OnBeginDrag(wxTreeEvent &event);
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

    myTreeCtrl(wxWindow* parent, const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize, long style = wxTR_HAS_BUTTONS);

    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif
