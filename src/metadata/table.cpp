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

  Contributor(s):
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include <ibpp.h>
#include "dberror.h"
#include "database.h"
#include "table.h"
#include "collection.h"
#include "metadataitemwithcolumns.h"
//------------------------------------------------------------------------------
YTable::YTable()
	:YxMetadataItemWithColumns()
{
	typeM = ntTable;
	primaryKeyLoadedM = false;
	foreignKeysLoadedM = false;
	checkConstraintsLoadedM = false;
	uniqueConstraintsLoadedM = false;
}
//------------------------------------------------------------------------------
bool YTable::loadColumns()			// update the keys info too
{
	primaryKeyLoadedM = false;			// force info to be reloaded if asked
	foreignKeysLoadedM = false;
	checkConstraintsLoadedM = false;
	uniqueConstraintsLoadedM = false;
	return YxMetadataItemWithColumns::loadColumns();
}
//------------------------------------------------------------------------------
std::string YTable::getInsertStatement()
{
	checkAndLoadColumns();
	std::string sql = "INSERT INTO " + nameM + " (";
	std::string collist, valist;
	for (YMetadataCollection<YColumn>::const_iterator i = columnsM.begin(); i != columnsM.end(); ++i)
	{
		if (!collist.empty())
		{
			valist += ", \n";
			collist += ", ";
		}
		collist += (*i).getName();

		if (!(*i).isNullable())
			valist += "*";
		valist += (*i).getName();
	}
	sql += collist + ")\n VALUES (\n" + valist + "\n)";
	return sql;
}
//------------------------------------------------------------------------------
//! reads checks info from database
bool YTable::loadCheckConstraints()
{
	if (checkConstraintsLoadedM)
		return true;

	checkConstraintsM.clear();
	YDatabase *d = getDatabase();
	if (!d)
	{
		lastError().setMessage("database not set");
		return false;
	}

	IBPP::Database& db = d->getDatabase();
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

		st1->Set(1, getName());
		st1->Execute();
		IBPP::Blob b = IBPP::BlobFactory(st1->Database(), st1->Transaction());
		while (st1->Fetch())
		{
			std::string cname, source;
			st1->Get(1, cname);
			source = "";
			if (!st1->IsNull(2))
			{
				st1->Get(2, b);
				b->Open();
				std::string desc;
				char buffer[8192];		// 8K block
				while (true)
				{
					int size = b->Read(buffer, 8192);
					if (size <= 0)
						break;
					buffer[size] = 0;
					source += buffer;
				}
				b->Close();
			}

			cname.erase(cname.find_last_not_of(" ") + 1);
			source.erase(source.find_last_not_of(" ") + 1);
			CheckConstraint c;
			c.setParent(this);
			c.setName(cname);
			c.sourceM = source;
			checkConstraintsM.push_back(c);
		}
		tr1->Commit();
		checkConstraintsLoadedM = true;
		return true;
	}
	catch (IBPP::Exception &e)
	{
		lastError().setMessage(e.ErrorMessage());
	}
	catch (...)
	{
		lastError().setMessage("System error.");
	}
	return false;
}
//------------------------------------------------------------------------------
//! reads primary key info from database
bool YTable::loadPrimaryKey()
{
	if (primaryKeyLoadedM)
		return true;

	primaryKeyM.columnsM.clear();
	YDatabase *d = getDatabase();
	if (!d)
	{
		lastError().setMessage("database not set");
		return false;
	}

	IBPP::Database& db = d->getDatabase();
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

		st1->Set(1, getName());
		st1->Execute();
		while (st1->Fetch())
		{
			std::string name, cname;
			st1->Get(1, cname);
			st1->Get(2, name);

			name.erase(name.find_last_not_of(" ") + 1);
			cname.erase(cname.find_last_not_of(" ") + 1);
			primaryKeyM.setName(cname);
			primaryKeyM.columnsM.push_back(name);
		}
		tr1->Commit();
		primaryKeyM.setParent(this);
		primaryKeyLoadedM = true;
		return true;
	}
	catch (IBPP::Exception &e)
	{
		lastError().setMessage(e.ErrorMessage());
	}
	catch (...)
	{
		lastError().setMessage("System error.");
	}
	return false;
}
//------------------------------------------------------------------------------
//! reads uniques from database
bool YTable::loadUniqueConstraints()
{
	if (uniqueConstraintsLoadedM)
		return true;

	uniqueConstraintsM.clear();
	YDatabase *d = getDatabase();
	if (!d)
	{
		lastError().setMessage("database not set");
		return false;
	}

	IBPP::Database& db = d->getDatabase();
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

		st1->Set(1, getName());
		st1->Execute();
		ColumnConstraint *cc = 0;
		while (st1->Fetch())
		{
			std::string name, cname;
			st1->Get(1, cname);
			st1->Get(2, name);
			name.erase(name.find_last_not_of(" ") + 1);
			cname.erase(cname.find_last_not_of(" ") + 1);

			if (cc && cc->getName() == cname)
				cc->columnsM.push_back(name);
			else
			{
				ColumnConstraint c;
				uniqueConstraintsM.push_back(c);
				cc = &uniqueConstraintsM.back();
				cc->setName(cname);
				cc->columnsM.push_back(name);
				cc->setParent(this);
			}
		}
		tr1->Commit();
		uniqueConstraintsLoadedM = true;
		return true;
	}
	catch (IBPP::Exception &e)
	{
		lastError().setMessage(e.ErrorMessage());
	}
	catch (...)
	{
		lastError().setMessage("System error.");
	}
	return false;
}
//------------------------------------------------------------------------------
ColumnConstraint *YTable::getPrimaryKey()
{
	if (!loadPrimaryKey() || primaryKeyM.columnsM.empty())	// no primary key on table
		return 0;
	return &primaryKeyM;
}
//------------------------------------------------------------------------------
std::vector<ForeignKey> *YTable::getForeignKeys()
{
	if (!loadForeignKeys())
		return 0;
	return &foreignKeysM;
}
//------------------------------------------------------------------------------
std::vector<CheckConstraint> *YTable::getCheckConstraints()
{
	if (!loadCheckConstraints())
		return 0;
	return &checkConstraintsM;
}
//------------------------------------------------------------------------------
std::vector<ColumnConstraint> *YTable::getUniqueConstraints()
{
	if (!loadUniqueConstraints())
		return 0;
	return &uniqueConstraintsM;
}
//------------------------------------------------------------------------------
//! reads foreign keys info from database
bool YTable::loadForeignKeys()
{
	if (foreignKeysLoadedM)
		return true;

	foreignKeysM.clear();
	YDatabase *d = getDatabase();
	if (!d)
	{
		lastError().setMessage("database not set");
		return false;
	}

	IBPP::Database& db = d->getDatabase();
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

		st1->Set(1, getName());
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

			if (fkp && fkp->getName() == cname)	// add column
				fkp->columnsM.push_back(name);
			else
			{
				ForeignKey fk;
				foreignKeysM.push_back(fk);
				fkp = &foreignKeysM.back();
				fkp->setName(cname);
				fkp->setParent(this);
				fkp->updateActionM = update_rule;
				fkp->deleteActionM = delete_rule;

				st2->Set(1, ref_constraint);
				st2->Execute();
				std::string rtable, rcolumn;
				while (st2->Fetch())
				{
					st2->Get(1, rtable);
					st2->Get(2, rcolumn);
					rcolumn.erase(rcolumn.find_last_not_of(" ") + 1);
					fkp->referencedColumnsM.push_back(rcolumn);
				}
				rtable.erase(rtable.find_last_not_of(" ") + 1);
				fkp->referencedTableM = rtable;
				fkp->columnsM.push_back(name);
			}
		}
		tr1->Commit();
		foreignKeysLoadedM = true;
		return true;
	}
	catch (IBPP::Exception &e)
	{
		lastError().setMessage(e.ErrorMessage());
	}
	catch (...)
	{
		lastError().setMessage("System error.");
	}
	return false;
}
//------------------------------------------------------------------------------
//! load list of triggers for table
//! link them to triggers in database's collection
bool YTable::getTriggers(std::vector<YTrigger *>& list, YTrigger::firingTimeType beforeOrAfter)
{
	YDatabase *d = getDatabase();
	if (!d)
	{
		lastError().setMessage("database not set");
		return false;
	}

	IBPP::Database& db = d->getDatabase();
	try
	{
		IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
		tr1->Start();
		IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
		st1->Prepare(
			"select rdb$trigger_name from rdb$triggers where rdb$relation_name = ? "
			"order by rdb$trigger_sequence"
		);
		st1->Set(1, getName());
		st1->Execute();
		while (st1->Fetch())
		{
			std::string name;
			st1->Get(1, name);
			name.erase(name.find_last_not_of(" ") + 1);
			YTrigger *t = dynamic_cast<YTrigger *>(d->findByNameAndType(ntTrigger, name));
			if (t && t->getFiringTime() == beforeOrAfter)
				list.push_back(t);
		}
		tr1->Commit();
		return true;
	}
	catch (IBPP::Exception &e)
	{
		lastError().setMessage(e.ErrorMessage());
	}
	catch (...)
	{
		lastError().setMessage("System error.");
	}
	return false;
}
//------------------------------------------------------------------------------
std::string YTable::getCreateSqlTemplate() const
{
	return	"CREATE TABLE table_name\n"
			"(\n"
			"    column_name {< datatype> | COMPUTED BY (< expr>) | domain}\n"
			"        [DEFAULT { literal | NULL | USER}] [NOT NULL]\n"
			"    ...\n"
			"    CONSTRAINT constraint_name\n"
			"        PRIMARY KEY (column_list),\n"
			"        UNIQUE      (column_list),\n"
			"        FOREIGN KEY (column_list) REFERENCES other_table (column_list),\n"
			"        CHECK       (condition),\n"
			"    ...\n"
			");\n";
}
//------------------------------------------------------------------------------
const std::string YTable::getTypeName() const
{
	return "TABLE";
}
//-----------------------------------------------------------------------------
// find all tables from "tables" which have foreign keys with "table"
// and return them in "list"
bool YTable::tablesRelate(std::vector<std::string>& tables, YTable *table, std::vector<Join>& list)
{
	// see if "table" references some of the "tables"
	std::vector<ForeignKey> *fks = table->getForeignKeys();
	for (std::vector<ForeignKey>::iterator it = fks->begin(); it != fks->end(); ++it)
	{
		ForeignKey& fk = (*it);
		for (std::vector<std::string>::iterator i2 = tables.begin(); i2 != tables.end(); ++i2)
		{
			if ((*i2) == fk.referencedTableM)
			{
				std::string join;
				for (unsigned int i=0; i < fk.referencedColumnsM.size(); ++i)
				{
					if (i > 0)
						join += " AND ";
					join += fk.referencedTableM + "." + fk.referencedColumnsM[i] + " = " +
						table->getName() + "." + fk.columnsM[i];
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
			for (std::vector<std::string>::iterator i2 = tables.begin(); i2 != tables.end(); ++i2)
			{
				if ((*i2) == (*it).getName())
				{
					// find foreign keys for that table
					YDatabase *d = table->getDatabase();
					YTable *other_table = dynamic_cast<YTable *>(d->findByNameAndType(ntTable, (*i2)));
					if (!other_table)
						break;

					std::vector<ForeignKey> *fks = other_table->getForeignKeys();
					for (std::vector<ForeignKey>::iterator it = fks->begin(); it != fks->end(); ++it)
					{
						ForeignKey& fk = (*it);
						if (table->getName() == fk.referencedTableM)
						{
							std::string join;
							for (unsigned int i=0; i < fk.referencedColumnsM.size(); ++i)
							{
								if (i > 0)
									join += " AND ";
								join += fk.referencedTableM + "." + fk.referencedColumnsM[i] + " = " +
									(*i2) + "." + fk.columnsM[i];
							}
							list.push_back(Join(fk.referencedTableM, join));
							break;	// no need for more
						}
					}
				}
			}
		}
	}

	return !list.empty();
}
//-----------------------------------------------------------------------------
