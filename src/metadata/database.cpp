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

  The Initial Developer of the Original Code is Milan Babuskov.

  Portions created by the original developer
  are Copyright (C) 2004 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s): Nando Dessena
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
#include "dberror.h"
#include "metadata/database.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/parameter.h"
#include "metadata/root.h"
#include "sql/SimpleParser.h"
#include "ugly.h"
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
Database::Database()
    : MetadataItem(), idM(0)
{
    typeM = ntDatabase;
    connectedM = false;
    connectionCredentialsM = 0;

    // has to be here, since notify() might be called before initChildren()
    domainsM.setProperties(this, wxT("Domains"), ntDomains);
    exceptionsM.setProperties(this, wxT("Exceptions"), ntExceptions);
    functionsM.setProperties(this, wxT("Functions"), ntFunctions);
    generatorsM.setProperties(this, wxT("Generators"), ntGenerators);
    proceduresM.setProperties(this, wxT("Procedures"), ntProcedures);
    rolesM.setProperties(this, wxT("Roles"), ntRoles);
    tablesM.setProperties(this, wxT("Tables"), ntTables);
    triggersM.setProperties(this, wxT("Triggers"), ntTriggers);
    viewsM.setProperties(this, wxT("Views"), ntViews);
}
//-----------------------------------------------------------------------------
Database::Database(const Database& rhs)
    : MetadataItem(rhs), databaseM(rhs.databaseM), connectedM(rhs.connectedM),
    databaseCharsetM(rhs.databaseCharsetM), pathM(rhs.pathM),
    credentialsM(rhs.credentialsM), connectionCredentialsM(0),
    domainsM(rhs.domainsM), exceptionsM(rhs.exceptionsM),
    functionsM(rhs.functionsM), generatorsM(rhs.generatorsM),
    proceduresM(rhs.proceduresM), rolesM(rhs.rolesM), tablesM(rhs.tablesM),
    triggersM(rhs.triggersM), viewsM(rhs.viewsM), collationsM(rhs.collationsM),
    idM(rhs.idM)
{
    if (rhs.connectionCredentialsM)
        connectionCredentialsM = new Credentials(*connectionCredentialsM);

    domainsM.setParent(this);
    exceptionsM.setParent(this);
    functionsM.setParent(this);
    generatorsM.setParent(this);
    proceduresM.setParent(this);
    rolesM.setParent(this);
    tablesM.setParent(this);
    triggersM.setParent(this);
    viewsM.setParent(this);
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
void Database::getIdentifiers(std::vector<wxString>& temp)
{
    tablesM.getChildrenNames(temp);
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
    try
    {
        IBPP::Transaction tr1 = IBPP::TransactionFactory(databaseM, IBPP::amRead);
        tr1->Start();
        IBPP::Statement st1 = IBPP::StatementFactory(databaseM, tr1);
        st1->Prepare(
            "select rdb$field_source from rdb$relation_fields where rdb$relation_name = ? and rdb$field_name = ?"
        );
        st1->Set(1, wx2std(table));
        st1->Set(2, wx2std(field));
        st1->Execute();
        st1->Fetch();
        std::string domain;
        st1->Get(1, domain);
        tr1->Commit();
        domain.erase(domain.find_last_not_of(" ") + 1);
        return std2wx(domain);
    }
    catch (IBPP::Exception &e)
    {
        lastError().setMessage(std2wx(e.ErrorMessage()));
    }
    catch (...)
    {
        lastError().setMessage(_("System error."));
    }

    ::wxMessageBox(lastError().getMessage(), _("Postprocessing error."), wxOK|wxICON_WARNING);
    return wxT("");
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
Domain *Database::loadMissingDomain(wxString name)
{
    try
    {
        IBPP::Transaction tr1 = IBPP::TransactionFactory(databaseM, IBPP::amRead);
        tr1->Start();
        IBPP::Statement st1 = IBPP::StatementFactory(databaseM, tr1);
        st1->Prepare(
            "select count(*) from rdb$fields f left outer join rdb$types t on f.rdb$field_type=t.rdb$type "
            "where t.rdb$field_name='RDB$FIELD_TYPE' and f.rdb$field_name = ?"
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
        tr1->Commit();
    }
    catch (IBPP::Exception &e)
    {
        lastError().setMessage(std2wx(e.ErrorMessage()));
    }
    catch (...)
    {
        lastError().setMessage(_("System error."));
    }
    return 0;
}
//-----------------------------------------------------------------------------
//! small helper function, reads sql and fills the vector with values
// this can be made template function in future
bool Database::fillVector(std::vector<wxString>& list, wxString sql)
{
    try
    {
        IBPP::Transaction tr1 = IBPP::TransactionFactory(databaseM, IBPP::amRead);
        tr1->Start();
        IBPP::Statement st1 = IBPP::StatementFactory(databaseM, tr1);
        st1->Prepare(wx2std(sql));
        st1->Execute();
        while (st1->Fetch())
        {
            std::string s;
            st1->Get(1, s);
            s.erase(s.find_last_not_of(" ") + 1); // trim
            list.push_back(std2wx(s));
        }
        tr1->Commit();
        return true;
    }
    catch (IBPP::Exception &e)
    {
        lastError().setMessage(std2wx(e.ErrorMessage()));
    }
    catch (...)
    {
        lastError().setMessage(_("System error."));
    }
    return false;
}
//-----------------------------------------------------------------------------
//! load charset-collation pairs if needed
void Database::loadCollations()
{
    if (!collationsM.empty())
        return;

    try
    {
        IBPP::Transaction tr1 = IBPP::TransactionFactory(databaseM, IBPP::amRead);
        tr1->Start();
        IBPP::Statement st1 = IBPP::StatementFactory(databaseM, tr1);
        st1->Prepare("select c.rdb$character_set_name, k.rdb$collation_name from rdb$character_sets c"
            " left outer join rdb$collations k on c.rdb$character_set_id = k.rdb$character_set_id order by 1, 2");
        st1->Execute();
        while (st1->Fetch())
        {
            std::string charset, collation;
            st1->Get(1, charset);
            st1->Get(2, collation);
            charset.erase(charset.find_last_not_of(" ") + 1);
            collation.erase(collation.find_last_not_of(" ") + 1);
            collationsM.insert(std::multimap<wxString, wxString>::value_type(std2wx(charset), std2wx(collation)));
        }
        tr1->Commit();
        return;
    }
    catch (IBPP::Exception &e)
    {
        lastError().setMessage(std2wx(e.ErrorMessage()));
    }
    catch (...)
    {
        lastError().setMessage(_("System error."));
    }

    ::wxMessageBox(lastError().getMessage(), _("Error while loading collations."), wxOK|wxICON_WARNING);
}
//-----------------------------------------------------------------------------
wxString Database::getTableForIndex(wxString indexName)
{
    try
    {
        IBPP::Transaction tr1 = IBPP::TransactionFactory(databaseM, IBPP::amRead);
        tr1->Start();
        IBPP::Statement st1 = IBPP::StatementFactory(databaseM, tr1);
        st1->Prepare("SELECT rdb$relation_name from rdb$indices where rdb$index_name = ?");
        st1->Set(1, wx2std(indexName));
        st1->Execute();
        if (st1->Fetch())
        {
            std::string retval;
            st1->Get(1, retval);
            retval.erase(retval.find_last_not_of(" ") + 1);
            return std2wx(retval);
        }
    }
    catch (IBPP::Exception &e)
    {
        lastError().setMessage(std2wx(e.ErrorMessage()));
    }
    catch (...)
    {
        lastError().setMessage(_("System error."));
    }

    ::wxMessageBox(lastError().getMessage(), _("Error while loading table for index."), wxOK|wxICON_WARNING);
    return wxT("");
}
//-----------------------------------------------------------------------------
//! load list of objects of type "type" from database, and fill the DBH
bool Database::loadObjects(NodeType type)
{
    switch (type)
    {
        case ntTable:       tablesM.clear();        break;
        case ntView:        viewsM.clear();         break;
        case ntProcedure:   proceduresM.clear();    break;
        case ntTrigger:     triggersM.clear();      break;
        case ntRole:        rolesM.clear();         break;
        case ntGenerator:   generatorsM.clear();    break;
        case ntFunction:    functionsM.clear();     break;
        case ntDomain:      domainsM.clear();       break;
        case ntException:   exceptionsM.clear();    break;
        default:            return false;
    };

    SubjectLocker locker(this);
    try
    {
        IBPP::Transaction tr1 = IBPP::TransactionFactory(databaseM, IBPP::amRead);
        tr1->Start();
        IBPP::Statement st1 = IBPP::StatementFactory(databaseM, tr1);
        st1->Prepare(wx2std(getLoadingSql(type)));
        st1->Execute();
        while (st1->Fetch())
        {
            std::string name;
            st1->Get(1, name);
            addObject(type, std2wx(name));
        }
        refreshByType(type);
        return true;
    }
    catch (IBPP::Exception &e)
    {
        lastError().setMessage(std2wx(e.ErrorMessage()));
    }
    catch (...)
    {
        lastError().setMessage(_("System error."));
    }

    return false;
}
//-----------------------------------------------------------------------------
bool Database::loadGeneratorValues()
{
    for (MetadataCollection<Generator>::iterator it = generatorsM.begin();
        it != generatorsM.end(); ++it)
    {
        if (!(*it).loadValue())
            return false;
    }
    // generatorsM.notify() not necessary, loadValue() notifies
    return true;
}
//-----------------------------------------------------------------------------
//! Notify the observers that collection has changed
void Database::refreshByType(NodeType type)
{
    switch (type)
    {
        case ntTable:       tablesM.notifyObservers();      break;
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
        case ntTable:       return tablesM.findByName(name);
        case ntView:        return viewsM.findByName(name);
        case ntTrigger:     return triggersM.findByName(name);
        case ntProcedure:   return proceduresM.findByName(name);
        case ntFunction:    return functionsM.findByName(name);
        case ntGenerator:   return generatorsM.findByName(name);
        case ntRole:        return rolesM.findByName(name);
        case ntDomain:      return domainsM.findByName(name);
        case ntException:   return exceptionsM.findByName(name);
        default:
            return 0;
    };
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
// a shortcut
inline void getCleanName(std::stringstream& strstrm, std::string& name)
{
    strstrm >> name;
    std::string::size_type p = name.find("(");     // TODO: we need a decent parser
    if (p != std::string::npos)                    //       but this hack will do until we have it
        name.erase(p);
    Identifier i(std2wx(name));
    name = wx2std(i.get());
}
//-----------------------------------------------------------------------------
//! reads a DDL statement and does accordingly

// drop [object_type] [name]
// alter [object_type] [name]
// create [object_type] [name]
// alter table [name] alter [column] type [domain or datatype]
// declare external function [name]
// set null flag via system tables update

//! FIXME: This NEEDS to be rewritten to use the Identifier class
//!        However, it would require a better tokenizer which would
//!        support identifiers which are quoted and have space in the name
bool Database::parseCommitedSql(wxString sql)
{
    sql += wxT("\n");    // if last line starts with --
    SimpleParser::removeComments(sql, wxT("/*"), wxT("*/"));
    SimpleParser::removeComments(sql, wxT("--"), wxT("\n"));
    sql = sql.Upper();              // make sql UpperCase for easier handling
    std::stringstream strstrm;      // parse statement into tokens
    std::string action, object_type, name;
    strstrm << wx2std(sql);
    strstrm >> action;
    strstrm >> object_type;
    getCleanName(strstrm, name);

    // patch for external functions whose name is made of two words. Shift the words.
    if ((action == "DECLARE" || action == "DROP") && object_type == "EXTERNAL" && name == "FUNCTION")
    {
        object_type = name;
        getCleanName(strstrm, name);
    }

    // support CREATE OR ALTER statements
    if (action == "CREATE" && object_type == "OR" && name == "ALTER")
    {
        strstrm >> object_type;
        getCleanName(strstrm, name);
        if (findByNameAndType(getTypeByName(std2wx(object_type)), std2wx(name))) // it is already CREATE
            action = "ALTER";
    }

    if (action == "SET" && object_type == "GENERATOR")
    {
        Generator* g = dynamic_cast<Generator*>(findByNameAndType(ntGenerator, std2wx(name)));
        if (!g)
            return true;
        g->loadValue(true);     // force (re)load of generator value
        action = "ALTER";       // behaves like "alter"
    }

    // indices update tables
    if (action == "DROP" && object_type == "INDEX")
    {
        // We cannot know which table is affected, so the only solution is that all tables reload their indices
        for (MetadataCollection<Table>::iterator it = tablesM.begin(); it != tablesM.end(); ++it)
            (*it).invalidateIndices();
        return true;
    }
    if (action == "ALTER" && object_type == "INDEX" || action == "SET" && object_type == "STATISTICS")  // refresh table
    {
        if (action == "SET")    // move by 1
            getCleanName(strstrm, name);
        wxString tableName = getTableForIndex(std2wx(name));
        Table* t = dynamic_cast<Table*>(findByNameAndType(ntTable, tableName));
        if (t)
            t->invalidateIndices();
        return true;
    }

    // triggers update tables and views
    if (action == "DROP" && object_type == "TRIGGER")   // update all tables
    {
        for (MetadataCollection<Table>::iterator it = tablesM.begin(); it != tablesM.end(); ++it)
            (*it).notifyObservers();
        for (MetadataCollection<View>::iterator it = viewsM.begin(); it != viewsM.end(); ++it)
            (*it).notifyObservers();
    }

    bool isIndex = false;
    if (action == "CREATE")                         // looking for CREATE INDEX
    {
        if (object_type == "INDEX")                 // CREATE [UNIQUE] [ASC[ENDING] | DESC[ENDING]] INDEX name ON table ...
            isIndex = true;
        else
        {
            bool found = false;
            wxString words[] = { wxT("UNIQUE"), wxT("ASC"), wxT("ASCENDING"), wxT("DESC"), wxT("DESCENDING") };
            for (int i = 0; i < sizeof(words) / sizeof(wxString); ++i)
            {
                if (words[i] == std2wx(object_type))
                {
                    found = true;
                    break;
                }
            }
            if (found)
            {
                if (name == "INDEX")
                {
                    isIndex = true;
                    getCleanName(strstrm, name);
                }
                else
                {
                    for (int i = 0; i < sizeof(words) / sizeof(wxString); ++i)
                    {
                        if (words[i] == std2wx(name))
                        {
                            found = true;
                            break;
                        }
                    }
                    if (found)
                    {
                        strstrm >> name;
                        if (name == "INDEX")
                        {
                            isIndex = true;
                            getCleanName(strstrm, name);
                        }
                    }
                }
            }
        }
    }
    if (isIndex)
    {
        strstrm >> name;    // ON
        getCleanName(strstrm, name);
        Table* t = dynamic_cast<Table*>(findByNameAndType(ntTable, std2wx(name)));
        if (t)
            t->invalidateIndices();
        return true;
    }

    // convert change in NULL flag to ALTER TABLE (since it should be processed like that)
    if (sql.substr(0, 44) == wxT("UPDATE RDB$RELATION_FIELDS SET RDB$NULL_FLAG"))
    {
        action = "ALTER";
        object_type = "TABLE";
        wxString::size_type pos = sql.find(wxT("RDB$RELATION_NAME = '"));
        if (pos == wxString::npos)
            return true;
        pos += 21;
        wxString::size_type end = sql.find(wxT("'"), pos);
        if (end == wxString::npos)
            return true;
        name = wx2std(sql.substr(pos, end - pos));
    }

    NodeType t = getTypeByName(std2wx(object_type));
    if (action == "RECREATE")
    {
        if (findByNameAndType(t, std2wx(name)))
            action = "ALTER";
        else
            action = "CREATE";
    }

    // process the action...
    if (action == "CREATE" || action == "DECLARE")
    {
        if (addObject(t, std2wx(name)))     // inserts object into collection
            refreshByType(t);
        if (object_type == "TRIGGER")       // new trigger created, alert tables/views
        {                                   // to update their property pages
            std::string relation;
            strstrm >> relation;    // FOR
            getCleanName(strstrm, relation);
            Relation* m = dynamic_cast<Relation*>(findByNameAndType(ntTable, std2wx(relation)));
            if (!m)
            {
                m = dynamic_cast<Relation*>(findByNameAndType(ntView, std2wx(relation)));
                if (!m)
                    return true;
            }
            m->notifyObservers();
        }
    }
    else if (action == "DROP" || action == "ALTER")
    {
        MetadataItem* object = findByNameAndType(t, std2wx(name));
        if (!object)
            return true;

        if (action == "DROP")
        {
            dropObject(object);
            if (t == ntTable || t == ntView)    // remove related triggers
            {
                while (true)
                {
                    Trigger* todrop = 0;
                    for (MetadataCollection<Trigger>::iterator it = triggersM.begin(); it != triggersM.end(); ++it)
                    {
                        wxString relname;            // trigger already gone => cannot fetch relation name
                        if (!(*it).getRelation(relname) || relname == std2wx(name))
                            todrop = &(*it);
                    }
                    if (todrop)
                        dropObject(todrop);
                    else
                        break;
                }
            }
        }
        else                        // ALTER
        {
            if (t == ntTable)       // ALTER TABLE xyz ALTER field TYPE {domain or datatype}
            {
                std::string alter, field_name, maybe_type, domain_or_datatype;
                strstrm >> alter;
                getCleanName(strstrm, field_name);
                if (field_name == "COLUMN") // ALTER TABLE xyz ALTER COLUMN field TYPE {domain or datatype}
                    getCleanName(strstrm, field_name);
                strstrm >> maybe_type;
                if (maybe_type == "TYPE")       // domain is either created/modified/deleted or none
                {                               // if we'd only know what was there before... life would be easier
                    strstrm >> domain_or_datatype;
                    wxString::size_type pos = domain_or_datatype.find("(");
                    if (pos != wxString::npos)
                        domain_or_datatype.erase(pos);      // remove if it has size/scale

                    std::vector<wxString> typenames;             // I first tried a simple array of strings
                    typenames.push_back(wxT("CHAR"));                    // but program kept crashing
                    typenames.push_back(wxT("VARCHAR"));
                    typenames.push_back(wxT("INTEGER"));
                    typenames.push_back(wxT("SMALLINT"));
                    typenames.push_back(wxT("NUMERIC"));
                    typenames.push_back(wxT("DECIMAL"));
                    typenames.push_back(wxT("FLOAT"));
                    typenames.push_back(wxT("DOUBLE PRECISION"));
                    typenames.push_back(wxT("DATE"));
                    typenames.push_back(wxT("TIME"));
                    typenames.push_back(wxT("TIMESTAMP"));
                    typenames.push_back(wxT("ARRAY"));
                    typenames.push_back(wxT("BLOB"));
                    bool is_datatype = false;
                    for (std::vector<wxString>::iterator it = typenames.begin(); it != typenames.end(); ++it)
                        if ((*it) == std2wx(domain_or_datatype))
                            is_datatype = true;

                    if (is_datatype)        // either existing domain is changing, or new is created
                    {
                        wxString domain_name = loadDomainNameForColumn(std2wx(name), std2wx(field_name));
                        MetadataItem* m = domainsM.findByName(domain_name);
                        if (m == 0)     // domain does not exist in DBH
                        {
                            m = domainsM.add();
                            m->setName_(domain_name);
                            m->setParent(this);
                            m->setType(ntDomain);   // just in case
                        }
                        ((Domain*)m)->loadInfo();
                    }
                    else
                    {
                        // there is (maybe) an extra RDB$domain in domainsM, but we can leave it there
                        // as it is not going to hurt anyone
                        // Besides, it appears that domain is left in database too (not cleared)
                        // so we won't call this: loadObjects(ntDomain);
                    }
                }
            }

            // TODO: this is a place where we would simply call virtual invalidate() function
            // and object would do wherever it needs to
            bool result = true;
            if (t == ntTable || t == ntView)
                result = ((Relation*)object)->loadColumns();
            else if (t == ntProcedure)
                result = ((Procedure*)object)->checkAndLoadParameters(true);   // force reload
            else if (t == ntException)
                ((Exception*)object)->loadProperties(true);
            else if (t == ntTrigger)
            {
                ((Trigger*)object)->loadInfo(true);
                wxString relation;                   // alert table/view
                Trigger* tr = dynamic_cast<Trigger*>(findByNameAndType(ntTrigger, std2wx(name)));
                if (!tr || !tr->getRelation(relation))
                    return true;
                Relation* m = dynamic_cast<Relation*>(findByNameAndType(ntTable, relation));
                if (!m)
                {
                    m = dynamic_cast<Relation*>(findByNameAndType(ntView, relation));
                    if (!m)
                        return true;
                }
                m->notifyObservers();
            }
            else
                object->notifyObservers();

            if (!result)
                return false;
        }
    }

    // TODO: also, it would be great to record all DDL SQL statements into some log file, so user can
    //       review it later. Perhaps add a configuration options:
    //       - Log all successful DDL statements (Y/N)
    //       - Log file (path to file on disk)
    //       - Log to db. (creates flamerobin_log table in database. That way the history of metadata changes
    //                      goes together with database, and mulitple developes can work together easily)
    //       Log can contain stuff like: Server,DB,User,Role,Data&Time,SQL statement
    //       In short: History
    return true;
}
//-----------------------------------------------------------------------------
bool Database::reconnect() const
{
    try
    {
        databaseM->Disconnect();
        databaseM->Connect();
        return true;
    }
    catch (IBPP::Exception &e)
    {
        lastError().setMessage(std2wx(e.ErrorMessage()));
    }
    catch (...)
    {
        lastError().setMessage(_("System error."));
    }
    return false;
}
//-----------------------------------------------------------------------------
// the caller of this function should check whether the database object has the
// password set, and if it does not, it should provide the password
//               and if it does, just provide that password
bool Database::connect(wxString password)
{
    if (connectedM)
        return true;

    try
    {
        databaseM = IBPP::DatabaseFactory("", wx2std(getConnectionString()), wx2std(getUsername()),
            wx2std(password), wx2std(getRole()), wx2std(getConnectionCharset()), "");
        databaseM->Connect();
        connectedM = true;
        notifyObservers();

        tablesM.setParent(this);

        // load database charset
        IBPP::Transaction tr1 = IBPP::TransactionFactory(databaseM, IBPP::amRead);
        tr1->Start();
        IBPP::Statement st1 = IBPP::StatementFactory(databaseM, tr1);
        st1->Prepare("select rdb$character_set_name from rdb$database");
        st1->Execute();
        if (st1->Fetch())
        {
            std::string databaseCharset;
            st1->Get(1, databaseCharset);
            databaseCharsetM = std2wx(databaseCharset);
        }
        databaseCharsetM.erase(databaseCharsetM.find_last_not_of(wxT(" ")) + 1);
        tr1->Commit();
        return true;
    }
    catch (IBPP::Exception &e)
    {
        lastError().setMessage(std2wx(e.ErrorMessage()));
    }
    catch (...)
    {
        lastError().setMessage(_("System error."));
    }

    databaseM.clear();
    return false;
}
//-----------------------------------------------------------------------------
bool Database::disconnect()
{
    if (!connectedM)
        return true;

    try
    {
        databaseM->Disconnect();
        resetCredentials();     // "forget" temporary username/password
        connectedM = false;

        // remove entire DBH beneath
        domainsM.clear();
        functionsM.clear();
        generatorsM.clear();
        proceduresM.clear();
        rolesM.clear();
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
        tablesM.detachAllObservers();
        triggersM.detachAllObservers();
        viewsM.detachAllObservers();
        exceptionsM.detachAllObservers();

        if (config().get(wxT("HideDisconnectedDatabases"), false))
            getServer()->notifyObservers();
        notifyObservers();
        return true;
    }
    catch (IBPP::Exception &e)
    {
        lastError().setMessage(std2wx(e.ErrorMessage()));
    }
    catch (...)
    {
        lastError().setMessage(_("System error."));
    }

    return false;
}
//-----------------------------------------------------------------------------
void Database::clear()
{
    setPath(wxT(""));
    setConnectionCharset(wxT(""));
    setUsername(wxT(""));
    setPassword(wxT(""));
    setRole(wxT(""));
}
//-----------------------------------------------------------------------------
bool Database::isConnected() const
{
    return connectedM;
}
//-----------------------------------------------------------------------------
bool Database::getChildren(std::vector<MetadataItem*>& temp)
{
    if (!connectedM)
        return false;

    getCollections(temp);
    return true;
}
//-----------------------------------------------------------------------------
// returns vector of all subitems
void Database::getCollections(std::vector<MetadataItem*>& temp)
{
    temp.push_back(&domainsM);
    temp.push_back(&exceptionsM);
    temp.push_back(&functionsM);
    temp.push_back(&generatorsM);
    temp.push_back(&proceduresM);
    temp.push_back(&rolesM);
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
wxString Database::getPassword() const
{
    if (connectionCredentialsM)
        return connectionCredentialsM->getPassword();
    else
        return credentialsM.getPassword();
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
void Database::setPassword(wxString value)
{
    if (connectionCredentialsM)
        connectionCredentialsM->setPassword(value);
    else
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
    visitor->visit(*this);
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
void Database::setName_(wxString s)
{
    identifierM.setDirect(s);
    notifyObservers();
}
//-----------------------------------------------------------------------------
