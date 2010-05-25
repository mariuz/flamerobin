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

#include <ibpp.h>

#include "core/StringUtils.h"
#include "core/FRError.h"
#include "engine/MetadataLoader.h"
#include "frutils.h"
#include "metadata/CreateDDLVisitor.h"
#include "metadata/database.h"
#include "metadata/relation.h"
#include "sql/StatementBuilder.h"
//-----------------------------------------------------------------------------
Relation::Relation()
    :relationInfoLoadedM(false)
{
    columnsM.setParent(this);
}
//-----------------------------------------------------------------------------
Relation::Relation(const Relation& rhs)
    : MetadataItem(rhs), columnsM(rhs.columnsM),
      relationInfoLoadedM(rhs.relationInfoLoadedM)
{
    columnsM.setParent(this);
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
size_t Relation::getColumnCount() const
{
    return columnsM.getChildrenCount();
}
//-----------------------------------------------------------------------------
void Relation::loadInfo()
{
    Database* d = getDatabase(wxT("Relation::loadInfo"));
    MetadataLoader* loader = d->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);

    std::string sql("select rdb$owner_name, ");
    if (d->getInfo().getODSVersionIsHigherOrEqualTo(11, 1))
        sql += "rdb$relation_type";
    else
        sql += "0";
    sql += " from rdb$relations where rdb$relation_name = ?";

    IBPP::Statement& st1 = loader->getStatement(sql);
    st1->Set(1, wx2std(getName_(), d->getCharsetConverter()));
    st1->Execute();
    if (st1->Fetch())
    {
        std::string name;
        st1->Get(1, name);
        ownerM = std2wxIdentifier(name, d->getCharsetConverter());
        st1->Get(2, relationTypeM);
        relationInfoLoadedM = true;
    }
}
//-----------------------------------------------------------------------------
wxString Relation::getOwner()
{
    if (!relationInfoLoadedM)
        loadInfo();
    return ownerM;
}
//-----------------------------------------------------------------------------
int Relation::getRelationType()
{
    if (!relationInfoLoadedM)
        loadInfo();
    return relationTypeM;
}
//-----------------------------------------------------------------------------
bool Relation::childrenLoaded() const
{
    // relations can't have 0 columns, so column info must have been loaded
    return columnsM.getChildrenCount() > 0;
}
//-----------------------------------------------------------------------------
void Relation::reloadChildren()
{
    columnsM.clear();

    Database *d = getDatabase(wxT("Relation::loadColumns"));
    MetadataLoader* loader = d->getMetadataLoader();
    // first start a transaction for metadata loading, then lock the relation
    // when objects go out of scope and are destroyed, object will be unlocked
    // before the transaction is committed - any update() calls on observers
    // can possibly use the same transaction
    MetadataLoaderTransaction tr(loader);
    SubjectLocker lock(this);

    IBPP::Statement& st1 = loader->getStatement(
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
    st1->Set(1, wx2std(getName_(), d->getCharsetConverter()));
    st1->Execute();
    while (st1->Fetch())
    {
        std::string s, coll;
        st1->Get(1, s);
        wxString fname(std2wxIdentifier(s, d->getCharsetConverter()));
        st1->Get(3, s);
        wxString source(std2wxIdentifier(s, d->getCharsetConverter()));
        if (!st1->IsNull(4))
            st1->Get(4, coll);
        wxString collation(std2wxIdentifier(coll, d->getCharsetConverter()));
        wxString computedSrc, defaultSrc;
        readBlob(st1, 5, computedSrc, d->getCharsetConverter());
        if (!st1->IsNull(6))
        {
            readBlob(st1, 6, defaultSrc, d->getCharsetConverter());
            // Some users reported two spaces before DEFAULT word in source
            // Perhaps some other tools can put garbage here? Should we
            // parse it as SQL to clean up comments, whitespace, etc?
            defaultSrc.Trim(false).Remove(0, 8);
        }

        Column *cc = columnsM.add();
        cc->setName_(fname);
        cc->setParent(this);
        cc->Init(!st1->IsNull(2), source,  computedSrc, collation,
            defaultSrc, !st1->IsNull(6));
    }
    notifyObservers();
}
//-----------------------------------------------------------------------------
//! holds all views + self (even if it's a table)
void Relation::getDependentViews(std::vector<Relation *>& views,
    const wxString& forColumn)
{
    std::vector<Dependency> list;
    if (forColumn.IsEmpty())
        getDependencies(list, false);
    else
        getDependencies(list, false, forColumn);
    for (std::vector<Dependency>::iterator it = list.begin();
        it != list.end(); ++it)
    {
        View *v = dynamic_cast<View *>((*it).getDependentObject());
        if (v && views.end() == std::find(views.begin(), views.end(), v))
            v->getDependentViews(views);
    }

    // add self
    views.push_back(this);
}
//-----------------------------------------------------------------------------
void Relation::getDependentChecks(std::vector<CheckConstraint>& checks)
{
    Database* d = getDatabase(wxT("Relation::getDependentChecks"));
    MetadataLoader* loader = d->getMetadataLoader();
    // first start a transaction for metadata loading, then lock the relation
    // when objects go out of scope and are destroyed, object will be unlocked
    // before the transaction is committed - any update() calls on observers
    // can possibly use the same transaction
    MetadataLoaderTransaction tr(loader);
    SubjectLocker lock(this);

    IBPP::Statement& st1 = loader->getStatement(
        "select c.rdb$constraint_name, t.rdb$relation_name, "
        "   t.rdb$trigger_source "
        "from rdb$check_constraints c "
        "join rdb$triggers t on c.rdb$trigger_name = t.rdb$trigger_name "
        "join rdb$dependencies d on "
        "   t.rdb$trigger_name = d.rdb$dependent_name "
        "where d.rdb$depended_on_name = ? "
        "and d.rdb$dependent_type = 2 and d.rdb$depended_on_type = 0 "
        "and t.rdb$trigger_type = 1 and d.rdb$field_name is null "
    );

    st1->Set(1, wx2std(getName_(), d->getCharsetConverter()));
    st1->Execute();
    while (st1->Fetch())
    {
        std::string s;
        st1->Get(1, s);
        wxString cname(std2wxIdentifier(s, d->getCharsetConverter()));
        st1->Get(2, s);
        wxString table(std2wxIdentifier(s, d->getCharsetConverter()));

        wxString source;
        readBlob(st1, 3, source, d->getCharsetConverter());

        Table* tab = dynamic_cast<Table*>(d->findByNameAndType(ntTable,
            table));
        if (!tab)
            continue;

        // check if it exists
        std::vector<CheckConstraint>::iterator it;
        for (it = checks.begin(); it != checks.end(); ++it)
            if ((*it).getTable() == tab && (*it).getName_() == cname)
                break;

        if (it != checks.end())     // already there
            continue;

        CheckConstraint c;
        c.setParent(tab);
        c.setName_(cname);
        c.sourceM = source;
        checks.push_back(c);
    }
}
//-----------------------------------------------------------------------------
// STEPS:
// * drop dependent check constraints (checks reference this view)
//      these checks can include other objects that might be dropped here
//      put CREATE of all checks at the end
// * alter all dep. stored procedures (empty)
// * drop dep. views (BUILD DEPENDENCY TREE) + TODO: computed table columns
// * drop this view
// * create this view
// * create dep. views + TODO: computed table columns
// * alter back SPs
// * recreate triggers
// * create checks
// * grant back privileges on all! dropped views
//
// TODO: we don't support computed columns. A table's column (if computed) can
//       reference the view, so view cannot be dropped. Since it is a column,
//       it can be referenced by other view, SP, trigger, etc.
//       All that could get a lot complicated, so we don't support it (yet).
//       P.S. Whoever builds a computed column based on a view, and decides
//            to reference it in other object, deserves the pleasure of
//            building the "rebuild" script manually ;)
wxString Relation::getRebuildSql(const wxString& forColumn)
{
    // 0. prepare stuff
    std::vector<Procedure *> procedures;
    std::vector<CheckConstraint> checks;
    wxString privileges, createTriggers, dropTriggers, dropViews, createViews;
    wxString fkDrop, fkDropSelf, fkCreate, fkCreateSelf, pkDrop, pkCreate;

    // 1. build view list (dependency tree) - ordered by DROP
    std::vector<Relation *> viewList;
    getDependentViews(viewList, forColumn);

    // 2. walk the tree and add stuff
    for (std::vector<Relation *>::iterator vi = viewList.begin();
        vi != viewList.end(); ++vi)
    {
        View *v = dynamic_cast<View*>(*vi);
        if (v)
        {
            dropViews += wxT("DROP VIEW ") + v->getQuotedName() + wxT(";\n");
            createViews = v->getCreateSql() + wxT("\n") + createViews;
        }
        else if (!forColumn.IsEmpty() && (*vi) != this)
            continue;
        std::vector<Dependency> list;               // procedures
        std::vector<Trigger *> trigs;
        // the following two lines are maybe not needed as own triggers should
        // be listed as dependencies as well:
        //(*vi)->getTriggers(trigs, Trigger::afterTrigger);   // own triggers
        //(*vi)->getTriggers(trigs, Trigger::beforeTrigger);
        if (v || forColumn.IsEmpty())
            (*vi)->getDependencies(list, false);
        else
            (*vi)->getDependencies(list, false, forColumn);
        for (std::vector<Dependency>::iterator it = list.begin();
            it != list.end(); ++it)
        {
            Procedure *p = dynamic_cast<Procedure *>(
                (*it).getDependentObject());
            if (p && procedures.end() == std::find(procedures.begin(),
                procedures.end(), p))
            {
                procedures.push_back(p);
            }

            Trigger *t = dynamic_cast<Trigger *>((*it).getDependentObject());
            if (t && trigs.end() == std::find(trigs.begin(), trigs.end(), t))
                trigs.push_back(t);
        }
        (*vi)->getDependentChecks(checks);

        for (std::vector<Trigger *>::iterator it = trigs.begin();
            it != trigs.end(); ++it)
        {
            // TODO: this would need a progress indicator that is created
            //       outside of the loop, otherwise several dialogs would be
            //       created and destroyed
            //       besides: this introduces GUI stuff into metadata, so
            //       we first need a global facility to create an instance
            //       of ProgressIndicator
            //       for the time being: don't use a progress indicator
            CreateDDLVisitor cdv(0);
            (*it)->acceptVisitor(&cdv);
            createTriggers += cdv.getSql() + wxT("\n");

            // view's triggers would be dropped together with view anyway
            // but it is much simpler this way
            dropTriggers += wxT("DROP TRIGGER ") + (*it)->getQuotedName()
                + wxT(";\n");
        }

        const std::vector<Privilege>* priv = (*vi)->getPrivileges();
        if (priv)
        {
            for (std::vector<Privilege>::const_iterator ci = priv->begin();
                ci != priv->end(); ++ci)
            {
                privileges += (*ci).getSql();
            }
        }
    }

    // only for tables
    Table *t1 = dynamic_cast<Table *>(this);
    if (t1)
    {
        // add own check constraints as well
        std::vector<CheckConstraint> *cc = t1->getCheckConstraints();
        if (cc)
        {
            for (std::vector<CheckConstraint>::iterator ccc = cc->begin();
                ccc != cc->end(); ++ccc)
            {
                if (forColumn.IsEmpty() || (*ccc).hasColumn(forColumn))
                    checks.push_back(*ccc);
            }
        }

        // The following must be done in this order (and reverse for CREATE):
        // a) drop and create foreign keys that reference this table
        // find all tables from "tables" which have foreign keys with "table"
        // and return them in "list"
        Database *d = getDatabase(wxT("Relation::getRebuildSql"));
        std::vector<ForeignKey> fkeys;
        MetadataCollection<Table> *tabs = d->getCollection<Table>();
        if (tabs)
        {
            for (MetadataCollection<Table>::iterator it = tabs->begin(); it !=
                tabs->end(); ++it)
            {
                std::vector<ForeignKey> *fk = (*it).getForeignKeys();
                if (fk)
                {
                    for (std::vector<ForeignKey>::iterator i2 = fk->begin();
                        i2 != fk->end(); ++i2)
                    {
                        if ((*i2).referencedTableM == getName_() &&
                            t1 != (*i2).getTable() && (
                            forColumn.IsEmpty() || (*i2).hasColumn(forColumn)))
                        {
                            fkeys.insert(fkeys.end(), (*i2));
                        }
                    }
                }
            }
        }
        // b) drop own primary and unique keys
        PrimaryKeyConstraint* pk = t1->getPrimaryKey();
        if (pk && (forColumn.IsEmpty() || pk->hasColumn(forColumn)))
        {
            pkDrop += wxT("ALTER TABLE ") + getQuotedName() +
                wxT(" DROP CONSTRAINT ") + pk->getQuotedName() + wxT(";\n");
            CreateDDLVisitor cdv(0);
            pk->acceptVisitor(&cdv);
            pkCreate += cdv.getSql();
        }
        std::vector<UniqueConstraint>* c = t1->getUniqueConstraints();
        if (c)
        {
            for (std::vector<UniqueConstraint>::iterator it = c->begin(); it != c->end(); ++it)
            {
                if (!forColumn.IsEmpty() && !(*it).hasColumn(forColumn))
                    continue;
                pkDrop += wxT("ALTER TABLE ") + getQuotedName() +
                    wxT(" DROP CONSTRAINT ") + (*it).getQuotedName() + wxT(";\n");
                CreateDDLVisitor cdv(0);
                (*it).acceptVisitor(&cdv);
                pkCreate += cdv.getSql();
            }
        }

        // b) drop foreign keys (others' and own)
        std::vector<ForeignKey> *fk = t1->getForeignKeys();
        if (fk)
            fkeys.insert(fkeys.end(), fk->begin(), fk->end());
        for (std::vector<ForeignKey>::iterator i2 = fkeys.begin();
            i2 != fkeys.end(); ++i2)
        {
            if (!forColumn.IsEmpty() && !(*i2).hasColumn(forColumn))
                continue;
            fkDrop += wxT("ALTER TABLE ") +
                (*i2).getTable()->getQuotedName() +
                wxT(" DROP CONSTRAINT ") +
                (*i2).getQuotedName() + wxT(";\n");
            CreateDDLVisitor cdv(0);
            (*i2).acceptVisitor(&cdv);
            fkCreate += cdv.getSql();
        }
    }

    wxString createChecks, dropChecks;
    for (std::vector<CheckConstraint>::iterator it = checks.begin();
        it != checks.end(); ++it)
    {
        if (!forColumn.IsEmpty() && !(*it).hasColumn(forColumn))
            continue;
        wxString cname = wxT("CONSTRAINT ") + (*it).getQuotedName();
        dropChecks += wxT("ALTER TABLE ") + (*it).getTable()->getQuotedName()
            + wxT(" DROP ") + cname + wxT(";\n");
        createChecks += wxT("ALTER TABLE ") +
            (*it).getTable()->getQuotedName() + wxT(" ADD ");
        if (!(*it).isSystem())
            createChecks += cname;
        createChecks += wxT("\n  ") + (*it).sourceM + wxT(";\n");
    }

    wxString sql(wxT("SET AUTODDL ON;\n\n"));
    sql += dropChecks;
    sql += dropTriggers;
    sql += fkDrop;
    sql += fkDropSelf;
    sql += pkDrop;
    for (std::vector<Procedure *>::iterator it = procedures.begin();
        it != procedures.end(); ++it)
    {
        sql += wxT("\n/* ------------------------------------------ */\n\n")
            + (*it)->getAlterSql(false);
    }
    sql += dropViews;
    sql += wxT("\n/**************** DROPPING COMPLETE ***************/\n\n");
    sql += createViews;
    for (std::vector<Procedure *>::iterator it = procedures.begin();
        it != procedures.end(); ++it)
    {
        sql += wxT("\n/* ------------------------------------------ */\n\n")
            + (*it)->getAlterSql(true);
    }
    sql += createTriggers;
    sql += createChecks;
    sql += pkCreate;
    sql += fkCreateSelf;
    sql += fkCreate;
    sql += privileges;

    // TODO: restore view and trigger descriptions

    return sql;
}
//-----------------------------------------------------------------------------
std::vector<Privilege>* Relation::getPrivileges()
{
    // load privileges from database and return the pointer to collection
    Database* d = getDatabase(wxT("Relation::getPrivileges"));
    MetadataLoader* loader = d->getMetadataLoader();

    privilegesM.clear();

    // first start a transaction for metadata loading, then lock the relation
    // when objects go out of scope and are destroyed, object will be unlocked
    // before the transaction is committed - any update() calls on observers
    // can possibly use the same transaction
    MetadataLoaderTransaction tr(loader);
    SubjectLocker lock(this);

    IBPP::Statement& st1 = loader->getStatement(
        "select RDB$USER, RDB$USER_TYPE, RDB$GRANTOR, RDB$PRIVILEGE, "
        "RDB$GRANT_OPTION, RDB$FIELD_NAME "
        "from RDB$USER_PRIVILEGES "
        "where RDB$RELATION_NAME = ? and rdb$object_type = 0 "
        "order by rdb$user, rdb$user_type, rdb$grant_option, rdb$privilege"
    );
    st1->Set(1, wx2std(getName_(), d->getCharsetConverter()));
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
            Privilege p(this, std2wx(user, d->getCharsetConverter()).Strip(),
                usertype);
            privilegesM.push_back(p);
            pr = &privilegesM.back();
            lastuser = user;
            lasttype = usertype;
        }
        pr->addPrivilege(privilege[0],
            std2wxIdentifier(grantor, d->getCharsetConverter()),
            grantoption == 1, std2wxIdentifier(field, d->getCharsetConverter()));
    }
    return &privilegesM;
}
//-----------------------------------------------------------------------------
//! load list of triggers for relation
//! link them to triggers in database's collection
void Relation::getTriggers(std::vector<Trigger *>& list,
    Trigger::fireTimeType beforeOrAfter)
{
    Database* d = getDatabase(wxT("Relation::getTriggers"));
    MetadataLoader* loader = d->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);

    IBPP::Statement& st1 = loader->getStatement(
        "select rdb$trigger_name from rdb$triggers where rdb$relation_name = ? "
        "order by rdb$trigger_sequence"
    );
    st1->Set(1, wx2std(getName_(), d->getCharsetConverter()));
    st1->Execute();
    while (st1->Fetch())
    {
        std::string name;
        st1->Get(1, name);
        Trigger* t = dynamic_cast<Trigger*>(d->findByNameAndType(ntTrigger,
            std2wxIdentifier(name, d->getCharsetConverter())));
        if (t && t->getFiringTime() == beforeOrAfter)
            list.push_back(t);
    }
}
//-----------------------------------------------------------------------------
wxString Relation::getSelectStatement()
{
    // get list of columns to SELECT
    ensureChildrenLoaded();
    wxArrayString cols;
    cols.Alloc(columnsM.getChildrenCount());
    for (MetadataCollection<Column>::const_iterator it = columnsM.begin();
        it != columnsM.end(); ++it)
    {
        cols.Add((*it).getQuotedName());
    }
    if (addRdbKeyToSelect())
        cols.Add(wxT("RDB$DB_KEY"));

    StatementBuilder sb;
    sb << kwSELECT << ' ' << StatementBuilder::IncIndent;

    // use "<<" only after concatenating everything
    // that shouldn't be split apart in line wrapping calculation
    for (size_t i = 0; i < cols.size() - 1; ++i)
        sb << wxT("r.") + cols[i] + wxT(", ");
    sb << wxT("r.") + cols.Last();

    sb << StatementBuilder::DecIndent << StatementBuilder::NewLine
        << kwFROM << ' ' << getQuotedName() << wxT(" r");
    return sb;
}
//-----------------------------------------------------------------------------
bool Relation::addRdbKeyToSelect()
{
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
