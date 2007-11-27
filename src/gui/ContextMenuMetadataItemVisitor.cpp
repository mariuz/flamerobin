/*
  Copyright (c) 2004-2007 The FlameRobin Development Team

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
#include "gui/CommandIds.h"
#include "gui/ContextMenuMetadataItemVisitor.h"
#include "metadata/database.h"
#include "metadata/table.h"
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
void ContextMenuMetadataItemVisitor::visitColumn(Column& column)
{
    Table* t = column.getTable();
    if (t && !t->isSystem()) // only for columns of non-system tables
        addRegularObjectMenu(false, true);
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitDatabase(Database&)
{
    menuM->Append(Cmds::Menu_Connect, _("&Connect"));
    menuM->Append(Cmds::Menu_ConnectAs, _("Connect &as..."));
    menuM->Append(Cmds::Menu_Disconnect, _("&Disconnect"));
    menuM->Append(Cmds::Menu_Reconnect, _("Reconnec&t"));
    menuM->Append(Cmds::Menu_Query, _("&Run a query..."));
    menuM->AppendSeparator();

    wxMenu* actions = new wxMenu();
    menuM->Append(0, _("Acti&ons"), actions);

    wxMenu* advanced = new wxMenu();
    menuM->Append(0, _("Ad&vanced"), advanced);

    menuM->Append(Cmds::Menu_DatabaseRegistrationInfo, _("Database registration &info..."));
    menuM->Append(Cmds::Menu_UnRegisterDatabase, _("&Unregister database"));

    // the actions submenu
    actions->Append(Cmds::Menu_DropDatabase, _("Drop database"));
    actions->Append(Cmds::Menu_Backup, _("&Backup database..."));
    actions->Append(Cmds::Menu_Restore, _("Rest&ore database..."));

    // the advanced submenu
    advanced->Append(Cmds::Menu_ShowConnectedUsers, _("&Show connected users"));
    advanced->Append(Cmds::Menu_MonitorEvents, _("&Monitor events"));
    advanced->Append(Cmds::Menu_DatabasePreferences, _("Database &preferences..."));
    advanced->Append(Cmds::Menu_GenerateData, _("&Test data generator..."));
    advanced->Append(Cmds::Menu_ExtractDatabaseDDL, _("&Extract metadata DDL..."));

    menuM->AppendSeparator();
    menuM->Append(Cmds::Menu_DatabaseProperties, _("Data&base Properties"));
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitDomain(Domain&)
{
    addRegularObjectMenu(false, true);
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitException(Exception&)
{
    addRegularObjectMenu(false, true);
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitFunction(Function&)
{
    addRegularObjectMenu(false, true);
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitGenerator(Generator&)
{
    menuM->Append(Cmds::Menu_ShowGeneratorValue, _("Show &value"));
    menuM->Append(Cmds::Menu_SetGeneratorValue, _("&Set value"));
    addRegularObjectMenu(false, true);
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitMetadataItem(MetadataItem& metadataItem)
{
    NodeType type = metadataItem.getType();
    if (type == ntFunctions)
    {
        menuM->Append(Cmds::Menu_CreateObject, _("&Declare new..."));
        return;
    }

    if (type == ntGenerators)
        menuM->Append(Cmds::Menu_ShowAllGeneratorValues, _("Show &all values"));

    if (type == ntGenerators || type == ntTables || type == ntViews || type == ntProcedures ||
        type == ntTriggers || type == ntDomains || type == ntRoles || type == ntExceptions)
            menuM->Append(Cmds::Menu_CreateObject, _("Create &new..."));
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitProcedure(Procedure&)
{
    menuM->Append(Cmds::Menu_ExecuteProcedure, _("&Execute..."));
    menuM->AppendSeparator();
    addSelectMenu(false, false); // selectable?, can not add columns
    addRegularObjectMenu(true, true); // add Alter and Drop menu
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitRole(Role&)
{
    addRegularObjectMenu(false, true);
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitRoot(Root&)
{
    menuM->Append(Cmds::Menu_RegisterServer, _("&Register server..."));
    menuM->AppendSeparator();
    menuM->Append(wxID_ABOUT, _("&About FlameRobin..."));
    menuM->Append(wxID_PREFERENCES, _("&Preferences..."));
    menuM->AppendSeparator();
    menuM->Append(wxID_EXIT, _("&Quit"));
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitServer(Server&)
{
    menuM->Append(Cmds::Menu_RegisterDatabase, _("&Register existing database..."));
    menuM->Append(Cmds::Menu_CreateDatabase, _("Create &new database..."));
    menuM->Append(Cmds::Menu_RestoreIntoNew, _("Restore bac&kup into new database..."));
    menuM->AppendSeparator();
    menuM->Append(Cmds::Menu_GetServerVersion, _("Retrieve server &version"));
    menuM->Append(Cmds::Menu_ManageUsers, _("&Manage users..."));
    menuM->AppendSeparator();
    menuM->Append(Cmds::Menu_UnRegisterServer, _("&Unregister server"));
    menuM->Append(Cmds::Menu_ServerProperties, _("Server registration &info..."));
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitTable(Table& table)
{
    // When a system table object is visited, only the select and regular object
    // menus should be used. No modifications should be made to system objects,
    // at least not from FR gui.
    bool isSystem = table.isSystem();
    if (!isSystem)
        menuM->Append(Cmds::Menu_Insert, _("&Insert into ..."));
    addSelectMenu(true, !isSystem); // selectable, can add columns if user
    if (!isSystem)
    {
        menuM->Append(Cmds::Menu_CreateTriggerForTable,
            _("Create new &trigger..."));
        menuM->Append(Cmds::Menu_CreateProcedureForTable,
            _("Create selectable &procedure"));
    }
    addRegularObjectMenu(false, !isSystem);
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitTrigger(Trigger&)
{
    addRegularObjectMenu(true, true); // add Alter and Drop menu
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitView(View&)
{
    addSelectMenu(true, false); // selectable, can not add columns
    addRegularObjectMenu(true, true); // add Alter and Drop menu
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::addSelectMenu(bool isSelectable,
    bool canAddColumn)
{
    unsigned added = 0;
    if (isSelectable)
    {
        menuM->Append(Cmds::Menu_BrowseColumns, _("&Select from ..."));
        ++added;
    }
    if (config().get(wxT("ShowColumnsInTree"), true))
    {
        if (added)
            menuM->AppendSeparator(); // between "Select from" and this one
        menuM->Append(Cmds::Menu_LoadColumnsInfo, _("Show columns in&fo"));
        ++added;
    }
    if (canAddColumn)
    {
        if (isSelectable && added < 2)
            menuM->AppendSeparator(); // between "Select from" and this one
        menuM->Append(Cmds::Menu_AddColumn, _("&Add column..."));
        ++added;
    }
    if (added)
        menuM->AppendSeparator();
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::addRegularObjectMenu(bool addAlter,
    bool addDrop)
{
    if (addAlter)
        menuM->Append(Cmds::Menu_AlterObject, _("&Alter..."));
    if (addDrop)
        menuM->Append(Cmds::Menu_DropObject, _("Dr&op..."));
    menuM->Append(Cmds::Menu_ObjectProperties, _("Show P&roperties"));
}
//-----------------------------------------------------------------------------
