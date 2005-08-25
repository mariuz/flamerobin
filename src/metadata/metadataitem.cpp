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

  Contributor(s): Nando Dessena
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

//------------------------------------------------------------------------------
#include <sstream>

#include "config/Config.h"
#include "database.h"
#include "dberror.h"
#include "frutils.h"
#include "metadataitem.h"
#include "visitor.h"
//------------------------------------------------------------------------------
using namespace std;
//------------------------------------------------------------------------------
const string MetadataItem::pathSeparator = "/";
//------------------------------------------------------------------------------
MetadataItem::MetadataItem()
	: Item()
{
	parentM = 0;
	typeM = ntUnknown;
	descriptionLoadedM = false;
}
//------------------------------------------------------------------------------
const string MetadataItem::getTypeName() const
{
	return "";
}
//------------------------------------------------------------------------------
const string MetadataItem::getItemPath() const
{
	string result = getTypeName() + "_" + getPathId();
 	if (parentM)
	{
		string parentItemPath = parentM->getItemPath();
		if (parentItemPath != "")
			result = parentItemPath + pathSeparator + result;
	}
	return result;
}
//------------------------------------------------------------------------------
const string MetadataItem::getPathId() const
{
	return getId();
}
//------------------------------------------------------------------------------
const string MetadataItem::getId() const
{
	return getName();
}
//------------------------------------------------------------------------------
NodeType getTypeByName(string name)
{
	if (name == "TABLE")
		return ntTable;
	else if (name == "VIEW")
		return ntView;
	else if (name == "PROCEDURE")
		return ntProcedure;
	else if (name == "TRIGGER")
		return ntTrigger;
	else if (name == "GENERATOR")
		return ntGenerator;
	else if (name == "FUNCTION")
		return ntFunction;
	else if (name == "DOMAIN")
		return ntDomain;
	else if (name == "ROLE")
		return ntRole;
	else if (name == "COLUMN")
		return ntColumn;
    else if (name == "EXCEPTION")
        return ntException;
	else
		return ntUnknown;
}
//------------------------------------------------------------------------------
bool MetadataItem::getChildren(vector<MetadataItem*>& /*temp*/)
{
	return false;
}
//------------------------------------------------------------------------------
//! removes its children (by calling drop() for each) and notifies it's parent
void MetadataItem::drop()
{
	vector<MetadataItem *>temp;
	if (getChildren(temp))
		for (vector<MetadataItem *>::iterator it = temp.begin(); it != temp.end(); ++it)
			(*it)->drop();

	// TODO: prehaps the whole DBH needs to be reconsidered
	// we could write: if (parentM) parentM->remove(this);
	// but we can't, since parent might not be a collection!
	// ie. currently it is a Database object
}
//------------------------------------------------------------------------------
Database *MetadataItem::getDatabase() const
{
	MetadataItem *m = const_cast<MetadataItem *>(this);
	while (m && m->getType() != ntDatabase)
		m = m->getParent();
	return (Database *)m;
}
//------------------------------------------------------------------------------
Root* MetadataItem::getRoot() const
{
	MetadataItem* m = const_cast<MetadataItem*>(this);
	while (m->getParent())
		m = m->getParent();
	return (Root*)m;
}
//------------------------------------------------------------------------------
//! virtual so it can eventually be delegated to Table, View, etc.
string MetadataItem::getDescriptionSql() const
{
	switch (typeM)
	{
		case ntView:
		case ntTable:		return "select rdb$description from rdb$relations where RDB$RELATION_NAME=?";
		case ntProcedure:	return "select rdb$description from rdb$procedures where RDB$procedure_NAME=?";
		case ntTrigger:		return "select rdb$description from rdb$triggers where RDB$trigger_NAME=?";
		case ntFunction:	return "select rdb$description from RDB$FUNCTIONS where RDB$FUNCTION_NAME=?";
		case ntColumn:		return "select rdb$description from rdb$relation_fields where rdb$field_name=? and rdb$relation_name=?";
		case ntParameter:	return "select rdb$description from rdb$procedure_parameters where rdb$parameter_name=? and rdb$procedure_name=?";
		case ntDomain:		return "select rdb$description from rdb$fields where rdb$field_name=?";
        case ntException:   return "select RDB$DESCRIPTION from RDB$EXCEPTIONS where RDB$EXCEPTION_NAME = ?";
		case ntIndex:		return "select rdb$description from rdb$indices where RDB$INDEX_NAME = ?";
        default:			return "";
	};
}
//------------------------------------------------------------------------------
//! virtual so it can eventually be delegated to Table, View, etc.
string MetadataItem::getChangeDescriptionSql() const
{
	switch (typeM)
	{
		case ntView:
		case ntTable:		return "update rdb$relations set rdb$description = ? where RDB$RELATION_NAME = ?";
		case ntProcedure:	return "update rdb$procedures set rdb$description = ? where RDB$PROCEDURE_name=?";
		case ntTrigger:		return "update rdb$triggers set rdb$description = ? where RDB$trigger_NAME=?";
		case ntFunction:	return "update RDB$FUNCTIONS set rdb$description = ? where RDB$FUNCTION_NAME=?";
		case ntColumn:		return "update rdb$relation_fields set rdb$description = ? where rdb$field_name=? and rdb$relation_name=?";
		case ntParameter:	return "update rdb$procedure_parameters set rdb$description = ? where rdb$parameter_name = ? and rdb$procedure_name=?";
		case ntDomain:		return "update rdb$fields set rdb$description = ? where rdb$field_name=?";
        case ntException:   return "update RDB$EXCEPTIONS set RDB$DESCRIPTION = ? where RDB$EXCEPTION_NAME = ?";
        case ntIndex:		return "update rdb$indices set rdb$description = ? where RDB$INDEX_NAME = ?";
		default:			return "";
	};
}
//------------------------------------------------------------------------------
//! ofObject = true   => returns list of objects this object depends on
//! ofObject = false  => returns list of objects that depend on this object
bool MetadataItem::getDependencies(vector<Dependency>& list, bool ofObject)
{
	Database *d = getDatabase();
	if (!d)
	{
		lastError().setMessage("Database not set");
		return false;
	}

	int mytype = -1;			// map DBH type to RDB$DEPENDENT TYPE
	NodeType dep_types[] = { 	ntTable, 	ntView, 	ntTrigger, 	ntUnknown,	ntUnknown,
								ntProcedure,ntUnknown, 	ntException,ntUnknown,	ntUnknown,
								ntUnknown,	ntUnknown,	ntUnknown,	ntUnknown,  ntGenerator,
								ntFunction
	};
	int type_count = sizeof(dep_types)/sizeof(NodeType);
	for (int i=0; i<type_count; i++)
		if (typeM == dep_types[i])
			mytype = i;
	if (typeM == ntUnknown || mytype == -1)
	{
		lastError().setMessage("Unsupported type");
		return false;
	}
	if (mytype == 1 && !ofObject)	// views count as relations(tables) when other object refer to them
		mytype = 0;

	try
	{
		IBPP::Database& db = d->getIBPPDatabase();
		IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
		tr1->Start();
		IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);

		string o1 = (ofObject ? "DEPENDENT" : "DEPENDED_ON");
		string o2 = (ofObject ? "DEPENDED_ON" : "DEPENDENT");
		string sql =
			"select RDB$" + o2 + "_TYPE, RDB$" + o2 + "_NAME, RDB$FIELD_NAME \n "
			" from RDB$DEPENDENCIES \n "
			" where RDB$" + o1 + "_TYPE = ? and RDB$" + o1 + "_NAME = ? \n ";
		if ((typeM == ntTable || typeM == ntView) && ofObject)	// get deps for computed columns
		{														// view needed to bind with generators
			sql += " union all \n"
				" SELECT DISTINCT d.rdb$depended_on_type, d.rdb$depended_on_name, d.rdb$field_name \n"
				" FROM rdb$relation_fields f \n"
				" LEFT JOIN rdb$dependencies d ON d.rdb$dependent_name = f.rdb$field_source \n"
				" WHERE d.rdb$dependent_type = 3 AND f.rdb$relation_name = ? \n";
		}
		if (!ofObject)						// find tables that have calculated columns based on "this" object
		{
			sql += "union all \n"
				" SELECT distinct cast(0 as smallint), f.rdb$relation_name, d.rdb$field_name \n"
				" from rdb$relation_fields f \n"
				" left join rdb$dependencies d on d.rdb$dependent_name = f.rdb$field_source \n"
				" where d.rdb$dependent_type = 3 and d.rdb$depended_on_name = ? ";
		}
		sql += " order by 1, 2, 3";
		st1->Prepare(sql);
		st1->Set(1, mytype);
		st1->Set(2, nameM);
		if (!ofObject || typeM == ntTable || typeM == ntView)
			st1->Set(3, nameM);
		st1->Execute();
		MetadataItem *last = 0;
		Dependency *dep = 0;
		while (st1->Fetch())
		{
			string object_name, field_name;
			int object_type;
			st1->Get(1, &object_type);
			st1->Get(2, object_name);
			object_name.erase(object_name.find_last_not_of(" ")+1);		// trim

			if (object_type > type_count)	// some system object, not interesting for us
				continue;
			NodeType t = dep_types[object_type];
			if (t == ntUnknown)				// ditto
				continue;
			MetadataItem *current = d->findByNameAndType(t, object_name);
			if (!current)
			{								// maybe it's a view masked as table
				if (t == ntTable)
					current = d->findByNameAndType(ntView, object_name);
				if (!current)
					continue;
			}
			if (current != last)			// new object
			{
				Dependency de(current);
				list.push_back(de);
				dep = &list.back();
				last = current;
			}
			if (!st1->IsNull(3))
			{
				st1->Get(3, field_name);
				field_name.erase(field_name.find_last_not_of(" ")+1);		// trim
				dep->addField(field_name);
			}
		}

		// TODO: perhaps this could be moved to Table?
		//       call MetadataItem::getDependencies() and then add this
		if (typeM == ntTable && ofObject)	// foreign keys of this table + computed columns
		{
			Table *t = dynamic_cast<Table *>(this);
			vector<ForeignKey> *f = t->getForeignKeys();
			for (vector<ForeignKey>::const_iterator it = f->begin(); it != f->end(); ++it)
			{
				MetadataItem *table = d->findByNameAndType(ntTable, (*it).referencedTableM);
				if (!table)
				{
					lastError().setMessage("Table " + (*it).referencedTableM + " not found.");
					return false;
				}
				Dependency de(table);
				de.setFields((*it).getReferencedColumnList());
				list.push_back(de);
			}
		}

		// TODO: perhaps this could be moved to Table?
		if (typeM == ntTable && !ofObject)	// foreign keys of other tables
		{
			st1->Prepare(
				"select r1.rdb$relation_name, i.rdb$field_name "
				" from rdb$relation_constraints r1 "
				" join rdb$ref_constraints c on r1.rdb$constraint_name = c.rdb$constraint_name "
				" join rdb$relation_constraints r2 on c.RDB$CONST_NAME_UQ = r2.rdb$constraint_name "
				" join rdb$index_segments i on r1.rdb$index_name=i.rdb$index_name "
				" where r2.rdb$relation_name=? "
				" and r1.rdb$constraint_type='FOREIGN KEY' "
			);
			st1->Set(1, nameM);
			st1->Execute();
			string lasttable;
			Dependency *dep = 0;
			while (st1->Fetch())
			{
				string table_name, field_name;
				st1->Get(1, table_name);
				st1->Get(2, field_name);
				table_name.erase(table_name.find_last_not_of(" ")+1);		// trim
				field_name.erase(field_name.find_last_not_of(" ")+1);		// trim
				if (table_name != lasttable)	// new
				{
					MetadataItem *table = d->findByNameAndType(ntTable, table_name);
					if (!table)
						continue;			// dummy check
					Dependency de(table);
					list.push_back(de);
					dep = &list.back();
					lasttable = table_name;
				}
				dep->addField(field_name);
			}
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
string MetadataItem::getDescription()
{
	if (descriptionLoadedM)
		return descriptionM;

	Database *d = getDatabase();
	string sql = getDescriptionSql();
	if (!d || sql == "")
		return "N/A";

	descriptionM = "";
	try
	{
		IBPP::Database& db = d->getIBPPDatabase();
		IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
		tr1->Start();
		IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
		st1->Prepare(sql);
		st1->Set(1, nameM);
		if (typeM == ntColumn || typeM == ntParameter)
			st1->Set(2, getParent()->getName());	// table/view/SP name
		st1->Execute();
		st1->Fetch();
		descriptionLoadedM = true;
        if (st1->IsNull(1))
            return "";
		/*
		IBPP::Blob b = IBPP::BlobFactory(st1->Database(), st1->Transaction());
		st1->Get(1, b);
		b->Open();
		char buffer[8192];		// 8K block
        while (true)
		{
			int size = b->Read(buffer, 8192);
			if (size <= 0)
				break;
			buffer[size] = 0;
			descriptionM += buffer;
		}
		b->Close();
		*/

		readBlob(st1, 1, descriptionM);

		tr1->Commit();
		descriptionLoadedM = true;
		return descriptionM;
	}
	catch (IBPP::Exception &e)
	{
		lastError().setMessage(e.ErrorMessage());
	}
	catch (...)
	{
		lastError().setMessage("System error.");
	}

	return lastError().getMessage();
}
//------------------------------------------------------------------------------
bool MetadataItem::setDescription(string description)
{
	Database *d = getDatabase();
	if (!d)
		return false;

	string sql = getChangeDescriptionSql();
	if (sql == "")
	{
		lastError().setMessage("The object does not support descriptions");
		return false;
	}

	try
	{
		IBPP::Database& db = d->getIBPPDatabase();
		IBPP::Transaction tr1 = IBPP::TransactionFactory(db);
		tr1->Start();
		IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
		st1->Prepare(sql);

		if (!description.empty())
		{
            IBPP::Blob b = IBPP::BlobFactory(st1->Database(), st1->Transaction());
            b->Create();
    		b->Write(description.c_str(), description.length());
    		b->Close();
    		st1->Set(1, b);
        }
        else
            st1->SetNull(1);
		st1->Set(2, nameM);
		if (typeM == ntColumn || typeM == ntParameter)
			st1->Set(3, getParent()->getName());
		st1->Execute();
		tr1->Commit();
		descriptionLoadedM = true;
		descriptionM = description;
		notify();
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
MetadataItem *MetadataItem::getParent() const
{
	return parentM;
}
//------------------------------------------------------------------------------
void MetadataItem::setParent(MetadataItem *parent)
{
	parentM = parent;
}
//------------------------------------------------------------------------------
string MetadataItem::getPrintableName()
{
	size_t n = getChildrenCount();
 	if (n)
	{
		ostringstream ss;
		ss << nameM << " (" << n << ")";
		return ss.str();
	}
	else
		return nameM;
}
//------------------------------------------------------------------------------
const string& MetadataItem::getName() const
{
	return nameM;
}
//------------------------------------------------------------------------------
void MetadataItem::setName(string name)
{
	name.erase(name.find_last_not_of(' ')+1);		// right trim
	nameM = name;
	notify();
}
//------------------------------------------------------------------------------
NodeType MetadataItem::getType() const
{
	return typeM;
}
//------------------------------------------------------------------------------
void MetadataItem::setType(NodeType type)
{
	typeM = type;
}
//------------------------------------------------------------------------------
bool MetadataItem::isSystem() const
{
	return getName().substr(0, 4) == "RDB$";
}
//------------------------------------------------------------------------------
string MetadataItem::getDropSqlStatement() const
{
    return "DROP " + getTypeName() + " " + getName() + ";";
}
//------------------------------------------------------------------------------
void MetadataItem::accept(Visitor *v)
{
	v->visit(*this);
}
//------------------------------------------------------------------------------
