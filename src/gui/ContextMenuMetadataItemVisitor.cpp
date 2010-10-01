/*
  Copyright (c) 2004-2010 The FlameRobin Development Team

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

#include <wx/dir.h>
#include <wx/menu.h>

#include "config/Config.h"
#include "gui/CommandIds.h"
#include "gui/ContextMenuMetadataItemVisitor.h"
#include "metadata/database.h"
#include "metadata/table.h"
#include "sql/SqlTemplateManager.h"

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
    if (!column.isSystem())
    {
        addGenerateScriptMenu(column);
        addSeparator();
        addDropItem(column);
        addSeparator();
        addPropertiesItem();
    }
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitDatabase(Database&)
{
    menuM->Append(Cmds::Menu_Connect, _("&Connect"));
    menuM->Append(Cmds::Menu_ConnectAs, _("Connect &as..."));
    menuM->Append(Cmds::Menu_Disconnect, _("&Disconnect"));
    menuM->Append(Cmds::Menu_Reconnect, _("Reconnec&t"));
    menuM->AppendSeparator();
    menuM->Append(Cmds::Menu_Query, _("Execute &SQL statements"));
    menuM->AppendSeparator();

    wxMenu* actions = new wxMenu();
    menuM->Append(0, _("Acti&ons"), actions);

    wxMenu* advanced = new wxMenu();
    menuM->Append(0, _("Ad&vanced"), advanced);

    menuM->Append(Cmds::Menu_DatabaseRegistrationInfo,
        _("Database registration &info"));
    menuM->Append(Cmds::Menu_UnRegisterDatabase, _("&Unregister database"));

    // the actions submenu
    actions->Append(Cmds::Menu_Backup, _("&Backup database"));
    actions->Append(Cmds::Menu_Restore, _("Rest&ore database"));
    actions->AppendSeparator();
    actions->Append(Cmds::Menu_RecreateDatabase, _("Recreate empty database"));
    actions->Append(Cmds::Menu_DropDatabase, _("Drop database"));

    // the advanced submenu
    advanced->Append(Cmds::Menu_ShowConnectedUsers,
        _("&Show connected users"));
    advanced->Append(Cmds::Menu_MonitorEvents, _("&Monitor events"));
    advanced->Append(Cmds::Menu_DatabasePreferences,
        _("Database &preferences..."));
    advanced->Append(Cmds::Menu_GenerateData, _("&Test data generator"));
    advanced->Append(Cmds::Menu_ExtractDatabaseDDL,
        _("&Extract metadata DDL"));

    menuM->AppendSeparator();
    addRefreshItem();
    addPropertiesItem();
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitDomain(Domain& domain)
{
    addGenerateScriptMenu(domain);
    addSeparator();
    addAlterItem(domain);
    addDropItem(domain);
    addSeparator();
    addRefreshItem();
    addPropertiesItem();
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitException(Exception& exception)
{
    addGenerateScriptMenu(exception);
    addSeparator();
    addDropItem(exception);
    addSeparator();
    addRefreshItem();
    addPropertiesItem();
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitFunction(Function& function)
{
    addGenerateScriptMenu(function);
    addSeparator();
    addDropItem(function);
    addSeparator();
    addPropertiesItem();
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitGenerator(Generator& generator)
{
    menuM->Append(Cmds::Menu_ShowGeneratorValue, _("Show &value"));
    menuM->Append(Cmds::Menu_SetGeneratorValue, _("&Set value..."));
    addSeparator();
    addGenerateScriptMenu(generator);
    addSeparator();
    addDropItem(generator);
    addSeparator();
    addRefreshItem();
    addPropertiesItem();
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitMetadataItem(
    MetadataItem& metadataItem)
{
    switch (metadataItem.getType())
    {
        case ntFunctions:
            menuM->Append(Cmds::Menu_CreateObject, _("Declare &new"));
            addSeparator();
            addRefreshItem();
            break;
        case ntGenerators:
            menuM->Append(Cmds::Menu_ShowAllGeneratorValues,
                _("Show &all values"));
            addSeparator();
            // fall through
        case ntTables:
        case ntViews:
        case ntProcedures:
        case ntTriggers:
        case ntDomains:
        case ntRoles:
        case ntExceptions:
            menuM->Append(Cmds::Menu_CreateObject, _("Create &new"));
            addSeparator();
            // fall through
        case ntSysTables:
            addRefreshItem();
            break;
    }
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitProcedure(Procedure& procedure)
{
    menuM->Append(Cmds::Menu_ExecuteProcedure, _("&Execute"));
    addShowColumnsItem();
    addGenerateScriptMenu(procedure);
    addSeparator();
    addAlterItem(procedure);
    addDropItem(procedure);
    addSeparator();
    addPropertiesItem();
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitRole(Role& role)
{
    addGenerateScriptMenu(role);
    addSeparator();
    addDropItem(role);
    addSeparator();
    addPropertiesItem();
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitRoot(Root&)
{
    menuM->Append(Cmds::Menu_RegisterServer, _("&Register server"));
    addSeparator();
    menuM->Append(wxID_ABOUT, _("&About FlameRobin"));
    menuM->Append(wxID_PREFERENCES, _("&Preferences..."));
    addSeparator();
    menuM->Append(wxID_EXIT, _("&Quit"));
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitServer(Server&)
{
    menuM->Append(Cmds::Menu_RegisterDatabase, 
        _("&Register existing database"));
    menuM->Append(Cmds::Menu_CreateDatabase, _("Create &new database"));
    menuM->Append(Cmds::Menu_RestoreIntoNew,
        _("Restore bac&kup into new database"));
    addSeparator();
    menuM->Append(Cmds::Menu_GetServerVersion, _("Retrieve server &version"));
    menuM->Append(Cmds::Menu_ManageUsers, _("&Manage users"));
    addSeparator();
    menuM->Append(Cmds::Menu_UnRegisterServer, _("&Unregister server"));
    menuM->Append(Cmds::Menu_ServerProperties,
        _("Server registration &info"));
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitTable(Table& table)
{
    addSelectItem();
    if (!table.isSystem())
        menuM->Append(Cmds::Menu_Insert, _("&Insert into"));
    addSeparator();
    addGenerateScriptMenu(table);
    addSeparator();
    if (!table.isSystem())
        menuM->Append(Cmds::Menu_AddColumn, _("&Add column..."));
    addShowColumnsItem();
    if (!table.isSystem())
    {
        menuM->Append(Cmds::Menu_CreateTriggerForTable, 
        _("Create new &trigger"));
    }
    addDropItem(table);
    addSeparator();
    addPropertiesItem();
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitTrigger(Trigger& trigger)
{
    addGenerateScriptMenu(trigger);
    addSeparator();
    addAlterItem(trigger);
    addDropItem(trigger);
    addSeparator();
    addPropertiesItem();
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitView(View& view)
{
    addSelectItem();
    addShowColumnsItem();
    addGenerateScriptMenu(view);
    addSeparator();
    addAlterItem(view);
    addDropItem(view);
    addSeparator();
    addPropertiesItem();
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::addAlterItem(MetadataItem& metadataItem)
{
    if (!metadataItem.isSystem())
        menuM->Append(Cmds::Menu_AlterObject, _("&Alter"));
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::addDropItem(MetadataItem& metadataItem)
{
    if (!metadataItem.isSystem())
        menuM->Append(Cmds::Menu_DropObject, _("Dr&op"));
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::addGenerateScriptMenu(
    MetadataItem& metadataItem)
{
    SqlTemplateManager tm(metadataItem);
    if (tm.descriptorsBegin() == tm.descriptorsEnd())
        return;

    int i = (int)Cmds::Menu_TemplateFirst;
    wxMenu* templateMenu = new wxMenu();
    for (TemplateDescriptorList::const_iterator it = tm.descriptorsBegin();
        it != tm.descriptorsEnd(); ++it, ++i)
    {
        templateMenu->Append(i, (*it)->getMenuCaption());
    }
    menuM->Append(Cmds::Menu_TemplateFirst, _("&Generate SQL"), templateMenu);
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::addPropertiesItem()
{
    menuM->Append(Cmds::Menu_ObjectProperties, _("P&roperties"));
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::addRefreshItem()
{
    menuM->Append(Cmds::Menu_ObjectRefresh, _("Re&fresh"));
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::addSelectItem()
{
    menuM->Append(Cmds::Menu_BrowseColumns, _("&Select from"));
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::addShowColumnsItem()
{
    if (config().get(wxT("ShowColumnsInTree"), true))
        menuM->Append(Cmds::Menu_LoadColumnsInfo, _("Show columns in&fo"));
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::addSeparator()
{
    size_t count = menuM->GetMenuItemCount();
    if (count > 0 && !menuM->FindItemByPosition(count - 1)->IsSeparator())
        menuM->AppendSeparator();
}
//-----------------------------------------------------------------------------
