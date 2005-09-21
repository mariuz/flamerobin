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

#include <ibpp.h>

#include "core/Visitor.h"
#include "database.h"
#include "dberror.h"
#include "frutils.h"
#include "MetadataItemVisitor.h"
#include "table.h"
#include "ugly.h"
//-----------------------------------------------------------------------------
Table::Table()
    : Relation()
{
    typeM = ntTable;
    primaryKeyLoadedM = false;
    foreignKeysLoadedM = false;
    checkConstraintsLoadedM = false;
    uniqueConstraintsLoadedM = false;
    indicesLoadedM = false;
}
//-----------------------------------------------------------------------------
void Table::invalidateIndices()
{
    if (indicesLoadedM)
    {
        indicesLoadedM = false;
        notifyObservers();
    }
}
//-----------------------------------------------------------------------------
bool Table::loadColumns()           // update the keys info too
{
    primaryKeyLoadedM = false;          // force info to be reloaded if asked
    foreignKeysLoadedM = false;
    checkConstraintsLoadedM = false;
    uniqueConstraintsLoadedM = false;
    indicesLoadedM = false;
    return Relation::loadColumns();
}
//-----------------------------------------------------------------------------
wxString Table::getInsertStatement()
{
    checkAndLoadColumns();
    wxString sql = wxT("INSERT INTO ") + getName() + wxT(" (");
    wxString collist, valist;
    for (MetadataCollection<Column>::const_iterator i = columnsM.begin(); i != columnsM.end(); ++i)
    {
        if ((*i).isComputed())
            continue;
        if (!collist.empty())
        {
            valist += wxT(", \n");
            collist += wxT(", ");
        }
        collist += (*i).getName();

        if (!(*i).isNullable())
            valist += wxT("*");
        valist += (*i).getName();
    }
    sql += collist + wxT(")\n VALUES (\n") + valist + wxT("\n)");
    return sql;
}
//-----------------------------------------------------------------------------
//! reads checks info from database
bool Table::loadCheckConstraints()
{
    if (checkConstraintsLoadedM)
        return true;

    checkConstraintsM.clear();
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
            "select r.rdb$constraint_name, t.rdb$trigger_source "
            " from rdb$relation_constraints r "
            " join rdb$check_constraints c on r.rdb$constraint_name=c.rdb$constraint_name and r.rdb$constraint_type = 'CHECK'"
            " join rdb$triggers t on c.rdb$trigger_name=t.rdb$trigger_name and t.rdb$trigger_type = 1 "
            " where r.rdb$relation_name=?"
        );

        st1->Set(1, wx2std(getName()));
        st1->Execute();
        while (st1->Fetch())
        {
            wxString source;
            std::string cname;
            st1->Get(1, cname);
            readBlob(st1, 2, source);
            cname.erase(cname.find_last_not_of(" ") + 1);
            source.erase(source.find_last_not_of(wxT(" ")) + 1);
            CheckConstraint c;
            c.setParent(this);
            c.setName(std2wx(cname));
            c.sourceM = source;
            checkConstraintsM.push_back(c);
        }
        tr1->Commit();
        checkConstraintsLoadedM = true;
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
//! reads primary key info from database
bool Table::loadPrimaryKey()
{
    if (primaryKeyLoadedM)
        return true;

    primaryKeyM.columnsM.clear();
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
            "select r.rdb$constraint_name, i.rdb$field_name "
            "from rdb$relation_constraints r, rdb$index_segments i "
            "where r.rdb$relation_name=? and r.rdb$index_name=i.rdb$index_name and "
            "(r.rdb$constraint_type='PRIMARY KEY') order by 1, 2"
        );

        st1->Set(1, wx2std(getName()));
        st1->Execute();
        while (st1->Fetch())
        {
            std::string name, cname;
            st1->Get(1, cname);
            st1->Get(2, name);

            name.erase(name.find_last_not_of(" ") + 1);
            cname.erase(cname.find_last_not_of(" ") + 1);
            primaryKeyM.setName(std2wx(cname));
            primaryKeyM.columnsM.push_back(std2wx(name));
        }
        tr1->Commit();
        primaryKeyM.setParent(this);
        primaryKeyLoadedM = true;
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
//! reads uniques from database
bool Table::loadUniqueConstraints()
{
    if (uniqueConstraintsLoadedM)
        return true;

    uniqueConstraintsM.clear();
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
            "select r.rdb$constraint_name, i.rdb$field_name "
            "from rdb$relation_constraints r, rdb$index_segments i "
            "where r.rdb$relation_name=? and r.rdb$index_name=i.rdb$index_name and "
            "(r.rdb$constraint_type='UNIQUE') order by 1, 2"
        );

        st1->Set(1, wx2std(getName()));
        st1->Execute();
        ColumnConstraint *cc = 0;
        while (st1->Fetch())
        {
            std::string name, cname;
            st1->Get(1, cname);
            st1->Get(2, name);
            name.erase(name.find_last_not_of(" ") + 1);
            cname.erase(cname.find_last_not_of(" ") + 1);

            if (cc && cc->getName() == std2wx(cname))
                cc->columnsM.push_back(std2wx(name));
            else
            {
                ColumnConstraint c;
                uniqueConstraintsM.push_back(c);
                cc = &uniqueConstraintsM.back();
                cc->setName(std2wx(cname));
                cc->columnsM.push_back(std2wx(name));
                cc->setParent(this);
            }
        }
        tr1->Commit();
        uniqueConstraintsLoadedM = true;
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
ColumnConstraint *Table::getPrimaryKey()
{
    if (!loadPrimaryKey() || primaryKeyM.columnsM.empty())  // no primary key on table
        return 0;
    return &primaryKeyM;
}
//-----------------------------------------------------------------------------
std::vector<ForeignKey> *Table::getForeignKeys()
{
    if (!loadForeignKeys())
        return 0;
    return &foreignKeysM;
}
//-----------------------------------------------------------------------------
std::vector<CheckConstraint> *Table::getCheckConstraints()
{
    if (!loadCheckConstraints())
        return 0;
    return &checkConstraintsM;
}
//-----------------------------------------------------------------------------
std::vector<ColumnConstraint> *Table::getUniqueConstraints()
{
    if (!loadUniqueConstraints())
        return 0;
    return &uniqueConstraintsM;
}
//-----------------------------------------------------------------------------
std::vector<Index> *Table::getIndices()
{
    if (!loadIndices())
        return 0;
    return &indicesM;
}
//-----------------------------------------------------------------------------
//! reads foreign keys info from database
bool Table::loadForeignKeys()
{
    if (foreignKeysLoadedM)
        return true;

    foreignKeysM.clear();
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
        IBPP::Statement st2 = IBPP::StatementFactory(db, tr1);
        st1->Prepare(
            "select r.rdb$constraint_name, i.rdb$field_name, c.rdb$update_rule, c.rdb$delete_rule, c.RDB$CONST_NAME_UQ "
            "from rdb$relation_constraints r, rdb$index_segments i, rdb$ref_constraints c "
            "where r.rdb$relation_name=? and r.rdb$index_name=i.rdb$index_name  "
            "and r.rdb$constraint_name = c.rdb$constraint_name "
            "and (r.rdb$constraint_type='FOREIGN KEY') order by 1, i.rdb$field_position"
        );

        st2->Prepare(
            "select r.rdb$relation_name, i.rdb$field_name"
            " from rdb$relation_constraints r"
            " join rdb$index_segments i on i.rdb$index_name = r.rdb$index_name "
            " where r.rdb$constraint_name = ?"
            " order by i.rdb$field_position "
        );

        st1->Set(1, wx2std(getName()));
        st1->Execute();
        ForeignKey *fkp = 0;
        while (st1->Fetch())
        {
            std::string name, cname, update_rule, delete_rule, ref_constraint;
            st1->Get(1, cname);
            st1->Get(2, name);
            st1->Get(3, update_rule);
            st1->Get(4, delete_rule);
            st1->Get(5, ref_constraint);

            name.erase(name.find_last_not_of(" ") + 1);
            cname.erase(cname.find_last_not_of(" ") + 1);
            ref_constraint.erase(ref_constraint.find_last_not_of(" ") + 1);

            if (fkp && fkp->getName() == std2wx(cname)) // add column
                fkp->columnsM.push_back(std2wx(name));
            else
            {
                ForeignKey fk;
                foreignKeysM.push_back(fk);
                fkp = &foreignKeysM.back();
                fkp->setName(std2wx(cname));
                fkp->setParent(this);
                fkp->updateActionM = std2wx(update_rule);
                fkp->deleteActionM = std2wx(delete_rule);

                st2->Set(1, ref_constraint);
                st2->Execute();
                std::string rtable, rcolumn;
                while (st2->Fetch())
                {
                    st2->Get(1, rtable);
                    st2->Get(2, rcolumn);
                    rcolumn.erase(rcolumn.find_last_not_of(" ") + 1);
                    fkp->referencedColumnsM.push_back(std2wx(rcolumn));
                }
                rtable.erase(rtable.find_last_not_of(" ") + 1);
                fkp->referencedTableM = std2wx(rtable);
                fkp->columnsM.push_back(std2wx(name));
            }
        }
        tr1->Commit();
        foreignKeysLoadedM = true;
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
//! reads indices from database
bool Table::loadIndices()
{
    if (indicesLoadedM)
        return true;

    indicesM.clear();
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
            "SELECT i.rdb$index_name, i.rdb$unique_flag, i.rdb$index_inactive, "
            " i.rdb$index_type, i.rdb$statistics, s.rdb$field_name"
            " from rdb$indices i"
            " join rdb$index_segments s on i.rdb$index_name = s.rdb$index_name"
            " where i.rdb$relation_name = ?"
            " order by i.rdb$index_id, s.rdb$field_position "
        );

        st1->Set(1, wx2std(getName()));
        st1->Execute();
        Index* i = 0;
        while (st1->Fetch())
        {
            std::string name, fname;
            short unq, inactive, type;
            double statistics;
            st1->Get(1, name);
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
            st1->Get(5, statistics);
            st1->Get(6, fname);
            name.erase(name.find_last_not_of(" ") + 1);
            fname.erase(fname.find_last_not_of(" ") + 1);

            if (i && i->getName() == std2wx(name))
                i->getSegments()->push_back(std2wx(fname));
            else
            {
                Index x(
                    unq == 1,
                    inactive == 0,
                    type == 0,
                    statistics
                );
                indicesM.push_back(x);
                i = &indicesM.back();
                i->setName(std2wx(name));
                i->getSegments()->push_back(std2wx(fname));
                i->setParent(this);
            }
        }
        tr1->Commit();
        indicesLoadedM = true;
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
wxString Table::getCreateSqlTemplate() const
{
    return  wxT("CREATE TABLE table_name\n")
            wxT("(\n")
            wxT("    column_name {< datatype> | COMPUTED BY (< expr>) | domain}\n")
            wxT("        [DEFAULT { literal | NULL | USER}] [NOT NULL]\n")
            wxT("    ...\n")
            wxT("    CONSTRAINT constraint_name\n")
            wxT("        PRIMARY KEY (column_list),\n")
            wxT("        UNIQUE      (column_list),\n")
            wxT("        FOREIGN KEY (column_list) REFERENCES other_table (column_list),\n")
            wxT("        CHECK       (condition),\n")
            wxT("    ...\n")
            wxT(");\n");
}
//-----------------------------------------------------------------------------
const wxString Table::getTypeName() const
{
    return wxT("TABLE");
}
//-----------------------------------------------------------------------------
// find all tables from "tables" which have foreign keys with "table"
// and return them in "list"
bool Table::tablesRelate(std::vector<wxString>& tables, Table* table, std::vector<Join>& list)
{
    // see if "table" references some of the "tables"
    std::vector<ForeignKey> *fks = table->getForeignKeys();
    for (std::vector<ForeignKey>::iterator it = fks->begin(); it != fks->end(); ++it)
    {
        ForeignKey& fk = (*it);
        for (std::vector<wxString>::iterator i2 = tables.begin(); i2 != tables.end(); ++i2)
        {
            if ((*i2) == fk.referencedTableM)
            {
                wxString join;
                for (unsigned int i = 0; i < fk.referencedColumnsM.size(); ++i)
                {
                    if (i > 0)
                        join += wxT(" AND ");
                    join += fk.referencedTableM + wxT(".") + fk.referencedColumnsM[i] +
                        wxT(" = ") + table->getName() + wxT(".") + fk.columnsM[i];
                }
                list.push_back(Join(fk.referencedTableM, join));

            }
        }
    }

    // see if some of the "tables" reference the "table"
    std::vector<Dependency> deplist;
    table->getDependencies(deplist, false);
    for (std::vector<Dependency>::iterator it = deplist.begin(); it != deplist.end(); ++it)
    {
        if ((*it).getType() == ntTable)
        {
            for (std::vector<wxString>::iterator i2 = tables.begin(); i2 != tables.end(); ++i2)
            {
                if ((*i2) == (*it).getName())
                {
                    // find foreign keys for that table
                    Database* d = table->getDatabase();
                    Table* other_table = dynamic_cast<Table*>(d->findByNameAndType(ntTable, (*i2)));
                    if (!other_table)
                        break;

                    std::vector<ForeignKey>* fks = other_table->getForeignKeys();
                    for (std::vector<ForeignKey>::iterator it = fks->begin(); it != fks->end(); ++it)
                    {
                        ForeignKey& fk = (*it);
                        if (table->getName() == fk.referencedTableM)
                        {
                            wxString join;
                            for (unsigned int i = 0; i < fk.referencedColumnsM.size(); ++i)
                            {
                                if (i > 0)
                                    join += wxT(" AND ");
                                join += fk.referencedTableM + wxT(".") + fk.referencedColumnsM[i] +
                                    wxT(" = ") + (*i2) + wxT(".") + fk.columnsM[i];
                            }
                            list.push_back(Join(fk.referencedTableM, join));
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
    visitor->visit(*this);
}
//-----------------------------------------------------------------------------
