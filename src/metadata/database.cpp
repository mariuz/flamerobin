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

#include <wx/encconv.h>
#include <wx/fontmap.h>

#include <functional>
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
// CharacterSet class
CharacterSet::CharacterSet(const wxString& name, int id, int bytesPerChar)
    : nameM(name), idM(id), bytesPerCharM(bytesPerChar)
{
}
//-----------------------------------------------------------------------------
bool CharacterSet::operator< (const CharacterSet& other) const
{
    return nameM < other.nameM;
}
//-----------------------------------------------------------------------------
int CharacterSet::getBytesPerChar() const
{
    return bytesPerCharM;
}
//-----------------------------------------------------------------------------
int CharacterSet::getId() const
{
    return idM;
}
//-----------------------------------------------------------------------------
wxString CharacterSet::getName() const
{
    return nameM;
}
//-----------------------------------------------------------------------------
// Credentials class
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
    return (int64_t)getPages() * (int64_t)getPageSize();
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
// DatabaseAuthenticationMode class
DatabaseAuthenticationMode::DatabaseAuthenticationMode()
    : modeM(UseSavedPassword)
{
}
//-----------------------------------------------------------------------------
int DatabaseAuthenticationMode::getMode() const
{
    return int(modeM);
}
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
wxString DatabaseAuthenticationMode::getConfigValue() const
{
    switch (modeM)
    {
        case UseSavedEncryptedPwd:
            return wxT("encpwd");
        case AlwaysEnterPassword:
            return wxT("askpwd");
        case TrustedUser:
            return wxT("trusted");
        default:
            return wxT("pwd");
    }
}
//-----------------------------------------------------------------------------
void DatabaseAuthenticationMode::setConfigValue(const wxString& value)
{
    if (value == wxT("pwd"))
        modeM = UseSavedPassword;
    else if (value == wxT("encpwd"))
        modeM = UseSavedEncryptedPwd;
    else if (value == wxT("askpwd"))
        modeM = AlwaysEnterPassword;
    else if (value == wxT("trusted"))
        modeM = TrustedUser;
    else
        wxASSERT(false);
}
//-----------------------------------------------------------------------------
void DatabaseAuthenticationMode::setStoreEncryptedPassword()
{
    // ignore if old setting found after new mode has been set already
    if (modeM == UseSavedPassword)
        modeM = UseSavedEncryptedPwd;
}
//-----------------------------------------------------------------------------
bool DatabaseAuthenticationMode::getAlwaysAskForPassword() const
{
    return modeM == AlwaysEnterPassword;
}
//-----------------------------------------------------------------------------
bool DatabaseAuthenticationMode::getIgnoreUsernamePassword() const
{
    return modeM == TrustedUser;
}
//-----------------------------------------------------------------------------
bool DatabaseAuthenticationMode::getUseEncryptedPassword() const
{
    return modeM == UseSavedEncryptedPwd;
}
//-----------------------------------------------------------------------------
// Database class
Database::Database()
    : MetadataItem(ntDatabase), metadataLoaderM(0), connectedM(false),
        connectionCredentialsM(0), charsetConverterM(0), idM(0)
{
    // has to be here, since notify() might be called before initChildren()
    domainsM.setProperties(this, wxT("Domains"), ntDomains);
    domainsM.setloadChildrenProc(
        std::bind1st(std::mem_fun(&Database::loadDomains), this));
    exceptionsM.setProperties(this, wxT("Exceptions"), ntExceptions);
    exceptionsM.setloadChildrenProc(
        std::bind1st(std::mem_fun(&Database::loadExceptions), this));
    functionsM.setProperties(this, wxT("Functions"), ntFunctions);
    functionsM.setloadChildrenProc(
        std::bind1st(std::mem_fun(&Database::loadFunctions), this));
    generatorsM.setProperties(this, wxT("Generators"), ntGenerators);
    generatorsM.setloadChildrenProc(
        std::bind1st(std::mem_fun(&Database::loadGenerators), this));
    proceduresM.setProperties(this, wxT("Procedures"), ntProcedures);
    proceduresM.setloadChildrenProc(
        std::bind1st(std::mem_fun(&Database::loadProcedures), this));
    rolesM.setProperties(this, wxT("Roles"), ntRoles);
    rolesM.setloadChildrenProc(
        std::bind1st(std::mem_fun(&Database::loadRoles), this));
    tablesM.setProperties(this, wxT("Tables"), ntTables);
    tablesM.setloadChildrenProc(
        std::bind1st(std::mem_fun(&Database::loadTables), this));

    sysTablesM.setProperties(this, wxT("System tables"), ntSysTables);
    sysTablesM.setloadChildrenProc(
        std::bind1st(std::mem_fun(&Database::loadSystemTables), this));

    triggersM.setProperties(this, wxT("Triggers"), ntTriggers);
    triggersM.setloadChildrenProc(
        std::bind1st(std::mem_fun(&Database::loadTriggers), this));
    viewsM.setProperties(this, wxT("Views"), ntViews);
    viewsM.setloadChildrenProc(
        std::bind1st(std::mem_fun(&Database::loadViews), this));
}
//-----------------------------------------------------------------------------
Database::~Database()
{
    resetCredentials();
    delete charsetConverterM;
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
// This could be moved to Column class
wxString Database::loadDomainNameForColumn(wxString table, wxString field)
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
//-----------------------------------------------------------------------------
void Database::getDatabaseTriggers(std::vector<Trigger *>& list)
{
    MetadataLoader* loader = getMetadataLoader();
    MetadataLoaderTransaction tr(loader);
    wxMBConv* converter = getCharsetConverter();

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
        Trigger* t = dynamic_cast<Trigger*>(findByNameAndType(ntTrigger,
            std2wxIdentifier(name, converter)));
        if (t)
            list.push_back(t);
    }
}
//-----------------------------------------------------------------------------
CharacterSet Database::getCharsetById(int id)
{
    // if it contains both charset and collation as 2 bytes
    id %= 256;

    loadCollations();
    for (std::multimap<CharacterSet, wxString>::iterator it =
        collationsM.begin(); it != collationsM.end(); ++it)
    {
        if ((*it).first.getId() == id)
            return (*it).first;
    }
    throw FRError(wxString::Format(_("Character set ID %d not found."), id));
}
//-----------------------------------------------------------------------------
//! returns all collations for a given charset
std::vector<wxString> Database::getCollations(const wxString& charset)
{
    loadCollations();
    std::vector<wxString> temp;
    std::multimap<CharacterSet, wxString>::iterator low, high;
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
    st1->Set(1, wx2std(name, getCharsetConverter()));
    st1->Execute();
    if (st1->Fetch())
    {
        int c;
        st1->Get(1, c);
        if (c > 0)
            return domainsM.insert(this, name, ntDomain);
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

    IBPP::Statement& st1 = loader->getStatement(wx2std(sql,
        getCharsetConverter()));
    st1->Execute();
    while (st1->Fetch())
    {
        std::string s;
        st1->Get(1, s);
        list.push_back(std2wxIdentifier(s, getCharsetConverter()));
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
        CharacterSet cs(charset, charsetId, bytesPerChar);
        collationsM.insert(std::multimap<CharacterSet, wxString>::value_type(
            cs, collation));
    }
}
//-----------------------------------------------------------------------------
wxString Database::getTableForIndex(wxString indexName)
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
        // make sure generator value is reloaded from database
        (*it).invalidate();
        (*it).ensurePropertiesLoaded();
    }
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
        it != tablesM.end(); ++it)
    {
        if ((*it).getIdentifier().equals(name))
            return &(*it);
    }
    for (MetadataCollection<View>::iterator it = viewsM.begin();
        it != viewsM.end(); ++it)
    {
        if ((*it).getIdentifier().equals(name))
            return &(*it);
    }
    for (MetadataCollection<Table>::iterator it = sysTablesM.begin();
        it != sysTablesM.end(); ++it)
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
void Database::addObject(NodeType type, wxString name)
{
    switch (type)
    {
        case ntTable:
            tablesM.insert(this, name, type);
            break;
        case ntSysTable:
            sysTablesM.insert(this, name, type);
            break;
        case ntView:
            viewsM.insert(this, name, type);
            break;
        case ntProcedure:
            proceduresM.insert(this, name, type);
            break;
        case ntTrigger:
            triggersM.insert(this, name, type);
            break;
        case ntRole:
            rolesM.insert(this, name, type);
            break;
        case ntGenerator:
            generatorsM.insert(this, name, type);
            break;
        case ntFunction:
            functionsM.insert(this, name, type);
            break;
        case ntDomain:
            domainsM.insert(this, name, type);
            break;
        case ntException:
            exceptionsM.insert(this, name, type);
            break;
    }
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
        addObject(stm.getObjectType(), stm.getName());
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
                m = domainsM.insert(this, domainName, ntDomain);
            m->invalidate();
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
            case ntFunction:
                dynamic_cast<Function*>(object)->loadInfo(true);
                break;
            case ntTrigger:
                {
                    Trigger* tr = dynamic_cast<Trigger*>(object);
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
                for (MetadataCollection<Table>::iterator it = tablesM.begin();
                    it != tablesM.end(); ++it)
                {
                    for (MetadataCollection<Column>::iterator itColumn = (*it).begin();
                        itColumn != (*it).end(); ++itColumn)
                    {
                        if ((*itColumn).getSource() == stm.getName())
                            (*itColumn).invalidate();
                    }
                }
                break;
            default:
                // calls notifyObservers() only in the base class
                // descendent classes are free to put there whatever it takes...
                object->invalidate();
                break;
        }
    }
}
//-----------------------------------------------------------------------------
void Database::drop()
{
    databaseM->Drop();
    setDisconnected();
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
        createCharsetConverter();

        if (indicator)
            indicator->initProgressIndeterminate(wxT("Establishing connection..."));

        bool useUserNamePwd = !authenticationModeM.getIgnoreUsernamePassword();
        databaseM = IBPP::DatabaseFactory("", wx2std(getConnectionString()),
            (useUserNamePwd ? wx2std(getUsername()) : ""),
            (useUserNamePwd ? wx2std(password) : ""),
            wx2std(getRole()), wx2std(getConnectionCharset()), "");

        databaseM->Connect();
        connectedM = true;

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
            // load database charset
            IBPP::Statement& st1 = loader->getStatement(
                "select rdb$character_set_name from rdb$database");
            st1->Execute();
            if (st1->Fetch())
            {
                std::string s;
                st1->Get(1, s);
                databaseCharsetM = std2wxIdentifier(s, getCharsetConverter());
            }
            checkProgressIndicatorCanceled(indicator);
            // load database information
            setPropertiesLoaded(false);
            databaseInfoM.load(databaseM);
            setPropertiesLoaded(true);

            // load collections of metadata objects
            setChildrenLoaded(false);
            loadCollections(indicator);
            setChildrenLoaded(true);
            if (indicator)
                indicator->initProgress(_("Complete"), 100, 100);
        }
        catch (CancelProgressException&)
        {
            disconnect();
        }
        notifyObservers();
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
void Database::loadCollections(ProgressIndicator* progressIndicator)
{
    // use a small helper to cut down on the repetition...
    struct ProgressIndicatorHelper
    {
    private:
        ProgressIndicator* progressIndicatorM;
    public:
        ProgressIndicatorHelper(ProgressIndicator* progressIndicator)
            : progressIndicatorM(progressIndicator) {};
        void init(wxString collectionName, int stepsTotal, int currentStep)
        {
            if (progressIndicatorM)
            {
                wxString msg(wxString::Format(_("Loading %s..."),
                    collectionName.c_str()));
                progressIndicatorM->initProgress(msg, stepsTotal,
                    currentStep, 1);
            }
        }
    };

    const int collectionCount = 10;
    std::string loadStmt;
    ProgressIndicatorHelper pih(progressIndicator);

    MetadataLoader* loader = getMetadataLoader();
    MetadataLoaderTransaction tr(loader);
    SubjectLocker lock(this);

    pih.init(_("tables"), collectionCount, 1);
    loadTables(progressIndicator);

    pih.init(_("system tables"), collectionCount, 2);
    loadSystemTables(progressIndicator);

    pih.init(_("views"), collectionCount, 3);
    loadViews(progressIndicator);

    pih.init(_("procedures"), collectionCount, 4);
    loadProcedures(progressIndicator);

    pih.init(_("triggers"), collectionCount, 5);
    loadTriggers(progressIndicator);

    pih.init(_("roles"), collectionCount, 6);
    loadRoles(progressIndicator);

    pih.init(_("domains"), collectionCount, 7);
    loadDomains(progressIndicator);

    pih.init(_("functions"), collectionCount, 8);
    loadFunctions(progressIndicator);

    pih.init(_("generators"), collectionCount, 9);
    loadGenerators(progressIndicator);

    pih.init(_("exceptions"), collectionCount, 10);
    loadExceptions(progressIndicator);
}
//-----------------------------------------------------------------------------
wxArrayString Database::loadNames(ProgressIndicator* progressIndicator,
    std::string loadStatement)
{
    MetadataLoader* loader = getMetadataLoader();
    MetadataLoaderTransaction tr(loader);
    wxMBConv* converter = getCharsetConverter();

    IBPP::Statement& st1 = loader->getStatement(loadStatement);
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
//-----------------------------------------------------------------------------
void Database::loadDomains(ProgressIndicator* progressIndicator)
{
    std::string stmt = "select f.rdb$field_name from rdb$fields f"
        " left outer join rdb$types t on f.rdb$field_type=t.rdb$type"
        " where t.rdb$field_name='RDB$FIELD_TYPE'"
        " and f.rdb$field_name not starting with 'RDB$'"
        " order by 1";
    // setUserItems() will do what setItems() does, but not delete any
    // system items already in the list. Doing so for domains would result
    // in them being reloaded immediately, so keep them in the list
    // a distinct system domain collection would probably be better...
    domainsM.setUserItems(this, ntDomain, loadNames(progressIndicator, stmt));
}
//-----------------------------------------------------------------------------
void Database::loadExceptions(ProgressIndicator* progressIndicator)
{
    std::string stmt = "select rdb$exception_name from rdb$exceptions"
        " order by 1";
    exceptionsM.setItems(this, ntException,
        loadNames(progressIndicator, stmt));
}
//-----------------------------------------------------------------------------
void Database::loadFunctions(ProgressIndicator* progressIndicator)
{
    std::string stmt = "select rdb$function_name from rdb$functions"
        " where (rdb$system_flag = 0 or rdb$system_flag is null)"
        " order by 1";
    functionsM.setItems(this, ntFunction, loadNames(progressIndicator, stmt));
}
//-----------------------------------------------------------------------------
void Database::loadGenerators(ProgressIndicator* progressIndicator)
{
    std::string stmt = "select rdb$generator_name from rdb$generators"
        " where (rdb$system_flag = 0 or rdb$system_flag is null)"
        " order by 1";
    generatorsM.setItems(this, ntGenerator,
        loadNames(progressIndicator, stmt));
}
//-----------------------------------------------------------------------------
void Database::loadProcedures(ProgressIndicator* progressIndicator)
{
    std::string stmt = "select rdb$procedure_name from rdb$procedures"
        " where (rdb$system_flag = 0 or rdb$system_flag is null)"
        " order by 1";
    proceduresM.setItems(this, ntProcedure,
        loadNames(progressIndicator, stmt));
}
//-----------------------------------------------------------------------------
void Database::loadRoles(ProgressIndicator* progressIndicator)
{
    std::string stmt = "select rdb$role_name from rdb$roles order by 1";
    rolesM.setItems(this, ntRole, loadNames(progressIndicator, stmt));
}
//-----------------------------------------------------------------------------
void Database::loadSystemTables(ProgressIndicator* progressIndicator)
{
    std::string stmt = "select rdb$relation_name from rdb$relations"
        " where rdb$system_flag = 1"
        " and rdb$view_source is null order by 1";
    sysTablesM.setItems(this, ntSysTable, loadNames(progressIndicator, stmt));
}
//-----------------------------------------------------------------------------
void Database::loadTables(ProgressIndicator* progressIndicator)
{
    std::string stmt = "select rdb$relation_name from rdb$relations"
        " where (rdb$system_flag = 0 or rdb$system_flag is null)"
        " and rdb$view_source is null order by 1";
    tablesM.setItems(this, ntTable, loadNames(progressIndicator, stmt));
}
//-----------------------------------------------------------------------------
void Database::loadTriggers(ProgressIndicator* progressIndicator)
{
    std::string stmt = "select rdb$trigger_name from rdb$triggers"
        " where (rdb$system_flag = 0 or rdb$system_flag is null)"
        " order by 1";
    triggersM.setItems(this, ntTrigger, loadNames(progressIndicator, stmt));
}
//-----------------------------------------------------------------------------
void Database::loadViews(ProgressIndicator* progressIndicator)
{
    std::string stmt = "select rdb$relation_name from rdb$relations"
        " where (rdb$system_flag = 0 or rdb$system_flag is null)"
        " and rdb$view_source is not null order by 1";
    viewsM.setItems(this, ntView, loadNames(progressIndicator, stmt));
}
//-----------------------------------------------------------------------------
void Database::disconnect()
{
    if (connectedM)
    {
        databaseM->Disconnect();
        setDisconnected();
    }
}
//-----------------------------------------------------------------------------
void Database::setDisconnected()
{
    delete metadataLoaderM;
    metadataLoaderM = 0;
    resetCredentials();     // "forget" temporary username/password
    connectedM = false;
    resetPendingLoadData();

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
    ensureChildrenLoaded();

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
void Database::loadChildren()
{
    // TODO: show progress dialog while reloading child object collections?
    loadCollections(0);
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
    // unlock in reverse order of locking - that way domains will still
    // be locked when relation and procedure updates happen - that way not
    // every added domain will cause all collection observers to update
    viewsM.unlockSubject();
    triggersM.unlockSubject();
    sysTablesM.unlockSubject();
    tablesM.unlockSubject();
    rolesM.unlockSubject();
    proceduresM.unlockSubject();
    generatorsM.unlockSubject();
    functionsM.unlockSubject();
    exceptionsM.unlockSubject();
    domainsM.unlockSubject();
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
wxString Database::getConnectionInfoString() const
{
    wxString info;
    if (authenticationModeM.getIgnoreUsernamePassword())
        info = _("[Trusted user]");
    else
        info = getUsername();
    info += wxT("@") + getConnectionString() + wxT(" (")
        + getConnectionCharset() + wxT(")");
    return info;
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

    if (authenticationModeM.getUseEncryptedPassword())
        return decryptPassword(raw, getUsername() + getConnectionString());
    else
        return raw;
}
//-----------------------------------------------------------------------------
DatabaseAuthenticationMode& Database::getAuthenticationMode()
{
    return authenticationModeM;
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
    // temporary credentials -> use password as entered
    if (connectionCredentialsM)
    {
        connectionCredentialsM->setPassword(value);
        return;
    }

    // password must not be empty to be encrypted
    if (authenticationModeM.getUseEncryptedPassword() && !value.IsEmpty())
        value = encryptPassword(value, getUsername() + getConnectionString());
    credentialsM.setPassword(value);
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
        idM = getUniqueId();
    wxString result = wxString::Format(wxT("%d"), idM);
    return result;
}
//-----------------------------------------------------------------------------
unsigned uniqueDatabaseId = 1;
//-----------------------------------------------------------------------------
void Database::setId(unsigned id)
{
    idM = id;
    // generator to be higher than ids of existing databases
    if (id >= uniqueDatabaseId)
        uniqueDatabaseId = id + 1;
}
//-----------------------------------------------------------------------------
/*static*/
unsigned Database::getUniqueId()
{
    return uniqueDatabaseId++;
}
//-----------------------------------------------------------------------------
/*static*/
unsigned Database::getUIDGeneratorValue()
{
    return uniqueDatabaseId;
}
//-----------------------------------------------------------------------------
/*static*/
void Database::setUIDGeneratorValue(unsigned value)
{
    uniqueDatabaseId = value;
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
wxString mapConnectionCharsetToSystemCharset(const wxString& connectionCharset)
{
    wxString charset(connectionCharset.Upper().Trim());
    charset.Trim(false);

    // fixes hang when character set name empty (invalid encoding is returned)
    if (charset.empty())
        charset = wxT("NONE");

    // Firebird charsets WIN125X need to be replaced with either
    // WINDOWS125X or CP125X - we take the latter
    if (charset.Mid(0, 5) == wxT("WIN12"))
        return wxT("CP12") + charset.Mid(5);

    // Firebird charsets ISO8859_X
    if (charset.Mid(0, 8) == wxT("ISO8859_"))
        return wxT("ISO-8859-") + charset.Mid(8);

    // all other mappings need to be added here...
    struct CharsetMapping { const wxChar* connCS; const wxChar* convCS; };
    static const CharsetMapping mappings[] = {
        { wxT("UTF8"), wxT("UTF-8") }, { wxT("UNICODE_FSS"), wxT("UTF-8") }
    };
    int mappingCount = sizeof(mappings) / sizeof(CharsetMapping);
    for (int i = 0; i < mappingCount; i++)
    {
        if (mappings[i].connCS == charset)
            return mappings[i].convCS;
    }

    return charset;
}
//-----------------------------------------------------------------------------
void Database::createCharsetConverter()
{
    delete charsetConverterM;
    charsetConverterM = 0;

    wxString cs(mapConnectionCharsetToSystemCharset(getConnectionCharset()));
    wxFontEncoding fe = wxFontMapperBase::Get()->CharsetToEncoding(cs, false);
    if (fe != wxFONTENCODING_SYSTEM)
        charsetConverterM = new wxCSConv(fe);
}
//-----------------------------------------------------------------------------
wxMBConv* Database::getCharsetConverter() const
{
    if (charsetConverterM)
        return charsetConverterM;
    return wxConvCurrent;
}
//-----------------------------------------------------------------------------
