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

  Contributor(s): Michael Hieke
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

#include <ibpp.h>

#include "dberror.h"
#include "frutils.h"
#include "metadata/database.h"
#include "metadata/relation.h"
//-----------------------------------------------------------------------------
Relation::Relation()
{
    columnsM.setParent(this);
}
//-----------------------------------------------------------------------------
Relation::Relation(const Relation& rhs)
    : MetadataItem(rhs), columnsM(rhs.columnsM)
{
    columnsM.setParent(this);
}
//-----------------------------------------------------------------------------
Column *Relation::addColumn(Column &c)
{
    checkAndLoadColumns();
    Column *cc = columnsM.add(c);
    cc->setParent(this);
    return cc;
}
//-----------------------------------------------------------------------------
MetadataCollection<Column>::iterator Relation::begin()
{
    // please - don't load here
    // this code is used to get columns we want to alert about changes
    // but if there aren't any columns, we don't want to waste time
    // loading them
    return columnsM.begin();
}
//-----------------------------------------------------------------------------
MetadataCollection<Column>::iterator Relation::end()
{
    // please see comment for begin()
    return columnsM.end();
}
//-----------------------------------------------------------------------------
MetadataCollection<Column>::const_iterator Relation::begin() const
{
    return columnsM.begin();
}
//-----------------------------------------------------------------------------
MetadataCollection<Column>::const_iterator Relation::end() const
{
    return columnsM.end();
}
//-----------------------------------------------------------------------------
bool Relation::checkAndLoadColumns()
{
    return (!columnsM.empty() || loadColumns());
}
//-----------------------------------------------------------------------------
//! returns false if error occurs, and places the error text in error variable
bool Relation::loadColumns()
{
    columnsM.clear();
    Database *d = getDatabase();
    if (!d)
    {
        lastError().setMessage(wxT("database not set"));
        return false;
    }

    IBPP::Database& db = d->getIBPPDatabase();

    try
    {
        IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
        tr1->Start();
        IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
        st1->Prepare(
            "select r.rdb$field_name, r.rdb$null_flag, r.rdb$field_source, "
            " l.rdb$collation_name,f.rdb$computed_source,r.rdb$default_source"
            " from rdb$fields f"
            " join rdb$relation_fields r "
            "     on f.rdb$field_name=r.rdb$field_source"
            " left outer join rdb$collations l "
            "     on l.rdb$collation_id = r.rdb$collation_id "
            "     and l.rdb$character_set_id = f.rdb$character_set_id"
            " where r.rdb$relation_name = ?"
            " order by r.rdb$field_position"
        );

        st1->Set(1, wx2std(getName_()));
        st1->Execute();
        while (st1->Fetch())
        {
            std::string name, source, collation;
            wxString computedSrc, defaultSrc;
            st1->Get(1, name);
            st1->Get(3, source);
            st1->Get(4, collation);
            readBlob(st1, 5, computedSrc);
            readBlob(st1, 6, defaultSrc);

            Column *cc = columnsM.add();
            cc->setName_(std2wx(name));
            cc->setParent(this);
            cc->Init(!st1->IsNull(2), std2wx(source),
                computedSrc, std2wx(collation), defaultSrc);
        }

        tr1->Commit();
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
const std::vector<Privilege>* Relation::getPrivileges()
{
    // load privileges from database and return the pointer to collection
    Database *d = getDatabase();
    if (!d)
    {
        lastError().setMessage(wxT("database not set"));
        return 0;
    }
    privilegesM.clear();
    IBPP::Database& db = d->getIBPPDatabase();
    try
    {
        IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
        tr1->Start();
        IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
        st1->Prepare(
            "select RDB$USER, RDB$USER_TYPE, RDB$GRANTOR, RDB$PRIVILEGE, "
            "RDB$GRANT_OPTION, RDB$FIELD_NAME "
            "from RDB$USER_PRIVILEGES "
            "where RDB$RELATION_NAME = ? and rdb$object_type = 0 "
            "order by rdb$user, rdb$user_type, rdb$privilege"
        );
        st1->Set(1, wx2std(getName_()));
        st1->Execute();
        std::string lastuser;
        int lasttype = -1;
        Privilege *pr = 0;
        while (st1->Fetch())
        {
            std::string user, grantor, privilege, field;
            int usertype, grantoption = 0;
            st1->Get(1, user);
            st1->Get(2, usertype);
            st1->Get(3, grantor);
            st1->Get(4, privilege);
            if (!st1->IsNull(5))
                st1->Get(5, grantoption);
            st1->Get(6, field);
            if (!pr || user != lastuser || usertype != lasttype)
            {
                Privilege p(this, std2wx(user).Strip(), usertype,
                    std2wx(grantor).Strip(), grantoption == 1);
                privilegesM.push_back(p);
                pr = &privilegesM.back();
                lastuser = user;
                lasttype = usertype;
            }
            pr->addPrivilege(privilege[0]);
            wxString wfield = std2wx(field).Strip();
            if (privilege[0] == 'U' && !wfield.IsEmpty())
                pr->addUpdateColumn(wfield);
            if (privilege[0] == 'R' && !wfield.IsEmpty())
                pr->addReferencesColumn(wfield);
        }
        tr1->Commit();
        return &privilegesM;
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
//! load list of triggers for relation
//! link them to triggers in database's collection
bool Relation::getTriggers(std::vector<Trigger *>& list, Trigger::firingTimeType beforeOrAfter)
{
    Database *d = getDatabase();
    if (!d)
    {
        lastError().setMessage(wxT("database not set"));
        return false;
    }

    IBPP::Database& db = d->getIBPPDatabase();
    try
    {
        IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
        tr1->Start();
        IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
        st1->Prepare(
            "select rdb$trigger_name from rdb$triggers where rdb$relation_name = ? "
            "order by rdb$trigger_sequence"
        );
        st1->Set(1, wx2std(getName_()));
        st1->Execute();
        while (st1->Fetch())
        {
            std::string name;
            st1->Get(1, name);
            name.erase(name.find_last_not_of(" ") + 1);
            Trigger* t = dynamic_cast<Trigger*>(d->findByNameAndType(ntTrigger, std2wx(name)));
            if (t && t->getFiringTime() == beforeOrAfter)
                list.push_back(t);
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
bool Relation::getChildren(std::vector<MetadataItem*>& temp)
{
    return columnsM.getChildren(temp);
}
//-----------------------------------------------------------------------------
void Relation::lockChildren()
{
    columnsM.lockSubject();
}
//-----------------------------------------------------------------------------
void Relation::unlockChildren()
{
    columnsM.unlockSubject();
}
//-----------------------------------------------------------------------------
void Relation::loadDescription()
{
    MetadataItem::loadDescription(
        wxT("select RDB$DESCRIPTION from RDB$RELATIONS ")
        wxT("where RDB$RELATION_NAME = ?"));
}
//-----------------------------------------------------------------------------
void Relation::saveDescription(wxString description)
{
    MetadataItem::saveDescription(
        wxT("update RDB$RELATIONS set RDB$DESCRIPTION = ? ")
        wxT("where RDB$RELATION_NAME = ?"),
        description);
}
//-----------------------------------------------------------------------------
