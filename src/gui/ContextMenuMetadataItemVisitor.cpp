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

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/dir.h>
#include <wx/menu.h>

#include "config/Config.h"
#include "gui/CommandIds.h"
#include "gui/ContextMenuMetadataItemVisitor.h"
#include "metadata/CharacterSet.h"
#include "metadata/Collation.h"
#include "metadata/column.h"
#include "metadata/domain.h"
#include "metadata/database.h"
#include "metadata/exception.h"
#include "metadata/function.h"
#include "metadata/generator.h"
#include "metadata/Index.h"
#include "metadata/MetadataTemplateManager.h"
#include "metadata/package.h"
#include "metadata/procedure.h"
#include "metadata/role.h"
#include "metadata/root.h"
#include "metadata/server.h"
#include "metadata/table.h"
#include "metadata/trigger.h"
#include "metadata/view.h"


MainObjectMenuMetadataItemVisitor::MainObjectMenuMetadataItemVisitor(
    wxMenu* menu)
    : MetadataItemVisitor(), menuM(menu)
{
}

MainObjectMenuMetadataItemVisitor::~MainObjectMenuMetadataItemVisitor()
{
}

void MainObjectMenuMetadataItemVisitor::visitColumn(Column& column)
{
    addGenerateCodeMenu(column);
    // do not show for system tables or views
    if (!column.isSystem() && column.getTable() != 0)
    {
        addSeparator();
        addDropItem(column);
        addSeparator();
        // TODO: addRefreshItem();
        addPropertiesItem();
    }
}

void MainObjectMenuMetadataItemVisitor::visitDatabase(Database& database)
{
    menuM->Append(Cmds::Menu_Connect, _("&Connect"));
    menuM->Append(Cmds::Menu_ConnectAs, _("Connect &as..."));
    menuM->Append(Cmds::Menu_Disconnect, _("&Disconnect"));
    menuM->Append(Cmds::Menu_Reconnect, _("Reconnec&t"));
    addSeparator();
    menuM->Append(Cmds::Menu_ExecuteStatements, _("Execute &SQL statements"));
    addGenerateCodeMenu(database);
    addSeparator();

    wxMenu* toolsMenu = new wxMenu();
    menuM->Append(0, _("&Tools"), toolsMenu);
    // Tools submenu
    toolsMenu->Append(Cmds::Menu_Backup, _("&Backup database"));
    toolsMenu->Append(Cmds::Menu_Restore, _("Rest&ore database"));
    addSeparator();
    toolsMenu->Append(Cmds::Menu_RecreateDatabase, _("Recreate empty database"));
    addSeparator(); 
    toolsMenu->Append(Cmds::Menu_ShutdownDatabase, _("Shutdown database"));
    toolsMenu->Append(Cmds::Menu_StartupDatabase, _("Startup database"));
    addSeparator();
    toolsMenu->Append(Cmds::Menu_MonitorEvents, _("&Monitor events"));
    toolsMenu->Append(Cmds::Menu_GenerateData, _("&Test data generator"));

    menuM->Append(Cmds::Menu_DropDatabase, _("Dr&op database"));
    addSeparator();
    menuM->Append(Cmds::Menu_DatabaseRegistrationInfo,
        _("Database registration &info"));
    menuM->Append(Cmds::Menu_CloneDatabase, _("C&lone registration info"));
    menuM->Append(Cmds::Menu_UnRegisterDatabase, _("&Unregister database"));
    menuM->Append(Cmds::Menu_DatabasePreferences,
        _("Database &preferences"));
    addSeparator();
    addRefreshItem();
    menuM->Append(Cmds::Menu_DatabaseProperties, _("P&roperties"));
}

void MainObjectMenuMetadataItemVisitor::visitDomain(Domain& domain)
{
    addAlterItem(domain);
    addDropItem(domain);
    addSeparator();
    addGenerateCodeMenu(domain);
    addSeparator();
    addRefreshItem();
    addPropertiesItem();
}

void MainObjectMenuMetadataItemVisitor::visitDomains(Domains& domains)
{
    addCreateItem();
    addSeparator();
    addGenerateCodeMenu(domains);
    addSeparator();
    addRefreshItem();
}

void MainObjectMenuMetadataItemVisitor::visitException(Exception& exception)
{
    addDropItem(exception);
    addSeparator();
    addGenerateCodeMenu(exception);
    addSeparator();
    addRefreshItem();
    addPropertiesItem();
}

void MainObjectMenuMetadataItemVisitor::visitExceptions(Exceptions& exceptions)
{
    addCreateItem();
    addSeparator();
    addGenerateCodeMenu(exceptions);
    addSeparator();
    addRefreshItem();
}

void MainObjectMenuMetadataItemVisitor::visitFunctionSQL(FunctionSQL& function)
{
    menuM->Append(Cmds::Menu_ExecuteFunction, _("&Execute"));
    if (function.getParent()->getType() == ntDatabase) {
        addAlterItem(function);
        addDropItem(function);
    }
    addSeparator();
    addGenerateCodeMenu(function);
    addSeparator();
    if (function.getParent()->getType() == ntDatabase)
        addRefreshItem();
    addPropertiesItem();
}

void MainObjectMenuMetadataItemVisitor::visitFunctionSQLs(FunctionSQLs& functions)
{
    addCreateItem();
    addSeparator();
    addGenerateCodeMenu(functions);
    addSeparator();
    addRefreshItem();
}

void MainObjectMenuMetadataItemVisitor::visitUDF(UDF& function)
{
    addDropItem(function);
    addSeparator();
    addGenerateCodeMenu(function);
    addSeparator();
    addRefreshItem();
    addPropertiesItem();
}

void MainObjectMenuMetadataItemVisitor::visitUDFs(UDFs& functions)
{
    addDeclareItem();
    addSeparator();
    addGenerateCodeMenu(functions);
    addSeparator();
    addRefreshItem();
}

void MainObjectMenuMetadataItemVisitor::visitUser(User& user)
{
    addAlterItem(user);
    addDropItem(user);
    addSeparator();
    addGenerateCodeMenu(user);
    addSeparator();
    addRefreshItem();
    addPropertiesItem();
}

void MainObjectMenuMetadataItemVisitor::visitUsers(Users& /*users*/)
{
    addCreateItem();
    addSeparator();
    //addGenerateCodeMenu(users);
    addSeparator();
    addRefreshItem();
}

void MainObjectMenuMetadataItemVisitor::visitGenerator(Generator& generator)
{
    menuM->Append(Cmds::Menu_ShowGeneratorValue, _("Show &value"));
    menuM->Append(Cmds::Menu_SetGeneratorValue, _("&Set value"));
    addSeparator();
    addDropItem(generator);
    addSeparator();
    addGenerateCodeMenu(generator);
    addSeparator();
    addRefreshItem();
    addPropertiesItem();
}

void MainObjectMenuMetadataItemVisitor::visitGenerators(Generators& generators)
{
    menuM->Append(Cmds::Menu_ShowAllGeneratorValues, _("Show &all values"));
    addSeparator();
    addCreateItem();
    addSeparator();
    addGenerateCodeMenu(generators);
    addSeparator();
    addRefreshItem();
}

void MainObjectMenuMetadataItemVisitor::visitGTTable(GTTable& table)
{
    addBrowseDataItem();
    addGenerateCodeMenu(table);
    addSeparator();
    if (!table.isSystem())
        menuM->Append(Cmds::Menu_AddColumn, _("&Add column"));
    addDropItem(table);
    addSeparator();
    addRefreshItem();
    addPropertiesItem();
}

void MainObjectMenuMetadataItemVisitor::visitPackage(Package& package)
{
    addAlterItem(package);
    addDropItem(package);
    addSeparator();
    addGenerateCodeMenu(package);
    addSeparator();
    addRefreshItem();
    addPropertiesItem();
}

void MainObjectMenuMetadataItemVisitor::visitPackages(Packages& packages)
{
    addCreateItem();
    addSeparator();
    addGenerateCodeMenu(packages); 
    addSeparator();
    addRefreshItem();
}

void MainObjectMenuMetadataItemVisitor::visitSysPackages(SysPackages& packages)
{
    addGenerateCodeMenu(packages);
    addSeparator();
    addRefreshItem();
}

void MainObjectMenuMetadataItemVisitor::visitProcedure(Procedure& procedure)
{
    menuM->Append(Cmds::Menu_ExecuteProcedure, _("&Execute"));
    if (procedure.getParent()->getType() == ntDatabase) {
        addAlterItem(procedure);
        addDropItem(procedure);
    }
    addSeparator();
    addGenerateCodeMenu(procedure);
    addSeparator();
    if (procedure.getParent()->getType() == ntDatabase)
        addRefreshItem();
    addPropertiesItem();
}

void MainObjectMenuMetadataItemVisitor::visitProcedures(Procedures& procedures)
{
    addCreateItem();
    addSeparator();
    addGenerateCodeMenu(procedures);
    addSeparator();
    addRefreshItem();
}

void MainObjectMenuMetadataItemVisitor::visitRole(Role& role)
{
    addDropItem(role);
    addSeparator();
    addGenerateCodeMenu(role);
    addSeparator();
    addRefreshItem();
    addPropertiesItem();
}

void MainObjectMenuMetadataItemVisitor::visitRoles(Roles& roles)
{
    addCreateItem();
    addSeparator();
    addGenerateCodeMenu(roles);
    addSeparator();
    addRefreshItem();
}

void MainObjectMenuMetadataItemVisitor::visitSysRoles(SysRoles& sysRoles)
{
    addGenerateCodeMenu(sysRoles);
    addSeparator();
    addRefreshItem();
}

void MainObjectMenuMetadataItemVisitor::visitRoot(Root& root)
{
    menuM->Append(Cmds::Menu_RegisterServer, _("&Register server"));
    addSeparator();
    addGenerateCodeMenu(root);
    addSeparator();
    menuM->Append(wxID_ABOUT, _("&About FlameRobin"));
    menuM->Append(wxID_PREFERENCES, _("&Preferences"));
    addSeparator();
    menuM->Append(wxID_EXIT, _("&Quit"));
}

void MainObjectMenuMetadataItemVisitor::visitServer(Server& server)
{
    menuM->Append(Cmds::Menu_RegisterDatabase,
        _("&Register existing database"));
    menuM->Append(Cmds::Menu_CreateDatabase, _("Create &new database"));
    menuM->Append(Cmds::Menu_RestoreIntoNew,
        _("Restore bac&kup into new database"));
    addSeparator();
    addGenerateCodeMenu(server);
    addSeparator();
    menuM->Append(Cmds::Menu_GetServerVersion, _("Retrieve server &version"));
    menuM->Append(Cmds::Menu_ManageUsers, _("&Manage users"));
    addSeparator();
    menuM->Append(Cmds::Menu_UnRegisterServer, _("&Unregister server"));
    menuM->Append(Cmds::Menu_ServerProperties,
        _("Server registration &info"));
}

void MainObjectMenuMetadataItemVisitor::visitTable(Table& table)
{
    addBrowseDataItem();
    addGenerateCodeMenu(table);
    addSeparator();
    if (!table.isSystem())
        menuM->Append(Cmds::Menu_AddColumn, _("&Add column"));
    addDropItem(table);
    addSeparator();
    addRefreshItem();
    addPropertiesItem();
}

void MainObjectMenuMetadataItemVisitor::visitTables(Tables& tables)
{
    addCreateItem();
    addSeparator();
    addGenerateCodeMenu(tables);
    addSeparator();
    addRefreshItem();
}


void MainObjectMenuMetadataItemVisitor::visitSysTables(SysTables& sysTables)
{
    addGenerateCodeMenu(sysTables);
    addSeparator();
    addRefreshItem();
}

void MainObjectMenuMetadataItemVisitor::visitGTTables(GTTables& tables)
{
    addCreateItem();
    addSeparator();
    addGenerateCodeMenu(tables);
    addSeparator();
    addRefreshItem();
}

void MainObjectMenuMetadataItemVisitor::visitDMLTrigger(DMLTrigger& trigger)
{
    addAlterItem(trigger);
    addDropItem(trigger);
    addActiveItem(trigger);
    addSeparator();
    addGenerateCodeMenu(trigger);
    addSeparator();
    addRefreshItem();
    addPropertiesItem();
}
void MainObjectMenuMetadataItemVisitor::visitDBTrigger(DBTrigger& trigger)
{
    addAlterItem(trigger);
    addDropItem(trigger);
    addActiveItem(trigger);
    addSeparator();
    addGenerateCodeMenu(trigger);
    addSeparator();
    addRefreshItem();
    addPropertiesItem();
}
void MainObjectMenuMetadataItemVisitor::visitDDLTrigger(DDLTrigger& trigger)
{
    addAlterItem(trigger);
    addDropItem(trigger);
    addActiveItem(trigger);
    addSeparator();
    addGenerateCodeMenu(trigger);
    addSeparator();
    addRefreshItem();
    addPropertiesItem();
}

void MainObjectMenuMetadataItemVisitor::visitDMLTriggers(DMLTriggers& triggers)
{
    addCreateItem();
    addSeparator();
    addGenerateCodeMenu(triggers);
    addSeparator();
    addRefreshItem();
}

void MainObjectMenuMetadataItemVisitor::visitDBTriggers(DBTriggers& triggers)
{
    addCreateItem();
    addSeparator();
    addGenerateCodeMenu(triggers); 
    addSeparator();
    addRefreshItem();
}

void MainObjectMenuMetadataItemVisitor::visitDDLTriggers(DDLTriggers& triggers)
{
    addCreateItem();
    addSeparator();
    addGenerateCodeMenu(triggers);
    addSeparator();
    addRefreshItem();
}

void MainObjectMenuMetadataItemVisitor::visitView(View& view)
{
    addBrowseDataItem();
    addGenerateCodeMenu(view);
    addSeparator();
    addAlterItem(view);
    menuM->Append(Cmds::Menu_RebuildObject, _("&Rebuild"));
    addDropItem(view);
    addSeparator();
    addRefreshItem();
    addPropertiesItem();
}

void MainObjectMenuMetadataItemVisitor::visitViews(Views& views)
{
    addCreateItem();
    addSeparator();
    addGenerateCodeMenu(views);
    addSeparator();
    addRefreshItem();
}

void MainObjectMenuMetadataItemVisitor::visitCharacterSet(CharacterSet& charset)
{
    addAlterItem(charset);
    addSeparator();
    addPropertiesItem();
}

void MainObjectMenuMetadataItemVisitor::visitCharacterSets(CharacterSets& /*charsets*/)
{
    addRefreshItem();
}

void MainObjectMenuMetadataItemVisitor::visitCollation(Collation& collation)
{
    addAlterItem(collation);
    addDropItem(collation);
    addSeparator();
    addPropertiesItem();
}

void MainObjectMenuMetadataItemVisitor::visitSysCollations(SysCollations& /*collations*/)
{
    addRefreshItem();
}

void MainObjectMenuMetadataItemVisitor::visitCollations(Collations& coolations)
{
    addCreateItem();
    addSeparator();
    addGenerateCodeMenu(coolations);
    addSeparator();
    addRefreshItem();
}

void MainObjectMenuMetadataItemVisitor::visitIndex(Index& index)
{
    //menuM->Append(Cmds::Menu_ShowStatisticsValue, _("Show &statistics"));
    menuM->Append(Cmds::Menu_SetStatisticsValue, _("&Recompute statistics"));
    addSeparator();
    addAlterItem(index);
    addDropItem(index);
    addActiveItem(index);
    addSeparator();
    addGenerateCodeMenu(index);
    addSeparator();
    addRefreshItem();
    addPropertiesItem();
}

void MainObjectMenuMetadataItemVisitor::visitIndices(Indices& indices)
{
    menuM->Append(Cmds::Menu_ShowAllStatisticsValue, _("Show &all statistics"));
    addSeparator();
    addCreateItem();
    addSeparator();
    addGenerateCodeMenu(indices);
    addSeparator();
    addRefreshItem();
}

void MainObjectMenuMetadataItemVisitor::visitMethod(Method& method)
{
    if (method.isFunction())
        menuM->Append(Cmds::Menu_ExecuteFunction, _("&Execute"));
    else
        menuM->Append(Cmds::Menu_ExecuteProcedure, _("&Execute"));

}

void MainObjectMenuMetadataItemVisitor::addAlterItem(MetadataItem& metadataItem)
{
    if (!metadataItem.isSystem())
        menuM->Append(Cmds::Menu_AlterObject, _("&Alter"));
}

void MainObjectMenuMetadataItemVisitor::addActiveItem(MetadataItem& metadataItem)
{
    Index* i = dynamic_cast<Index*>(&metadataItem);
    if (i) {
        if (i->isActive())
            menuM->Append(Cmds::Menu_InactiveObject, _("&Inactive"));
        else
            menuM->Append(Cmds::Menu_ActiveObject, _("&Active"));
    }

    Trigger* t = dynamic_cast<Trigger*>(&metadataItem);
    if (t) {
        if (t->isActive())
            menuM->Append(Cmds::Menu_InactiveObject, _("&Inactive"));
        else
            menuM->Append(Cmds::Menu_ActiveObject, _("&Active"));
    }

}

void MainObjectMenuMetadataItemVisitor::addCreateItem()
{
    // This menu command is redundant in the main Object menu.
}

void MainObjectMenuMetadataItemVisitor::addDeclareItem()
{
    // This menu command is redundant in the main Object menu.
}

void MainObjectMenuMetadataItemVisitor::addDropItem(MetadataItem& metadataItem)
{
    if (!metadataItem.isSystem())
        menuM->Append(Cmds::Menu_DropObject, _("Dr&op"));
}

void MainObjectMenuMetadataItemVisitor::addGenerateCodeMenu(
    MetadataItem& metadataItem, wxMenu* parent)
{
    MetadataTemplateManager tm(&metadataItem);
    if (tm.descriptorsBegin() == tm.descriptorsEnd())
        return;

    int i = (int)Cmds::Menu_TemplateFirst;
    wxMenu* templateMenu = new wxMenu();
    for (TemplateDescriptorList::const_iterator it = tm.descriptorsBegin();
        it != tm.descriptorsEnd(); ++it, ++i)
    {
        templateMenu->Append(i, (*it)->getMenuCaption());
    }
    if (!parent)
        parent = menuM;
    parent->Append(Cmds::Menu_TemplateMenu, _("&Generate code"), templateMenu);
}

void MainObjectMenuMetadataItemVisitor::addPropertiesItem()
{
    menuM->Append(Cmds::Menu_ObjectProperties, _("P&roperties"));
}

void MainObjectMenuMetadataItemVisitor::addRefreshItem()
{
    menuM->Append(Cmds::Menu_ObjectRefresh, _("Re&fresh"));
}

void MainObjectMenuMetadataItemVisitor::addBrowseDataItem()
{
    menuM->Append(Cmds::Menu_BrowseData, _("Brow&se data"));
}

void MainObjectMenuMetadataItemVisitor::addSeparator()
{
    size_t count = menuM->GetMenuItemCount();
    if (count > 0 && !menuM->FindItemByPosition(count - 1)->IsSeparator())
        menuM->AppendSeparator();
}

ContextMenuMetadataItemVisitor::ContextMenuMetadataItemVisitor(
    wxMenu* menu)
    : MainObjectMenuMetadataItemVisitor(menu)
{
}

ContextMenuMetadataItemVisitor::~ContextMenuMetadataItemVisitor()
{
}

void ContextMenuMetadataItemVisitor::addCreateItem()
{
    menuM->Append(Cmds::Menu_CreateObject, _("Create &new"));
}

void ContextMenuMetadataItemVisitor::addDeclareItem()
{
    menuM->Append(Cmds::Menu_CreateObject, _("Declare &new"));
}

