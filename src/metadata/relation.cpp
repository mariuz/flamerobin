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

#include <ibpp.h>

#include "core/StringUtils.h"
#include "core/FRError.h"
#include "engine/MetadataLoader.h"
#include "frutils.h"
#include "constants.h"
#include "metadata/column.h"
#include "metadata/CreateDDLVisitor.h"
#include "metadata/database.h"
#include "metadata/procedure.h"
#include "metadata/table.h"
#include "metadata/view.h"
#include "sql/StatementBuilder.h"

Relation::Relation(NodeType type, DatabasePtr database, const wxString& name)
    : MetadataItem(type, database.get(), name)
{
}

ColumnPtrs::iterator Relation::begin()
{
    // please - don't load here
    // this code is used to get columns we want to alert about changes
    // but if there aren't any columns, we don't want to waste time
    // loading them
    return columnsM.begin();
}

ColumnPtrs::iterator Relation::end()
{
    // please see comment for begin()
    return columnsM.end();
}

ColumnPtrs::const_iterator Relation::begin() const
{
    return columnsM.begin();
}

ColumnPtrs::const_iterator Relation::end() const
{
    return columnsM.end();
}

ColumnPtr Relation::findColumn(const wxString& name) const
{
    for (ColumnPtrs::const_iterator it = columnsM.begin();
        it != columnsM.end(); ++it)
    {
        if ((*it)->getName_() == name)
            return *it;
    }
    return ColumnPtr();
}

size_t Relation::getColumnCount() const
{
    return columnsM.size();
}

void Relation::loadProperties()
{
    setPropertiesLoaded(false);

    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);
    wxMBConv* converter = db->getCharsetConverter();

    std::string sql("select rdb$owner_name, ");
    sql += db->getInfo().getODSVersionIsHigherOrEqualTo(11, 1) ? "rdb$relation_type, " :" 0, ";
    // for tables: path to external file as string
    sql += "rdb$external_file, ";
    // for views: source as blob
    sql += "rdb$view_source ";
    sql += db->getInfo().getODSVersionIsHigherOrEqualTo(13, 0)? ", rdb$sql_security " : ", null ";
    sql += "from rdb$relations where rdb$relation_name = ?";

    IBPP::Statement& st1 = loader->getStatement(sql);
    st1->Set(1, wx2std(getName_(), converter));
    st1->Execute();
    if (st1->Fetch())
    {
        std::string name;
        st1->Get(1, name);
        ownerM = std2wxIdentifier(name, converter);
        st1->Get(2, relationTypeM);

        wxString value;
        // for tables: path to external file
        if (!st1->IsNull(3))
        {
            std::string s;
            st1->Get(3, s);
            setExternalFilePath(wxString(s.c_str(), *converter));
        }
        else
            setExternalFilePath(wxEmptyString);

        // for views: source
        if (!st1->IsNull(4))
        {
            readBlob(st1, 4, value, converter);
            setSource(value);
        }
        else
            setSource(wxEmptyString);
        // Sql Security
        if (!st1->IsNull(5))
        {
            bool b;
            st1->Get(5, b);
            sqlSecurityM = wxString(b ? "SQL SECURITY DEFINER" : "SQL SECURITY INVOKER");

        }
        else
            sqlSecurityM.clear();
    }

    setPropertiesLoaded(true);
}

void Relation::setExternalFilePath(const wxString& /*value*/)
{
}

void Relation::setSource(const wxString& /*value*/)
{
}

wxString Relation::getOwner()
{
    ensurePropertiesLoaded();
    return ownerM;
}

wxString Relation::getSqlSecurity()
{
    ensurePropertiesLoaded();
    return sqlSecurityM;
}

int Relation::getRelationType()
{
    ensurePropertiesLoaded();
    return relationTypeM;
}

void Relation::loadChildren()
{
    // in case an exception is thrown this should be repeated
    setChildrenLoaded(false);

    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();
    // first start a transaction for metadata loading, then lock the database
    // (lock the database instead of the relation itself, as loading columns
    // will cause domains to be loaded as well, so the domain collection
    // should be locked as well)
    // when objects go out of scope and are destroyed, object will be unlocked
    // before the transaction is committed - any update() calls on observers
    // can possibly use the same transaction
    MetadataLoaderTransaction tr(loader);
    SubjectLocker lock(db.get());
    wxMBConv* converter = db->getCharsetConverter();
    std::string sql(
            "select r.rdb$field_name, r.rdb$null_flag, r.rdb$field_source,"         //1,2,3
            " l.rdb$collation_name, f.rdb$computed_source, r.rdb$default_source,"   //4,5,6
            " r.rdb$description ");                                                 //7
    sql += db->getInfo().getODSVersionIsHigherOrEqualTo(12, 0) ? ", r.RDB$GENERATOR_NAME, r.RDB$IDENTITY_TYPE, g.RDB$INITIAL_VALUE, RDB$GENERATOR_INCREMENT " : ", null, null, null, null "; //8,9, 10, 11
    sql +=  " from rdb$fields f"
            " join rdb$relation_fields r "
            "     on f.rdb$field_name=r.rdb$field_source"
            " left outer join rdb$collations l "
            "     on l.rdb$collation_id = r.rdb$collation_id "
            "     and l.rdb$character_set_id = f.rdb$character_set_id";
    
    if (db->getInfo().getODSVersionIsHigherOrEqualTo(12, 0))
        sql += " left join RDB$GENERATORS g on g.RDB$GENERATOR_NAME = r.RDB$GENERATOR_NAME ";
    sql +=  " where r.rdb$relation_name = ?"
            " order by r.rdb$field_position";
    
    IBPP::Statement& st1 = loader->getStatement(sql);
    st1->Set(1, wx2std(getName_(), converter));
    st1->Execute();

    ColumnPtrs columns;
    while (st1->Fetch())
    {
        std::string s, coll;
        st1->Get(1, s);
        wxString fname(std2wxIdentifier(s, converter));
        bool notNull = false;
        if (!st1->IsNull(2))
            st1->Get(2, &notNull);
        st1->Get(3, s);
        wxString source(std2wxIdentifier(s, converter));
        if (!st1->IsNull(4))
            st1->Get(4, coll);
        wxString collation(std2wxIdentifier(coll, converter));
        wxString computedSrc, defaultSrc;
        readBlob(st1, 5, computedSrc, converter);
        bool hasDefault = !st1->IsNull(6);
        if (hasDefault)
        {
            readBlob(st1, 6, defaultSrc, converter);
            // Some users reported two spaces before DEFAULT word in source
            // Perhaps some other tools can put garbage here? Should we
            // parse it as SQL to clean up comments, whitespace, etc?
            defaultSrc.Trim(false).Remove(0, 8);
        }
        bool hasDescription = !st1->IsNull(7);
        wxString identityType = "";
        int initialValue = 0, incrementValue = 0;
        if (!st1->IsNull(8)) {
            int i;
            st1->Get(9, i);
            identityType = i == IDENT_TYPE_BY_DEFAULT ? "BY DEFAULT" : i == IDENT_TYPE_ALWAYS ? "ALWAYS" : "";
            st1->Get(10, initialValue);
            st1->Get(11, incrementValue);
        }


        ColumnPtr col = findColumn(fname);
        if (!col)
        {
            col.reset(new Column(this, fname));
            initializeLockCount(col, getLockCount());
        }
        columns.push_back(col);
        col->initialize(source, computedSrc, collation, !notNull,
            defaultSrc, hasDefault, hasDescription, identityType, initialValue, incrementValue);
    }

    setChildrenLoaded(true);
    if (columnsM != columns)
    {
        columnsM.swap(columns);
        notifyObservers();
    }
}

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

void Relation::getDependentChecks(std::vector<CheckConstraint>& checks)
{
    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();
    // first start a transaction for metadata loading, then lock the relation
    // when objects go out of scope and are destroyed, object will be unlocked
    // before the transaction is committed - any update() calls on observers
    // can possibly use the same transaction
    MetadataLoaderTransaction tr(loader);
    SubjectLocker lock(this);
    wxMBConv* converter = db->getCharsetConverter();

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

    st1->Set(1, wx2std(getName_(), converter));
    st1->Execute();
    while (st1->Fetch())
    {
        std::string s;
        st1->Get(1, s);
        wxString cname(std2wxIdentifier(s, converter));
        st1->Get(2, s);
        wxString table(std2wxIdentifier(s, converter));

        wxString source;
        readBlob(st1, 3, source, converter);

        Table* tab = dynamic_cast<Table*>(db->findByNameAndType(ntTable,
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
            dropViews += "DROP VIEW " + v->getQuotedName() + ";\n";
            createViews = v->getCreateSql() + "\n" + createViews;
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
            createTriggers += cdv.getSql() + "\n";

            // view's triggers would be dropped together with view anyway
            // but it is much simpler this way
            dropTriggers += "DROP TRIGGER " + (*it)->getQuotedName()
                + ";\n";
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
        DatabasePtr db = getDatabase();
        std::vector<ForeignKey> fkeys;
        TablesPtr tables(db->getTables());
        for (Tables::iterator it = tables->begin(); it != tables->end(); ++it)
        {
            std::vector<ForeignKey> *fk = (*it)->getForeignKeys();
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
        // b) drop own primary and unique keys
        PrimaryKeyConstraint* pk = t1->getPrimaryKey();
        if (pk && (forColumn.IsEmpty() || pk->hasColumn(forColumn)))
        {
            pkDrop += "ALTER TABLE " + getQuotedName() +
                " DROP CONSTRAINT " + pk->getQuotedName() + ";\n";
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
                pkDrop += "ALTER TABLE " + getQuotedName() +
                    " DROP CONSTRAINT " + (*it).getQuotedName() + ";\n";
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
            fkDrop += "ALTER TABLE " +
                (*i2).getTable()->getQuotedName() +
                " DROP CONSTRAINT " +
                (*i2).getQuotedName() + ";\n";
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
        wxString cname = "CONSTRAINT " + (*it).getQuotedName();
        dropChecks += "ALTER TABLE " + (*it).getTable()->getQuotedName()
            + " DROP " + cname + ";\n";
        createChecks += "ALTER TABLE " +
            (*it).getTable()->getQuotedName() + " ADD ";
        if (!(*it).isSystem())
            createChecks += cname;
        createChecks += "\n  " + (*it).sourceM + ";\n";
    }

    wxString sql("SET AUTODDL ON;\n\n");
    sql += dropChecks;
    sql += dropTriggers;
    sql += fkDrop;
    sql += fkDropSelf;
    sql += pkDrop;
    for (std::vector<Procedure *>::iterator it = procedures.begin();
        it != procedures.end(); ++it)
    {
        sql += "\n/* ------------------------------------------ */\n\n"
            + (*it)->getAlterSql(false);
    }
    sql += dropViews;
    sql += "\n/**************** DROPPING COMPLETE ***************/\n\n";
    sql += createViews;
    for (std::vector<Procedure *>::iterator it = procedures.begin();
        it != procedures.end(); ++it)
    {
        sql += "\n/* ------------------------------------------ */\n\n"
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

std::vector<Privilege>* Relation::getPrivileges()
{
    // load privileges from database and return the pointer to collection
    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();

    privilegesM.clear();

    // first start a transaction for metadata loading, then lock the relation
    // when objects go out of scope and are destroyed, object will be unlocked
    // before the transaction is committed - any update() calls on observers
    // can possibly use the same transaction
    MetadataLoaderTransaction tr(loader);
    SubjectLocker lock(this);
    wxMBConv* converter = db->getCharsetConverter();

    IBPP::Statement& st1 = loader->getStatement(
        "select RDB$USER, RDB$USER_TYPE, RDB$GRANTOR, RDB$PRIVILEGE, "
        "RDB$GRANT_OPTION, RDB$FIELD_NAME "
        "from RDB$USER_PRIVILEGES "
        "where RDB$RELATION_NAME = ? and rdb$object_type = 0 "
        "order by rdb$user, rdb$user_type, rdb$grant_option, rdb$privilege"
    );
    st1->Set(1, wx2std(getName_(), converter));
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
            Privilege p(this, wxString(user.c_str(), *converter).Strip(), usertype);
            privilegesM.push_back(p);
            pr = &privilegesM.back();
            lastuser = user;
            lasttype = usertype;
        }
        pr->addPrivilege(privilege[0], std2wxIdentifier(grantor, converter),
            grantoption == 1, std2wxIdentifier(field, converter));
    }
    return &privilegesM;
}

//! load list of triggers for relation
//! link them to triggers in database's collection
void Relation::getTriggers(std::vector<Trigger *>& list,
    Trigger::FiringTime time)
{
    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);
    wxMBConv* converter = db->getCharsetConverter();

    IBPP::Statement& st1 = loader->getStatement(
        "select rdb$trigger_name from rdb$triggers"
        " where rdb$relation_name = ?"
        " order by rdb$trigger_sequence"
    );
    st1->Set(1, wx2std(getName_(), converter));
    st1->Execute();
    while (st1->Fetch())
    {
        std::string name;
        st1->Get(1, name);
        Trigger* t = dynamic_cast<Trigger*>(db->findByNameAndType(ntDMLTrigger,
            std2wxIdentifier(name, converter)));
        if (t && t->getFiringTime() == time)
            list.push_back(t);
    }
}

bool Relation::getChildren(std::vector<MetadataItem*>& temp)
{
    if (columnsM.empty())
        return false;
    std::transform(columnsM.begin(), columnsM.end(), std::back_inserter(temp),
        std::mem_fn(&ColumnPtr::get));
    return !columnsM.empty();
}

void Relation::lockChildren()
{
    std::for_each(columnsM.begin(), columnsM.end(),
        std::mem_fn(&Column::lockSubject));
}

void Relation::unlockChildren()
{
    std::for_each(columnsM.begin(), columnsM.end(),
        std::mem_fn(&Column::unlockSubject));
}

