/*
  Copyright (c) 2004-2011 The FlameRobin Development Team

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

#include "core/FRError.h"
#include "core/StringUtils.h"
#include "engine/MetadataLoader.h"
#include "frutils.h"
#include "metadata/column.h"
#include "metadata/domain.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/table.h"
//-----------------------------------------------------------------------------
Table::Table(DatabasePtr database, const wxString& name)
    : Relation((hasSystemPrefix(name) ? ntSysTable : ntTable), database, name),
        primaryKeyLoadedM(false), foreignKeysLoadedM(false),
        checkConstraintsLoadedM(false), uniqueConstraintsLoadedM(false),
        indicesLoadedM(false)
{
}
//-----------------------------------------------------------------------------
wxString Table::getExternalPath()
{
    ensurePropertiesLoaded();
    return externalPathM;
}
//-----------------------------------------------------------------------------
void Table::setExternalFilePath(const wxString& value)
{
    externalPathM = value;
}
//-----------------------------------------------------------------------------
void Table::invalidateIndices(const wxString& forIndex)
{
    if (indicesLoadedM)
    {
        bool hasit = false;
        if (forIndex.IsEmpty())
            hasit = true;
        else
        {
            for (std::vector<Index>::iterator it = indicesM.begin();
                it != indicesM.end(); ++it)
            {
                if ((*it).getName_() == forIndex)
                    hasit = true;
            }
        }
        if (hasit)
        {
            indicesLoadedM = false;
            notifyObservers();
        }
    }
}
//-----------------------------------------------------------------------------
void Table::loadChildren()
{
    // force info to be reloaded if asked
    primaryKeyLoadedM = false;
    foreignKeysLoadedM = false;
    checkConstraintsLoadedM = false;
    uniqueConstraintsLoadedM = false;
    indicesLoadedM = false;

    Relation::loadChildren();
}
//-----------------------------------------------------------------------------
wxString Table::getProcedureTemplate()
{
    ensureChildrenLoaded();
    wxString spname = wxT("SP_") + getName_();
    Identifier id(spname);
    wxString sql = wxT("SET TERM !! ;\nCREATE PROCEDURE ") + id.getQuoted() +
        wxT("\nRETURNS (");
    wxString collist, valist, parlist;

    Database* db = getDatabase(wxT("Table::getProcedureTemplate"));
    wxString dbcharset = db->getDatabaseCharset();
    for (ColumnPtrs::iterator i = columnsM.begin();
        i != columnsM.end(); ++i)
    {
        wxString datatype;
        DomainPtr d = (*i)->getDomain();
        if (!d)
            datatype = (*i)->getDatatype();
        else
        {
            datatype = d->getDatatypeAsString();
            wxString charset(d->getCharset());
            if (!charset.IsEmpty() && dbcharset != charset)
                datatype += wxT(" CHARACTER SET ") + charset;
        }

        if (!collist.empty())
        {
            valist += wxT(", ");
            collist += wxT(", ");
            parlist += wxT(",");
        }
        parlist += wxT("\n\t") + (*i)->getQuotedName() + wxT(" ") + datatype;
        collist += wxT("a.") + (*i)->getQuotedName();
        valist += wxT(":") + (*i)->getQuotedName();
    }
    sql += parlist;
    sql += wxT(")\nAS\nBEGIN\n\tFOR SELECT ") + collist + wxT("\n\t    FROM ")
        + getQuotedName() + wxT(" a\n\t    INTO ") + valist
        + wxT("\n\tDO\n\tBEGIN\n\t\tSUSPEND;\n\tEND\nEND!!\nSET TERM ; !!\n");
    return sql;
}
//-----------------------------------------------------------------------------
wxString Table::getInsertStatement()
{
    ensureChildrenLoaded();
    wxString sql = wxT("INSERT INTO ") + getQuotedName() + wxT(" (");
    wxString collist, valist;
    for (ColumnPtrs::const_iterator i = columnsM.begin();
        i != columnsM.end(); ++i)
    {
        if (!(*i)->getComputedSource().empty())
            continue;
        if (!collist.empty())
        {
            valist += wxT(", \n");
            collist += wxT(", ");
        }
        collist += (*i)->getQuotedName();

        if (!(*i)->isNullable() && (!(*i)->hasDefault()))
            valist += wxT("*");

        if (!(*i)->hasDefault())
            valist += (*i)->getName_();
        else
            valist += (*i)->getDefault();

    }
    sql += collist + wxT(")\n VALUES (\n") + valist + wxT("\n)");
    return sql;
}
//-----------------------------------------------------------------------------
//! reads checks info from database
void Table::loadCheckConstraints()
{
    if (checkConstraintsLoadedM)
        return;
    checkConstraintsM.clear();

    Database* d = getDatabase(wxT("Table::loadCheckConstraints"));
    MetadataLoader* loader = d->getMetadataLoader();
    // first start a transaction for metadata loading, then lock the table
    // when objects go out of scope and are destroyed, table will be unlocked
    // before the transaction is committed - any update() calls on observers
    // can possibly use the same transaction
    MetadataLoaderTransaction tr(loader);
    SubjectLocker lock(this);

    IBPP::Statement& st1 = loader->getStatement(
        "select r.rdb$constraint_name, t.rdb$trigger_source, d.rdb$field_name "
        " from rdb$relation_constraints r "
        " join rdb$check_constraints c on r.rdb$constraint_name=c.rdb$constraint_name and r.rdb$constraint_type = 'CHECK'"
        " join rdb$triggers t on c.rdb$trigger_name=t.rdb$trigger_name and t.rdb$trigger_type = 1 "
        " left join rdb$dependencies d on t.rdb$trigger_name = d.rdb$dependent_name "
        "      and d.rdb$depended_on_name = r.rdb$relation_name "
        "      and d.rdb$depended_on_type = 0 "
        " where r.rdb$relation_name=? "
        " order by 1 "
    );

    st1->Set(1, wx2std(getName_(), d->getCharsetConverter()));
    st1->Execute();
    CheckConstraint *cc = 0;
    while (st1->Fetch())
    {
        std::string s;
        st1->Get(1, s);
        wxString cname(std2wxIdentifier(s, d->getCharsetConverter()));
        if (!cc || cname != cc->getName_()) // new constraint
        {
            wxString source;
            readBlob(st1, 2, source, d->getCharsetConverter());

            CheckConstraint c;
            c.setParent(this);
            c.setName_(cname);
            c.sourceM = source;
            checkConstraintsM.push_back(c);
            cc = &checkConstraintsM.back();
        }

        if (!st1->IsNull(3))
        {
            st1->Get(3, s);
            wxString fname(std2wxIdentifier(s, d->getCharsetConverter()));
            cc->columnsM.push_back(fname);
        }
    }
    checkConstraintsLoadedM = true;
}
//-----------------------------------------------------------------------------
//! reads primary key info from database
void Table::loadPrimaryKey()
{
    if (primaryKeyLoadedM)
        return;
    primaryKeyM.columnsM.clear();

    Database* d = getDatabase(wxT("Table::loadPrimaryKey"));
    MetadataLoader* loader = d->getMetadataLoader();
    // first start a transaction for metadata loading, then lock the table
    // when objects go out of scope and are destroyed, table will be unlocked
    // before the transaction is committed - any update() calls on observers
    // can possibly use the same transaction
    MetadataLoaderTransaction tr(loader);
    SubjectLocker lock(this);

    IBPP::Statement& st1 = loader->getStatement(
        "select r.rdb$constraint_name, i.rdb$field_name, r.rdb$index_name "
        "from rdb$relation_constraints r, rdb$index_segments i "
        "where r.rdb$relation_name=? and r.rdb$index_name=i.rdb$index_name and "
        "(r.rdb$constraint_type='PRIMARY KEY') order by r.rdb$constraint_name, i.rdb$field_position"
    );

    st1->Set(1, wx2std(getName_(), d->getCharsetConverter()));
    st1->Execute();
    while (st1->Fetch())
    {
        std::string s;
        st1->Get(1, s);
        wxString cname(std2wxIdentifier(s, d->getCharsetConverter()));
        st1->Get(2, s);
        wxString fname(std2wxIdentifier(s, d->getCharsetConverter()));
        st1->Get(3, s);
        wxString ixname(std2wxIdentifier(s, d->getCharsetConverter()));

        primaryKeyM.setName_(cname);
        primaryKeyM.columnsM.push_back(fname);
        primaryKeyM.indexNameM = ixname;
    }
    primaryKeyM.setParent(this);
    primaryKeyLoadedM = true;
}
//-----------------------------------------------------------------------------
//! reads uniques from database
void Table::loadUniqueConstraints()
{
    if (uniqueConstraintsLoadedM)
        return;
    uniqueConstraintsM.clear();

    Database* d = getDatabase(wxT("Table::loadUniqueConstraints"));
    MetadataLoader* loader = d->getMetadataLoader();
    // first start a transaction for metadata loading, then lock the table
    // when objects go out of scope and are destroyed, table will be unlocked
    // before the transaction is committed - any update() calls on observers
    // can possibly use the same transaction
    MetadataLoaderTransaction tr(loader);
    SubjectLocker lock(this);

    IBPP::Statement& st1 = loader->getStatement(
        "select r.rdb$constraint_name, i.rdb$field_name, r.rdb$index_name "
        "from rdb$relation_constraints r, rdb$index_segments i "
        "where r.rdb$relation_name=? and r.rdb$index_name=i.rdb$index_name and "
        "(r.rdb$constraint_type='UNIQUE') order by r.rdb$constraint_name, i.rdb$field_position"
    );

    st1->Set(1, wx2std(getName_(), d->getCharsetConverter()));
    st1->Execute();
    UniqueConstraint *cc = 0;
    while (st1->Fetch())
    {
        std::string s;
        st1->Get(1, s);
        wxString cname(std2wxIdentifier(s, d->getCharsetConverter()));
        st1->Get(2, s);
        wxString fname(std2wxIdentifier(s, d->getCharsetConverter()));
        st1->Get(3, s);
        wxString ixname(std2wxIdentifier(s, d->getCharsetConverter()));

        if (cc && cc->getName_() == cname)
            cc->columnsM.push_back(fname);
        else
        {
            UniqueConstraint c;
            uniqueConstraintsM.push_back(c);
            cc = &uniqueConstraintsM.back();
            cc->indexNameM = ixname;
            cc->setName_(cname);
            cc->columnsM.push_back(fname);
            cc->setParent(this);
        }
    }
    uniqueConstraintsLoadedM = true;
}
//-----------------------------------------------------------------------------
PrimaryKeyConstraint *Table::getPrimaryKey()
{
    loadPrimaryKey();
    if (primaryKeyM.columnsM.empty())  // no PK
        return 0;
    return &primaryKeyM;
}
//-----------------------------------------------------------------------------
std::vector<ForeignKey> *Table::getForeignKeys()
{
    loadForeignKeys();
    return &foreignKeysM;
}
//-----------------------------------------------------------------------------
std::vector<CheckConstraint> *Table::getCheckConstraints()
{
    loadCheckConstraints();
    return &checkConstraintsM;
}
//-----------------------------------------------------------------------------
std::vector<UniqueConstraint> *Table::getUniqueConstraints()
{
    loadUniqueConstraints();
    return &uniqueConstraintsM;
}
//-----------------------------------------------------------------------------
std::vector<Index> *Table::getIndices()
{
    loadIndices();
    return &indicesM;
}
//-----------------------------------------------------------------------------
//! reads foreign keys info from database
void Table::loadForeignKeys()
{
    if (foreignKeysLoadedM)
        return;
    foreignKeysM.clear();

    Database* d = getDatabase(wxT("Table::loadForeignKeys"));
    MetadataLoader* loader = d->getMetadataLoader();
    // first start a transaction for metadata loading, then lock the table
    // when objects go out of scope and are destroyed, table will be unlocked
    // before the transaction is committed - any update() calls on observers
    // can possibly use the same transaction
    MetadataLoaderTransaction tr(loader);
    SubjectLocker lock(this);

    IBPP::Statement& st1 = loader->getStatement(
        "select r.rdb$constraint_name, i.rdb$field_name, c.rdb$update_rule, "
        " c.rdb$delete_rule, c.RDB$CONST_NAME_UQ, r.rdb$index_name "
        "from rdb$relation_constraints r, rdb$index_segments i, rdb$ref_constraints c "
        "where r.rdb$relation_name=? and r.rdb$index_name=i.rdb$index_name  "
        "and r.rdb$constraint_name = c.rdb$constraint_name "
        "and (r.rdb$constraint_type='FOREIGN KEY') order by 1, i.rdb$field_position"
    );

    IBPP::Statement& st2 = loader->getStatement(
        "select r.rdb$relation_name, i.rdb$field_name"
        " from rdb$relation_constraints r"
        " join rdb$index_segments i on i.rdb$index_name = r.rdb$index_name "
        " where r.rdb$constraint_name = ?"
        " order by i.rdb$field_position "
    );

    st1->Set(1, wx2std(getName_(), d->getCharsetConverter()));
    st1->Execute();
    ForeignKey *fkp = 0;
    while (st1->Fetch())
    {
        std::string s;
        st1->Get(1, s);
        wxString cname(std2wxIdentifier(s, d->getCharsetConverter()));
        st1->Get(2, s);
        wxString fname(std2wxIdentifier(s, d->getCharsetConverter()));
        st1->Get(3, s);
        wxString update_rule(std2wxIdentifier(s, d->getCharsetConverter()));
        st1->Get(4, s);
        wxString delete_rule(std2wxIdentifier(s, d->getCharsetConverter()));
        std::string ref_constraint;
        st1->Get(5, ref_constraint);
        st1->Get(6, s);
        wxString ixname(std2wxIdentifier(s, d->getCharsetConverter()));

        if (fkp && fkp->getName_() == cname) // add column
            fkp->columnsM.push_back(fname);
        else
        {
            ForeignKey fk;
            foreignKeysM.push_back(fk);
            fkp = &foreignKeysM.back();
            fkp->setName_(cname);
            fkp->setParent(this);
            fkp->updateActionM = update_rule;
            fkp->deleteActionM = delete_rule;
            fkp->indexNameM = ixname;

            st2->Set(1, ref_constraint);
            st2->Execute();
            std::string rtable;
            while (st2->Fetch())
            {
                st2->Get(1, rtable);
                st2->Get(2, s);
                fkp->referencedColumnsM.push_back(
                    std2wxIdentifier(s, d->getCharsetConverter()));
            }
            fkp->referencedTableM = std2wxIdentifier(rtable,
                d->getCharsetConverter());
            fkp->columnsM.push_back(fname);
        }
    }
    foreignKeysLoadedM = true;
}
//-----------------------------------------------------------------------------
//! reads indices from database
void Table::loadIndices()
{
    if (indicesLoadedM)
        return;
    indicesM.clear();

    Database* d = getDatabase(wxT("Table::loadIndices"));
    MetadataLoader* loader = d->getMetadataLoader();
    // first start a transaction for metadata loading, then lock the table
    // when objects go out of scope and are destroyed, table will be unlocked
    // before the transaction is committed - any update() calls on observers
    // can possibly use the same transaction
    MetadataLoaderTransaction tr(loader);
    SubjectLocker lock(this);

    IBPP::Statement& st1 = loader->getStatement(
        "SELECT i.rdb$index_name, i.rdb$unique_flag, i.rdb$index_inactive, "
        " i.rdb$index_type, i.rdb$statistics, "
        " s.rdb$field_name, rc.rdb$constraint_name, i.rdb$expression_source "
        " from rdb$indices i "
        " left join rdb$index_segments s on i.rdb$index_name = s.rdb$index_name "
        " left join rdb$relation_constraints rc "
        "   on rc.rdb$index_name = i.rdb$index_name "
        " where i.rdb$relation_name = ? "
        " order by i.rdb$index_name, s.rdb$field_position "
    );

    st1->Set(1, wx2std(getName_(), d->getCharsetConverter()));
    st1->Execute();
    Index* i = 0;
    while (st1->Fetch())
    {
        std::string s;
        st1->Get(1, s);
        wxString ixname(std2wxIdentifier(s, d->getCharsetConverter()));

        short unq, inactive, type;
        if (st1->IsNull(2))     // null = non-unique
            unq = 0;
        else
            st1->Get(2, unq);
        if (st1->IsNull(3))     // null = active
            inactive = 0;
        else
            st1->Get(3, inactive);
        if (st1->IsNull(4))     // null = ascending
            type = 0;
        else
            st1->Get(4, type);
        double statistics;
        if (st1->IsNull(5))     // this can happen, see bug #1825725
            statistics = -1;
        else
            st1->Get(5, statistics);

        st1->Get(6, s);
        wxString fname(std2wxIdentifier(s, d->getCharsetConverter()));
        wxString expression;
        readBlob(st1, 8, expression, d->getCharsetConverter());

        if (i && i->getName_() == ixname)
            i->getSegments()->push_back(fname);
        else
        {
            Index x(
                unq == 1,
                inactive == 0,
                type == 0,
                statistics,
                !st1->IsNull(7),
                expression
            );
            indicesM.push_back(x);
            i = &indicesM.back();
            i->setName_(ixname);
            i->getSegments()->push_back(fname);
            i->setParent(this);
        }
    }
    indicesLoadedM = true;
}
//-----------------------------------------------------------------------------
const wxString Table::getTypeName() const
{
    return wxT("TABLE");
}
//-----------------------------------------------------------------------------
bool Table::addRdbKeyToSelect()
{
    // add DB_KEY only when table doesn't have a PK/UNQ constraint
    return !getPrimaryKey() && getUniqueConstraints()->size() == 0;
}
//-----------------------------------------------------------------------------
// find all tables from "tables" which have foreign keys with "table"
// and return them in "list"
bool Table::tablesRelate(const std::vector<wxString>& tables, Table* table,
                          std::vector<ForeignKey>& list)
{
    // see if "table" references some of the "tables"
    std::vector<ForeignKey> *fks = table->getForeignKeys();
    for (std::vector<ForeignKey>::iterator it = fks->begin(); it != fks->end(); ++it)
    {
        ForeignKey& fk = (*it);
        for (std::vector<wxString>::const_iterator i2 = tables.begin(); i2 != tables.end(); ++i2)
            if ((*i2) == fk.referencedTableM)
                list.push_back(fk);
    }

    // see if some of the "tables" reference the "table"
    std::vector<Dependency> deplist;
    table->getDependencies(deplist, false);
    for (std::vector<Dependency>::iterator it = deplist.begin(); it != deplist.end(); ++it)
    {
        if ((*it).getType() == ntTable)
        {
            for (std::vector<wxString>::const_iterator i2 = tables.begin(); i2 != tables.end(); ++i2)
            {
                if ((*i2) == (*it).getName_())
                {
                    // find foreign keys for that table
                    Database* d = table->getDatabase(wxT("Table::tablesRelate"));
                    Table* other_table = dynamic_cast<Table*>(d->findByNameAndType(ntTable, (*i2)));
                    if (!other_table)
                        break;

                    std::vector<ForeignKey>* fks = other_table->getForeignKeys();
                    for (std::vector<ForeignKey>::iterator it = fks->begin(); it != fks->end(); ++it)
                    {
                        ForeignKey& fk = (*it);
                        if (table->getName_() == fk.referencedTableM)
                        {
                            list.push_back(fk);
                            break;  // no need for more
                        }
                    }
                }
            }
        }
    }

    return !list.empty();
}
//-----------------------------------------------------------------------------
void Table::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitTable(*this);
}
//-----------------------------------------------------------------------------
// System tables collection
SysTables::SysTables(DatabasePtr database)
    : MetadataCollection<Table>(ntSysTables, database, _("System tables"))
{
}
//-----------------------------------------------------------------------------
void SysTables::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitSysTables(*this);
}
//-----------------------------------------------------------------------------
bool SysTables::isSystem() const
{
    return true;
}
//-----------------------------------------------------------------------------
void SysTables::load(ProgressIndicator* progressIndicator)
{
    wxString stmt = wxT("select rdb$relation_name from rdb$relations")
        wxT(" where rdb$system_flag = 1")
        wxT(" and rdb$view_source is null order by 1");
    setItems(getDatabase()->loadIdentifiers(stmt, progressIndicator));
}
//-----------------------------------------------------------------------------
void SysTables::loadChildren()
{
    load(0);
}
//-----------------------------------------------------------------------------
// Tables collection
Tables::Tables(DatabasePtr database)
    : MetadataCollection<Table>(ntTables, database, _("Tables"))
{
}
//-----------------------------------------------------------------------------
void Tables::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitTables(*this);
}
//-----------------------------------------------------------------------------
void Tables::load(ProgressIndicator* progressIndicator)
{
    wxString stmt = wxT("select rdb$relation_name from rdb$relations")
        wxT(" where (rdb$system_flag = 0 or rdb$system_flag is null)")
        wxT(" and rdb$view_source is null order by 1");
    setItems(getDatabase()->loadIdentifiers(stmt, progressIndicator));
}
//-----------------------------------------------------------------------------
void Tables::loadChildren()
{
    load(0);
}
//-----------------------------------------------------------------------------
