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
#include "contextmenuvisitor.h"
#include "myTreeCtrl.h"
//-----------------------------------------------------------------------------
ContextMenuVisitor::ContextMenuVisitor(wxMenu* menu)
    :Visitor(), menuM(menu)
{
}
//-----------------------------------------------------------------------------
ContextMenuVisitor::~ContextMenuVisitor()
{
}
//-----------------------------------------------------------------------------
void ContextMenuVisitor::visit(Column& item)
{
    if (dynamic_cast<Table *>(item.getParent()))        // only for table columns
        addRegularObjectMenu();
}
//-----------------------------------------------------------------------------
void ContextMenuVisitor::visit(Database&)
{
    menuM->Append(myTreeCtrl::Menu_Connect, _("&Connect"));
    menuM->Append(myTreeCtrl::Menu_ConnectAs, _("Connect &as..."));
    menuM->Append(myTreeCtrl::Menu_Disconnect, _("&Disconnect"));
    menuM->Append(myTreeCtrl::Menu_Reconnect, _("Reconnec&t"));
    menuM->Append(myTreeCtrl::Menu_Query, _("&Run a query..."));
    menuM->AppendSeparator();
    menuM->Append(myTreeCtrl::Menu_ShowConnectedUsers, _("&Show connected users"));
    menuM->Append(myTreeCtrl::Menu_DatabaseRegistrationInfo, _("Database registration &info..."));
    menuM->Append(myTreeCtrl::Menu_UnRegisterDatabase, _("&Unregister database"));
    menuM->AppendSeparator();
    menuM->Append(myTreeCtrl::Menu_Backup, _("&Backup database..."));
    menuM->Append(myTreeCtrl::Menu_Restore, _("Rest&ore database..."));
}
//-----------------------------------------------------------------------------
void ContextMenuVisitor::visit(Domain&)
{
    addRegularObjectMenu();
}
//-----------------------------------------------------------------------------
void ContextMenuVisitor::visit(Exception&)
{
    addRegularObjectMenu();
}
//-----------------------------------------------------------------------------
void ContextMenuVisitor::visit(Function&)
{
    addRegularObjectMenu();
}
//-----------------------------------------------------------------------------
void ContextMenuVisitor::visit(Generator&)
{
    menuM->Append(myTreeCtrl::Menu_ShowGeneratorValue, _("Show &value"));
    menuM->Append(myTreeCtrl::Menu_SetGeneratorValue, _("&Set value"));
    addRegularObjectMenu();
}
//-----------------------------------------------------------------------------
void ContextMenuVisitor::visit(Procedure&)
{
    addSelectMenu(false);    // false = not a table
    addRegularObjectMenu();
}
//-----------------------------------------------------------------------------
void ContextMenuVisitor::visit(Role&)
{
    addRegularObjectMenu();
}
//-----------------------------------------------------------------------------
void ContextMenuVisitor::visit(Server&)
{
    menuM->Append(myTreeCtrl::Menu_RegisterDatabase, _("&Register existing database..."));
    menuM->Append(myTreeCtrl::Menu_CreateDatabase, _("Create &new database..."));
    menuM->Append(myTreeCtrl::Menu_ManageUsers, _("&Manage users..."));
    menuM->AppendSeparator();
    menuM->Append(myTreeCtrl::Menu_UnRegisterServer, _("&Unregister server"));
    menuM->Append(myTreeCtrl::Menu_ServerProperties, _("Server registration &info..."));
}
//-----------------------------------------------------------------------------
void ContextMenuVisitor::visit(Table&)
{
    menuM->Append(myTreeCtrl::Menu_Insert, _("&Insert into ..."));
    addSelectMenu(true);
    menuM->Append(myTreeCtrl::Menu_CreateTriggerForTable, _("Create new &trigger..."));
    addRegularObjectMenu();
}
//-----------------------------------------------------------------------------
void ContextMenuVisitor::visit(Trigger&)
{
    addRegularObjectMenu();
}
//-----------------------------------------------------------------------------
void ContextMenuVisitor::visit(View&)
{
    addSelectMenu(false);    // false = not a table
    addRegularObjectMenu();
}
//-----------------------------------------------------------------------------
void ContextMenuVisitor::visit(MetadataItem &item)
{
    NodeType type = item.getType();
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
void ContextMenuVisitor::addSelectMenu(bool isTable)
{
    menuM->Append(myTreeCtrl::Menu_Browse, _("&Select * from ..."));
    menuM->Append(myTreeCtrl::Menu_BrowseColumns, _("Select &col1, col2, ... from ..."));
    menuM->AppendSeparator();
    if (config().get("ShowColumnsInTree", true))
    {
        menuM->Append(myTreeCtrl::Menu_LoadColumnsInfo, _("Show columns in&fo"));
        if (isTable)
            menuM->Append(myTreeCtrl::Menu_AddColumn, _("&Add column..."));
        menuM->AppendSeparator();
    }
}
//-----------------------------------------------------------------------------
void ContextMenuVisitor::addRegularObjectMenu()
{
    menuM->Append(myTreeCtrl::Menu_DropObject, _("Dr&op"));
    menuM->Append(myTreeCtrl::Menu_ObjectProperties, _("P&roperties..."));
}
//-----------------------------------------------------------------------------
