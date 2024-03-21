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

#include <wx/encconv.h>
#include <wx/fontmap.h>

#include <algorithm>
#include <functional>

#include <thread>
#include <future>
#include <chrono>

#include "config/Config.h"
#include "config/DatabaseConfig.h"
#include "core/FRError.h"
#include "core/ProgressIndicator.h"
#include "core/StringUtils.h"
#include "engine/MetadataLoader.h"
#include "MasterPassword.h"
#include "metadata/CharacterSet.h"
#include "metadata/column.h"
#include "metadata/database.h"
#include "metadata/domain.h"
#include "metadata/exception.h"
#include "metadata/function.h"
#include "metadata/generator.h"
#include "metadata/Index.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/parameter.h"
#include "metadata/package.h"
#include "metadata/procedure.h"
#include "metadata/role.h"
#include "metadata/root.h"
#include "metadata/server.h"
#include "metadata/table.h"
#include "metadata/trigger.h"
#include "metadata/view.h"
#include "sql/SqlStatement.h"
#include "sql/SqlTokenizer.h"

// Credentials class
void Credentials::setCharset(const wxString& value)
{
    charsetM = value;
}

void Credentials::setUsername(const wxString& value)
{
    usernameM = value;
}

void Credentials::setPassword(const wxString& value)
{
    passwordM = value;
}

void Credentials::setRole(const wxString& value)
{
    roleM = value;
}

wxString Credentials::getCharset() const
{
    return charsetM;
}

wxString Credentials::getUsername() const
{
    return usernameM;
}

wxString Credentials::getPassword() const
{
    return passwordM;
}

wxString Credentials::getRole() const
{
    return roleM;
}

int DatabaseInfo::getBuffers() const
{
    return buffersM;
}

bool DatabaseInfo::getForcedWrites() const
{
    return forcedWritesM;
}

bool DatabaseInfo::getReserve() const
{
    return reserveM;
}


int DatabaseInfo::getNextTransaction() const
{
    return nextTransactionM;
}

int DatabaseInfo::getODS() const
{
    return odsM;
}

int DatabaseInfo::getODSMinor() const
{
    return odsMinorM;
}

bool DatabaseInfo::getODSVersionIsHigherOrEqualTo(int versionMajor) const
{
    return odsM >= versionMajor;
}

bool DatabaseInfo::getODSVersionIsHigherOrEqualTo(int versionMajor,
    int versionMinor) const
{
    return odsM > versionMajor
        || (odsM == versionMajor && odsMinorM >= versionMinor);
}

int DatabaseInfo::getOldestActiveTransaction() const
{
    return oldestActiveTransactionM;
}

int DatabaseInfo::getOldestSnapshot() const
{
    return oldestSnapshotM;
}

int DatabaseInfo::getOldestTransaction() const
{
    return oldestTransactionM;
}

int DatabaseInfo::getPageSize() const
{
    return pageSizeM;
}

int DatabaseInfo::getPages() const
{
    return pagesM;
}

bool DatabaseInfo::getReadOnly() const
{
    return readOnlyM;
}

int64_t DatabaseInfo::getSizeInBytes() const
{
    return static_cast<int64_t>(getPages()) * getPageSize();
}

int DatabaseInfo::getSweep() const
{
    return sweepM;
}

void DatabaseInfo::load(const IBPP::Database database)
{
    database->Info(&odsM, &odsMinorM, &pageSizeM, &pagesM,
        &buffersM, &sweepM, &forcedWritesM, &reserveM, &readOnlyM);
    database->TransactionInfo(&oldestTransactionM, &oldestActiveTransactionM,
        &oldestSnapshotM, &nextTransactionM);
    loadTimeMillisM = ::wxGetLocalTimeMillis();
}

void DatabaseInfo::reloadIfNecessary(const IBPP::Database database)
{
    wxLongLong millisNow = ::wxGetLocalTimeMillis();
    // value may jump or even actually decrease, for instance on timezone
    // change or when daylight saving time ends...
    wxLongLong millisDelta = millisNow - loadTimeMillisM;
    if (millisDelta >= 1000 || millisDelta <= -1000)
        load(database);
}

// DatabaseAuthenticationMode class
DatabaseAuthenticationMode::DatabaseAuthenticationMode()
    : modeM(UseSavedPassword)
{
}

int DatabaseAuthenticationMode::getMode() const
{
    return int(modeM);
}

void DatabaseAuthenticationMode::setMode(int mode)
{
    switch (mode)
    {
        case UseSavedPassword:
        case UseSavedEncryptedPwd:
        case AlwaysEnterPassword:
        case TrustedUser:
            modeM = Mode(mode);
            break;
        default:
            wxASSERT(false);
    }
}

wxString DatabaseAuthenticationMode::getConfigValue() const
{
    switch (modeM)
    {
        case UseSavedEncryptedPwd:
            return "encpwd";
        case AlwaysEnterPassword:
            return "askpwd";
        case TrustedUser:
            return "trusted";
        default:
            return "pwd";
    }
}

void DatabaseAuthenticationMode::setConfigValue(const wxString& value)
{
    if (value == "pwd")
        modeM = UseSavedPassword;
    else if (value == "encpwd")
        modeM = UseSavedEncryptedPwd;
    else if (value == "askpwd")
        modeM = AlwaysEnterPassword;
    else if (value == "trusted")
        modeM = TrustedUser;
    else
        wxASSERT(false);
}

void DatabaseAuthenticationMode::setStoreEncryptedPassword()
{
    // ignore if old setting found after new mode has been set already
    if (modeM == UseSavedPassword)
        modeM = UseSavedEncryptedPwd;
}

bool DatabaseAuthenticationMode::getAlwaysAskForPassword() const
{
    return modeM == AlwaysEnterPassword;
}

bool DatabaseAuthenticationMode::getIgnoreUsernamePassword() const
{
    return modeM == TrustedUser;
}

bool DatabaseAuthenticationMode::getUseEncryptedPassword() const
{
    return modeM == UseSavedEncryptedPwd;
}

// Database class
Database::Database()
    : MetadataItem(ntDatabase), metadataLoaderM(0), connectedM(false),
        connectionCredentialsM(0), dialectM(3), idM(0), volatileM(false)
{
    defaultTimezoneM.name = "";
    defaultTimezoneM.id = 0;
}

Database::~Database()
{
    resetCredentials();
}

void Database::prepareTemporaryCredentials()
{
    resetCredentials();
    connectionCredentialsM = new Credentials;
    connectionCredentialsM->setCharset(credentialsM.getCharset()); // default to database charset
}

void Database::resetCredentials()
{
    if (connectionCredentialsM)  // i.e. there is some other
    {
        delete connectionCredentialsM;
        connectionCredentialsM = 0;
    }
}

void Database::getIdentifiers(std::vector<Identifier>& temp)
{
    checkConnected(_("getIdentifiers"));
    std::transform(characterSetsM->begin(), characterSetsM->end(),
        std::back_inserter(temp), std::mem_fn(&MetadataItem::getIdentifier));
    std::transform(collationsM->begin(), collationsM->end(),
        std::back_inserter(temp), std::mem_fn(&MetadataItem::getIdentifier));
    std::transform(tablesM->begin(), tablesM->end(),
        std::back_inserter(temp), std::mem_fn(&MetadataItem::getIdentifier));
    std::transform(sysTablesM->begin(), sysTablesM->end(),
        std::back_inserter(temp), std::mem_fn(&MetadataItem::getIdentifier));
    std::transform(GTTablesM->begin(), GTTablesM->end(),
        std::back_inserter(temp), std::mem_fn(&MetadataItem::getIdentifier));
    std::transform(viewsM->begin(), viewsM->end(),
        std::back_inserter(temp), std::mem_fn(&MetadataItem::getIdentifier));
    std::transform(packagesM->begin(), packagesM->end(),
        std::back_inserter(temp), std::mem_fn(&MetadataItem::getIdentifier));
    std::transform(sysPackagesM->begin(), sysPackagesM->end(),
        std::back_inserter(temp), std::mem_fn(&MetadataItem::getIdentifier));
    std::transform(proceduresM->begin(), proceduresM->end(),
        std::back_inserter(temp), std::mem_fn(&MetadataItem::getIdentifier));
    std::transform(DMLtriggersM->begin(), DMLtriggersM->end(),
        std::back_inserter(temp), std::mem_fn(&MetadataItem::getIdentifier));
    std::transform(DBTriggersM->begin(), DBTriggersM->end(),
        std::back_inserter(temp), std::mem_fn(&MetadataItem::getIdentifier));
    std::transform(DDLTriggersM->begin(), DDLTriggersM->end(),
        std::back_inserter(temp), std::mem_fn(&MetadataItem::getIdentifier));
    std::transform(rolesM->begin(), rolesM->end(),
        std::back_inserter(temp), std::mem_fn(&MetadataItem::getIdentifier));
    std::transform(generatorsM->begin(), generatorsM->end(),
        std::back_inserter(temp), std::mem_fn(&MetadataItem::getIdentifier));
    std::transform(functionSQLsM->begin(), functionSQLsM->end(),
        std::back_inserter(temp), std::mem_fn(&MetadataItem::getIdentifier));
    std::transform(UDFsM->begin(), UDFsM->end(),
        std::back_inserter(temp), std::mem_fn(&MetadataItem::getIdentifier));
    std::transform(userDomainsM->begin(), userDomainsM->end(),
        std::back_inserter(temp), std::mem_fn(&MetadataItem::getIdentifier));
    std::transform(exceptionsM->begin(), exceptionsM->end(),
        std::back_inserter(temp), std::mem_fn(&MetadataItem::getIdentifier));
    std::transform(sysDomainsM->begin(), sysDomainsM->end(),
        std::back_inserter(temp), std::mem_fn(&MetadataItem::getIdentifier));
    std::transform(indicesM->begin(), indicesM->end(),
        std::back_inserter(temp), std::mem_fn(&MetadataItem::getIdentifier));
    std::transform(sysIndicesM->begin(), sysIndicesM->end(),
        std::back_inserter(temp), std::mem_fn(&MetadataItem::getIdentifier));
    std::transform(usrIndicesM->begin(), usrIndicesM->end(),
        std::back_inserter(temp), std::mem_fn(&MetadataItem::getIdentifier));
}

// This could be moved to Column class
wxString Database::loadDomainNameForColumn(const wxString& table,
    const wxString& field)
{
    MetadataLoader* loader = getMetadataLoader();
    MetadataLoaderTransaction tr(loader);
    wxMBConv* converter = getCharsetConverter();

    IBPP::Statement& st1 = loader->getStatement(
        "select rdb$field_source from rdb$relation_fields"
        " where rdb$relation_name = ? and rdb$field_name = ?"
    );
    st1->Set(1, wx2std(table, converter));
    st1->Set(2, wx2std(field, converter));
    st1->Execute();
    st1->Fetch();
    std::string domain;
    st1->Get(1, domain);
    return std2wxIdentifier(domain, converter);
}

void Database::getDatabaseTriggers(std::vector<Trigger *>& list)
{
    std::transform(DBTriggersM->begin(), DBTriggersM->end(),
        std::back_inserter(list), std::mem_fn(&DBTriggerPtr::get));
}

bool Database::getIsVolative()
{
    return volatileM;
}

CharacterSetPtr Database::getCharsetById(int id)
{
    // if it contains both charset and collation as 2 bytes
    id %= 256;
    //CharacterSetPtr cs = getCharacterSets()->findByMetadataId(id);

    return  getCharacterSets()->findByMetadataId(id);
}

wxArrayString Database::getCharacterSet()
{
    wxArrayString temp;
    std::vector<CharacterSet*> list;

    std::transform(characterSetsM->begin(), characterSetsM->end(),
        std::back_inserter(list), std::mem_fn(&CharacterSetPtr::get));

    for (MetadataItem* c : list)
        temp.push_back(c->getName_());
    
    return temp;
}


//! returns all collations for a given charset
wxArrayString Database::getCollations(const wxString& charset)
{
    CharacterSetPtr  characterSet = characterSetsM->findByName(charset);
    if (!characterSet)
        return wxArrayString();
    characterSet->ensureChildrenLoaded();

    return characterSet->getCollations();
}

DomainPtr Database::getDomain(const wxString& name)
{
    if (MetadataItem::hasSystemPrefix(name))
        return sysDomainsM->getDomain(name);
    else
        return userDomainsM->getDomain(name);
}

bool Database::isDefaultCollation(const wxString& charset,
    const wxString& collate)
{
    CharacterSetPtr  characterSet =  characterSetsM->findByName(charset);
    if (!characterSet)
        return false;

    characterSet->ensureChildrenLoaded();

    return characterSet->getCollationDefault() == collate;
}

//! load charset-collation pairs if needed
void Database::loadCollations()
{
    //if (!collationsM.empty())
    //    return;

    MetadataLoader* loader = getMetadataLoader();
    MetadataLoaderTransaction tr(loader);
    wxMBConv* converter = getCharsetConverter();

    IBPP::Statement& st1 = loader->getStatement(
        "select c.rdb$character_set_name, k.rdb$collation_name, "
        " c.RDB$CHARACTER_SET_ID, c.RDB$BYTES_PER_CHARACTER "
        " from rdb$character_sets c"
        " left outer join rdb$collations k "
        "   on c.rdb$character_set_id = k.rdb$character_set_id "
        " order by c.rdb$character_set_name, k.rdb$collation_id");
    st1->Execute();
    while (st1->Fetch())
    {
        std::string s;
        st1->Get(1, s);
        wxString charset(std2wxIdentifier(s, converter));
        st1->Get(2, s);
        wxString collation(std2wxIdentifier(s, converter));
        int charsetId, bytesPerChar;
        st1->Get(3, &charsetId);
        st1->Get(4, &bytesPerChar);
        //CharacterSet cs(charset, charsetId, bytesPerChar);
        //collationsM.insert(std::multimap<CharacterSet, wxString>::value_type(
        //    cs, collation));
    }
}

wxString Database::getTableForIndex(const wxString& indexName)
{
    MetadataLoader* loader = getMetadataLoader();
    MetadataLoaderTransaction tr(loader);

    IBPP::Statement& st1 = loader->getStatement(
        "SELECT rdb$relation_name from rdb$indices where rdb$index_name = ?");
    st1->Set(1, wx2std(indexName, getCharsetConverter()));
    st1->Execute();

    wxString tableName;
    if (st1->Fetch())
    {
        std::string s;
        st1->Get(1, s);
        tableName = std2wxIdentifier(s, getCharsetConverter());
    }
    return tableName;
}

void Database::loadGeneratorValues()
{
    MetadataLoader* loader = getMetadataLoader();
    // first start a transaction for metadata loading, then lock the database
    // when objects go out of scope and are destroyed, database will be
    // unlocked before the transaction is committed - any update() calls on
    // observers can possibly use the same transaction
    MetadataLoaderTransaction tr(loader);
    SubjectLocker lock(this);

    for (Generators::iterator it = generatorsM->begin();
        it != generatorsM->end(); ++it)
    {
        // make sure generator value is reloaded from database
        (*it)->invalidate();
        (*it)->ensurePropertiesLoaded();
    }
}

DatabasePtr Database::getDatabase() const
{
    return (const_cast<Database*>(this))->shared_from_this();
}

MetadataItem* Database::findByName(const wxString& name)
{
    if (!isConnected())
        return 0;

    for (int n = (int)ntTable; n < (int)ntLastType; n++)
    {
        MetadataItem* m = findByNameAndType((NodeType)n, name);
        if (m)
            return m;
    }
    return 0;
}

MetadataItem* Database::findByIdAndType(NodeType nt, const int id)
{
    if (!isConnected())
        return 0;
    switch (nt)
    {
        case ntCharacterSet:
            return characterSetsM->findByMetadataId(id).get();
            break;
        default:
            return 0;
    }
}

MetadataItem* Database::findByNameAndType(NodeType nt, const wxString& name)
{
    if (!isConnected())
        return 0;
    MetadataItem* item;

    switch (nt)
    {
        case ntDatabase:
            return this;
            break;
        case ntCharacterSet:
            return characterSetsM->findByName(name).get();
            break;
        case ntCollation:
            return collationsM->findByName(name).get();
            break;
        case ntTable:
            return tablesM->findByName(name).get();
            break;
        case ntSysTable:
            return sysTablesM->findByName(name).get();
            break;
        case ntGTT:
            return GTTablesM->findByName(name).get();
            break;
        case ntView:
            return viewsM->findByName(name).get();
            break;
        case ntTrigger:
        case ntDMLTrigger:
            if ( item = DMLtriggersM->findByName(name).get() ) {
                return item;
                break;
            }
        case ntDBTrigger:
            if ( item = DBTriggersM->findByName(name).get()) {
                return item;
                break;
            }
        case ntDDLTrigger:
            if (item = DDLTriggersM->findByName(name).get()) {
                return item;
                break;
            }
        case ntProcedure:
            return proceduresM->findByName(name).get();
            break;
        //case ntFunction:
        case ntFunctionSQL:
            return functionSQLsM->findByName(name).get();
            break;
        case ntUDF:
            return UDFsM->findByName(name).get();
            break;
        case ntGenerator:
            return generatorsM->findByName(name).get();
            break;
        case ntRole:
            return rolesM->findByName(name).get();
            break;
        case ntSysRole:
            return sysRolesM->findByName(name).get();
            break;
        case ntDomain:
            return userDomainsM->findByName(name).get();
            break;
        case ntSysDomain:
            return sysDomainsM->findByName(name).get();
            break;
        case ntException:
            return exceptionsM->findByName(name).get();
            break;
        case ntPackage:
            return packagesM->findByName(name).get();
            break;
        case ntSysPackage:
            return sysPackagesM->findByName(name).get();
            break;
        case ntIndex:
            return indicesM->findByName(name).get();
            break;
        case ntSysIndices:
            return sysIndicesM->findByName(name).get();
            break;
        case ntUsrIndices:
            return usrIndicesM->findByName(name).get();
            break;
        default:
            return 0;
    };
}

Relation* Database::findRelation(const Identifier& name)
{
    wxString s(name.get());
    if (TablePtr t = tablesM->findByName(s))
        return t.get();
    if (ViewPtr v = viewsM->findByName(s))
        return v.get();
    if (TablePtr t = sysTablesM->findByName(s))
        return t.get();
    if (TablePtr t = GTTablesM->findByName(s))
        return t.get();
    return 0;
}

Relation* Database::getRelationForTrigger(DMLTrigger* trigger)
{
    if (!trigger)
        return 0;
    wxString relName = trigger->getRelationName();
    if (relName.empty())
        return 0;
    return findRelation(Identifier(relName));
}

void Database::dropObject(MetadataItem* object)
{
    // find the collection that contains it, and remove it
    NodeType nt = object->getType();
    switch (nt)
    {
        case ntCharacterSet:
            characterSetsM->remove((CharacterSet*)object);
            break;
        case ntCollation:
            collationsM->remove((Collation*)object);
            break;
        case ntTable:
            tablesM->remove((Table*)object);
            break;
        case ntSysTable:
            sysTablesM->remove((Table*)object);
            break;
        case ntGTT:
            GTTablesM->remove((Table*)object);
            break;
        case ntView:
            viewsM->remove((View*)object);
            break;
        case ntDMLTrigger:
            DMLtriggersM->remove((DMLTrigger*)object);
            break;
        case ntProcedure:
            proceduresM->remove((Procedure*)object);
            break;
        case ntFunctionSQL:
            functionSQLsM->remove((FunctionSQL*)object);
            break;
        case ntUDF:
            UDFsM->remove((UDF*)object);
            break;
        case ntGenerator:
            generatorsM->remove((Generator*)object);
            break;
        case ntRole:
            rolesM->remove((Role*)object);
            break;
        case ntSysRole:
            sysRolesM->remove((Role*)object);
            break;
        case ntDomain:
            userDomainsM->remove((Domain*)object);
            break;
        case ntSysDomain:
            sysDomainsM->remove((Domain*)object);
            break;
        case ntException:
            exceptionsM->remove((Exception*)object);
            break;
        case ntPackage:
            packagesM->remove((Package*)object);
            break;
        case ntSysPackage:
            sysPackagesM->remove((Package*)object);
            break;
        case ntDBTrigger:
            DBTriggersM->remove((DBTrigger*)object);
            break;
        case ntDDLTrigger:
            DDLTriggersM->remove((DDLTrigger*)object);
            break;
        case ntIndex:
            indicesM->remove((Index*)object);
            break;
        default:
            return;
    };
}

void Database::addObject(NodeType type, const wxString& name)
{
    switch (type)
    {
        case ntCharacterSet:
            characterSetsM->insert(name);
            break;
        case ntCollation:
            collationsM->insert(name);
            break;
        case ntTable:
            tablesM->insert(name);
            break;
        case ntSysTable:
            sysTablesM->insert(name);
            break;
        case ntGTT:
            GTTablesM->insert(name);
            break;
        case ntView:
            viewsM->insert(name);
            break;
        case ntProcedure:
            proceduresM->insert(name);
            break;
        case ntDMLTrigger:
            DMLtriggersM->insert(name);
            break;
        case ntRole:
            rolesM->insert(name);
            break;
        case ntSysRole:
            sysRolesM->insert(name);
            break;
        case ntGenerator:
            generatorsM->insert(name);
            break;
        case ntFunctionSQL:
            functionSQLsM->insert(name);
            break;
        case ntUDF:
            UDFsM->insert(name);
            break;
        case ntDomain:
            userDomainsM->insert(name);
            break;
        case ntSysDomain:
            sysDomainsM->insert(name);
            break;
        case ntException:
            exceptionsM->insert(name);
            break;
        case ntPackage:
            packagesM->insert(name);
            break;
        case ntSysPackage:
            sysPackagesM->insert(name);
            break;
        case ntDBTrigger:
            DBTriggersM->insert(name);
            break;
        case ntDDLTrigger:
            DDLTriggersM->insert(name);
            break;
        case ntIndex:
            indicesM->insert(name);
            break;
        default:
            break;
    }
}

//! reads a DDL statement and acts accordingly
//
// drop [object_type] [name]
// alter [object_type] [name]
// create [object_type] [name]
// alter table [name] alter [column] type [domain or datatype]
// declare external function [name]
// set null flag via system tables update
void Database::parseCommitedSql(const SqlStatement& stm)
{
    if (!stm.isDDL())
        return;    // return false only on IBPP exception

    if (stm.actionIs(actGRANT))
    {
        MetadataItem *obj = stm.getObject();
        if (obj)
            obj->notifyObservers();
        return;
    }

    if (stm.actionIs(actDROP, ntIndex))
    {
        // the affected table will recognize its index (if loaded)
        Tables::iterator it;
        for (it = tablesM->begin(); it != tablesM->end(); ++it)
            (*it)->invalidateIndices(stm.getName());
        return;
    }

    // handle "CREATE INDEX", "ALTER INDEX" and "SET STATISTICS INDEX"
    if (stm.getObjectType() == ntIndex && ( stm.actionIs(actCREATE)
        || stm.actionIs(actALTER) || stm.actionIs(actSET)))
    {
        wxString tableName = getTableForIndex(stm.getName());
        MetadataItem* m = findByNameAndType(ntTable, tableName);
        if (Table* t = dynamic_cast<Table*>(m))
            t->invalidateIndices();

        if (Index* i = dynamic_cast<Index*>(stm.getObject())) {
            i->invalidate();
            i->ensurePropertiesLoaded();
            i->notifyObservers();
        }
        notifyObservers();

        return;
    }

    // update all TABLEs, VIEWs and DATABASE on "DROP TRIGGER"
    if (stm.actionIs(actDROP, ntDMLTrigger))
    {
        Tables::iterator itt;
        for (itt = tablesM->begin(); itt != tablesM->end(); itt++)
            (*itt)->notifyObservers();
        Views::iterator itv;
        for (itv = viewsM->begin(); itv != viewsM->end(); itv++)
            (*itv)->notifyObservers();
        notifyObservers();
    }

    if (stm.actionIs(actCREATE) || stm.actionIs(actDECLARE))
    {
        addObject(stm.getObjectType(), stm.getName());
        // when trigger created: force relations to update their property pages
        Relation *r = stm.getCreateTriggerRelation();
        if (r)
            r->notifyObservers();
        else if (stm.getObjectType() == ntDMLTrigger) // database trigger probably
            notifyObservers();
        return;
    }

    MetadataItem *object = stm.getObject();
    if (!object)
        return;

    if (stm.actionIs(actSET, ntGenerator) ||
        stm.actionIs(actALTER, ntGenerator))
    {
        if (Generator* g = dynamic_cast<Generator*>(object))
        {
            // make sure value is reloaded from database
            g->invalidate();
            g->ensurePropertiesLoaded();
        }
        return;
    }

    if (stm.actionIs(actDROP))
    {
        dropObject(object);
        if (stm.getObjectType() == ntTable || stm.getObjectType() == ntView)
        {
            DMLTriggers::iterator it = DMLtriggersM->begin();
            while (it != DMLtriggersM->end())
            {
                Relation* r = getRelationForTrigger((*it).get());
                if (!r || r->getIdentifier().equals(stm.getIdentifier()))
                {
                    dropObject((*it).get());
                    it = DMLtriggersM->begin();
                }
                else
                    it++;
            }
        }
        return;
    }

    if (stm.isAlterColumn())
    {
        if (stm.isDatatype())
        {
            wxString domainName(loadDomainNameForColumn(stm.getName(),
                stm.getFieldName()));
            if (MetadataItem::hasSystemPrefix(domainName))
            {
                DomainPtr d = sysDomainsM->findByName(domainName);
                if (!d)     // domain does not exist in DBH
                    d = sysDomainsM->insert(domainName);
                d->invalidate();
            }
            else
            {
                DomainPtr d = userDomainsM->findByName(domainName);
                if (!d)     // domain does not exist in DBH
                    d = userDomainsM->insert(domainName);
                d->invalidate();
            }
        }
        else
        {
            // there is (maybe) an extra RDB$domain in domainsM, but we can leave it there
            // as it is not going to hurt anyone
            // Besides, it appears that domain is left in database too (not cleared)
            // so we won't call this: loadObjects(ntDomain);
        }
    }

    if (stm.actionIs(actCOMMENT))
    {
        object->invalidateDescription();
        return;
    }

    if (stm.actionIs(actALTER))
    {
        // TODO: this is a place where we would simply call virtual invalidate() function
        // and object would do whatever it needs to
        switch (stm.getObjectType())
        {
            case ntProcedure:
                object->invalidate();
                dynamic_cast<Procedure*>(object)->checkDependentProcedures();
                break;
            case ntDDLTrigger:
            {
                DDLTrigger* tr = dynamic_cast<DDLTrigger*>(object);
                tr->invalidate();
                tr->ensurePropertiesLoaded();
                tr->notifyObservers();
                notifyObservers();
                break;
            }
            case ntDBTrigger:
            {
                DBTrigger* tr = dynamic_cast<DBTrigger*>(object);
                tr->invalidate();
                tr->ensurePropertiesLoaded();
                tr->notifyObservers();
                notifyObservers();
                break;
            }
            case ntDMLTrigger:
            {
                DMLTrigger* tr = dynamic_cast<DMLTrigger*>(object);
                if (tr)
                {
                    tr->invalidate();
                    Relation* r = getRelationForTrigger(tr);
                    if (r)
                        r->notifyObservers();
                    else  // database trigger
                        notifyObservers();
                }
                break;
            }
            case ntDomain:
                object->invalidate();
                // notify all table columns with that domain
                for (Tables::iterator it = tablesM->begin();
                    it != tablesM->end(); ++it)
                {
                    for (ColumnPtrs::iterator itColumn = (*it)->begin();
                        itColumn != (*it)->end(); ++itColumn)
                    {
                        if ((*itColumn)->getSource() == stm.getName())
                            (*itColumn)->invalidate();
                    }
                }
                break;
            default:
                // calls notifyObservers() only in the base class
                // descendent classes are free to put there whatever it takes...
                object->invalidate();
                object->notifyObservers();
                //object->ensurePropertiesLoaded();
                notifyObservers();
                break;
        }
    }
}

void Database::create(int pagesize, int dialect)
{
    wxString extra_params;
    if (pagesize)
        extra_params << " PAGE_SIZE " << pagesize;

    wxString charset(getConnectionCharset());
    if (!charset.empty())
        extra_params << " DEFAULT CHARACTER SET " << charset;

    bool useUserNamePwd = !authenticationModeM.getIgnoreUsernamePassword();
    IBPP::Database db = IBPP::DatabaseFactory("",
        wx2std(getConnectionString()),
        (useUserNamePwd ? wx2std(getUsername()) : ""),
        (useUserNamePwd ? wx2std(getDecryptedPassword()) : ""),
        "", wx2std(charset), wx2std(extra_params),
        wx2std(getClientLibrary())
    );
    db->Create(dialect);
}

void Database::drop()
{
    databaseM->Drop();
    setDisconnected();
}

void Database::reconnect()
{
    // must recreate, because IBPP::Database member will become invalid
    delete metadataLoaderM;
    metadataLoaderM = 0;

    databaseM->Disconnect();
    databaseM->Connect();
}

// the caller of this function should check whether the database object has the
// password set, and if it does not, it should provide the password
//               and if it does, just provide that password
void Database::connect(const wxString& password, ProgressIndicator* indicator)
{
    if (connectedM)
        return;

    //SubjectLocker lock(this);
    try
    {
        if (indicator)
        {
            indicator->doShow();
            indicator->initProgressIndeterminate("Establishing connection...");
        }

        databaseM.clear();

        auto connect = [this, &password]() {
            bool useUserNamePwd = !authenticationModeM.getIgnoreUsernamePassword();
            IBPP::Database db = IBPP::DatabaseFactory("",
                wx2std(getConnectionString()),
                (useUserNamePwd ? wx2std(getUsername()) : ""),
                (useUserNamePwd ? wx2std(password) : ""),
                wx2std(getRole()), wx2std(getConnectionCharset()), 
                "", wx2std(getClientLibrary())
            );
            db->Connect();  // As standard, will block for 180 seconds or until connected
            return db;
        };

        if (indicator)
        {
            // We can't just do a std::async here, we need to detach the thread to allow for user canceling
            std::promise<IBPP::Database> promise;
            auto future = promise.get_future();
            std::thread thread([&connect](std::promise<IBPP::Database> p) {
                try {
                    p.set_value(connect());
                }
                catch (...) {
                    try {
                        p.set_exception(std::current_exception());
                    }
                    catch (...) {} // set_exception() may throw too, apparently
                }
            }, std::move(promise));
            thread.detach();  // No use for this, just using the future

            while (future.wait_for(std::chrono::milliseconds(50)) == std::future_status::timeout)
            {
                indicator->stepProgress();
                if (indicator->isCanceled())
                {
                    // There is no safe, clean way to kill the thread. So it will continue in
                    // the background, and clean itself up when done.
                    // If user quits app before then, technically 'undefined behaviour' in the
                    // C++ standard, but all the main OS's just quietly kill the thread and
                    // release resources.
                    break;
                }
            }
            if (!indicator->isCanceled())
            {
                // Will throw exception in this thread context if Connect() call failed
                databaseM = future.get();
            }
        } else
        {
            databaseM = connect();
        }

        if (databaseM != 0 && databaseM->Connected())
        {
            connectedM = true;

            createCharsetConverter();

            DatabasePtr me(shared_from_this());
            unsigned lockCount = getLockCount();

            characterSetsM.reset(new CharacterSets(me));
            initializeLockCount(characterSetsM, lockCount);
            collationsM.reset(new Collations(me));
            initializeLockCount(collationsM, lockCount);
            userDomainsM.reset(new Domains(me));
            initializeLockCount(userDomainsM, lockCount);
            sysDomainsM.reset(new SysDomains(me));
            initializeLockCount(sysDomainsM, lockCount);
            exceptionsM.reset(new Exceptions(me));
            initializeLockCount(exceptionsM, lockCount);
            functionSQLsM.reset(new FunctionSQLs(me));
            initializeLockCount(functionSQLsM, lockCount);
            generatorsM.reset(new Generators(me));
            initializeLockCount(generatorsM, lockCount);
            proceduresM.reset(new Procedures(me));
            initializeLockCount(proceduresM, lockCount);
            rolesM.reset(new Roles(me));
            initializeLockCount(rolesM, lockCount);
            sysRolesM.reset(new SysRoles(me));
            initializeLockCount(sysRolesM, lockCount);
            DMLtriggersM.reset(new DMLTriggers(me));
            initializeLockCount(DMLtriggersM, lockCount);
            tablesM.reset(new Tables(me));
            initializeLockCount(tablesM, lockCount);
            sysTablesM.reset(new SysTables(me));
            GTTablesM.reset(new GTTables(me));
            initializeLockCount(sysTablesM, lockCount);
            UDFsM.reset(new UDFs(me));
            initializeLockCount(UDFsM, lockCount);
            viewsM.reset(new Views(me));
            initializeLockCount(viewsM, lockCount);
            packagesM.reset(new Packages(me));
            initializeLockCount(packagesM, lockCount);
            sysPackagesM.reset(new SysPackages(me));
            initializeLockCount(sysPackagesM, lockCount);
            DBTriggersM.reset(new DBTriggers(me));
            initializeLockCount(DBTriggersM, lockCount);
            DDLTriggersM.reset(new DDLTriggers(me));
            initializeLockCount(DDLTriggersM, lockCount);
            indicesM.reset(new Indices(me));
            initializeLockCount(indicesM, lockCount);
            sysIndicesM.reset(new SysIndices(me));
            initializeLockCount(sysIndicesM, lockCount);
            usrIndicesM.reset(new UsrIndices(me));
            initializeLockCount(usrIndicesM, lockCount);

            // first start a transaction for metadata loading, then lock the
            // database
            // when objects go out of scope and are destroyed, database will be
            // unlocked before the transaction is committed - any update() calls
            // on observers can possibly use the same transaction
            MetadataLoader* loader = getMetadataLoader();
            MetadataLoaderTransaction tr(loader);
            SubjectLocker lock(this); 

            try
            {
                checkProgressIndicatorCanceled(indicator);
                loadDatabaseInfo();
                checkProgressIndicatorCanceled(indicator);
                // load database information
                setPropertiesLoaded(false);
                dialectM = databaseM->Dialect();
                databaseInfoM.load(databaseM);
                setPropertiesLoaded(true);

                // load default timezone
                loadDefaultTimezone();
                loadTimezones();

                // load collections of metadata objects
                setChildrenLoaded(false);
                loadCollections(indicator);
                setChildrenLoaded(true);
                if (indicator)
                    indicator->initProgress(_("Complete"), 1, 1);
            }
            catch (CancelProgressException&)
            {
                disconnect();
            }
            notifyObservers();
        }
    }
    catch (...)
    {
        try
        {
            disconnect();
            databaseM.clear();
        }
        catch (...) // we don't care as we already have an error to report
        {
        }
        throw;
    }
}

void Database::loadCollections(ProgressIndicator* progressIndicator)
{
    // use a small helper to cut down on the repetition...
    struct ProgressIndicatorHelper
    {
    private:
        ProgressIndicator* progressIndicatorM;
    public:
        ProgressIndicatorHelper(ProgressIndicator* progressIndicator)
            : progressIndicatorM(progressIndicator) {}
        void init(wxString collectionName, int stepsTotal, int currentStep)
        {
            if (progressIndicatorM)
            {
                wxString msg(wxString::Format(_("Loading %s..."),
                    collectionName.c_str()));
                progressIndicatorM->initProgress(msg, stepsTotal,currentStep, 1);
            }
        }
    };

    const int collectionCount = 22;
    std::string loadStmt;
    ProgressIndicatorHelper pih(progressIndicator);

    MetadataLoader* loader = getMetadataLoader();
    MetadataLoaderTransaction tr(loader);
    SubjectLocker lock(this);

    pih.init(_("tables"), collectionCount, 0);
    tablesM->load(progressIndicator);

    pih.init(_("system tables"), collectionCount, 1);
    sysTablesM->load(progressIndicator);

    if (getInfo().getODSVersionIsHigherOrEqualTo(11.1)) {
        pih.init(_("global temporary table"), collectionCount, 2);
        GTTablesM->load(progressIndicator);
    }

    pih.init(_("views"), collectionCount, 3);
    viewsM->load(progressIndicator);

    pih.init(_("procedures"), collectionCount, 4);
    proceduresM->load(progressIndicator);

    pih.init(_("DML triggers"), collectionCount, 5);
    DMLtriggersM->load(progressIndicator);

    pih.init(_("roles"), collectionCount, 6);
    rolesM->load(progressIndicator);

    pih.init(_("system roles"), collectionCount, 7);
    sysRolesM->load(progressIndicator);

    pih.init(_("domains"), collectionCount, 8);
    userDomainsM->load(progressIndicator);

    if (getInfo().getODSVersionIsHigherOrEqualTo(12.0)) {
        pih.init(_("functions SQL"), collectionCount, 9);
        functionSQLsM->load(progressIndicator);
    }

    pih.init(_("functions UDF"), collectionCount, 10);
    UDFsM->load(progressIndicator);

    pih.init(_("generators"), collectionCount, 11);
    generatorsM->load(progressIndicator);

    pih.init(_("exceptions"), collectionCount, 12);
    exceptionsM->load(progressIndicator);

    if (getInfo().getODSVersionIsHigherOrEqualTo(12.0)) {
        pih.init(_("packages"), collectionCount, 13);
        packagesM->load(progressIndicator);

        pih.init(_("system packages"), collectionCount, 14);
        sysPackagesM->load(progressIndicator);
    }

    if (getInfo().getODSVersionIsHigherOrEqualTo(11.1)) {
        pih.init(_("DBTriggers"), collectionCount, 15);
        DBTriggersM->load(progressIndicator);
    }

    if (getInfo().getODSVersionIsHigherOrEqualTo(12.0)) {
        pih.init(_("DDLTriggers"), collectionCount, 16);
        DDLTriggersM->load(progressIndicator);
    }

    pih.init(_("system domains"), collectionCount, 17);
    sysDomainsM->load(progressIndicator);

    pih.init(_("indices"), collectionCount, 18);
    indicesM->load(progressIndicator);

    pih.init(_("system indices"), collectionCount, 19);
    sysIndicesM->load(progressIndicator);

    pih.init(_("indices"), collectionCount, 20);
    usrIndicesM->load(progressIndicator);

    pih.init(_("CharacterSet"), collectionCount, 21);
    characterSetsM->load(progressIndicator);

    pih.init(_("User Collations"), collectionCount, 22);
    collationsM->load(progressIndicator);

}

void Database::loadDatabaseInfo()
{
    MetadataLoader* loader = getMetadataLoader();
    MetadataLoaderTransaction tr(loader);

    // load database charset
    std::string stmt = "select rdb$character_set_name, current_user, current_role, ";
    stmt += getInfo().getODSVersionIsHigherOrEqualTo(12, 0) ? " rdb$linger, " : " null, ";
    stmt += getInfo().getODSVersionIsHigherOrEqualTo(13, 0) ? " rdb$sql_security   " : " null  ";
    stmt += " from rdb$database ";
    IBPP::Statement& st1 = loader->getStatement(stmt);

    st1->Execute();
    if (st1->Fetch())
    {
        std::string s;
        st1->Get(1, s);
        databaseCharsetM = std2wxIdentifier(s, getCharsetConverter());
        st1->Get(2, s);
        connectionUserM = std2wxIdentifier(s, getCharsetConverter());
        st1->Get(3, s);
        connectionRoleM = std2wxIdentifier(s, getCharsetConverter());
        if (connectionRoleM == "NONE")
            connectionRoleM.clear();
        if (!st1->IsNull(4))
            st1->Get(4, lingerM);
        else
            lingerM = 0;
        if (!st1->IsNull(5))
        {
            bool b;
            st1->Get(5, b);
            sqlSecurityM = wxString(b ? "SQL SECURITY DEFINER" : "SQL SECURITY INVOKER");
        }
        else
            sqlSecurityM.clear();
    }
}

wxArrayString Database::loadIdentifiers(const wxString& loadStatement,
    ProgressIndicator* progressIndicator)
{
    MetadataLoader* loader = getMetadataLoader();
    MetadataLoaderTransaction tr(loader);
    wxMBConv* converter = getCharsetConverter();

    IBPP::Statement& st1 = loader->getStatement(
        wx2std(loadStatement, getCharsetConverter()));
    st1->Execute();

    wxArrayString names;
    while (st1->Fetch())
    {
        checkProgressIndicatorCanceled(progressIndicator);
        if (!st1->IsNull(1))
        {
            std::string s;
            st1->Get(1, s);
            names.push_back(std2wxIdentifier(s, converter));
        }
    }
    return names;
}

void Database::disconnect()
{
    if (connectedM)
    {
        databaseM->Disconnect();
        setDisconnected();
    }
}

void Database::setDisconnected()
{
    delete metadataLoaderM;
    metadataLoaderM = 0;
    resetCredentials();     // "forget" temporary username/password
    connectedM = false;
    resetPendingLoadData();

    // remove entire DBH beneath
    userDomainsM.reset();
    sysDomainsM.reset();
    functionSQLsM.reset();
    generatorsM.reset();
    proceduresM.reset();
    rolesM.reset();
    tablesM.reset();
    sysTablesM.reset();
    GTTablesM.reset();
    DMLtriggersM.reset();
    UDFsM.reset();
    viewsM.reset();
    exceptionsM.reset();
    packagesM.reset();
    sysPackagesM.reset();
    DBTriggersM.reset();
    DDLTriggersM.reset();
    indicesM.reset();
    sysIndicesM.reset();
    usrIndicesM.reset();
    characterSetsM.reset();
    collationsM.reset();

    if (config().get("HideDisconnectedDatabases", false))
        getServer()->notifyObservers();
    notifyObservers();
}

bool Database::isConnected() const
{
    return connectedM;
}

MetadataLoader* Database::getMetadataLoader()
{
    if (metadataLoaderM == 0)
        metadataLoaderM = new MetadataLoader(*this, 8);
    return metadataLoaderM;
}

bool Database::getChildren(std::vector<MetadataItem*>& temp)
{
    if (!connectedM)
        return false;

    getCollections(temp, true);
    return true;
}

DomainsPtr Database::getDomains()
{
    wxASSERT(userDomainsM);
    userDomainsM->ensureChildrenLoaded();
    return userDomainsM;
}

SysDomainsPtr Database::getSysDomains()
{
    wxASSERT(sysDomainsM);
    sysDomainsM->ensureChildrenLoaded();
    return sysDomainsM;
}

ExceptionsPtr Database::getExceptions()
{
    wxASSERT(exceptionsM);
    exceptionsM->ensureChildrenLoaded();
    return exceptionsM;
}

UDFsPtr Database::getUDFs()
{
    wxASSERT(UDFsM);
    UDFsM->ensureChildrenLoaded();
    return UDFsM;
}

UsersPtr Database::getUsers()
{
    wxASSERT(usersM);
//    usersM->ensureChildrenLoaded();
    return usersM;
}

UsrIndicesPtr Database::getUsrIndices()
{
    wxASSERT(usrIndicesM);
    usrIndicesM->ensureChildrenLoaded();
    return usrIndicesM;
}

FunctionSQLsPtr Database::getFunctionSQLs()
{
    wxASSERT(functionSQLsM);
    functionSQLsM->ensureChildrenLoaded();
    return functionSQLsM;
}

GeneratorsPtr Database::getGenerators()
{
    wxASSERT(generatorsM);
    generatorsM->ensureChildrenLoaded();
    return generatorsM;
}

IndicesPtr Database::getIndices()
{
    wxASSERT(indicesM);
    indicesM->ensureChildrenLoaded();
    return indicesM;
}

PackagesPtr Database::getPackages()
{
    wxASSERT(packagesM);
    packagesM->ensureChildrenLoaded();
    return packagesM;
}

SysPackagesPtr Database::getSysPackages()
{
    wxASSERT(sysPackagesM);
    sysPackagesM->ensureChildrenLoaded();
    return sysPackagesM;
}

ProceduresPtr Database::getProcedures()
{
    wxASSERT(proceduresM);
    proceduresM->ensureChildrenLoaded();
    return proceduresM;
}

RolesPtr Database::getRoles()
{
    wxASSERT(rolesM);
    rolesM->ensureChildrenLoaded();
    return rolesM;
}

SysIndicesPtr Database::getSysIndices()
{
    wxASSERT(sysIndicesM);
    sysIndicesM->ensureChildrenLoaded();
    return sysIndicesM;
}

SysRolesPtr Database::getSysRoles()
{
    wxASSERT(sysRolesM);
    sysRolesM->ensureChildrenLoaded();
    return sysRolesM;
}

SysTablesPtr Database::getSysTables()
{
    wxASSERT(sysTablesM);
    sysTablesM->ensureChildrenLoaded();
    return sysTablesM;
}

GTTablesPtr Database::getGTTables()
{
    wxASSERT(GTTablesM);
    GTTablesM->ensureChildrenLoaded();
    return GTTablesM;
}

TablesPtr Database::getTables()
{
    wxASSERT(tablesM);
    tablesM->ensureChildrenLoaded();
    return tablesM;
}

DMLTriggersPtr Database::getDMLTriggers()
{
    wxASSERT(DMLtriggersM);
    DMLtriggersM->ensureChildrenLoaded();
    return DMLtriggersM;
}

CharacterSetsPtr Database::getCharacterSets()
{
    wxASSERT(characterSetsM);
    characterSetsM->ensureChildrenLoaded();
    return characterSetsM;
}

CollationsPtr Database::getCollations()
{
    wxASSERT(collationsM);
    collationsM->ensureChildrenLoaded();
    return collationsM;
}

DBTriggersPtr Database::getDBTriggers()
{
    wxASSERT(DBTriggersM);
    DBTriggersM->ensureChildrenLoaded();
    return DBTriggersM;
}
DDLTriggersPtr Database::getDDLTriggers()
{
    wxASSERT(DDLTriggersM);
    DDLTriggersM->ensureChildrenLoaded();
    return DDLTriggersM;
}
ViewsPtr Database::getViews()
{
    wxASSERT(viewsM);
    viewsM->ensureChildrenLoaded();
    return viewsM;
}

// returns vector of all subitems
void Database::getCollections(std::vector<MetadataItem*>& temp, bool system)
{
    if (!isConnected())
        return;

    ensureChildrenLoaded();
    //if (system && showSystemCharacterSet())
    //    temp.push_back(characterSetsM.get());
    
    temp.push_back(collationsM.get());

    if (getInfo().getODSVersionIsHigherOrEqualTo(11.1)) 
        temp.push_back(DBTriggersM.get());
    if (getInfo().getODSVersionIsHigherOrEqualTo(12.0)) 
        temp.push_back(DDLTriggersM.get());
    temp.push_back(userDomainsM.get());
    temp.push_back(exceptionsM.get());
    if (getInfo().getODSVersionIsHigherOrEqualTo(12.0))
        temp.push_back(functionSQLsM.get());
    temp.push_back(generatorsM.get());
    if (getInfo().getODSVersionIsHigherOrEqualTo(11.1)) 
        temp.push_back(GTTablesM.get());
    
    if (showOneNodeIndices() && showSystemIndices())
        temp.push_back(indicesM.get());
    else
        temp.push_back(usrIndicesM.get());

    if (getInfo().getODSVersionIsHigherOrEqualTo(12.0)) 
        temp.push_back(packagesM.get());
    temp.push_back(proceduresM.get());
    temp.push_back(rolesM.get());
    // Only push back system objects when they should be shown
    if (getInfo().getODSVersionIsHigherOrEqualTo(12.0)) {
        if (system && showSystemPackages())
            temp.push_back(sysPackagesM.get());
    }
    if (system && showSystemDomains())
        temp.push_back(sysDomainsM.get());
    if (system && showSystemIndices() && !showOneNodeIndices())
        temp.push_back(sysIndicesM.get());
    if (system && showSystemRoles())
        temp.push_back(sysRolesM.get());
    if (system && showSystemTables())
        temp.push_back(sysTablesM.get());
    temp.push_back(tablesM.get());
    temp.push_back(DMLtriggersM.get());
    temp.push_back(UDFsM.get());
    temp.push_back(viewsM.get());

}

void Database::loadChildren()
{
    // TODO: show progress dialog while reloading child object collections?
    loadCollections(0);
}

void Database::lockChildren()
{
    if (isConnected())
    {
        userDomainsM->lockSubject();
        sysDomainsM->lockSubject();
        exceptionsM->lockSubject();
        functionSQLsM->lockSubject();
        generatorsM->lockSubject();
        packagesM->lockSubject();
        sysPackagesM->lockSubject();
        proceduresM->lockSubject();
        rolesM->lockSubject();
        tablesM->lockSubject();
        sysTablesM->lockSubject();
        GTTablesM->lockSubject();
        DMLtriggersM->lockSubject();
        DBTriggersM->lockSubject();
        DDLTriggersM->lockSubject();
        UDFsM->lockSubject();
        viewsM->lockSubject();
        indicesM->lockSubject();
        sysIndicesM->lockSubject();
        usrIndicesM->lockSubject();
        characterSetsM->lockSubject();
        collationsM->lockSubject();
    }
}

void Database::unlockChildren()
{
    // unlock in reverse order of locking - that way domains will still
    // be locked when relation and procedure updates happen - that way not
    // every added domain will cause all collection observers to update
    if (isConnected())
    {
        usrIndicesM->unlockSubject();
        sysIndicesM->unlockSubject();
        indicesM->unlockSubject();
        viewsM->unlockSubject();
        UDFsM->unlockSubject();
        DDLTriggersM->unlockSubject();
        DBTriggersM->unlockSubject();
        DMLtriggersM->unlockSubject();
        GTTablesM->unlockSubject();
        sysTablesM->unlockSubject();
        tablesM->unlockSubject();
        rolesM->unlockSubject();
        proceduresM->unlockSubject();
        sysPackagesM->unlockSubject();
        packagesM->unlockSubject();
        generatorsM->unlockSubject();
        functionSQLsM->unlockSubject();
        exceptionsM->unlockSubject();
        sysDomainsM->unlockSubject();
        userDomainsM->unlockSubject();
        characterSetsM->unlockSubject();
        collationsM->unlockSubject();
    }
}

wxString Database::getPath() const
{
    return pathM;
}

wxString Database::getClientLibrary() const
{
    /*Todo: Implement FB library per conexion */
    //return clientLibraryM;
#if defined(_WIN64)
    return config().get("x64LibraryFile", wxString(""));
#else
    return config().get("x86LibraryFile", wxString(""));
#endif
}

int Database::getSqlDialect() const
{
    return dialectM;
}

wxString Database::getDatabaseCharset() const
{
    return databaseCharsetM;
}

wxString Database::getConnectionCharset() const
{
    if (connectionCredentialsM)
        return connectionCredentialsM->getCharset();
    else
        return credentialsM.getCharset();
}

wxString Database::getConnectionInfoString() const
{
    wxString username(getUsername());
    if (authenticationModeM.getIgnoreUsernamePassword())
    {
        if (connectedM)
        {
            username = "[" + connectionUserM;
            if (!connectionRoleM.empty())
                username += " (" + connectionRoleM + ")";
            username += "]";
        }
        else
            username = _("[Trusted user]");
    }
    return wxString(username + "@" + getConnectionString() + " (")
        + getConnectionCharset() + ")";
}

bool Database::usesDifferentConnectionCharset() const
{
    wxString charset(getConnectionCharset().Upper());
    if (databaseCharsetM.empty() && charset == "NONE")
        return false;
    return (charset.compare(databaseCharsetM.Upper()) != 0);
}

wxString Database::getUsername() const
{
    if (connectionCredentialsM)
        return connectionCredentialsM->getUsername();
    else
        return credentialsM.getUsername();
}

wxString Database::getRawPassword() const
{
    if (connectionCredentialsM)
        return connectionCredentialsM->getPassword();
    else
        return credentialsM.getPassword();
}

wxString Database::getDecryptedPassword() const
{
    // if we already have an established connection return that password
    if (databaseM != 0 && databaseM->Connected())
        return databaseM->UserPassword();

    // temporary connection
    if (connectionCredentialsM)
        return connectionCredentialsM->getPassword();
    wxString raw = credentialsM.getPassword();
    if (raw.IsEmpty())
        return wxEmptyString;

    if (authenticationModeM.getUseEncryptedPassword())
        return decryptPassword(raw, getUsername() + getConnectionString());
    else
        return raw;
}

DatabaseAuthenticationMode& Database::getAuthenticationMode()
{
    return authenticationModeM;
}

wxString Database::getRole() const
{
    if (connectionCredentialsM)
        return connectionCredentialsM->getRole();
    else
        return credentialsM.getRole();
}

IBPP::Database& Database::getIBPPDatabase()
{
    return databaseM;
}

void Database::setIsVolatile(const bool isVolatile)
{
    volatileM = isVolatile;
}

void Database::setPath(const wxString& value)
{
    pathM = value;
}

void Database::setClientLibrary(const wxString& value)
{
    clientLibraryM = value;
}

void Database::setConnectionCharset(const wxString& value)
{
    if (connectionCredentialsM)
        connectionCredentialsM->setCharset(value);
    else
        credentialsM.setCharset(value);
}

void Database::setUsername(const wxString& value)
{
    if (connectionCredentialsM)
        connectionCredentialsM->setUsername(value);
    else
        credentialsM.setUsername(value);
}

void Database::setRawPassword(const wxString& value)
{
    if (connectionCredentialsM)
        connectionCredentialsM->setPassword(value);
    else
        credentialsM.setPassword(value);
}

void Database::setEncryptedPassword(const wxString& value)
{
    // temporary credentials -> use password as entered
    if (connectionCredentialsM)
    {
        connectionCredentialsM->setPassword(value);
        return;
    }

    // password must not be empty to be encrypted

    if (authenticationModeM.getUseEncryptedPassword() && !value.empty())
    {
        credentialsM.setPassword(
            encryptPassword(value, getUsername() + getConnectionString()));
    }
    else
    {
        credentialsM.setPassword(value);
    }
}

void Database::setRole(const wxString& value)
{
    if (connectionCredentialsM)
        connectionCredentialsM->setRole(value);
    else
        credentialsM.setRole(value);
}

const wxString Database::getTypeName() const
{
    return "DATABASE";
}

void Database::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitDatabase(*this);
}

ServerPtr Database::getServer() const
{
    return ServerPtr(serverM);
}

void Database::setServer(ServerPtr server)
{
    wxASSERT(server);
    serverM = server;
    setParent(server.get());
}

wxString Database::getConnectionString() const
{
    wxString serverConnStr = getServer()->getConnectionString();
    if (!serverConnStr.empty())
        return serverConnStr + ":" + pathM;
    else
        return pathM;
}

/* static */
wxString Database::extractNameFromConnectionString(const wxString& path)
{
    wxString name = path;
    wxString::size_type p = name.find_last_of("/\\:");
    if (p != wxString::npos)
        name.erase(0, p + 1);
    p = name.find_last_of(".");
    if (p != wxString::npos)
        name.erase(p, name.length());
    return name;
}

const wxString Database::getId() const
{
    if (idM == 0)
        idM = getUniqueId();
    wxString result = wxString::Format("%d", idM);
    return result;
}

unsigned uniqueDatabaseId = 1;

void Database::setId(unsigned id)
{
    idM = id;
    // generator to be higher than ids of existing databases
    if (id >= uniqueDatabaseId)
        uniqueDatabaseId = id + 1;
}

/*static*/
unsigned Database::getUniqueId()
{
    return uniqueDatabaseId++;
}

/*static*/
unsigned Database::getUIDGeneratorValue()
{
    return uniqueDatabaseId;
}

/*static*/
void Database::setUIDGeneratorValue(unsigned value)
{
    uniqueDatabaseId = value;
}

const DatabaseInfo& Database::getInfo()
{
    databaseInfoM.reloadIfNecessary(databaseM);
    return databaseInfoM;
}

void Database::loadInfo()
{
    databaseInfoM.load(databaseM);
    loadDatabaseInfo();
    notifyObservers();
}

bool Database::showSystemCharacterSet()
{
    const wxString SHOW_SYSCHARACTERSET = "ShowSystemCharacterSet";

    bool b;
    if (!DatabaseConfig(this, config()).getValue(SHOW_SYSCHARACTERSET, b))
        b = config().get(SHOW_SYSCHARACTERSET, true);
    
    return b;
}

bool Database::showSystemIndices()
{
    const wxString SHOW_SYSINDICES = "ShowSystemIndices";

    bool b;
    if (!DatabaseConfig(this, config()).getValue(SHOW_SYSINDICES, b))
        b = config().get(SHOW_SYSINDICES, true);

    return b;
}

bool Database::showSystemDomains()
{
    const wxString SHOW_SYSDOMAINS = "ShowSystemDomains";

    bool b;
    if (!DatabaseConfig(this, config()).getValue(SHOW_SYSDOMAINS, b))
        b = config().get(SHOW_SYSDOMAINS, true);

    return b;
}

bool Database::showSystemPackages()
{
    if (!getInfo().getODSVersionIsHigherOrEqualTo(12, 0))
        return false;

    const wxString SHOW_SYSPACKAGES = "ShowSystemPackages";

    bool b;
    if (!DatabaseConfig(this, config()).getValue(SHOW_SYSPACKAGES, b))
        b = config().get(SHOW_SYSPACKAGES, true);

    return b;
}

bool Database::showSystemRoles()
{
    if (!getInfo().getODSVersionIsHigherOrEqualTo(11, 1))
        return false;

    const wxString SHOW_SYSROLES = "ShowSystemRoles";

    bool b;
    if (!DatabaseConfig(this, config()).getValue(SHOW_SYSROLES, b))
        b = config().get(SHOW_SYSROLES, true);

    return b;
}

bool Database::showSystemTables()
{
    const wxString SHOW_SYSTABLES = "ShowSystemTables";

    bool b;
    if (!DatabaseConfig(this, config()).getValue(SHOW_SYSTABLES, b))
        b = config().get(SHOW_SYSTABLES, true);

    return b;
}

bool Database::showOneNodeIndices()
{
    const wxString SHOW_ONENODEINDICES = "ShowOneNodeIndices";

    bool b;
    if (!DatabaseConfig(this, config()).getValue(SHOW_ONENODEINDICES, b))
        b = config().get(SHOW_ONENODEINDICES, false);

    return b;
}

wxString mapConnectionCharsetToSystemCharset(const wxString& connectionCharset)
{
    wxString charset(connectionCharset.Upper().Trim(true).Trim(false));

    // fixes hang when character set name empty (invalid encoding is returned)
    if (charset.empty())
        charset = "NONE";

    // Firebird charsets WIN125X need to be replaced with either
    // WINDOWS125X or CP125X - we take the latter
    if (charset.Mid(0, 5) == "WIN12")
        return "CP12" + charset.Mid(5);

    // Firebird charsets ISO8859_X
    if (charset.Mid(0, 8) == "ISO8859_")
        return "ISO-8859-" + charset.Mid(8);

    // all other mappings need to be added here...
    struct CharsetMapping { wxString connCS; wxString convCS; };
    static const CharsetMapping mappings[] = {
        { "UTF8", "UTF-8" }, { "UNICODE_FSS", "UTF-8" }
    };
    int mappingCount = sizeof(mappings) / sizeof(CharsetMapping);
    for (int i = 0; i < mappingCount; i++)
    {
        if (mappings[i].connCS == charset)
            return mappings[i].convCS;
    }

    return charset;
}

void Database::createCharsetConverter()
{
    charsetConverterM.reset();

    wxString cs(mapConnectionCharsetToSystemCharset(getConnectionCharset()));
    wxFontEncoding fe = wxFontMapperBase::Get()->CharsetToEncoding(cs, false);
    if (fe != wxFONTENCODING_SYSTEM)
        charsetConverterM.reset(new wxCSConv(fe));
}

wxMBConv* Database::getCharsetConverter() const
{
    if (wxMBConv* conv = charsetConverterM.get())
        return conv;
    return wxConvCurrent;
}

void Database::getConnectedUsers(wxArrayString& users) const
{
    if (databaseM != 0 && databaseM->Connected())
    {
        std::vector<std::string> userNames;
        databaseM->Users(userNames);

        // replace multiple occurences of same user name by "username (N)"
        std::map<std::string, size_t> counts;
        for (std::vector<std::string>::const_iterator it = userNames.begin();
            it != userNames.end(); ++it)
        {
            counts[*it] += 1;
        }

        for (std::map<std::string, size_t>::iterator it = counts.begin();
            it != counts.end(); ++it)
        {
            wxString name((*it).first);
            if ((*it).second > 1)
                name += wxString::Format(" (%zu)", (*it).second);
            users.Add(name);
        }
    }
}

int Database::getLinger() const
{
    return lingerM;
}

wxString Database::getSqlSecurity() const
{
    return sqlSecurityM;
}

void Database::checkConnected(const wxString& operation) const
{
    if (!connectedM)
    {
        throw FRError(wxString::Format(_("Operation \"%s\" not allowed on a disconnected database."),
            operation.c_str()));
    }
}

void Database::loadDefaultTimezone()
{
    MetadataLoader* loader = getMetadataLoader();
    wxMBConv* converter = getCharsetConverter();
    std::string tzName;
    int tzId;

    // RDB$TIME_ZONES is available on Firebird 4 (ODS Ver 13) or higher
    if (!getInfo().getODSVersionIsHigherOrEqualTo(13, 0))
        return;

    IBPP::Statement& st1 = loader->getStatement(
        "select z.RDB$TIME_ZONE_ID, "
        "       z.RDB$TIME_ZONE_NAME "
        "from RDB$TIME_ZONES z "
        "where z.RDB$TIME_ZONE_NAME = RDB$GET_CONTEXT('SYSTEM', 'SESSION_TIMEZONE');");

    st1->Execute();
    st1->Fetch();
    st1->Get(1, tzId);
    st1->Get(2, tzName);

    defaultTimezoneM.id = tzId;
    defaultTimezoneM.name = std2wxIdentifier(tzName, converter);
}

void Database::loadTimezones()
{
    MetadataLoader* loader = getMetadataLoader();
    wxMBConv* converter = getCharsetConverter();
    std::string tzName;
    int tzId;
    TimezoneInfo* tzItm;

    // RDB$TIME_ZONES is available on Firebird 4 (ODS Ver 13) or higher
    if (!getInfo().getODSVersionIsHigherOrEqualTo(13, 0))
        return;

    IBPP::Statement& st1 = loader->getStatement(
        "select z.RDB$TIME_ZONE_ID, "
        "       z.RDB$TIME_ZONE_NAME "
        "from RDB$TIME_ZONES z");

    st1->Execute();

    while (st1->Fetch())
    {
        st1->Get(1, tzId);
        st1->Get(2, tzName);

        tzItm = new TimezoneInfo;
        tzItm->id = tzId;
        tzItm->name = std2wxIdentifier(tzName, converter);
        timezonesM.push_back(tzItm);
    }
}

TimezoneInfo Database::getDefaultTimezone()
{
    loadDefaultTimezone();
    return defaultTimezoneM;
}

wxString Database::getTimezoneName(int timezone)
{
    std::vector<TimezoneInfo*>::iterator it;
    for (it = timezonesM.begin(); it != timezonesM.end(); it++)
    {
        if ((*it)->id != timezone)
            continue;
        return (*it)->name;
    }
    // not found
    return wxString::Format("TZ %d", timezone);
}
