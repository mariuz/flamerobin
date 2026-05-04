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
#include "metadata/trigger.h"
#include "metadata/MetadataItemDescriptionVisitor.h"
#include "metadata/constraints.h"
#include "engine/MetadataLoader.h"
#include "config/Config.h"

// --- MetadataItem Stubs ---
const wxString MetadataItem::getTypeName() const { return ""; }
void MetadataItem::loadDescription() {}
void MetadataItem::saveDescription(const wxString&, const wxString&) {}

// --- Database Stubs ---
MetadataLoader* Database::getMetadataLoader() { return nullptr; }
fr::ICharsetConverterPtr Database::getCharsetConverter() const { return fr::ICharsetConverterPtr(); }
std::vector<Identifier> Database::loadIdentifiers(const wxString&, ProgressIndicator*) { return std::vector<Identifier>(); }
int Database::getSqlDialect() const { return 3; }
MetadataItem* Database::findByNameAndType(NodeType, const wxString&) { return nullptr; }
Relation* Database::findRelation(const Identifier&) { return nullptr; }
CharacterSetPtr Database::getCharsetById(int) { return CharacterSetPtr(); }

// --- MetadataLoader Stubs ---
MetadataLoaderTransaction::MetadataLoaderTransaction(MetadataLoader*) {}
MetadataLoaderTransaction::~MetadataLoaderTransaction() {}
fr::IStatementPtr& MetadataLoader::getStatement(const std::string&) 
{ 
    static fr::IStatementPtr dummy;
    return dummy;
}

// --- Table Stubs ---
std::vector<ForeignKey> Table::getForeignKeys() { return std::vector<ForeignKey>(); }

// --- Trigger Stubs ---
Trigger::Trigger(NodeType type, Database* database, const wxString& name)
    : MetadataItem(type, database, name) {}
Trigger::~Trigger() {}

DMLTrigger::DMLTrigger(DatabasePtr, const wxString&) 
    : Trigger(ntTrigger, nullptr, "") {}
DMLTrigger::~DMLTrigger() {}

// --- Visitor Stubs ---
LoadDescriptionVisitor::LoadDescriptionVisitor() : MetadataItemVisitor() {}
bool LoadDescriptionVisitor::descriptionAvailable() const { return false; }
wxString LoadDescriptionVisitor::getDescription() const { return ""; }

SaveDescriptionVisitor::SaveDescriptionVisitor(wxString) : MetadataItemVisitor() {}
SaveDescriptionVisitor::~SaveDescriptionVisitor() {}

// --- Constraint Stubs ---
Constraint::Constraint(NodeType type, Table* table, const wxString& name)
    : MetadataItem(type, table, name) {}
Constraint::~Constraint() {}
const wxString Constraint::getTypeName() const { return ""; }
bool Constraint::isSystem() const { return false; }
Table* Constraint::getTable() const { return nullptr; }

ColumnConstraint::ColumnConstraint(NodeType type, Table* table, const wxString& name)
    : Constraint(type, table, name) {}
ColumnConstraint::~ColumnConstraint() {}

ForeignKey::ForeignKey() : ColumnConstraint(ntForeignKey, nullptr, "") {}
ForeignKey::~ForeignKey() {}

// --- Config Stubs ---
FRConfig& config() 
{ 
    static FRConfig c;
    return c;
}

wxFileName FRConfig::getConfigFileName() const { return wxFileName(); }

ConfigCache::ConfigCache(Config&) {}
ConfigCache::~ConfigCache() {}
void ConfigCache::ensureCacheValid() {}
void ConfigCache::update() {}

template<> bool Config::get<bool>(const wxString&, const bool& def) { return def; }
bool Config::getValue(const wxString&, bool&) { return false; }

// --- Utils Stubs ---
wxString unquote(const wxString& s, const wxString&) { return s; }
