/*
  Copyright (c) 2026 The FlameRobin Development Team

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

#include "metadata/database.h"
#include "metadata/relation.h"
#include "metadata/procedure.h"
#include "metadata/table.h"
#include "metadata/view.h"
#include "metadata/trigger.h"
#include "metadata/MetadataItemDescriptionVisitor.h"
#include "metadata/constraints.h"
#include "metadata/package.h"
#include "metadata/column.h"
#include "metadata/parameter.h"
#include "engine/MetadataLoader.h"
#include "config/Config.h"

// --- MetadataItem Stubs ---
MetadataItem::MetadataItem() : parentM(0), typeM(ntUnknown), metadataIdM(-1) {}
MetadataItem::MetadataItem(NodeType type, MetadataItem* parent, const wxString& name, int id) 
    : parentM(parent), typeM(type), identifierM(name), metadataIdM(id) {}
MetadataItem::~MetadataItem() {}

wxString MetadataItem::getName_() const { return identifierM.get(); }
wxString MetadataItem::getQuotedName() const { return identifierM.getQuoted(); }
Identifier MetadataItem::getIdentifier() const { return identifierM; }
void MetadataItem::setName_(const wxString& name) { identifierM.setText(name); }
NodeType MetadataItem::getType() const { return typeM; }
void MetadataItem::setType(NodeType type) { typeM = type; }
int MetadataItem::getMetadataId() { return metadataIdM; }
void MetadataItem::setMetadataId(int id) { metadataIdM = id; }

const wxString MetadataItem::getTypeName() const { return ""; }
const wxString MetadataItem::getItemPath() const { return ""; }
const wxString MetadataItem::getPathId() const { return ""; }
const wxString MetadataItem::getId() const { return getName_(); }

void MetadataItem::loadDescription() {}
void MetadataItem::saveDescription(const wxString&) {}
void MetadataItem::saveDescription(const wxString&, const wxString&) {}
wxString MetadataItem::getDescription() { return ""; }
bool MetadataItem::getDescription(wxString&) { return false; }

void MetadataItem::loadProperties() {}
void MetadataItem::loadChildren() {}
void MetadataItem::lockChildren() {}
void MetadataItem::unlockChildren() {}
void MetadataItem::doSetChildrenLoaded(bool) {}
bool MetadataItem::getChildren(std::vector<MetadataItem *>&) { return false; }
void MetadataItem::ensureChildrenLoaded() {}

void MetadataItem::lockSubject() {}
void MetadataItem::unlockSubject() {}
void MetadataItem::invalidate() {}

DatabasePtr MetadataItem::getDatabase() const { return DatabasePtr(); }
fr::IDatabasePtr MetadataItem::getDALDatabase() const { return fr::IDatabasePtr(); }
wxString MetadataItem::getDropSqlStatement() const { return ""; }
MetadataItem* MetadataItem::getParent() const { return parentM; }
void MetadataItem::setParent(MetadataItem* parent) { parentM = parent; }

bool MetadataItem::isSystem() const { return false; }
void MetadataItem::acceptVisitor(MetadataItemVisitor*) {}

// --- MetadataItemVisitor Stubs ---
MetadataItemVisitor::MetadataItemVisitor() {}
MetadataItemVisitor::~MetadataItemVisitor() {}
void MetadataItemVisitor::visitCharacterSet(CharacterSet&) {}
void MetadataItemVisitor::visitCharacterSets(CharacterSets&) {}
void MetadataItemVisitor::visitCollation(Collation&) {}
void MetadataItemVisitor::visitSysCollations(SysCollations&) {}
void MetadataItemVisitor::visitCollations(Collations&) {}
void MetadataItemVisitor::visitColumn(Column&) {}
void MetadataItemVisitor::visitDatabase(Database&) {}
void MetadataItemVisitor::visitDBTrigger(DBTrigger&) {}
void MetadataItemVisitor::visitDBTriggers(DBTriggers&) {}
void MetadataItemVisitor::visitDDLTrigger(DDLTrigger&) {}
void MetadataItemVisitor::visitDDLTriggers(DDLTriggers&) {}
void MetadataItemVisitor::visitDMLTrigger(DMLTrigger&) {}
void MetadataItemVisitor::visitDMLTriggers(DMLTriggers&) {}
void MetadataItemVisitor::visitDomain(Domain&) {}
void MetadataItemVisitor::visitDomains(Domains&) {}
void MetadataItemVisitor::visitException(Exception&) {}
void MetadataItemVisitor::visitExceptions(Exceptions&) {}
void MetadataItemVisitor::visitForeignKey(ForeignKey&) {}
void MetadataItemVisitor::visitFunctionSQL(FunctionSQL&) {}
void MetadataItemVisitor::visitFunctionSQLs(FunctionSQLs&) {}
void MetadataItemVisitor::visitGenerator(Generator&) {}
void MetadataItemVisitor::visitGenerators(Generators&) {}
void MetadataItemVisitor::visitGTTable(GTTable&) {}
void MetadataItemVisitor::visitGTTables(GTTables&) {}
void MetadataItemVisitor::visitIndex(Index&) {}
void MetadataItemVisitor::visitIndices(Indices&) {}
void MetadataItemVisitor::visitMetadataItem(MetadataItem&) {}
void MetadataItemVisitor::visitMethod(Method&) {}
void MetadataItemVisitor::visitParameter(Parameter&) {}
void MetadataItemVisitor::visitPrimaryKeyConstraint(PrimaryKeyConstraint&) {}
void MetadataItemVisitor::visitPackage(Package&) {}
void MetadataItemVisitor::visitPackages(Packages&) {}
void MetadataItemVisitor::visitProcedure(Procedure&) {}
void MetadataItemVisitor::visitProcedures(Procedures&) {}
void MetadataItemVisitor::visitPublication(Publication&) {}
void MetadataItemVisitor::visitPublications(Publications&) {}
void MetadataItemVisitor::visitRelation(Relation&) {}
void MetadataItemVisitor::visitReplication(Replication&) {}
void MetadataItemVisitor::visitRole(Role&) {}
void MetadataItemVisitor::visitRoles(Roles&) {}
void MetadataItemVisitor::visitRoot(Root&) {}
void MetadataItemVisitor::visitServer(Server&) {}
void MetadataItemVisitor::visitSysIndices(SysIndices&) {}
void MetadataItemVisitor::visitUsrIndices(UsrIndices&) {}
void MetadataItemVisitor::visitSysDomains(SysDomains&) {}
void MetadataItemVisitor::visitSysPackages(SysPackages&) {}
void MetadataItemVisitor::visitSysRoles(SysRoles&) {}
void MetadataItemVisitor::visitSysTable(SysTable&) {}
void MetadataItemVisitor::visitSysTables(SysTables&) {}
void MetadataItemVisitor::visitTable(Table&) {}
void MetadataItemVisitor::visitTables(Tables&) {}
void MetadataItemVisitor::visitTrigger(Trigger&) {}
void MetadataItemVisitor::visitTriggers(Triggers&) {}
void MetadataItemVisitor::visitUDF(UDF&) {}
void MetadataItemVisitor::visitUDFs(UDFs&) {}
void MetadataItemVisitor::visitUser(User&) {}
void MetadataItemVisitor::visitUsers(Users&) {}
void MetadataItemVisitor::visitUniqueConstraint(UniqueConstraint&) {}
void MetadataItemVisitor::visitView(View&) {}
void MetadataItemVisitor::visitViews(Views&) {}

// --- Relation Stubs ---
Relation::Relation(NodeType type, DatabasePtr database, const wxString& name) 
    : MetadataItem(type, (MetadataItem*)database.get(), name, 0) {}
ColumnPtr Relation::findColumn(const wxString&) const { return ColumnPtr(); }
void Relation::loadProperties() {}
void Relation::loadChildren() {}
void Relation::lockChildren() {}
void Relation::unlockChildren() {}
void Relation::setExternalFilePath(const wxString&) {}
void Relation::setSource(const wxString&) {}
bool Relation::getChildren(std::vector<MetadataItem *>&) { return false; }

// --- Table Stubs ---
Table::Table(DatabasePtr database, const wxString& name) 
    : Relation(ntTable, database, name) {}
std::vector<ForeignKey>* Table::getForeignKeys() { return nullptr; }
const wxString Table::getTypeName() const { return ""; }
void Table::acceptVisitor(MetadataItemVisitor*) {}
void Table::loadChildren() {}
void Table::setExternalFilePath(const wxString&) {}

// --- View Stubs ---
View::View(DatabasePtr database, const wxString& name) 
    : Relation(ntView, database, name) {}
const wxString View::getTypeName() const { return ""; }
void View::acceptVisitor(MetadataItemVisitor*) {}
void View::setSource(const wxString&) {}

// --- Procedure Stubs ---
Procedure::Procedure(DatabasePtr database, const wxString& name) 
    : MetadataItem(ntProcedure, (MetadataItem*)database.get(), name, 0) {}
ParameterPtr Procedure::findParameter(const wxString&) const { return ParameterPtr(); }
const wxString Procedure::getTypeName() const { return ""; }
void Procedure::acceptVisitor(MetadataItemVisitor*) {}
void Procedure::loadChildren() {}
void Procedure::lockChildren() {}
void Procedure::unlockChildren() {}
bool Procedure::getChildren(std::vector<MetadataItem *>&) { return false; }
wxString Procedure::getQuotedName() const { return getIdentifier().getQuoted(); }

// --- Trigger Stubs ---
Trigger::Trigger(NodeType type, DatabasePtr database, const wxString& name)
    : MetadataItem(type, database.get(), name) {}
const wxString Trigger::getTypeName() const { return ""; }
void Trigger::acceptVisitor(MetadataItemVisitor*) {}
void Trigger::loadProperties() {}

DMLTrigger::DMLTrigger(DatabasePtr database, const wxString& name) 
    : Trigger(ntDMLTrigger, database, name) {}
void DMLTrigger::acceptVisitor(MetadataItemVisitor*) {}

DDLTrigger::DDLTrigger(DatabasePtr database, const wxString& name) 
    : Trigger(ntDDLTrigger, database, name) {}
void DDLTrigger::acceptVisitor(MetadataItemVisitor*) {}

DBTrigger::DBTrigger(DatabasePtr database, const wxString& name) 
    : Trigger(ntDBTrigger, database, name) {}
void DBTrigger::acceptVisitor(MetadataItemVisitor*) {}

// --- Database Stubs ---
DatabaseAuthenticationMode::DatabaseAuthenticationMode() {}
Database::Database() : MetadataItem(ntDatabase, nullptr, "", 0) {}
Database::~Database() {}
MetadataLoader* Database::getMetadataLoader() { return nullptr; }
wxMBConv* Database::getCharsetConverter() const { return nullptr; }
wxArrayString Database::loadIdentifiers(const wxString&, ProgressIndicator*) { return wxArrayString(); }
int Database::getSqlDialect() const { return 3; }
MetadataItem* Database::findByNameAndType(NodeType, const wxString&) { return nullptr; }
Relation* Database::findRelation(const Identifier&) { return nullptr; }
CharacterSetPtr Database::getCharsetById(int) { return CharacterSetPtr(); }
DatabasePtr Database::getDatabase() const { return std::const_pointer_cast<Database>(shared_from_this()); }
const wxString Database::getTypeName() const { return ""; }
const wxString Database::getId() const { return getName_(); }
void Database::acceptVisitor(MetadataItemVisitor*) {}
fr::IDatabasePtr Database::getDALDatabase() const { return fr::IDatabasePtr(); }
void Database::loadChildren() {}
void Database::lockChildren() {}
void Database::unlockChildren() {}
bool Database::getChildren(std::vector<MetadataItem *>&) { return false; }

// --- MetadataLoader Stubs ---
MetadataLoaderTransaction::MetadataLoaderTransaction(MetadataLoader*) {}
MetadataLoaderTransaction::~MetadataLoaderTransaction() {}
fr::IStatementPtr& MetadataLoader::getStatement(const std::string&) 
{ 
    static fr::IStatementPtr dummy;
    return dummy;
}

// --- Visitor Stubs ---
LoadDescriptionVisitor::LoadDescriptionVisitor() : MetadataItemVisitor() {}
bool LoadDescriptionVisitor::descriptionAvailable() const { return false; }
wxString LoadDescriptionVisitor::getDescription() const { return ""; }

void LoadDescriptionVisitor::visitCharcterSet(CharacterSet&) {}
void LoadDescriptionVisitor::visitCollation(Collation&) {}
void LoadDescriptionVisitor::visitColumn(Column&) {}
void LoadDescriptionVisitor::visitDomain(Domain&) {}
void LoadDescriptionVisitor::visitException(Exception&) {}
void LoadDescriptionVisitor::visitFunctionSQL(FunctionSQL&) {}
void LoadDescriptionVisitor::visitUDF(UDF&) {}
void LoadDescriptionVisitor::visitGenerator(Generator&) {}
void LoadDescriptionVisitor::visitIndex(Index&) {}
void LoadDescriptionVisitor::visitParameter(Parameter&) {}
void LoadDescriptionVisitor::visitPackage(Package&) {}
void LoadDescriptionVisitor::visitProcedure(Procedure&) {}
void LoadDescriptionVisitor::visitPublication(Publication&) {}
void LoadDescriptionVisitor::visitRelation(Relation&) {}
void LoadDescriptionVisitor::visitRole(Role&) {}
void LoadDescriptionVisitor::visitTrigger(Trigger&) {}

SaveDescriptionVisitor::SaveDescriptionVisitor(wxString) : MetadataItemVisitor() {}
void SaveDescriptionVisitor::visitCharacterSet(CharacterSet&) {}
void SaveDescriptionVisitor::visitCollation(Collation&) {}
void SaveDescriptionVisitor::visitColumn(Column&) {}
void SaveDescriptionVisitor::visitDomain(Domain&) {}
void SaveDescriptionVisitor::visitException(Exception&) {}
void SaveDescriptionVisitor::visitFunctionSQL(FunctionSQL&) {}
void SaveDescriptionVisitor::visitUDF(UDF&) {}
void SaveDescriptionVisitor::visitGenerator(Generator&) {}
void SaveDescriptionVisitor::visitIndex(Index&) {}
void SaveDescriptionVisitor::visitParameter(Parameter&) {}
void SaveDescriptionVisitor::visitProcedure(Procedure&) {}
void SaveDescriptionVisitor::visitPublication(Publication&) {}
void SaveDescriptionVisitor::visitRelation(Relation&) {}
void SaveDescriptionVisitor::visitRole(Role&) {}
void SaveDescriptionVisitor::visitTrigger(Trigger&) {}

// --- Constraint Stubs ---
const wxString Constraint::getTypeName() const { return ""; }
bool Constraint::isSystem() const { return false; }
Table* Constraint::getTable() const { return nullptr; }

void UniqueConstraint::acceptVisitor(MetadataItemVisitor*) {}
void PrimaryKeyConstraint::acceptVisitor(MetadataItemVisitor*) {}
void ForeignKey::acceptVisitor(MetadataItemVisitor*) {}

// --- Column Stubs ---
ColumnBase::ColumnBase(NodeType type, MetadataItem* parent, const wxString& name)
    : MetadataItem(type, parent, name) {}
wxString ColumnBase::getComputedSource() const { return ""; }
wxString ColumnBase::getDatatype(bool) { return ""; }
wxString ColumnBase::getSource(bool) { return ""; }
wxString ColumnBase::getTypeOf(bool) { return ""; }
bool ColumnBase::isTypeOf() { return false; }

Column::Column(Relation* relation, const wxString& name)
    : ColumnBase(ntColumn, relation, name) {}
const wxString Column::getTypeName() const { return ""; }
wxString Column::getDropSqlStatement() const { return ""; }
wxString Column::getComputedSource() const { return ""; }
wxString Column::getSource(bool) { return ""; }
void Column::acceptVisitor(MetadataItemVisitor*) {}

Parameter::Parameter(MetadataItem* parent, const wxString& name)
    : ColumnBase(ntParameter, parent, name) {}
const wxString Parameter::getTypeName() const { return ""; }
wxString Parameter::getTypeOf(bool) { return ""; }
bool Parameter::isTypeOf() { return false; }
void Parameter::acceptVisitor(MetadataItemVisitor*) {}

// --- Config Stubs ---
Config::Config() {}
Config::~Config() {}
wxFileName Config::getConfigFileName() const { return wxFileName(); }
bool Config::keyExists(const wxString&) const { return false; }
bool Config::getValue(const wxString&, wxString&) { return false; }
bool Config::setValue(const wxString&, const wxString&) { return false; }
void Config::lockedChanged(bool) {}

FRConfig& config() 
{ 
    static FRConfig c;
    return c;
}

wxFileName FRConfig::getConfigFileName() const { return wxFileName(); }

ConfigCache::ConfigCache(Config&) {}
void ConfigCache::ensureCacheValid() {}
void ConfigCache::update() {}
void ConfigCache::loadFromConfig() {}

template<> bool Config::get<bool>(const wxString&, const bool& def) { return def; }
bool Config::getValue(const wxString&, bool&) { return false; }

// --- Utils Stubs ---
wxString unquote(const wxString& s, const wxString&) { return s; }
