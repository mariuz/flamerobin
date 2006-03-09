//-----------------------------------------------------------------------------
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

  The Initial Developer of the Original Code is Gregory Sapunkov.

  Portions created by Gregory Sapunkov
  are Copyright (C) 2004, 2005 Gregory Sapunkov.

  All Rights Reserved.

  $Id$

  Contributor(s): Milan Babuskov, Nando Dessena
*/
//-----------------------------------------------------------------------------
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/menu.h>

#include "config/Config.h"
#include "ContextMenuMetadataItemVisitor.h"
#include "myTreeCtrl.h"
//-----------------------------------------------------------------------------
ContextMenuMetadataItemVisitor::ContextMenuMetadataItemVisitor(wxMenu* menu)
    : MetadataItemVisitor(), menuM(menu)
{
}
//-----------------------------------------------------------------------------
ContextMenuMetadataItemVisitor::~ContextMenuMetadataItemVisitor()
{
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visit(Column& column)
{
    if (column.getTable()) // only for table columns
        addRegularObjectMenu();
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visit(Database&)
{
    menuM->Append(myTreeCtrl::Menu_Connect, _("&Connect"));
    menuM->Append(myTreeCtrl::Menu_ConnectAs, _("Connect &as..."));
    menuM->Append(myTreeCtrl::Menu_Disconnect, _("&Disconnect"));
    menuM->Append(myTreeCtrl::Menu_Reconnect, _("Reconnec&t"));
    menuM->Append(myTreeCtrl::Menu_Query, _("&Run a query..."));
    menuM->AppendSeparator();
    menuM->Append(myTreeCtrl::Menu_ShowConnectedUsers, _("&Show connected users"));
    menuM->Append(myTreeCtrl::Menu_MonitorEvents, _("&Monitor events"));
    menuM->Append(myTreeCtrl::Menu_DatabaseRegistrationInfo, _("Database registration &info..."));
    menuM->Append(myTreeCtrl::Menu_UnRegisterDatabase, _("&Unregister database"));
    menuM->AppendSeparator();
    menuM->Append(myTreeCtrl::Menu_Backup, _("&Backup database..."));
    menuM->Append(myTreeCtrl::Menu_Restore, _("Rest&ore database..."));
    menuM->Append(myTreeCtrl::Menu_DropDatabase, _("Drop database"));
    menuM->AppendSeparator();
    menuM->Append(myTreeCtrl::Menu_DatabasePreferences, _("Database &preferences..."));
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visit(Domain&)
{
    addRegularObjectMenu();
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visit(Exception&)
{
    addRegularObjectMenu();
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visit(Function&)
{
    addRegularObjectMenu();
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visit(Generator&)
{
    menuM->Append(myTreeCtrl::Menu_ShowGeneratorValue, _("Show &value"));
    menuM->Append(myTreeCtrl::Menu_SetGeneratorValue, _("&Set value"));
    addRegularObjectMenu();
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visit(MetadataItem& metadataItem)
{
    NodeType type = metadataItem.getType();
    if (type == ntFunctions)
    {
        menuM->Append(myTreeCtrl::Menu_CreateObject, _("&Declare new..."));
        return;
    }

    if (type == ntGenerators)
        menuM->Append(myTreeCtrl::Menu_ShowAllGeneratorValues, _("Show &all values"));

    if (type == ntGenerators || type == ntTables || type == ntViews || type == ntProcedures ||
        type == ntTriggers || type == ntDomains || type == ntRoles || type == ntExceptions)
            menuM->Append(myTreeCtrl::Menu_CreateObject, _("Create &new..."));
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visit(Procedure&)
{
    menuM->Append(myTreeCtrl::Menu_ExecuteProcedure, _("&Execute..."));
    addSelectMenu(false);       // false = not a table
    addRegularObjectMenu(true); // true = add Alter menu
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visit(Role&)
{
    addRegularObjectMenu();
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visit(Root&)
{
    menuM->Append(myTreeCtrl::Menu_RegisterServer, _("&Register server..."));
    menuM->AppendSeparator();
    menuM->Append(wxID_ABOUT, _("&About FlameRobin..."));
    menuM->Append(wxID_PREFERENCES, _("&Preferences..."));
    menuM->AppendSeparator();
    menuM->Append(wxID_EXIT, _("&Quit"));
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visit(Server&)
{
    menuM->Append(myTreeCtrl::Menu_RegisterDatabase, _("&Register existing database..."));
    menuM->Append(myTreeCtrl::Menu_CreateDatabase, _("Create &new database..."));
    menuM->Append(myTreeCtrl::Menu_RestoreIntoNew, _("Restore bac&kup into new database..."));
    menuM->AppendSeparator();
    menuM->Append(myTreeCtrl::Menu_GetServerVersion, _("Retrieve server &version"));
    menuM->Append(myTreeCtrl::Menu_ManageUsers, _("&Manage users..."));
    menuM->AppendSeparator();
    menuM->Append(myTreeCtrl::Menu_UnRegisterServer, _("&Unregister server"));
    menuM->Append(myTreeCtrl::Menu_ServerProperties, _("Server registration &info..."));
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visit(Table&)
{
    menuM->Append(myTreeCtrl::Menu_Insert, _("&Insert into ..."));
    addSelectMenu(true);
    menuM->Append(myTreeCtrl::Menu_CreateTriggerForTable, _("Create new &trigger..."));
    addRegularObjectMenu();
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visit(Trigger&)
{
    addRegularObjectMenu(true); // true = add Alter menu
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visit(View&)
{
    addSelectMenu(false);       // false = not a table
    addRegularObjectMenu(true); // true = add Alter menu
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::addSelectMenu(bool isTable)
{
    menuM->Append(myTreeCtrl::Menu_Browse, _("&Select * from ..."));
    menuM->Append(myTreeCtrl::Menu_BrowseColumns, _("Select &col1, col2, ... from ..."));
    menuM->AppendSeparator();
    if (config().get(wxT("ShowColumnsInTree"), true))
    {
        menuM->Append(myTreeCtrl::Menu_LoadColumnsInfo, _("Show columns in&fo"));
        if (isTable)
            menuM->Append(myTreeCtrl::Menu_AddColumn, _("&Add column..."));
        menuM->AppendSeparator();
    }
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::addRegularObjectMenu(bool alter)
{
    if (alter)
        menuM->Append(myTreeCtrl::Menu_AlterObject, _("&Alter..."));
    menuM->Append(myTreeCtrl::Menu_DropObject, _("Dr&op..."));
    menuM->Append(myTreeCtrl::Menu_ObjectProperties, _("Show P&roperties"));
}
//-----------------------------------------------------------------------------
