/*
  Copyright (c) 2004-2008 The FlameRobin Development Team

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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include <sstream>

#include "config/Config.h"
#include "config/DatabaseConfig.h"
#include "core/FRError.h"
#include "core/ProgressIndicator.h"
#include "core/StringUtils.h"
#include "engine/MetadataLoader.h"
#include "MasterPassword.h"
#include "metadata/database.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/parameter.h"
#include "metadata/root.h"
#include "sql/SqlStatement.h"
#include "sql/SqlTokenizer.h"
//-----------------------------------------------------------------------------
using namespace std;
//-----------------------------------------------------------------------------
void Credentials::setCharset(wxString value)
{
    charsetM = value;
}
//-----------------------------------------------------------------------------
void Credentials::setUsername(wxString value)
{
    usernameM = value;
}
//-----------------------------------------------------------------------------
void Credentials::setPassword(wxString value)
{
    passwordM = value;
}
//-----------------------------------------------------------------------------
void Credentials::setRole(wxString value)
{
    roleM = value;
}
//-----------------------------------------------------------------------------
wxString Credentials::getCharset() const
{
    return charsetM;
}
//-----------------------------------------------------------------------------
wxString Credentials::getUsername() const
{
    return usernameM;
}
//-----------------------------------------------------------------------------
wxString Credentials::getPassword() const
{
    return passwordM;
}
//-----------------------------------------------------------------------------
wxString Credentials::getRole() const
{
    return roleM;
}
//-----------------------------------------------------------------------------
int DatabaseInfo::getBuffers() const
{
    return buffersM;
}
//-----------------------------------------------------------------------------
wxString DatabaseInfo::getCreated() const
{
    return wxT("not supported");
}
//-----------------------------------------------------------------------------
int DatabaseInfo::getDialect() const
{
    return dialectM;
}
//-----------------------------------------------------------------------------
bool DatabaseInfo::getForcedWrites() const
{
    return forcedWritesM;
}
//-----------------------------------------------------------------------------
int DatabaseInfo::getNextTransaction() const
{
    return nextTransactionM;
}
//-----------------------------------------------------------------------------
int DatabaseInfo::getODS() const
{
    return odsM;
}
//-----------------------------------------------------------------------------
int DatabaseInfo::getODSMinor() const
{
    return odsMinorM;
}
//-----------------------------------------------------------------------------
bool DatabaseInfo::getODSVersionIsHigherOrEqualTo(int versionMajor) const
{
    return odsM >= versionMajor;
}
//-----------------------------------------------------------------------------
bool DatabaseInfo::getODSVersionIsHigherOrEqualTo(int versionMajor,
    int versionMinor) const
{
    return odsM > versionMajor
        || (odsM == versionMajor && odsMinorM >= versionMinor);
}
//-----------------------------------------------------------------------------
int DatabaseInfo::getOldestTransaction() const
{
    return oldestTransactionM;
}
//-----------------------------------------------------------------------------
int DatabaseInfo::getPageSize() const
{
    return pageSizeM;
}
//-----------------------------------------------------------------------------
int DatabaseInfo::getPages() const
{
    return pagesM;
}
//-----------------------------------------------------------------------------
bool DatabaseInfo::getReadOnly() const
{
    return readOnlyM;
}
//-----------------------------------------------------------------------------
int64_t DatabaseInfo::getSizeInBytes() const
{
    return getPages() * getPageSize();
}
//-----------------------------------------------------------------------------
int DatabaseInfo::getSweep() const
{
    return sweepM;
}
//-----------------------------------------------------------------------------
void DatabaseInfo::load(const IBPP::Database database)
{
    database->Info(&odsM, &odsMinorM, &pageSizeM, &pagesM,
        &buffersM, &sweepM, &forcedWritesM, &reserveM, &readOnlyM);
    dialectM = database->Dialect();
    loadTimeMillisM = ::wxGetLocalTimeMillis();
}
//-----------------------------------------------------------------------------
void DatabaseInfo::reloadIfNecessary(const IBPP::Database database)
{
    wxLongLong millisNow = ::wxGetLocalTimeMillis();
    // value may jump or even actually decrease, for instance on timezone
    // change or when daylight saving time ends...
    wxLongLong millisDelta = millisNow - loadTimeMillisM;
    if (millisDelta >= 1000 || millisDelta <= -1000)
        load(database);
}
//-----------------------------------------------------------------------------
Database::Database()
    : MetadataItem(), metadataLoaderM(0), connectedM(false),
        connectionCredentialsM(0), storeEncryptedPasswordM(false), idM(0)
{
    typeM = ntDatabase;

    // has to be here, since notify() might be called before initChildren()
    domainsM.setProperties(this, wxT("Domains"), ntDomains);
    exceptionsM.setProperties(this, wxT("Exceptions"), ntExceptions);
    functionsM.setProperties(this, wxT("Functions"), ntFunctions);
    generatorsM.setProperties(this, wxT("Generators"), ntGenerators);
    proceduresM.setProperties(this, wxT("Procedures"), ntProcedures);
    rolesM.setProperties(this, wxT("Roles"), ntRoles);
    tablesM.setProperties(this, wxT("Tables"), ntTables);

    sysTablesM.setProperties(this, wxT("System tables"), ntSysTables);

    triggersM.setProperties(this, wxT("Triggers"), ntTriggers);
    viewsM.setProperties(this, wxT("Views"), ntViews);
}
//-----------------------------------------------------------------------------
Database::Database(const Database& rhs)
    : MetadataItem(rhs), databaseM(rhs.databaseM),
        metadataLoaderM(0), connectedM(rhs.connectedM),
        databaseCharsetM(rhs.databaseCharsetM), pathM(rhs.pathM),
        credentialsM(rhs.credentialsM), connectionCredentialsM(0),
        domainsM(rhs.domainsM), exceptionsM(rhs.exceptionsM),
        functionsM(rhs.functionsM), generatorsM(rhs.generatorsM),
        proceduresM(rhs.proceduresM), rolesM(rhs.rolesM), tablesM(rhs.tablesM),
        sysTablesM(rhs.sysTablesM), triggersM(rhs.triggersM),
        viewsM(rhs.viewsM), collationsM(rhs.collationsM), idM(rhs.idM),
        storeEncryptedPasswordM(rhs.storeEncryptedPasswordM)
{
    if (rhs.connectionCredentialsM)
        connectionCredentialsM = new Credentials(*rhs.connectionCredentialsM);

    domainsM.setParent(this);
    exceptionsM.setParent(this);
    functionsM.setParent(this);
    generatorsM.setParent(this);
    proceduresM.setParent(this);
    rolesM.setParent(this);
    tablesM.setParent(this);
    sysTablesM.setParent(this);
    triggersM.setParent(this);
    viewsM.setParent(this);
}
//-----------------------------------------------------------------------------
Database::~Database()
{
    resetCredentials();
}
//-----------------------------------------------------------------------------
void Database::prepareTemporaryCredentials()
{
    resetCredentials();
    connectionCredentialsM = new Credentials;
    connectionCredentialsM->setCharset(credentialsM.getCharset()); // default to database charset
}
//-----------------------------------------------------------------------------
void Database::resetCredentials()
{
    if (connectionCredentialsM)  // i.e. there is some other
    {
        delete connectionCredentialsM;
        connectionCredentialsM = 0;
    }
}
//-----------------------------------------------------------------------------
void Database::getIdentifiers(std::vector<Identifier>& temp)
{
    tablesM.getChildrenNames(temp);
    sysTablesM.getChildrenNames(temp);
    viewsM.getChildrenNames(temp);
    proceduresM.getChildrenNames(temp);
    triggersM.getChildrenNames(temp);
    rolesM.getChildrenNames(temp);
    generatorsM.getChildrenNames(temp);
    functionsM.getChildrenNames(temp);
    domainsM.getChildrenNames(temp);
    exceptionsM.getChildrenNames(temp);
}
//-----------------------------------------------------------------------------
wxString getLoadingSql(NodeType type)
{
    switch (type)
    {
        case ntTable:       return wxT("select rr.rdb$relation_name from rdb$relations rr ")
            wxT(" where (rr.RDB$SYSTEM_FLAG = 0 or rr.RDB$SYSTEM_FLAG is null) ")
            wxT(" and rr.RDB$VIEW_SOURCE is null order by 1");

        case ntSysTable:    return wxT("select rr.rdb$relation_name from rdb$relations rr ")
            wxT(" where (rr.RDB$SYSTEM_FLAG = 1) ")
            wxT(" and rr.RDB$VIEW_SOURCE is null order by 1");

        case ntView:        return wxT("select rr.rdb$relation_name from rdb$relations rr ")
            wxT(" where (rr.RDB$SYSTEM_FLAG = 0 or rr.RDB$SYSTEM_FLAG is null) ")
            wxT(" and rr.RDB$VIEW_SOURCE is not null order by 1");

        case ntProcedure:   return wxT("select pp.rdb$PROCEDURE_name from rdb$procedures pp ")
            wxT(" where (pp.RDB$SYSTEM_FLAG = 0 or pp.RDB$SYSTEM_FLAG is null) order by 1");

        case ntTrigger:     return wxT("select RDB$TRIGGER_NAME from RDB$TRIGGERS where ")
            wxT("(RDB$SYSTEM_FLAG = 0 or RDB$SYSTEM_FLAG is null) ORDER BY 1");

        case ntRole:        return wxT("select RDB$ROLE_NAME from RDB$ROLES ORDER BY 1");

        case ntGenerator:   return wxT("select rdb$generator_name from rdb$generators where ")
            wxT("(RDB$SYSTEM_FLAG = 0 or RDB$SYSTEM_FLAG is null) ORDER BY 1");

        case ntFunction:    return wxT("select RDB$FUNCTION_NAME from RDB$FUNCTIONS where ")
            wxT("(RDB$SYSTEM_FLAG = 0 or RDB$SYSTEM_FLAG is null) ORDER BY 1");

        case ntDomain:      return wxT("select f.rdb$field_name from rdb$fields f ")
            wxT("left outer join rdb$types t on f.rdb$field_type=t.rdb$type ")
            wxT("where t.rdb$field_name='RDB$FIELD_TYPE' and f.rdb$field_name not starting with 'RDB$' order by 1");
        case ntException:   return wxT("select RDB$EXCEPTION_NAME from RDB$EXCEPTIONS ORDER BY 1");
        default:            return wxT("");
    };
}
//-----------------------------------------------------------------------------
// This could be moved to Column class
wxString Database::loadDomainNameForColumn(wxString table, wxString field)
{
    MetadataLoader* loader = getMetadataLoader();
    MetadataLoaderTransaction tr(loader);

    IBPP::Statement& st1 = loader->getStatement(
        "select rdb$field_source from rdb$relation_fields"
        " where rdb$relation_name = ? and rdb$field_name = ?"
    );
    st1->Set(1, wx2std(table));
    st1->Set(2, wx2std(field));
    st1->Execute();
    st1->Fetch();
    std::string domain;
    st1->Get(1, domain);
    domain.erase(domain.find_last_not_of(" ") + 1);
    return std2wx(domain);
}
//-----------------------------------------------------------------------------
void Database::getDatabaseTriggers(std::vector<Trigger *>& list)
{
    MetadataLoader* loader = getMetadataLoader();
    MetadataLoaderTransaction tr(loader);

    IBPP::Statement& st1 = loader->getStatement(
        "select rdb$trigger_name from rdb$triggers "
        "where rdb$relation_name is null "
        "order by rdb$trigger_sequence"
    );
    st1->Execute();
    while (st1->Fetch())
    {
        std::string name;
        st1->Get(1, name);
        name.erase(name.find_last_not_of(" ") + 1);
        Trigger* t = dynamic_cast<Trigger*>(findByNameAndType(ntTrigger,
            std2wx(name)));
        if (t)
            list.push_back(t);
    }
}
//-----------------------------------------------------------------------------
//! returns all collations for a given charset
std::vector<wxString> Database::getCollations(wxString charset)
{
    loadCollations();
    std::vector<wxString> temp;
    std::multimap<wxString, wxString>::iterator low, high;

    high = collationsM.upper_bound(charset);
    for (low = collationsM.lower_bound(charset); low != high; ++low)
        temp.push_back((*low).second);
    return temp;
}
//-----------------------------------------------------------------------------
Domain* Database::loadMissingDomain(wxString name)
{
    MetadataLoader* loader = getMetadataLoader();
    MetadataLoaderTransaction tr(loader);

    IBPP::Statement& st1 = loader->getStatement(
        "select count(*) from rdb$fields f"
        " left outer join rdb$types t on f.rdb$field_type=t.rdb$type"
        " where t.rdb$field_name='RDB$FIELD_TYPE' and f.rdb$field_name = ?"
    );
    st1->Set(1, wx2std(name));
    st1->Execute();
    if (st1->Fetch())
    {
        int c;
        st1->Get(1, c);
        if (c > 0)
        {
            Domain* d = domainsM.add(name); // add domain to collection
            d->setParent(this);
            if (name.substr(0, 4) != wxT("RDB$"))
                refreshByType(ntDomain);
            return d;
        }
    }
    return 0;
}
//-----------------------------------------------------------------------------
//! small helper function, reads sql and fills the vector with values
// this can be made template function in future
void Database::fillVector(std::vector<wxString>& list, wxString sql)
{
    MetadataLoader* loader = getMetadataLoader();
    MetadataLoaderTransaction tr(loader);

    IBPP::Statement& st1 = loader->getStatement(wx2std(sql));
    st1->Execute();
    while (st1->Fetch())
    {
        std::string s;
        st1->Get(1, s);
        s.erase(s.find_last_not_of(" ") + 1); // trim
        list.push_back(std2wx(s));
    }
}
//-----------------------------------------------------------------------------
bool Database::isDefaultCollation(const wxString& charset,
    const wxString& collate)
{
    loadCollations();
    // no collations for charset
    if (collationsM.lower_bound(charset) == collationsM.upper_bound(charset))
        return false;
    return ((*(collationsM.lower_bound(charset))).second == collate);
}
//-----------------------------------------------------------------------------
//! load charset-collation pairs if needed
void Database::loadCollations()
{
    if (!collationsM.empty())
        return;

    MetadataLoader* loader = getMetadataLoader();
    MetadataLoaderTransaction tr(loader);

    IBPP::Statement& st1 = loader->getStatement(
        "select c.rdb$character_set_name, k.rdb$collation_name "
        " from rdb$character_sets c"
        " left outer join rdb$collations k "
        "   on c.rdb$character_set_id = k.rdb$character_set_id "
        " order by c.rdb$character_set_name, k.rdb$collation_id");
    st1->Execute();
    while (st1->Fetch())
    {
        std::string charset, collation;
        st1->Get(1, charset);
        st1->Get(2, collation);
        charset.erase(charset.find_last_not_of(" ") + 1);
        collation.erase(collation.find_last_not_of(" ") + 1);
        collationsM.insert(std::multimap<wxString, wxString>::value_type(
            std2wx(charset), std2wx(collation)));
    }
}
//-----------------------------------------------------------------------------
wxString Database::getTableForIndex(wxString indexName)
{
    MetadataLoader* loader = getMetadataLoader();
    MetadataLoaderTransaction tr(loader);

    IBPP::Statement& st1 = loader->getStatement(
        "SELECT rdb$relation_name from rdb$indices where rdb$index_name = ?");
    st1->Set(1, wx2std(indexName));
    st1->Execute();

    wxString tableName;
    if (st1->Fetch())
    {
        std::string retval;
        st1->Get(1, retval);
        retval.erase(retval.find_last_not_of(" ") + 1);
        tableName = std2wx(retval);
    }
    return tableName;
}
//-----------------------------------------------------------------------------
//! load list of objects of type "type" from database, and fill the DBH
void Database::loadObjects(NodeType type, ProgressIndicator* indicator)
{
    switch (type)
    {
        case ntTable:       tablesM.clear();        break;
        case ntSysTable:    sysTablesM.clear();     break;
        case ntView:        viewsM.clear();         break;
        case ntProcedure:   proceduresM.clear();    break;
        case ntTrigger:     triggersM.clear();      break;
        case ntRole:        rolesM.clear();         break;
        case ntGenerator:   generatorsM.clear();    break;
        case ntFunction:    functionsM.clear();     break;
        case ntDomain:      domainsM.clear();       break;
        case ntException:   exceptionsM.clear();    break;
        default:            return;
    };

    MetadataLoader* loader = getMetadataLoader();
    // first start a transaction for metadata loading, then lock the database
    // when objects go out of scope and are destroyed, database will be
    // unlocked before the transaction is committed - any update() calls on
    // observers can possibly use the same transaction
    MetadataLoaderTransaction tr(loader);
    SubjectLocker lock(this);

    IBPP::Statement& st1 = loader->getStatement(wx2std(getLoadingSql(type)));
    st1->Execute();
    while (st1->Fetch())
    {
        std::string name;
        st1->Get(1, name);
        name.erase(name.find_last_not_of(" ") + 1);
        addObject(type, std2wx(name));

        if (indicator && indicator->isCanceled())
            break;
    }
    refreshByType(type);
}
//-----------------------------------------------------------------------------
void Database::loadGeneratorValues()
{
    MetadataLoader* loader = getMetadataLoader();
    // first start a transaction for metadata loading, then lock the database
    // when objects go out of scope and are destroyed, database will be
    // unlocked before the transaction is committed - any update() calls on
    // observers can possibly use the same transaction
    MetadataLoaderTransaction tr(loader);
    SubjectLocker lock(this);

    for (MetadataCollection<Generator>::iterator it = generatorsM.begin();
        it != generatorsM.end(); ++it)
    {
        (*it).loadValue();
    }
    // generatorsM.notify() not necessary, loadValue() notifies
}
//-----------------------------------------------------------------------------
//! Notify the observers that collection has changed
void Database::refreshByType(NodeType type)
{
    switch (type)
    {
        case ntTable:       tablesM.notifyObservers();      break;
        case ntSysTable:    sysTablesM.notifyObservers();   break;
        case ntView:        viewsM.notifyObservers();       break;
        case ntProcedure:   proceduresM.notifyObservers();  break;
        case ntTrigger:     triggersM.notifyObservers();    break;
        case ntRole:        rolesM.notifyObservers();       break;
        case ntGenerator:   generatorsM.notifyObservers();  break;
        case ntFunction:    functionsM.notifyObservers();   break;
        case ntDomain:      domainsM.notifyObservers();     break;
        case ntException:   exceptionsM.notifyObservers();  break;
        default:            return;
    };
}
//-----------------------------------------------------------------------------
MetadataItem* Database::findByName(wxString name)
{
    for (int n = (int)ntTable; n < (int)ntLastType; n++)
    {
        MetadataItem* m = findByNameAndType((NodeType)n, name);
        if (m)
            return m;
    }
    return 0;
}
//-----------------------------------------------------------------------------
MetadataItem* Database::findByNameAndType(NodeType nt, wxString name)
{
    switch (nt)
    {
        case ntTable:       return tablesM.findByName(name);     break;
        case ntSysTable:    return sysTablesM.findByName(name);  break;
        case ntView:        return viewsM.findByName(name);      break;
        case ntTrigger:     return triggersM.findByName(name);   break;
        case ntProcedure:   return proceduresM.findByName(name); break;
        case ntFunction:    return functionsM.findByName(name);  break;
        case ntGenerator:   return generatorsM.findByName(name); break;
        case ntRole:        return rolesM.findByName(name);      break;
        case ntDomain:      return domainsM.findByName(name);    break;
        case ntException:   return exceptionsM.findByName(name); break;
        default:
            return 0;
    };
}
//-----------------------------------------------------------------------------
Relation* Database::findRelation(const Identifier& name)
{
    for (MetadataCollection<Table>::iterator it = tablesM.begin();
        it != tablesM.end(); it++)
    {
        if ((*it).getIdentifier().equals(name))
            return &(*it);
    }
    for (MetadataCollection<View>::iterator it = viewsM.begin();
        it != viewsM.end(); it++)
    {
        if ((*it).getIdentifier().equals(name))
            return &(*it);
    }
    return 0;
}
//-----------------------------------------------------------------------------
Relation* Database::getRelationForTrigger(Trigger* trigger)
{
    if (!trigger)
        return 0;
    wxString relName = trigger->getTriggerRelation();
    if (relName.IsEmpty())
        return 0;
    return findRelation(Identifier(relName));
}
//-----------------------------------------------------------------------------
void Database::dropObject(MetadataItem* object)
{
    object->drop();     // alert the children if any

    // find the collection that contains it, and remove it
    NodeType nt = object->getType();
    switch (nt)
    {
        case ntTable:       tablesM.remove((Table*)object);            break;
        case ntSysTable:    sysTablesM.remove((Table*)object);         break;
        case ntView:        viewsM.remove((View*)object);              break;
        case ntTrigger:     triggersM.remove((Trigger*)object);        break;
        case ntProcedure:   proceduresM.remove((Procedure*)object);    break;
        case ntFunction:    functionsM.remove((Function*)object);      break;
        case ntGenerator:   generatorsM.remove((Generator*)object);    break;
        case ntRole:        rolesM.remove((Role*)object);              break;
        case ntDomain:      domainsM.remove((Domain*)object);          break;
        case ntException:   exceptionsM.remove((Exception*)object);    break;
        default:
            return;
    };
}
//-----------------------------------------------------------------------------
bool Database::addObject(NodeType type, wxString name)
{
    MetadataItem* m;
    switch (type)
    {
        case ntTable:       m = tablesM.add(name);      break;
        case ntSysTable:    m = sysTablesM.add(name);   break;
        case ntView:        m = viewsM.add(name);       break;
        case ntProcedure:   m = proceduresM.add(name);  break;
        case ntTrigger:     m = triggersM.add(name);    break;
        case ntRole:        m = rolesM.add(name);       break;
        case ntGenerator:   m = generatorsM.add(name);  break;
        case ntFunction:    m = functionsM.add(name);   break;
        case ntDomain:      m = domainsM.add(name);     break;
        case ntException:   m = exceptionsM.add(name);  break;
        default:            return false;
    }

    if (!m)     // should never happen, but just in case
        return false;
    m->setName_(name);
    m->setParent(this);
    m->setType(type);   // in case it doesn't have ctor to set it
    return true;
}
//-----------------------------------------------------------------------------
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

    // TODO: check that there are no unwanted side-effects to this
    // SubjectLocker locker(this);

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
        MetadataCollection<Table>::iterator it;
        for (it = tablesM.begin(); it != tablesM.end(); ++it)
            (*it).invalidateIndices(stm.getName());
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
        return;
    }

    // update all TABLEs, VIEWs and DATABASE on "DROP TRIGGER"
    if (stm.actionIs(actDROP, ntTrigger))
    {
        MetadataCollection<Table>::iterator itt;
        for (itt = tablesM.begin(); itt != tablesM.end(); itt++)
            (*itt).notifyObservers();
        MetadataCollection<View>::iterator itv;
        for (itv = viewsM.begin(); itv != viewsM.end(); itv++)
            (*itv).notifyObservers();
        notifyObservers();
    }

    if (stm.actionIs(actCREATE) || stm.actionIs(actDECLARE))
    {
        if (addObject(stm.getObjectType(), stm.getName()))
            refreshByType(stm.getObjectType());
        // when trigger created: force relations to update their property pages
        Relation *r = stm.getCreateTriggerRelation();
        if (r)
            r->notifyObservers();
        else if (stm.getObjectType() == ntTrigger) // database trigger probably
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
            g->loadValue();
        return;
    }

    if (stm.actionIs(actDROP))
    {
        dropObject(object);
        if (stm.getObjectType() == ntTable || stm.getObjectType() == ntView)
        {
            MetadataCollection<Trigger>::iterator it = triggersM.begin();
            while (it != triggersM.end())
            {
                Relation* r = getRelationForTrigger(&(*it));
                if (!r || r->getIdentifier().equals(stm.getIdentifier()))
                {
                    dropObject(&(*it));
                    it = triggersM.begin();
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
            MetadataItem* m = domainsM.findByName(domainName);
            if (!m)     // domain does not exist in DBH
            {
                m = domainsM.add();
                m->setName_(domainName);
                m->setParent(this);
                m->setType(ntDomain);   // just in case
            }
            dynamic_cast<Domain*>(m)->loadInfo();
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
        object->getDescription(true);   // force reload
        object->notifyObservers();
        return;
    }

    if (stm.actionIs(actALTER))
    {
        // TODO: this is a place where we would simply call virtual invalidate() function
        // and object would do wherever it needs to
        switch (stm.getObjectType())
        {
            case ntTable:
            case ntView:
                dynamic_cast<Relation*>(object)->loadColumns();
                break;
            case ntProcedure:
                dynamic_cast<Procedure*>(object)->checkAndLoadParameters(true); // force reload
                dynamic_cast<Procedure*>(object)->checkDependentProcedures();
                break;
            case ntException:
                dynamic_cast<Exception*>(object)->loadProperties(true);
                break;
            case ntFunction:
                dynamic_cast<Function*>(object)->loadInfo(true);
                break;
            case ntTrigger:
                {
                    Trigger* tr = dynamic_cast<Trigger*>(object);
                    if (tr)
                    {
                        tr->loadInfo(true);
                        Relation* r = getRelationForTrigger(tr);
                        if (r)
                            r->notifyObservers();
                        else  // database trigger
                            notifyObservers();
                    }
                    break;
                }
            case ntDomain:
                dynamic_cast<Domain *>(object)->loadInfo();
                // notify all table columns with that domain
                for (MetadataCollection<Table>::iterator it = tablesM.begin(); it != tablesM.end(); ++it)
                    for (MetadataCollection<Column>::iterator i2 = (*it).begin(); i2 != (*it).end(); ++i2)
                        if ((*i2).getSource() == stm.getName())
                            (*i2).notifyObservers();
                break;
            default:
                object->notifyObservers();
                break;
        }
    }
}
//-----------------------------------------------------------------------------
void Database::drop()
{
    databaseM->Drop();
}
//-----------------------------------------------------------------------------
void Database::reconnect()
{
    // must recreate, because IBPP::Database member will become invalid
    delete metadataLoaderM;
    metadataLoaderM = 0;

    databaseM->Disconnect();
    databaseM->Connect();
}
//-----------------------------------------------------------------------------
// the caller of this function should check whether the database object has the
// password set, and if it does not, it should provide the password
//               and if it does, just provide that password
void Database::connect(wxString password, ProgressIndicator* indicator)
{
    if (connectedM)
        return;

    try
    {
        if (indicator)
            indicator->initProgressIndeterminate(wxT("Establishing connection..."));
        databaseM = IBPP::DatabaseFactory("", wx2std(getConnectionString()),
            wx2std(getUsername()), wx2std(password), wx2std(getRole()),
            wx2std(getConnectionCharset()), "");

        databaseM->Connect();
        connectedM = true;
        notifyObservers();
        tablesM.setParent(this);
        sysTablesM.setParent(this);

        // first start a transaction for metadata loading, then lock the
        // database
        // when objects go out of scope and are destroyed, database will be
        // unlocked before the transaction is committed - any update() calls
        // on observers can possibly use the same transaction
        MetadataLoaderTransaction tr(getMetadataLoader());
        SubjectLocker lock(this);

        bool canceled = (indicator && indicator->isCanceled());
        if (!canceled)
        {
            // load database charset
            IBPP::Statement& st1 = metadataLoaderM->getStatement(
                "select rdb$character_set_name from rdb$database");
            st1->Execute();
            if (st1->Fetch())
            {
                std::string databaseCharset;
                st1->Get(1, databaseCharset);
                databaseCharsetM = std2wx(databaseCharset).Strip();
            }

            // load metadata information
#if wxCHECK_VERSION(2, 9, 0)
            struct NodeTypeName { NodeType type; const char* name; };
#else
            struct NodeTypeName { NodeType type; const wxChar* name; };
#endif
            static const NodeTypeName nodetypes[] = {
                { ntTable, wxTRANSLATE("Tables") },
                { ntSysTable, wxTRANSLATE("System tables") },
                { ntView, wxTRANSLATE("Views") },
                { ntProcedure, wxTRANSLATE("Procedures") },
                { ntTrigger, wxTRANSLATE("Triggers") },
                { ntRole, wxTRANSLATE("Roles") },
                { ntDomain, wxTRANSLATE("Domains") },
                { ntFunction, wxTRANSLATE("Functions") },
                { ntGenerator, wxTRANSLATE("Generators") },
                { ntException, wxTRANSLATE("Exceptions") }
            };

            int typeCount = sizeof(nodetypes) / sizeof(NodeTypeName);
            for (int i = 0; i < typeCount; i++)
            {
                if (indicator)
                {
                    wxString typeName(wxGetTranslation(nodetypes[i].name));
                    indicator->initProgress(wxString::Format(_("Loading %s..."),
                        typeName.c_str()), typeCount, i);
                }
                loadObjects(nodetypes[i].type, indicator);
                if (indicator && indicator->isCanceled())
                {
                    canceled = true;
                    break;
                }
            }
            if (!canceled && indicator)
                indicator->initProgress(_("Complete"), typeCount, typeCount);
        }
        if (canceled)
            disconnect();

        if (connectedM)
            databaseInfoM.load(databaseM);
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
//-----------------------------------------------------------------------------
void Database::disconnect(bool onlyDBH)
{
    if (!connectedM && !onlyDBH)
        return;

    if (!onlyDBH)
        databaseM->Disconnect();
    delete metadataLoaderM;
    metadataLoaderM = 0;
    resetCredentials();     // "forget" temporary username/password
    connectedM = false;

    // remove entire DBH beneath
    domainsM.clear();
    functionsM.clear();
    generatorsM.clear();
    proceduresM.clear();
    rolesM.clear();
    sysTablesM.clear();
    tablesM.clear();
    triggersM.clear();
    viewsM.clear();
    exceptionsM.clear();

    // this a special case for Database only since it doesn't destroy its subitems
    // but only hides them (i.e. getChildren returns nothing, but items are present)
    // so observers must get removed
    domainsM.detachAllObservers();
    functionsM.detachAllObservers();
    generatorsM.detachAllObservers();
    proceduresM.detachAllObservers();
    rolesM.detachAllObservers();
    sysTablesM.detachAllObservers();
    tablesM.detachAllObservers();
    triggersM.detachAllObservers();
    viewsM.detachAllObservers();
    exceptionsM.detachAllObservers();

    if (config().get(wxT("HideDisconnectedDatabases"), false))
        getServer()->notifyObservers();
    notifyObservers();
}
//-----------------------------------------------------------------------------
void Database::clear()
{
    setPath(wxT(""));
    setConnectionCharset(wxT(""));
    setUsername(wxT(""));
    setRawPassword(wxT(""));
    setRole(wxT(""));
}
//-----------------------------------------------------------------------------
bool Database::isConnected() const
{
    return connectedM;
}
//-----------------------------------------------------------------------------
MetadataLoader* Database::getMetadataLoader()
{
    if (metadataLoaderM == 0)
        metadataLoaderM = new MetadataLoader(*this, 8);
    return metadataLoaderM;
}
//-----------------------------------------------------------------------------
bool Database::getChildren(std::vector<MetadataItem*>& temp)
{
    if (!connectedM)
        return false;

    getCollections(temp, true);
    return true;
}
//-----------------------------------------------------------------------------
// returns vector of all subitems
void Database::getCollections(std::vector<MetadataItem*>& temp, bool system)
{
    temp.push_back(&domainsM);
    temp.push_back(&exceptionsM);
    temp.push_back(&functionsM);
    temp.push_back(&generatorsM);
    temp.push_back(&proceduresM);
    temp.push_back(&rolesM);
    // Only push back system tables when they should be shown
    if (system && showSysTables())
        temp.push_back(&sysTablesM);
    temp.push_back(&tablesM);
    temp.push_back(&triggersM);
    temp.push_back(&viewsM);
}
//-----------------------------------------------------------------------------
void Database::lockChildren()
{
    domainsM.lockSubject();
    exceptionsM.lockSubject();
    functionsM.lockSubject();
    generatorsM.lockSubject();
    proceduresM.lockSubject();
    rolesM.lockSubject();
    tablesM.lockSubject();
    sysTablesM.lockSubject();
    triggersM.lockSubject();
    viewsM.lockSubject();
}
//-----------------------------------------------------------------------------
void Database::unlockChildren()
{
    domainsM.unlockSubject();
    exceptionsM.unlockSubject();
    functionsM.unlockSubject();
    generatorsM.unlockSubject();
    proceduresM.unlockSubject();
    rolesM.unlockSubject();
    tablesM.unlockSubject();
    sysTablesM.unlockSubject();
    triggersM.unlockSubject();
    viewsM.unlockSubject();
}
//-----------------------------------------------------------------------------
MetadataCollection<Generator>::const_iterator Database::generatorsBegin()
{
    return generatorsM.begin();
}
//-----------------------------------------------------------------------------
MetadataCollection<Generator>::const_iterator Database::generatorsEnd()
{
    return generatorsM.end();
}
//-----------------------------------------------------------------------------
MetadataCollection<Domain>::const_iterator Database::domainsBegin()
{
    return domainsM.begin();
}
//-----------------------------------------------------------------------------
MetadataCollection<Domain>::const_iterator Database::domainsEnd()
{
    return domainsM.end();
}
//-----------------------------------------------------------------------------
MetadataCollection<Table>::const_iterator Database::tablesBegin()
{
    return tablesM.begin();
}
//-----------------------------------------------------------------------------
MetadataCollection<Table>::const_iterator Database::tablesEnd()
{
    return tablesM.end();
}
//-----------------------------------------------------------------------------
wxString Database::getPath() const
{
    return pathM;
}
//-----------------------------------------------------------------------------
wxString Database::getDatabaseCharset() const
{
    return databaseCharsetM;
}
//-----------------------------------------------------------------------------
wxString Database::getConnectionCharset() const
{
    if (connectionCredentialsM)
        return connectionCredentialsM->getCharset();
    else
        return credentialsM.getCharset();
}
//-----------------------------------------------------------------------------
bool Database::usesDifferentConnectionCharset() const
{
    wxString charset(getConnectionCharset().Upper());
    if (databaseCharsetM.IsEmpty() && charset == wxT("NONE"))
        return false;
    return (charset.compare(databaseCharsetM.Upper()) != 0);
}
//-----------------------------------------------------------------------------
wxString Database::getUsername() const
{
    if (connectionCredentialsM)
        return connectionCredentialsM->getUsername();
    else
        return credentialsM.getUsername();
}
//-----------------------------------------------------------------------------
wxString Database::getRawPassword() const
{
    if (connectionCredentialsM)
        return connectionCredentialsM->getPassword();
    else
        return credentialsM.getPassword();
}
//-----------------------------------------------------------------------------
wxString Database::getDecryptedPassword() const
{
    // if we already have an established connection return that password
    if (databaseM != 0 && databaseM->Connected())
        return std2wx(databaseM->UserPassword());

    // temporary connection
    if (connectionCredentialsM)
        return connectionCredentialsM->getPassword();
    wxString raw = credentialsM.getPassword();
    if (raw.IsEmpty())
        return wxEmptyString;

    if (storeEncryptedPasswordM)
        return decryptPassword(raw, getUsername() + getConnectionString());
    else
        return raw;
}
//-----------------------------------------------------------------------------
bool Database::getStoreEncryptedPassword() const
{
    return storeEncryptedPasswordM;
}
//-----------------------------------------------------------------------------
wxString Database::getRole() const
{
    if (connectionCredentialsM)
        return connectionCredentialsM->getRole();
    else
        return credentialsM.getRole();
}
//-----------------------------------------------------------------------------
IBPP::Database& Database::getIBPPDatabase()
{
    return databaseM;
}
//-----------------------------------------------------------------------------
void Database::setPath(wxString value)
{
    pathM = value;
}
//-----------------------------------------------------------------------------
void Database::setConnectionCharset(wxString value)
{
    if (connectionCredentialsM)
        connectionCredentialsM->setCharset(value);
    else
        credentialsM.setCharset(value);
}
//-----------------------------------------------------------------------------
void Database::setUsername(wxString value)
{
    if (connectionCredentialsM)
        connectionCredentialsM->setUsername(value);
    else
        credentialsM.setUsername(value);
}
//-----------------------------------------------------------------------------
void Database::setRawPassword(wxString value)
{
    if (connectionCredentialsM)
        connectionCredentialsM->setPassword(value);
    else
        credentialsM.setPassword(value);
}
//-----------------------------------------------------------------------------
void Database::setEncryptedPassword(wxString value)
{
    // temporary connection
    if (connectionCredentialsM)
    {
        connectionCredentialsM->setPassword(value);
        return;
    }
    if (value.IsEmpty())
    {
        credentialsM.setPassword(value);
        return;
    }

    if (storeEncryptedPasswordM)
    {
        credentialsM.setPassword(encryptPassword(value,
            getUsername()+getConnectionString()));
    }
    else
        credentialsM.setPassword(value);
}
//-----------------------------------------------------------------------------
void Database::setStoreEncryptedPassword(bool value)
{
    storeEncryptedPasswordM = value;
}
//-----------------------------------------------------------------------------
void Database::setRole(wxString value)
{
    if (connectionCredentialsM)
        connectionCredentialsM->setRole(value);
    else
        credentialsM.setRole(value);
}
//-----------------------------------------------------------------------------
const wxString Database::getTypeName() const
{
    return wxT("DATABASE");
}
//-----------------------------------------------------------------------------
void Database::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitDatabase(*this);
}
//-----------------------------------------------------------------------------
Server* Database::getServer() const
{
    return dynamic_cast<Server*>(getParentObjectOfType(ntServer));
}
//-----------------------------------------------------------------------------
wxString Database::getConnectionString() const
{
    wxString serverConnStr = getServer()->getConnectionString();
    if (!serverConnStr.empty())
        return serverConnStr + wxT(":") + pathM;
    else
        return pathM;
}
//-----------------------------------------------------------------------------
wxString Database::extractNameFromConnectionString() const
{
    wxString name = pathM;
    wxString::size_type p = name.find_last_of(wxT("/\\:"));
    if (p != wxString::npos)
        name.erase(0, p + 1);
    p = name.find_last_of(wxT("."));
    if (p != wxString::npos)
        name.erase(p, name.length());
    return name;
}
//-----------------------------------------------------------------------------
const wxString Database::getId() const
{
    if (idM == 0)
        idM = getRoot()->getNextId();
    wxString result = wxString::Format(wxT("%d"), idM);
    return result;
}
//-----------------------------------------------------------------------------
void Database::setId(int id)
{
    idM = id;
}
//-----------------------------------------------------------------------------
const DatabaseInfo& Database::getInfo()
{
    databaseInfoM.reloadIfNecessary(databaseM);
    return databaseInfoM;
}
//-----------------------------------------------------------------------------
void Database::loadInfo()
{
    databaseInfoM.load(databaseM);
    notifyObservers();
}
//-----------------------------------------------------------------------------
bool Database::showSysTables()
{
    const wxString SHOW_SYSTABLES = wxT("ShowSystemTables");

    bool b;
    if (!DatabaseConfig(this).getValue(SHOW_SYSTABLES, b))
        b = config().get(SHOW_SYSTABLES, true);

    return b;
}
//-----------------------------------------------------------------------------
