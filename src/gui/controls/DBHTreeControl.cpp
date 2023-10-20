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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/artprov.h>
#include <wx/dataobj.h>
#include <wx/dnd.h>
#include <wx/imaglist.h>

#include <algorithm>
#include <map>
#include <vector>

#include "config/Config.h"
#include "core/ArtProvider.h"
#include "core/Observer.h"
#include "core/StringUtils.h"
#include "core/Subject.h"
#include "gui/ContextMenuMetadataItemVisitor.h"
#include "gui/controls/DBHTreeControl.h"

#include "metadata/CharacterSet.h"
#include "metadata/Collation.h"
#include "metadata/database.h"
#include "metadata/domain.h"
#include "metadata/exception.h"
#include "metadata/function.h"
#include "metadata/generator.h"
#include "metadata/Index.h"
#include "metadata/parameter.h"
#include "metadata/package.h"
#include "metadata/procedure.h"
#include "metadata/role.h"
#include "metadata/root.h"
#include "metadata/server.h"
#include "metadata/table.h"
#include "metadata/trigger.h"
#include "metadata/view.h"
#include "sql/SqlTokenizer.h"

// DBHTreeConfigCache: class to cache config data for tree control behaviour
class DBHTreeConfigCache: public ConfigCache, public Subject
{
private:
    bool allowDragM;
    bool hideDisconnectedDatabasesM;
    bool showColumnParamCountM;
    bool showColumnsM;
    int showComputedM;
    int showDomainsM;
    bool showSystemIndicesM;
    bool showSystemRolesM;
    bool showSystemTablesM;
    bool sortDatabasesM;
    bool sortServersM;
    bool sqlKeywordsUpperCaseM;
    template<typename T>
    unsigned setValue(T& field, T newValue);
protected:
    virtual void loadFromConfig();
    virtual void update();
public:
    DBHTreeConfigCache();

    static DBHTreeConfigCache& get();

    bool allowDnD() { return allowDragM; }
    bool getHideDisconnectedDatabases()
        { return hideDisconnectedDatabasesM; }
    bool getShowColumnParamCount() { return showColumnParamCountM; }
    bool getShowColumns() { return showColumnsM; }
    bool getSortDatabases() { return sortDatabasesM; }
    bool getSortServers() { return sortServersM; }
};

DBHTreeConfigCache::DBHTreeConfigCache()
    : ConfigCache(config()), Subject()
{
    loadFromConfig();
}

DBHTreeConfigCache& DBHTreeConfigCache::get()
{
    static DBHTreeConfigCache dndc;
    return dndc;
}

void DBHTreeConfigCache::loadFromConfig()
{
    Config& cfg(config());
    unsigned changes = 0;

    changes += setValue(allowDragM,
        cfg.get("allowDragAndDrop", false));
    changes += setValue(hideDisconnectedDatabasesM,
        cfg.get("HideDisconnectedDatabases", false));
    changes += setValue(showColumnParamCountM,
        cfg.get("ShowColumnAndParameterCountInTree", false));
    changes += setValue(showColumnsM,
        cfg.get("ShowColumnsInTree", true));
    changes += setValue(sortDatabasesM,
        cfg.get("OrderDatabasesInTree", false));
    changes += setValue(sortServersM,
        cfg.get("OrderServersInTree", false));
    // these aren't surfaced by methods, but needed to cause observing tree
    // nodes to update themselves
    changes += setValue(showSystemIndicesM,
        cfg.get("ShowSystemIndices", false));
    changes += setValue(showSystemRolesM,
        cfg.get("ShowSystemRoles", false));
    changes += setValue(showSystemTablesM,
        cfg.get("ShowSystemTables", true));
    changes += setValue(showComputedM,
        cfg.get("ShowComputed", 1));
    changes += setValue(showDomainsM,
        cfg.get("ShowDomains", 2));
    changes += setValue(sqlKeywordsUpperCaseM,
        cfg.get("SQLKeywordsUpperCase", false));

    if (changes)
        notifyObservers();
}

template<typename T>
unsigned DBHTreeConfigCache::setValue(T& field, T newValue)
{
    if (field == newValue)
        return 0;
    field = newValue;
    return 1;
}

void DBHTreeConfigCache::update()
{
    ConfigCache::update();
    // load changed settings immediately and notify observers 
    loadFromConfig();
}

// DBHTreeImageList class
class DBHTreeImageList: public wxImageList
{
private:
    std::map<wxArtID, int> artIdIndicesM;
    void addImage(const wxArtID& art);
public:
    DBHTreeImageList();

    static DBHTreeImageList& get();
    int getImageIndex(const wxArtID& id);
};

DBHTreeImageList::DBHTreeImageList()
    : wxImageList(16, 16)
{
    addImage(ART_Object);
    addImage(ART_Column);
    addImage(ART_CharacterSet);
    addImage(ART_CharacterSets);
    addImage(ART_Collation);
    addImage(ART_Collations);
    addImage(ART_Computed);
    addImage(ART_DatabaseConnected);
    addImage(ART_DatabaseDisconnected);
    addImage(ART_DatabaseServer);
    addImage(ART_DBTrigger);
    addImage(ART_DBTriggers);
    addImage(ART_DDLTrigger);
    addImage(ART_DDLTriggers);
    addImage(ART_DMLTrigger);
    addImage(ART_DMLTriggers);
    addImage(ART_Domain);
    addImage(ART_Domains);
    addImage(ART_Exception);
    addImage(ART_Exceptions);
    addImage(ART_ForeignKey);
    addImage(ART_Function);
    addImage(ART_Functions);
    addImage(ART_Generator);
    addImage(ART_Generators);
    addImage(ART_GlobalTemporary);
    addImage(ART_GlobalTemporaries);
    addImage(ART_Index);
    addImage(ART_Indices);
    addImage(ART_Input);
    addImage(ART_Output);
    addImage(ART_Package);
    addImage(ART_Packages);
    addImage(ART_ParameterInput);
    addImage(ART_ParameterOutput);
    addImage(ART_PrimaryAndForeignKey);
    addImage(ART_PrimaryKey);
    addImage(ART_Procedure);
    addImage(ART_Procedures);
    addImage(ART_Role);
    addImage(ART_Roles);
    addImage(ART_Root);
    addImage(ART_Server);
    addImage(ART_SystemIndex);
    addImage(ART_SystemIndices);
    addImage(ART_SystemDomain);
    addImage(ART_SystemDomains);
    addImage(ART_SystemPackage);
    addImage(ART_SystemPackages);
    addImage(ART_SystemRole);
    addImage(ART_SystemRoles);
    addImage(ART_SystemTable);
    addImage(ART_SystemTables);
    addImage(ART_Table);
    addImage(ART_Tables);
    addImage(ART_Trigger);
    addImage(ART_Triggers);
    addImage(ART_UDF);
    addImage(ART_UDFs);
    addImage(ART_User);
    addImage(ART_Users);
    addImage(ART_View);
    addImage(ART_Views);
}

/*static*/ DBHTreeImageList& DBHTreeImageList::get()
{
    static DBHTreeImageList til;
    return til;
}

void DBHTreeImageList::addImage(const wxArtID& art)
{
    wxBitmap bmp(wxArtProvider::GetBitmap(art, wxART_OTHER, wxSize(16, 16)));
    if (!bmp.Ok())
        return;
    wxIcon icon;
    icon.CopyFromBitmap(bmp);
    artIdIndicesM[art] = Add(icon);
}

int DBHTreeImageList::getImageIndex(const wxArtID& id)
{
    std::map<wxArtID, int>::const_iterator it = artIdIndicesM.find(id);
    if (it != artIdIndicesM.end())
        return (*it).second;
    return -1;
}

// DBHTreeItemVisitor class
class DBHTreeItemVisitor: public MetadataItemVisitor
{
private:
    DBHTreeControl* treeM;

    bool nodeVisibleM;
    bool nodeTextBoldM;
    bool nodeEnabledM;
    wxString nodeTextM;
    int nodeImageIndexM;
    bool showChildrenM;
    bool showNodeExpanderM;
    bool sortChildrenM;
    bool nodeConfigSensitiveM;

    void setNodeProperties(MetadataItem* metadataItem, const wxArtID& artId);
protected:
    virtual void defaultAction();
public:
    DBHTreeItemVisitor(DBHTreeControl* tree);

    bool getNodeVisible() { return nodeVisibleM; }
    wxString getNodeText() { return nodeTextM; }
    bool getNodeTextBold() { return nodeTextBoldM; }
    bool getNodeEnabled() { return nodeEnabledM; }
    int getNodeImage() { return nodeImageIndexM; }
    bool getShowChildren() { return showChildrenM; }
    bool getShowNodeExpander() { return showNodeExpanderM; }
    bool getSortChildren() { return sortChildrenM; }
    bool isConfigSensitive() { return nodeConfigSensitiveM; }

    virtual void visitCharacterSet(CharacterSet& characterset);
    virtual void visitCharacterSets(CharacterSets& charactersets);
    virtual void visitCollation(Collation& collation);
    virtual void visitSysCollations(SysCollations& collations);
    virtual void visitCollations(Collations& collations);
    virtual void visitColumn(Column& column);
    virtual void visitDatabase(Database& database);
    virtual void visitDomain(Domain& domain);
    virtual void visitDomains(Domains& domains);
    virtual void visitSysDomains(SysDomains& domains);
    virtual void visitException(Exception& exception);
    virtual void visitExceptions(Exceptions& exceptions);
    virtual void visitFunctionSQL(FunctionSQL& function);
    virtual void visitFunctionSQLs(FunctionSQLs& functions);
    virtual void visitGenerator(Generator& generator);
    virtual void visitGenerators(Generators& generators);
    virtual void visitMethod(Method& method);
    virtual void visitPackage(Package& package);
    virtual void visitPackages(Packages& packages);
    virtual void visitSysPackages(SysPackages& packages);
    virtual void visitProcedure(Procedure& procedure);
    virtual void visitProcedures(Procedures& procedures);
    virtual void visitParameter(Parameter& parameter);
    virtual void visitRole(Role& role);
    virtual void visitRoles(Roles& roles);
    virtual void visitSysRoles(SysRoles& roles);
    virtual void visitRoot(Root& root);
    virtual void visitServer(Server& server);
    virtual void visitTable(Table& table);
    virtual void visitTables(Tables& tables);
    virtual void visitSysTable(SysTable& table);
    virtual void visitSysTables(SysTables& tables);
    virtual void visitGTTable(GTTable& table);
    virtual void visitGTTables(GTTables& tables);
    virtual void visitDMLTrigger(DMLTrigger& trigger);
    virtual void visitDMLTriggers(DMLTriggers& triggers);
    virtual void visitDBTrigger(DBTrigger& trigger);
    virtual void visitDBTriggers(DBTriggers& triggers);
    virtual void visitDDLTrigger(DDLTrigger& trigger);
    virtual void visitDDLTriggers(DDLTriggers& triggers);
    virtual void visitUDF(UDF& function);
    virtual void visitUDFs(UDFs& functions);
    virtual void visitUser(User& user);
    virtual void visitUsers(Users& users);
    virtual void visitView(View& view);
    virtual void visitViews(Views& views);
    virtual void visitIndex(Index& index);
    virtual void visitIndices(Indices& indices);
    virtual void visitSysIndices(SysIndices& sysIndices);
    virtual void visitUsrIndices(UsrIndices& UsrIndices);
};

DBHTreeItemVisitor::DBHTreeItemVisitor(DBHTreeControl* tree)
    : MetadataItemVisitor(), treeM(tree), nodeVisibleM(true),
        nodeTextBoldM(false), nodeTextM(), nodeImageIndexM(-1),
        showChildrenM(false), showNodeExpanderM(false),
        sortChildrenM(false), nodeConfigSensitiveM(false),
        nodeEnabledM(true)
{
}

void DBHTreeItemVisitor::defaultAction()
{
    // all classes that have corresponding tree nodes must have visitClass()
    wxASSERT_MSG(false, "DBHTreeItemVisitor::visit[Classname]() missing");
}

void DBHTreeItemVisitor::setNodeProperties(MetadataItem* metadataItem,
    const wxArtID& artId)
{
    wxASSERT(metadataItem);
    nodeVisibleM = true;
    nodeTextBoldM = false;

    nodeImageIndexM = DBHTreeImageList::get().getImageIndex(artId);

    size_t childCount = metadataItem->getChildrenCount();
    // don't touch node caption if it has already been set
    if (nodeTextM.empty())
    {
        nodeTextM = metadataItem->getName_();
        if (childCount)
            nodeTextM << " (" << childCount << ")";
    }

    showChildrenM = childCount > 0;
    sortChildrenM = false;
}

void DBHTreeItemVisitor::visitCharacterSet(CharacterSet& characterset)
{
    setNodeProperties(&characterset, ART_CharacterSet);
}

void DBHTreeItemVisitor::visitCharacterSets(CharacterSets& charactersets)
{
    setNodeProperties(&charactersets, ART_CharacterSets);
}

void DBHTreeItemVisitor::visitCollation(Collation& collation)
{
    setNodeProperties(&collation, ART_Collation);
}

void DBHTreeItemVisitor::visitSysCollations(SysCollations& collations)
{
    setNodeProperties(&collations, ART_Collations);
}

void DBHTreeItemVisitor::visitCollations(Collations& collations)
{
    setNodeProperties(&collations, ART_Collations);
}

void DBHTreeItemVisitor::visitColumn(Column& column)
{
    // node text: column_name + column_datatype [+ "not null"]
    nodeTextM = column.getName_() + " " + column.getDatatype();
    // only show first line of multiline text
    size_t nl = nodeTextM.find_first_of("\n\r");
    if (nl != wxString::npos)
    {
        nodeTextM.Truncate(nl);
        nodeTextM += "...";
    }
    if (!column.isNullable(CheckDomainNullability))
    {
        nodeTextM << " " << SqlTokenizer::getKeyword(kwNOT)
            << " " << SqlTokenizer::getKeyword(kwNULL);
    }

    // image index depends on participation in primary and foreign keys
    // and is different for computed columns
    bool isPK = column.isPrimaryKey();
    bool isFK = column.isForeignKey();

    wxArtID artId = ART_Column;
    if (isPK && isFK)
        artId = ART_PrimaryAndForeignKey;
    else if (isPK)
        artId = ART_PrimaryKey;
    else if (isFK)
        artId = ART_ForeignKey;
    else if (!column.getComputedSource().IsEmpty())
        artId = ART_Computed;

    // set remaining default properties, nodeTextM will not be touched
    setNodeProperties(&column, artId);
}

void DBHTreeItemVisitor::visitDatabase(Database& database)
{
    // show different images for connected and disconnected databases
    bool connected = database.isConnected();
    wxArtID artId = connected ?
        ART_DatabaseConnected : ART_DatabaseDisconnected;
    setNodeProperties(&database, artId);

    // hide disconnected databases
    if (DBHTreeConfigCache::get().getHideDisconnectedDatabases())
        nodeVisibleM = connected;
    // show Collection nodes even though Database::getChildrenCount() returns 0
    showChildrenM = true;
    // update if settings change: "Show system roles/tables in tree"
    nodeConfigSensitiveM = true;
}

void DBHTreeItemVisitor::visitDomain(Domain& domain)
{
        // skip autogenerated domains
    nodeVisibleM = !domain.isSystem();
    setNodeProperties(&domain, domain.isSystem() ? ART_SystemDomain : ART_Domain);
    if (!nodeVisibleM)
        return;
        // node text: domain_name + domain_datatype [+ "not null"]
    if (domain.propertiesLoaded()) {
        nodeTextM = domain.getName_() + " " + domain.getDatatypeAsString();
        if (!domain.isNullable())
        {
            nodeTextM << " " << SqlTokenizer::getKeyword(kwNOT)
                << " " << SqlTokenizer::getKeyword(kwNULL);
        }
    }
    // set remaining default properties, nodeTextM will not be touched
//    setNodeProperties(&domain, domain.isSystem() ? ART_SystemDomain : ART_Domain);
}

void DBHTreeItemVisitor::visitDomains(Domains& domains)
{
    setNodeProperties(&domains, ART_Domains);
}

void DBHTreeItemVisitor::visitSysDomains(SysDomains& domains)
{
    setNodeProperties(&domains, ART_SystemDomains);
}

void DBHTreeItemVisitor::visitException(Exception& exception)
{
    setNodeProperties(&exception, ART_Exception);
}

void DBHTreeItemVisitor::visitExceptions(Exceptions& exceptions)
{
    setNodeProperties(&exceptions, ART_Exceptions);
}

void DBHTreeItemVisitor::visitFunctionSQL(FunctionSQL& function)
{
    setNodeProperties(&function, ART_Function);
    if (function.childrenLoaded())
    {
        // make node caption bold when parameter data is loaded
        // (even if the procedure has no parameters at all)
        nodeTextBoldM = true;
        // show number of parameters?
        if (DBHTreeConfigCache::get().getShowColumnParamCount())
        {
            size_t ins = 0, outs = 0;
            for (ParameterPtrs::const_iterator it = function.begin();
                it != function.end(); ++it)
            {
                if ((*it)->isOutputParameter())
                    ++outs;
                else
                    ++ins;
            }
            nodeTextM += wxString::Format(" (%zu, %zu)", ins, outs);
        }
    }
    // show Parameter nodes if Config setting is on
    showChildrenM = DBHTreeConfigCache::get().getShowColumns();
    showNodeExpanderM = showChildrenM && !function.childrenLoaded();
    // update if settings change
    nodeConfigSensitiveM = true;

}

void DBHTreeItemVisitor::visitFunctionSQLs(FunctionSQLs& functions)
{
    setNodeProperties(&functions, ART_Functions);
}

void DBHTreeItemVisitor::visitUDF(UDF& function)
{
    setNodeProperties(&function, ART_UDF);
    if (function.childrenLoaded())
    {
        // make node caption bold when parameter data is loaded
        // (even if the procedure has no parameters at all)
        nodeTextBoldM = true;
        // show number of parameters?
        if (DBHTreeConfigCache::get().getShowColumnParamCount())
        {
            size_t ins = 0, outs = 0;
            for (ParameterPtrs::const_iterator it = function.begin();
                it != function.end(); ++it)
            {
                if ((*it)->isOutputParameter())
                    ++outs;
                else
                    ++ins;
            }
            nodeTextM += wxString::Format(" (%zu, %zu)", ins, outs);
        }
    }
    // show Parameter nodes if Config setting is on
    showChildrenM = DBHTreeConfigCache::get().getShowColumns();
    showNodeExpanderM = showChildrenM && !function.childrenLoaded();
    // update if settings change
    nodeConfigSensitiveM = true;

}

void DBHTreeItemVisitor::visitUDFs(UDFs& functions)
{
    setNodeProperties(&functions, ART_UDFs);
}

void DBHTreeItemVisitor::visitUser(User& user)
{
    setNodeProperties(&user, ART_User);
}

void DBHTreeItemVisitor::visitUsers(Users& /*users*/)
{
    //setNodeProperties(&users, ART_Users);
}

void DBHTreeItemVisitor::visitGenerator(Generator& generator)
{
    // show generator value, but only if it has been loaded already
    generator.loadPendingData();
    if (generator.propertiesLoaded())
    {
        nodeTextM = generator.getName_() + " = ";
        nodeTextM << generator.getValue();
    }
    // set remaining default properties, nodeTextM will not be touched
    setNodeProperties(&generator, ART_Generator);
}

void DBHTreeItemVisitor::visitGenerators(Generators& generators)
{
    setNodeProperties(&generators, ART_Generators);
}

void DBHTreeItemVisitor::visitMethod(Method& method)
{
    bool isFunction = method.isFunction();
    nodeTextM = (isFunction ? "Function " : " Procedure ") + method.getName_();

    setNodeProperties(&method,
        isFunction ? ART_Function : ART_Procedure);
}

void DBHTreeItemVisitor::visitParameter(Parameter& parameter)
{
    bool isOutput = parameter.isOutputParameter();
    nodeTextM = (isOutput ? "out " : "in ")
        + parameter.getName_() + " " + parameter.getDatatype();
    if (!parameter.isNullable(CheckDomainNullability))
    {
        nodeTextM << " " << SqlTokenizer::getKeyword(kwNOT)
            << " " << SqlTokenizer::getKeyword(kwNULL);
    }
    // set remaining default properties, nodeTextM will not be touched
    setNodeProperties(&parameter,
        isOutput ? ART_ParameterOutput : ART_ParameterInput);
}

void DBHTreeItemVisitor::visitPackage(Package& package)
{
    setNodeProperties(&package, ART_Package);
   
   if (package.childrenLoaded())
    {
        // make node caption bold when parameter data is loaded
        // (even if the package has no parameters at all)
        nodeTextBoldM = true;
        // show number of parameters?
        if (DBHTreeConfigCache::get().getShowColumnParamCount())
        {
            size_t ins = 0, outs = 0;
            for (MethodPtrs::const_iterator it = package.begin();
                it != package.end(); ++it)
            {
                if ((*it)->isFunction())
                    ++outs;
                else
                    ++ins;
            }
            nodeTextM += wxString::Format(" (%zu, %zu)", ins, outs);
        }
    }
    // show Parameter nodes if Config setting is on
    showChildrenM = DBHTreeConfigCache::get().getShowColumns();
    showNodeExpanderM = showChildrenM && !package.childrenLoaded();

    // update if settings change
    nodeConfigSensitiveM = true;
}

void DBHTreeItemVisitor::visitPackages(Packages& packages)
{
    setNodeProperties(&packages, ART_Packages); 
}

void DBHTreeItemVisitor::visitSysPackages(SysPackages& packages)
{
    setNodeProperties(&packages, ART_SystemPackages);
}

void DBHTreeItemVisitor::visitProcedure(Procedure& procedure)
{
    setNodeProperties(&procedure, ART_Procedure);
    if (procedure.childrenLoaded())
    {
        // make node caption bold when parameter data is loaded
        // (even if the procedure has no parameters at all)
        nodeTextBoldM = true;
        // show number of parameters?
        if (DBHTreeConfigCache::get().getShowColumnParamCount())
        {
            size_t ins = 0, outs = 0;
            for (ParameterPtrs::const_iterator it = procedure.begin();
                it != procedure.end(); ++it)
            {
                if ((*it)->isOutputParameter())
                    ++outs;
                else
                    ++ins;
            }
            nodeTextM += wxString::Format(" (%zu, %zu)", ins, outs);
        }
    }
    // show Parameter nodes if Config setting is on
    showChildrenM = DBHTreeConfigCache::get().getShowColumns();
    showNodeExpanderM = showChildrenM && !procedure.childrenLoaded();
    // update if settings change
    nodeConfigSensitiveM = true;
}

void DBHTreeItemVisitor::visitProcedures(Procedures& procedures)
{
    setNodeProperties(&procedures, ART_Procedures);
}

void DBHTreeItemVisitor::visitRole(Role& role)
{
    setNodeProperties(&role,
        role.isSystem() ? ART_SystemRole : ART_Role);
}

void DBHTreeItemVisitor::visitRoles(Roles& roles)
{
    setNodeProperties(&roles, ART_Roles);
}

void DBHTreeItemVisitor::visitSysRoles(SysRoles& roles)
{
    setNodeProperties(&roles, ART_SystemRoles);
}

void DBHTreeItemVisitor::visitRoot(Root& root)
{
    setNodeProperties(&root, ART_Root);
    // make root node caption bold even if it has no registered servers
    nodeTextBoldM = true;
    // show Server nodes even though Root::getChildrenCount() returns 0
    showChildrenM = true;
    sortChildrenM = DBHTreeConfigCache::get().getSortServers();
    // update if settings change (sort servers?)
    nodeConfigSensitiveM = true;
}

void DBHTreeItemVisitor::visitServer(Server& server)
{
    setNodeProperties(&server, ART_Server);
    // make server node caption bold even if it has no registered databases
    nodeTextBoldM = true;
    // show Database nodes even though Server::getChildrenCount() returns 0
    showChildrenM = true;
    sortChildrenM = DBHTreeConfigCache::get().getSortDatabases();
    // update if settings change (sort databases?)
    nodeConfigSensitiveM = true;
}

void DBHTreeItemVisitor::visitSysTables(SysTables& tables)
{
    setNodeProperties(&tables, ART_SystemTables); 
}

void DBHTreeItemVisitor::visitGTTable(GTTable& table)
{
    setNodeProperties(&table, ART_GlobalTemporary);

    if (table.childrenLoaded())
    {
        // make node caption bold when column data has been loaded
        nodeTextBoldM = true;
        // show number of columns?
        if (DBHTreeConfigCache::get().getShowColumnParamCount())
        {
            size_t colCount = table.getColumnCount();
            nodeTextM += wxString::Format(" (%zu)", colCount);
        }
    }
    // show Column nodes if Config setting is on
    showChildrenM = DBHTreeConfigCache::get().getShowColumns();
    showNodeExpanderM = showChildrenM && !table.childrenLoaded();
    // update if settings change
    nodeConfigSensitiveM = true;
}

void DBHTreeItemVisitor::visitGTTables(GTTables& tables)
{
    setNodeProperties(&tables, ART_GlobalTemporaries);

}

void DBHTreeItemVisitor::visitTable(Table& table)
{
    setNodeProperties(&table, ART_Table);
    if (table.childrenLoaded())
    {
        // make node caption bold when column data has been loaded
        nodeTextBoldM = true;
        // show number of columns?
        if (DBHTreeConfigCache::get().getShowColumnParamCount())
        {
            size_t colCount = table.getColumnCount();
            
            nodeTextM += wxString::Format(" (%zu)", colCount);
        }
    }
    // show Column nodes if Config setting is on
    showChildrenM = DBHTreeConfigCache::get().getShowColumns();
    showNodeExpanderM = showChildrenM && !table.childrenLoaded();
    // update if settings change
    nodeConfigSensitiveM = true;
}

void DBHTreeItemVisitor::visitTables(Tables& tables)
{
    setNodeProperties(&tables, ART_Tables);
}

void DBHTreeItemVisitor::visitSysTable(SysTable& table)
{
    setNodeProperties(&table, ART_SystemTable);
    if (table.childrenLoaded())
    {
        // make node caption bold when column data has been loaded
        nodeTextBoldM = true;
        // show number of columns?
        if (DBHTreeConfigCache::get().getShowColumnParamCount())
        {
            size_t colCount = table.getColumnCount();
            nodeTextM += wxString::Format(" (%zu)", colCount);
        }
    }
    // show Column nodes if Config setting is on
    showChildrenM = DBHTreeConfigCache::get().getShowColumns();
    showNodeExpanderM = showChildrenM && !table.childrenLoaded();
    // update if settings change
    nodeConfigSensitiveM = true;
}

void DBHTreeItemVisitor::visitDMLTrigger(DMLTrigger& trigger)
{
    nodeEnabledM = trigger.getActive();
    setNodeProperties(&trigger, ART_DMLTrigger);
}

void DBHTreeItemVisitor::visitDMLTriggers(DMLTriggers& triggers)
{
    setNodeProperties(&triggers, ART_DMLTriggers);
}

void DBHTreeItemVisitor::visitDBTrigger(DBTrigger& trigger)
{
    nodeEnabledM = trigger.getActive();
    setNodeProperties(&trigger, ART_DBTrigger);
}

void DBHTreeItemVisitor::visitDBTriggers(DBTriggers& triggers)
{
    setNodeProperties(&triggers, ART_DBTriggers); 
}

void DBHTreeItemVisitor::visitDDLTrigger(DDLTrigger& trigger)
{
    nodeEnabledM = trigger.getActive();
    setNodeProperties(&trigger, ART_DDLTrigger);
}

void DBHTreeItemVisitor::visitDDLTriggers(DDLTriggers& triggers)
{
    setNodeProperties(&triggers, ART_DDLTriggers); 
}

void DBHTreeItemVisitor::visitView(View& view)
{
    setNodeProperties(&view, ART_View);
    if (view.childrenLoaded())
    {
        // make node caption bold when column data has been loaded
        nodeTextBoldM = true;
        // show number of columns?
        if (DBHTreeConfigCache::get().getShowColumnParamCount())
        {
            size_t colCount = view.getColumnCount();
            nodeTextM += wxString::Format(" (%zu)", colCount);
        }
    }
    // show Column nodes if Config setting is on
    showChildrenM = DBHTreeConfigCache::get().getShowColumns();
    showNodeExpanderM = showChildrenM && !view.childrenLoaded();
    // update if settings change
    nodeConfigSensitiveM = true;
}

void DBHTreeItemVisitor::visitViews(Views& views)
{
    setNodeProperties(&views, ART_Views);
}

void DBHTreeItemVisitor::visitIndex(Index& index)
{
    nodeEnabledM = index.isActive();
    
    //setNodeProperties(&index, index.isSystem() ? ART_SystemIndex : ART_Index);
    setNodeProperties(&index, ART_Index);
}

void DBHTreeItemVisitor::visitIndices(Indices& indices)
{
    setNodeProperties(&indices, ART_Indices);
}

void DBHTreeItemVisitor::visitSysIndices(SysIndices& sysIndices)
{
    setNodeProperties(&sysIndices, ART_SystemIndices);
}

void DBHTreeItemVisitor::visitUsrIndices(UsrIndices& usrIndices)
{
    setNodeProperties(&usrIndices, ART_Indices);
}

// TreeSelectionRestorer class
class TreeSelectionRestorer
{
private:
    struct TreeSelectionData
    {
    private:
        MetadataItem* itemM;
        unsigned refCountM;
    public:
        TreeSelectionData() : itemM(0), refCountM(0) {}
        TreeSelectionData(MetadataItem* item) : itemM(item), refCountM(1) {}
        unsigned addRef() { return ++refCountM; }
        unsigned decRef() { return (refCountM > 0) ? --refCountM : 0; }
        MetadataItem* getSelectedItem() { return itemM; }
    };

    typedef std::map<DBHTreeControl*, TreeSelectionData> SelectionMap;
    static SelectionMap& getSelections();
    DBHTreeControl* treeM;
public:
    TreeSelectionRestorer(DBHTreeControl* tree);
    ~TreeSelectionRestorer();
};

TreeSelectionRestorer::TreeSelectionRestorer(DBHTreeControl* tree)
    : treeM(tree)
{
    SelectionMap::iterator pos = getSelections().find(tree);
    if (pos != getSelections().end())
        (*pos).second.addRef();
    else
    {
        MetadataItem* selItem = tree->getSelectedMetadataItem();
        getSelections()[tree] = TreeSelectionData(selItem);
    }
}

TreeSelectionRestorer::~TreeSelectionRestorer()
{
    SelectionMap::iterator pos = getSelections().find(treeM);
    wxCHECK_RET(pos != getSelections().end(),
        "tree selection data not found");
    if ((*pos).second.decRef() == 0)
    {
        MetadataItem* origSelItem = (*pos).second.getSelectedItem();
        if (origSelItem != treeM->getSelectedMetadataItem())
            treeM->selectMetadataItem(origSelItem);
        getSelections().erase(pos);
    }
}

/*static*/
TreeSelectionRestorer::SelectionMap& TreeSelectionRestorer::getSelections()
{
    static SelectionMap sm;
    return sm;
}

// DBHTreeItem is a special kind of observer, which observes special kind
// of subjects: MetadataItem instances
class DBHTreeItemData: public wxTreeItemData, public Observer
{
private:
    DBHTreeControl* treeM;
    MetadataItem* observedItemM;
protected:
    virtual void update();
public:
    DBHTreeItemData(DBHTreeControl* tree);

    wxTreeItemId findSubNode(MetadataItem* item);
    MetadataItem* getObservedMetadata();
    void setObservedMetadata(MetadataItem* item);
};

DBHTreeItemData::DBHTreeItemData(DBHTreeControl* tree)
    : Observer(), treeM(tree), observedItemM(0)
{
}

//! returns tree subnode that points to given metadata object
wxTreeItemId DBHTreeItemData::findSubNode(MetadataItem* item)
{
    wxTreeItemIdValue cookie;
    wxTreeItemId id = GetId();
    for (wxTreeItemId ci = treeM->GetFirstChild(id, cookie); ci.IsOk();
        ci = treeM->GetNextChild(id, cookie))
    {
        if (treeM->getMetadataItem(ci) == item)
            return ci;
    }
    return wxTreeItemId();
}

MetadataItem* DBHTreeItemData::getObservedMetadata()
{
    return observedItemM;
}

void DBHTreeItemData::setObservedMetadata(MetadataItem* item)
{
    if (observedItemM != item)
    {
        if (observedItemM)
            observedItemM->detachObserver(this);
        observedItemM = item;
        if (observedItemM)
            observedItemM->attachObserver(this, true);
    }
}

struct MetadataItemSorter
{
    bool operator() (MetadataItem* item1, MetadataItem* item2)
    {
        wxString name1(item1->getName_());
        wxString name2(item2->getName_());

        // use (very basic) natural sort
        // this makes sure that for example "Server10" comes after "Server2"
        wxArrayString letterChunks1, letterChunks2;

        wxString digits("0123456789");
        size_t start1 = 0, start2 = 0;
        size_t len1 = name1.size(), len2 = name2.size();
        while (start1 < len1 || start2 < len2)
        {
            // partial strings from consecutive non-digits
            size_t end1 = wxMin(name1.find_first_of(digits, start1), len1);
            size_t end2 = wxMin(name2.find_first_of(digits, start2), len2);
            if (end1 > 0 || end2 > 0)
            {
                wxString chunk1(name1, start1, end1);
                wxString chunk2(name2, start2, end2);
                // return immediately when case-insensitive comparison
                // gives a result
                int i = chunk1.CmpNoCase(chunk2);
                if (i != 0)
                    return i < 0;
                // otherwise store the chunks for second comparison
                // (case-sensitive)
                letterChunks1.push_back(chunk1);
                letterChunks2.push_back(chunk2);

                start1 = end1;
                start2 = end2;
            }

            // partial strings from consecutive digits
            end1 = wxMin(name1.find_first_not_of(digits, start1), len1);
            end2 = wxMin(name2.find_first_not_of(digits, start2), len2);
            if (end1 > 0 || end2 > 0)
            {
                wxString chunk1(name1, start1, end1);
                wxString chunk2(name2, start2, end2);
                wxLongLong_t l1, l2;
                if (!chunk1.ToLongLong(&l1))
                    l1 = 0;
                if (!chunk2.ToLongLong(&l2))
                    l2 = 0;
                if (l1 != l2)
                    return l1 < l2;
                // don't store numbers: if they are equal they need not be 
                // checked again in second loop (case-sensitive comparison)
                start1 = end1;
                start2 = end2;
            }
        }

        int res = 0;
        for (size_t i = 0; res == 0 && i < letterChunks1.size(); ++i)
            res = letterChunks1[i].Cmp(letterChunks2[i]);
        return res < 0;
/*
        int i = name1.CmpNoCase(name2);
        if (i == 0)
            i = name1.Cmp(name2);
        return i < 0;
*/
    };
};

//! parent nodes are responsible for "insert" / "delete"
//! node is responsible for "update"
void DBHTreeItemData::update()
{
    wxTreeItemId id = GetId();
    if (!id.IsOk())
        return;

    MetadataItem* object = getObservedMetadata();
    if (!object)
        return;

    // keep the currently selected item selected when this method ends,
    // even though its position in the tree may have changed
    TreeSelectionRestorer tsr(treeM);

    // set node properties of current item
    DBHTreeItemVisitor tivObject(treeM);
    object->loadPendingData();
    object->acceptVisitor(&tivObject);
    if (treeM->GetItemText(id) != tivObject.getNodeText())
        treeM->SetItemText(id, tivObject.getNodeText());
    if (treeM->GetItemImage(id) != tivObject.getNodeImage())
        treeM->SetItemImage(id, tivObject.getNodeImage());
    // track number of visible child nodes for SetItemHasChildren() calls
    unsigned numVisibleChildren = 0;
    // check subitems
    std::vector<MetadataItem*> children;
    std::vector<MetadataItem*>::iterator itChild;
    if (tivObject.getShowChildren())
    {
        if (object->getChildren(children))
        {
            // sort child nodes if necessary
            if (tivObject.getSortChildren())
            {
                MetadataItemSorter sorter;
                std::sort(children.begin(), children.end(), sorter);
            }

            wxTreeItemId prevId;
            // create or update child nodes
            for (itChild = children.begin(); itChild != children.end(); ++itChild)
            {
                DBHTreeItemVisitor tivChild(treeM);
                (*itChild)->loadPendingData();
                (*itChild)->acceptVisitor(&tivChild);
                if (!tivChild.getNodeVisible())
                    continue;
                ++numVisibleChildren;

                wxTreeItemId childId = findSubNode(*itChild);
                // order of child nodes may have changed
                // since nodes can't be moved they have to be recreated
                if (childId.IsOk())
                {
                    wxTreeItemId prevChildId = treeM->GetPrevSibling(childId);
                    if (prevChildId != prevId)
                    {
                        treeM->Delete(childId);
                        childId.Unset();
                    }
                }

                if (!childId.IsOk())
                {
                    DBHTreeItemData* newItem = new DBHTreeItemData(treeM);
                    if (prevId.IsOk())
                    {
                        childId = treeM->InsertItem(id, prevId,
                            tivChild.getNodeText(), tivChild.getNodeImage(),
                            -1, newItem);
                    }
                    else // first
                    {
                        childId = treeM->PrependItem(id, tivChild.getNodeText(),
                            tivChild.getNodeImage(), -1, newItem);
                    }
                    // setObservedMetadata() calls attachObserver(), which
                    // calls update() on the newly created child node
                    // this will correctly populate the tree
                    newItem->setObservedMetadata(*itChild);
                    // tree node data objects may optionally observe the settings
                    // cache object, for example to create / delete column and
                    // parameter nodes if the "ShowColumnsInTree" setting changes
                    if (tivChild.isConfigSensitive())
                        DBHTreeConfigCache::get().attachObserver(newItem, false);
                }
                else
                {
                    if (treeM->GetItemText(childId) != tivChild.getNodeText())
                        treeM->SetItemText(childId, tivChild.getNodeText());
                    if (treeM->GetItemImage(childId) != tivChild.getNodeImage())
                        treeM->SetItemImage(childId, tivChild.getNodeImage());
                }
                prevId = childId;
            }
        }
    }

    bool canCollapseNode = id != treeM->GetRootItem()
        || (treeM->GetWindowStyle() & wxTR_HIDE_ROOT) == 0;

    // remove all children at once
    if (numVisibleChildren == 0)
    {
        if (treeM->ItemHasChildren(id))
        {
            if (canCollapseNode)
                treeM->Collapse(id);
            treeM->DeleteChildren(id);
        }
        // allow for on-demand-loading of children
        treeM->SetItemHasChildren(id, tivObject.getShowNodeExpander());
        treeM->SetItemBold(id, tivObject.getNodeTextBold());
        if (!tivObject.getNodeEnabled())
            treeM->SetItemTextColour(id, wxColour(0x080, 0x080, 0x080));
            //treeM->SetItemTextColour(id, wxSYS_COLOUR_GRAYTEXT);
        else
            treeM->SetItemTextColour(id, wxSystemSettings::GetColour(wxSYS_COLOUR_CAPTIONTEXT));
        




        return;
    }
    treeM->SetItemHasChildren(id, true);

    // remove delete items - one by one
    bool itemsDeleted = false;
    wxTreeItemIdValue cookie;
    wxTreeItemId item = treeM->GetFirstChild(id, cookie);
    while (item.IsOk())
    {
        itChild = find(children.begin(), children.end(),
            treeM->getMetadataItem(item));
        // we may need to hide disconnected databases
        if (DBHTreeConfigCache::get().getHideDisconnectedDatabases()
            && itChild != children.end())
        {
            Database* db = dynamic_cast<Database*>(*itChild);
            if (db && !db->isConnected())
                itChild = children.end();
        }
        // delete tree node and all children if metadata item not found
        if (itChild == children.end())
        {
            itemsDeleted = true;
            treeM->DeleteChildren(item);
            treeM->Delete(item);
            item = treeM->GetFirstChild(id, cookie);
            continue;
        }
        item = treeM->GetNextChild(id, cookie);
    }
    // force-collapse node if all children deleted
    if (itemsDeleted && 0 == treeM->GetChildrenCount(id, false)
        && canCollapseNode)
    {
        treeM->Collapse(id);
    }

    treeM->SetItemBold(id, tivObject.getNodeTextBold()
        || treeM->ItemHasChildren(id));
    //treeM->SetBackgroundColour(wxYELLOW);
}

BEGIN_EVENT_TABLE(DBHTreeControl, wxTreeCtrl)
    EVT_CONTEXT_MENU(DBHTreeControl::OnContextMenu)
    EVT_TREE_BEGIN_DRAG(wxID_ANY, DBHTreeControl::OnBeginDrag)
    EVT_TREE_ITEM_EXPANDING(wxID_ANY, DBHTreeControl::OnTreeItemExpanding)
END_EVENT_TABLE()

void DBHTreeControl::OnBeginDrag(wxTreeEvent& event)
{
    if (!DBHTreeConfigCache::get().allowDnD())
    {
        event.Skip();
        return;
    }

    wxTreeItemId item = event.GetItem();
    if (item.IsOk())
    {
        SelectItem(item);   // Needed for MSW!!!
        MetadataItem *m = getMetadataItem(item);
        if (!m)
            return;
        wxString test;
        test.Printf("OBJECT:%p", m);
        wxTextDataObject textData(test);
        wxDropSource source(textData, this);
        source.DoDragDrop(wxDrag_AllowMove);
    }
    else
        event.Skip();
}

//! Creates context menu
void DBHTreeControl::OnContextMenu(wxContextMenuEvent& event)
{
    if (!allowContextMenuM)
        return;

    // select item under the mouse first, since right-click doesn't change
    // the selection under GTK
    // NOTE: removing the SelectItem() call for wxMSW does not work either,
    // because commands will be enabled for the selected, not for the
    // highlighted item :-(
    wxPoint pos = ScreenToClient(event.GetPosition());
    int flags;
    const int checkFlags = wxTREE_HITTEST_ONITEMBUTTON
#ifdef __WXMSW__
        | wxTREE_HITTEST_ONITEMINDENT | wxTREE_HITTEST_ONITEMRIGHT
#endif
        | wxTREE_HITTEST_ONITEMICON | wxTREE_HITTEST_ONITEMLABEL;
    wxTreeItemId item = HitTest(pos, flags);
    if (item.IsOk() && (flags & checkFlags))
        SelectItem(item);
    else
    {
        item = GetSelection();
        wxRect r;
        if (item.IsOk() && GetBoundingRect(item, r, true))
        {
            pos = r.GetPosition();
            pos.y += r.height/2;
        }
    }

    wxMenu MyMenu;    // create context menu, depending on type of clicked item
    if (!item.IsOk())
        item = GetRootItem();
    MetadataItem* i = getMetadataItem(item);
    if (!i)
        return;
    ContextMenuMetadataItemVisitor cmv(&MyMenu);
    i->acceptVisitor(&cmv);

    if (MyMenu.GetMenuItemCount())
        PopupMenu(&MyMenu, pos);
}

void DBHTreeControl::OnTreeItemExpanding(wxTreeEvent& event)
{
    MetadataItem* mi = getMetadataItem(event.GetItem());
    if (mi)
        mi->ensureChildrenLoaded();
    event.Skip();
}

DBHTreeControl::DBHTreeControl(wxWindow* parent, const wxPoint& pos,
        const wxSize& size, long style)
    : wxTreeCtrl(parent, ID_tree_ctrl, pos, size, style)
{
    allowContextMenuM = true;
/*  FIXME: dows not play nice with wxGenericImageList...
           need to check whether sharing the image list has bad side-effects!

    // create copy of static image list, set to be autodeleted
    AssignImageList(new wxImageList(DBHTreeImageList::get()));
*/
    SetImageList(&DBHTreeImageList::get());
}

void DBHTreeControl::allowContextMenu(bool doAllow)
{
    allowContextMenuM = doAllow;
}

//! Override wxWidgets method, since it's buggy (doesn't handle negative values properly)
void DBHTreeControl::SetSpacing(short spacing)
{
    wxTreeCtrl::SetSpacing(spacing);
    m_spacing = spacing;
}

wxTreeItemId DBHTreeControl::addRootNode(MetadataItem* rootItem)
{
    wxASSERT(rootItem);
    // no need to set node text and image index,
    // because DBHTreeItemData::update() will do it (and create child nodes)
    wxTreeItemId id = AddRoot(wxEmptyString);
    DBHTreeItemData* rootdata = new DBHTreeItemData(this);
    SetItemData(id, rootdata);
    rootdata->setObservedMetadata(rootItem);
    // server nodes may need to be reordered
    DBHTreeConfigCache::get().attachObserver(rootdata, false);
    return id;
}

//! returns the object that selected wxTree node observes
MetadataItem* DBHTreeControl::getSelectedMetadataItem()
{
    return getMetadataItem(GetSelection());
}

//! returns the object that some wxTree node observes
MetadataItem* DBHTreeControl::getMetadataItem(wxTreeItemId item)
{
    if (item.IsOk())
    {
        if (DBHTreeItemData* tid = (DBHTreeItemData*)GetItemData(item))
            return tid->getObservedMetadata();
    }
    return 0;
}

// recursively searches children for item
bool DBHTreeControl::findMetadataItem(MetadataItem *item, wxTreeItemId parent)
{
    wxTreeItemIdValue cookie;
    if (item == getMetadataItem(parent))
    {
        SelectItem(parent);
        EnsureVisible(parent);
        return true;
    }
    for (wxTreeItemId node = GetFirstChild(parent, cookie); node.IsOk();
        node = GetNextChild(parent, cookie))
    {
        if (findMetadataItem(item, node))
            return true;
    }
    return false;
}

bool DBHTreeControl::selectMetadataItem(MetadataItem* item)
{
    return item && findMetadataItem(item, GetRootItem());
}

//! recursively get the last child of item
wxTreeItemId DBHTreeControl::getLastItem(wxTreeItemId id)
{
    wxTreeItemId temp = GetLastChild(id);
    if (temp.IsOk())
        return getLastItem(temp);
    else
        return id;
}

//! get the previous item vertically
wxTreeItemId DBHTreeControl::getPreviousItem(wxTreeItemId current)
{
    wxTreeItemId temp = current;
    temp = GetPrevSibling(temp);
    if (!temp.IsOk())
    {
        temp = GetItemParent(current);
        if (temp.IsOk())
            return temp;
        else
            return getLastItem(GetRootItem());
    }
    else
        return getLastItem(temp);
}

//! get the next item vertically
wxTreeItemId DBHTreeControl::getNextItem(wxTreeItemId current)
{
    wxTreeItemId temp = current;
    wxTreeItemIdValue cookie;   // dummy - not really used
    if (((ItemHasChildren(temp)) && (GetFirstChild(temp, cookie).IsOk()))) //It tries to read de node content, but for FB objects not loaded, it raises error, the ideal is to skip, or to load (all) the database content?
        temp = GetFirstChild(temp, cookie);
    else
    {
        while (true)
        {
            if (temp == GetRootItem()) // back to the root (start search from top)
                break;
            wxTreeItemId t = temp;
            temp = GetNextSibling(t);
            if (temp.IsOk())
                break;
            else
                temp = GetItemParent(t);
        }
    }
    return temp;
}

//! searches for next node whose text starts with "text"
//! where "text" can contain wildcards: * and ?
bool DBHTreeControl::findText(const wxString& text, bool forward)
{
    wxString searchString = text.Upper() + "*";
    // start from the current position in tree and look forward
    // for item that starts with that name
    wxTreeItemId start = GetSelection();
    wxTreeItemId temp = start;
    while (true)
    {
        wxString current = GetItemText(temp);
        if (current.Upper().Matches(searchString))   // found?
        {
            if (temp != start)
                SelectItem(temp);
            EnsureVisible(temp);
            return true;
        }

        if (forward)
            temp = getNextItem(temp);
        else
            temp = getPreviousItem(temp);
        if (temp == start)  // not found (we wrapped around completely)
            return false;
    }
}

