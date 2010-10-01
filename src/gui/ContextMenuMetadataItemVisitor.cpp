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
    addGenerateScriptMenu(column, false, true);
    addDropItem(column, false, false);
    addPropertiesItem(true);
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
    actions->Append(Cmds::Menu_Backup, _("&Backup database..."));
    actions->Append(Cmds::Menu_Restore, _("Rest&ore database..."));
    actions->AppendSeparator();
    actions->Append(Cmds::Menu_RecreateDatabase, _("Recreate empty database"));
    actions->Append(Cmds::Menu_DropDatabase, _("Drop database"));

    // the advanced submenu
    advanced->Append(Cmds::Menu_ShowConnectedUsers, _("&Show connected users"));
    advanced->Append(Cmds::Menu_MonitorEvents, _("&Monitor events"));
    advanced->Append(Cmds::Menu_DatabasePreferences, _("Database &preferences..."));
    advanced->Append(Cmds::Menu_GenerateData, _("&Test data generator..."));
    advanced->Append(Cmds::Menu_ExtractDatabaseDDL, _("&Extract metadata DDL..."));

    menuM->AppendSeparator();
    addRefreshItem();
    menuM->AppendSeparator();
    menuM->Append(Cmds::Menu_DatabaseProperties, _("Data&base Properties"));
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitDomain(Domain& domain)
{
    addGenerateScriptMenu(domain, false, true);
    addAlterItem();
    addDropItem(domain, false, true);
    addRefreshItem();
    addPropertiesItem(true);
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitException(Exception& exception)
{
    addGenerateScriptMenu(exception, false, true);
    addDropItem(exception, false, true);
    addRefreshItem();
    addPropertiesItem(true);
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitFunction(Function& function)
{
    addGenerateScriptMenu(function, false, true);
    addDropItem(function, false, false);
    addPropertiesItem(true);
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitGenerator(Generator& generator)
{
    menuM->Append(Cmds::Menu_ShowGeneratorValue, _("Show &value"));
    menuM->Append(Cmds::Menu_SetGeneratorValue, _("&Set value..."));
    addGenerateScriptMenu(generator, true, true);
    addDropItem(generator, false, true);
    addRefreshItem();
    addPropertiesItem(true);
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitMetadataItem(
    MetadataItem& metadataItem)
{
    switch (metadataItem.getType())
    {
        case ntFunctions:
            menuM->Append(Cmds::Menu_CreateObject, _("&Declare new..."));
            menuM->AppendSeparator();
            addRefreshItem();
            break;
        case ntGenerators:
            menuM->Append(Cmds::Menu_ShowAllGeneratorValues,
                _("Show &all values"));
            menuM->AppendSeparator();
            // fall through
        case ntTables:
        case ntViews:
        case ntProcedures:
        case ntTriggers:
        case ntDomains:
        case ntRoles:
        case ntExceptions:
            menuM->Append(Cmds::Menu_CreateObject, _("Create &new..."));
            menuM->AppendSeparator();
            // fall through
        case ntSysTables:
            addRefreshItem();
            break;
    }
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitProcedure(Procedure& procedure)
{
    menuM->Append(Cmds::Menu_ExecuteProcedure, _("&Execute..."));
    addShowColumnsItem();
    addGenerateScriptMenu(procedure, false, true);
    addAlterItem();
    addDropItem(procedure, false, false);
    addPropertiesItem(true);
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitRole(Role& role)
{
    addGenerateScriptMenu(role, false, true);
    addDropItem(role, false, false);
    addPropertiesItem(true);
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
    addSelectItem(false);
    if (!table.isSystem())
        menuM->Append(Cmds::Menu_Insert, _("&Insert into"));
    addGenerateScriptMenu(table, true, true);
    if (!table.isSystem())
        menuM->Append(Cmds::Menu_AddColumn, _("&Add column..."));
    addShowColumnsItem();
    if (!table.isSystem())
        menuM->Append(Cmds::Menu_CreateTriggerForTable,
            _("Create new &trigger..."));
    addDropItem(table, false, true);
    addPropertiesItem(false);
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitTrigger(Trigger& trigger)
{
    addGenerateScriptMenu(trigger, false, true);
    addAlterItem();
    addDropItem(trigger, false, false);
    addPropertiesItem(true);
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::visitView(View& view)
{
    addSelectItem(false);
    addShowColumnsItem();
    addGenerateScriptMenu(view, false, true);
    addAlterItem();
    addDropItem(view, false, false);
    addPropertiesItem(true);
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::addAlterItem()
{
    menuM->Append(Cmds::Menu_AlterObject, _("&Alter..."));
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::addDropItem(MetadataItem& metadataItem,
    bool separatorBefore, bool separatorAfter)
{
    if (!metadataItem.isSystem())
    {
        if (separatorBefore)
            menuM->AppendSeparator();
        menuM->Append(Cmds::Menu_DropObject, _("Dr&op"));
        if (separatorAfter)
            menuM->AppendSeparator();
    }
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::addPropertiesItem(bool separatorBefore)
{
    if (separatorBefore)
        menuM->AppendSeparator();
    menuM->Append(Cmds::Menu_ObjectProperties, _("P&roperties"));
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::addRefreshItem()
{
    menuM->Append(Cmds::Menu_ObjectRefresh, _("Re&fresh"));
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::addSelectItem(bool separatorAfter)
{
    menuM->Append(Cmds::Menu_BrowseColumns, _("&Select from"));
    if (separatorAfter)
        menuM->AppendSeparator();
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::addShowColumnsItem()
{
    if (config().get(wxT("ShowColumnsInTree"), true))
        menuM->Append(Cmds::Menu_LoadColumnsInfo, _("Show columns in&fo"));
}
//-----------------------------------------------------------------------------
void ContextMenuMetadataItemVisitor::addGenerateScriptMenu(
    MetadataItem& metadataItem, bool separatorBefore, bool separatorAfter)
{
    wxMenu *templateMenu = new wxMenu();

    SqlTemplateManager tm(metadataItem);
        
    bool templatesExist = false;
    int i = (int)Cmds::Menu_TemplateFirst;
    for (TemplateDescriptorList::const_iterator it = tm.descriptorsBegin();
        it != tm.descriptorsEnd(); ++it, ++i)
    {
        templateMenu->Append(i, (*it)->getMenuCaption());
        templatesExist = true;
    }
    if (templatesExist)
    {
        if (separatorBefore)
            menuM->AppendSeparator();
        menuM->Append(Cmds::Menu_TemplateFirst, _("&Generate SQL"),
            templateMenu);
        if (separatorAfter)
            menuM->AppendSeparator();
    }
}
//-----------------------------------------------------------------------------
