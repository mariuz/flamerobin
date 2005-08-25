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

#include <sstream>

#include "config/Config.h"
#include "database.h"
#include "dberror.h"
#include "parameter.h"
#include "root.h"
#include "simpleparser.h"
#include "ugly.h"
#include "visitor.h"
//------------------------------------------------------------------------------
using namespace std;
//------------------------------------------------------------------------------
void Credentials::setCharset(std::string value)
{
	charsetM = value;
}
//------------------------------------------------------------------------------
void Credentials::setUsername(std::string value)
{
	usernameM = value;
}
//------------------------------------------------------------------------------
void Credentials::setPassword(std::string value)
{
	passwordM = value;
}
//------------------------------------------------------------------------------
void Credentials::setRole(std::string value)
{
	roleM = value;
}
//------------------------------------------------------------------------------
std::string Credentials::getCharset() const
{
	return charsetM;
}
//------------------------------------------------------------------------------
std::string Credentials::getUsername() const
{
	return usernameM;
}
//------------------------------------------------------------------------------
std::string Credentials::getPassword() const
{
	return passwordM;
}
//------------------------------------------------------------------------------
std::string Credentials::getRole() const
{
	return roleM;
}
//------------------------------------------------------------------------------
Database::Database()
    : MetadataItem(), idM(0)
{
	typeM = ntDatabase;
	connectedM = false;
	connectionCredentials = 0;

	// has to be here, since notify() might be called before initChildren()
	domainsM.setName("Domains");
	domainsM.setType(ntDomains);
	exceptionsM.setName("Exceptions");
	exceptionsM.setType(ntExceptions);
    functionsM.setName("Functions");
	functionsM.setType(ntFunctions);
	generatorsM.setName("Generators");
	generatorsM.setType(ntGenerators);
	proceduresM.setName("Procedures");
	proceduresM.setType(ntProcedures);
	rolesM.setName("Roles");
	rolesM.setType(ntRoles);
	tablesM.setName("Tables");
	tablesM.setType(ntTables);
	triggersM.setName("Triggers");
	triggersM.setType(ntTriggers);
	viewsM.setName("Views");
	viewsM.setType(ntViews);
}
//------------------------------------------------------------------------------
void Database::initChildren()
{
	std::vector<MetadataItem *> temp;
	getCollections(temp);
	for (std::vector<MetadataItem *>::iterator it = temp.begin(); it != temp.end(); ++it)
		(*it)->setParent(this);
}
//------------------------------------------------------------------------------
void Database::prepareTemporaryCredentials()
{
	resetCredentials();
	connectionCredentials = new Credentials;
	connectionCredentials->setCharset(credentials.getCharset());	// default to database charset
}
//------------------------------------------------------------------------------
void Database::resetCredentials()
{
	if (connectionCredentials)	// i.e. there is some other
	{
		delete connectionCredentials;
		connectionCredentials = 0;
	}
}
//------------------------------------------------------------------------------
void Database::getIdentifiers(std::vector<std::string>& temp)
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
//------------------------------------------------------------------------------
std::string getLoadingSql(NodeType type)
{
	switch (type)
	{
		case ntTable:		return "select rr.rdb$relation_name from rdb$relations rr "
			" where (rr.RDB$SYSTEM_FLAG = 0 or rr.RDB$SYSTEM_FLAG is null) "
			" and rr.RDB$VIEW_SOURCE is null order by 1";

		case ntView:		return "select rr.rdb$relation_name from rdb$relations rr "
			" where (rr.RDB$SYSTEM_FLAG = 0 or rr.RDB$SYSTEM_FLAG is null) "
			" and rr.RDB$VIEW_SOURCE is not null order by 1";

		case ntProcedure:	return "select pp.rdb$PROCEDURE_name from rdb$procedures pp "
			" where (pp.RDB$SYSTEM_FLAG = 0 or pp.RDB$SYSTEM_FLAG is null) order by 1";

		case ntTrigger:		return "select RDB$TRIGGER_NAME from RDB$TRIGGERS where "
			"(RDB$SYSTEM_FLAG = 0 or RDB$SYSTEM_FLAG is null) ORDER BY 1";

		case ntRole:		return "select RDB$ROLE_NAME from RDB$ROLES ORDER BY 1";

		case ntGenerator:	return "select rdb$generator_name from rdb$generators where "
			"(RDB$SYSTEM_FLAG = 0 or RDB$SYSTEM_FLAG is null) ORDER BY 1";

		case ntFunction:	return "select RDB$FUNCTION_NAME from RDB$FUNCTIONS where "
			"(RDB$SYSTEM_FLAG = 0 or RDB$SYSTEM_FLAG is null) ORDER BY 1";

		case ntDomain:		return "select f.rdb$field_name from rdb$fields f "
			"left outer join rdb$types t on f.rdb$field_type=t.rdb$type "
			"where t.rdb$field_name='RDB$FIELD_TYPE' and f.rdb$field_name not starting with 'RDB$' order by 1";
		case ntException:   return "select RDB$EXCEPTION_NAME from RDB$EXCEPTIONS ORDER BY 1";
		default:			return "";
	};
}
//------------------------------------------------------------------------------
// This could be moved to Column class
std::string Database::loadDomainNameForColumn(std::string table, std::string field)
{
	try
	{
		IBPP::Transaction tr1 = IBPP::TransactionFactory(databaseM, IBPP::amRead);
		tr1->Start();
		IBPP::Statement st1 = IBPP::StatementFactory(databaseM, tr1);
		st1->Prepare(
			"select rdb$field_source from rdb$relation_fields where rdb$relation_name = ? and rdb$field_name = ?"
		);
		st1->Set(1, table);
		st1->Set(2, field);
		st1->Execute();
		st1->Fetch();
		std::string domain;
		st1->Get(1, domain);
		tr1->Commit();
		domain.erase(domain.find_last_not_of(" ") + 1);
		return domain;
	}
	catch (IBPP::Exception &e)
	{
		lastError().setMessage(e.ErrorMessage());
	}
	catch (...)
	{
		lastError().setMessage(_("System error."));
	}

	::wxMessageBox(std2wx(lastError().getMessage()), _("Postprocessing error."), wxICON_WARNING);
	return "";
}
//------------------------------------------------------------------------------
//! returns all collations for a given charset
std::vector<std::string> Database::getCollations(std::string charset)
{
	loadCollations();
	std::vector<std::string> temp;
	std::multimap<std::string, std::string>::iterator low, high;

	high = collationsM.upper_bound(charset);
	for (low = collationsM.lower_bound(charset); low != high; ++low)
		temp.push_back((*low).second);
	return temp;
}
//------------------------------------------------------------------------------
Domain *Database::loadMissingDomain(std::string name)
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
		st1->Set(1, name);
		st1->Execute();
		if (st1->Fetch())
		{
			int c;
			st1->Get(1, c);
			if (c > 0)
			{
				Domain *d = domainsM.add(name);	// add domain to collection
				d->setParent(this);
				if (name.substr(0, 4) != "RDB$")
					refreshByType(ntDomain);
				return d;
			}
		}
		tr1->Commit();
	}
	catch (IBPP::Exception &e)
	{
		lastError().setMessage(e.ErrorMessage());
	}
	catch (...)
	{
		lastError().setMessage(_("System error."));
	}
	return 0;
}
//------------------------------------------------------------------------------
//! small helper function, reads sql and fills the vector with values
// this can be made template function in future
bool Database::fillVector(std::vector<std::string>& list, std::string sql)
{
	try
	{
		IBPP::Transaction tr1 = IBPP::TransactionFactory(databaseM, IBPP::amRead);
		tr1->Start();
		IBPP::Statement st1 = IBPP::StatementFactory(databaseM, tr1);
		st1->Prepare(sql);
		st1->Execute();
		while (st1->Fetch())
		{
			std::string s;
			st1->Get(1, s);
			s.erase(s.find_last_not_of(" ")+1);	// trim
			list.push_back(s);
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
		st1->Prepare(	"select c.rdb$character_set_name, k.rdb$collation_name from rdb$character_sets c"
			" left outer join rdb$collations k on c.rdb$character_set_id = k.rdb$character_set_id order by 1, 2");
		st1->Execute();
		while (st1->Fetch())
		{
			std::string charset, collation;
			st1->Get(1, charset);
			st1->Get(2, collation);
			charset.erase(charset.find_last_not_of(" ") + 1);
			collation.erase(collation.find_last_not_of(" ") + 1);
			collationsM.insert(std::multimap<std::string,std::string>::value_type(charset,collation));
		}
		tr1->Commit();
		return;
	}
	catch (IBPP::Exception &e)
	{
		lastError().setMessage(e.ErrorMessage());
	}
	catch (...)
	{
		lastError().setMessage(_("System error."));
	}

	::wxMessageBox(std2wx(lastError().getMessage()), _("Error while loading collations."), wxICON_WARNING);
}
//------------------------------------------------------------------------------
std::string Database::getTableForIndex(std::string indexName)
{
	try
	{
		IBPP::Transaction tr1 = IBPP::TransactionFactory(databaseM, IBPP::amRead);
		tr1->Start();
		IBPP::Statement st1 = IBPP::StatementFactory(databaseM, tr1);
		st1->Prepare("SELECT rdb$relation_name from rdb$indices where rdb$index_name = ?");
		st1->Set(1, indexName);
		st1->Execute();
		if (st1->Fetch())
		{
			std::string retval;
			st1->Get(1, retval);
			retval.erase(retval.find_last_not_of(" ") + 1);
			return retval;
		}
	}
	catch (IBPP::Exception &e)
	{
		lastError().setMessage(e.ErrorMessage());
	}
	catch (...)
	{
		lastError().setMessage(_("System error."));
	}

	::wxMessageBox(std2wx(lastError().getMessage()), _("Error while loading table for index."), wxICON_WARNING);
	return "";
}
//------------------------------------------------------------------------------
//! load list of objects of type "type" from database, and fill the DBH
bool Database::loadObjects(NodeType type)
{
	switch (type)
	{
		case ntTable:		tablesM.clear();		break;
		case ntView:		viewsM.clear();			break;
		case ntProcedure:	proceduresM.clear();	break;
		case ntTrigger:		triggersM.clear();		break;
		case ntRole:		rolesM.clear();			break;
		case ntGenerator:	generatorsM.clear();	break;
		case ntFunction:	functionsM.clear();		break;
		case ntDomain:		domainsM.clear();		break;
		case ntException:	exceptionsM.clear();	break;
		default:			return false;
	};

	try
	{
		IBPP::Transaction tr1 = IBPP::TransactionFactory(databaseM, IBPP::amRead);
		tr1->Start();
		IBPP::Statement st1 = IBPP::StatementFactory(databaseM, tr1);
		st1->Prepare(getLoadingSql(type));
		st1->Execute();
		while (st1->Fetch())
		{
			std::string name;
			st1->Get(1, name);
			addObject(type, name);
		}
		refreshByType(type);
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
//! Notify the observers that collection has changed
void Database::refreshByType(NodeType type)
{
	switch (type)
	{
		case ntTable:		tablesM.notify();		break;
		case ntView:		viewsM.notify();		break;
		case ntProcedure:	proceduresM.notify();	break;
		case ntTrigger:		triggersM.notify();		break;
		case ntRole:		rolesM.notify();		break;
		case ntGenerator:	generatorsM.notify();	break;
		case ntFunction:	functionsM.notify();	break;
		case ntDomain:		domainsM.notify();		break;
        case ntException:   exceptionsM.notify();   break;
		default:			return;
	};
}
//------------------------------------------------------------------------------
MetadataItem *Database::findByName(std::string name)
{
	for (int n = (int)ntTable; n < (int)ntLastType; n++)
	{
		MetadataItem *m = findByNameAndType((NodeType)n, name);
		if (m)
			return m;
	}
	return 0;
}
//------------------------------------------------------------------------------
MetadataItem *Database::findByNameAndType(NodeType nt, std::string name)
{
	switch (nt)
	{
		case ntTable:		return tablesM.findByName(name);
		case ntView:		return viewsM.findByName(name);
		case ntTrigger:		return triggersM.findByName(name);
		case ntProcedure:	return proceduresM.findByName(name);
		case ntFunction:	return functionsM.findByName(name);
		case ntGenerator:	return generatorsM.findByName(name);
		case ntRole:		return rolesM.findByName(name);
		case ntDomain:		return domainsM.findByName(name);
        case ntException:   return exceptionsM.findByName(name);
		default:
			return 0;
	};
}
//------------------------------------------------------------------------------
void Database::dropObject(MetadataItem *object)
{
	object->drop();		// alert the children if any

	// find the collection that contains it, and remove it
	NodeType nt = object->getType();
	switch (nt)
	{
		case ntTable:		tablesM.remove((Table *)object);			break;
		case ntView:		viewsM.remove((View *)object);				break;
		case ntTrigger:		triggersM.remove((Trigger *)object);		break;
		case ntProcedure:	proceduresM.remove((Procedure *)object);	break;
		case ntFunction:	functionsM.remove((Function *)object);		break;
		case ntGenerator:	generatorsM.remove((Generator *)object);	break;
		case ntRole:		rolesM.remove((Role *)object);				break;
		case ntDomain:		domainsM.remove((Domain *)object);			break;
        case ntException:   exceptionsM.remove((Exception *)object);   break;
		default:
			return;
	};
}
//------------------------------------------------------------------------------
bool Database::addObject(NodeType type, std::string name)
{
	MetadataItem *m;
	switch (type)
	{
		case ntTable:		m = tablesM.add(name);		break;
		case ntView:		m = viewsM.add(name);		break;
		case ntProcedure:	m = proceduresM.add(name);	break;
		case ntTrigger:		m = triggersM.add(name);	break;
		case ntRole:		m = rolesM.add(name);		break;
		case ntGenerator:	m = generatorsM.add(name);	break;
		case ntFunction:	m = functionsM.add(name);	break;
		case ntDomain:		m = domainsM.add(name);		break;
        case ntException:   m = exceptionsM.add(name);  break;
		default:			return false;
	}

	if (!m)		// should never happen, but just in case
		return false;
	m->setName(name);
	m->setParent(this);
	m->setType(type);	// in case it doesn't have ctor to set it
	return true;
}
//------------------------------------------------------------------------------
//! reads a DDL statement and does accordingly

// drop [object_type] [name]
// alter [object_type] [name]
// create [object_type] [name]
// alter table [name] alter [column] type [domain or datatype]
// declare external function [name]
// set null flag via system tables update
bool Database::parseCommitedSql(std::string sql)
{
	sql += "\n";	// if last line starts with --
	Parser::removeComments(sql, "/*", "*/");
	Parser::removeComments(sql, "--", "\n");
	sql = upcase(sql);				// make sql UpperCase for easier handling
	std::stringstream strstrm;		// parse statement into tokens
	std::string action, object_type, name;
	strstrm << sql;
	strstrm >> action;
	strstrm >> object_type;
	strstrm >> name;
    // patch for external functions whose name is made of two words. Shift the words.
    if ((action == "DECLARE" || action == "DROP") && object_type == "EXTERNAL" && name == "FUNCTION")
    {
        object_type = name;
        strstrm >> name;
    }

	if (action == "SET" && object_type == "GENERATOR")
	{
		Generator *g = dynamic_cast<Generator *>(findByNameAndType(ntGenerator, name));
		if (!g)
			return true;
		g->loadValue(true);		// force (re)load of generator value
		action = "ALTER";		// behaves like "alter"
	}

	// indices update tables
	if (action == "DROP" && object_type == "INDEX")
	{
		// We cannot know which table is affected, so the only solution is that all tables reload their indices
		for (MetadataCollection<Table>::iterator it = tablesM.begin(); it != tablesM.end(); ++it)
			(*it).invalidateIndices();
		return true;
	}
	if (action == "ALTER" && object_type == "INDEX" || action == "SET" && object_type == "STATISTICS")	// refresh table
	{
		if (action == "SET") 	// move by 1
			strstrm >> name;
		std::string tableName = getTableForIndex(name);
		Table *t = dynamic_cast<Table *>(findByNameAndType(ntTable, tableName));
		if (t)
			t->invalidateIndices();
		return true;
	}

	// triggers update tables and views
	if (action == "DROP" && object_type == "TRIGGER")	// update all tables
	{
		for (MetadataCollection<Table>::iterator it = tablesM.begin(); it != tablesM.end(); ++it)
			(*it).notify();
		for (MetadataCollection<View>::iterator it = viewsM.begin(); it != viewsM.end(); ++it)
			(*it).notify();
	}

	bool isIndex = false;
	if (action == "CREATE")							// looking for CREATE INDEX
	{
		if (object_type == "INDEX")					// CREATE [UNIQUE] [ASC[ENDING] | DESC[ENDING]] INDEX name ON table ...
			isIndex = true;
		else
		{
			bool found = false;
			std::string words[] = { "UNIQUE", "ASC", "ASCENDING", "DESC", "DESCENDING" };
			for (int i=0; i<sizeof(words) / sizeof(std::string); ++i)
			{
				if (words[i] == object_type)
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
					strstrm >> name;
				}
				else
				{
					for (int i=0; i<sizeof(words) / sizeof(std::string); ++i)
					{
						if (words[i] == name)
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
							strstrm >> name;
						}
					}
				}
			}
		}
	}
	if (isIndex)
	{
		strstrm >> name;	// ON
		strstrm >> name;	// table name
		std::string::size_type p = name.find("(");
		if (p != std::string::npos)
			name.erase(p);
		Table *t = dynamic_cast<Table *>(findByNameAndType(ntTable, name));
		if (t)
			t->invalidateIndices();
		return true;
	}


	// convert change in NULL flag to ALTER TABLE (since it should be processed like that)
	if (sql.substr(0, 44) == "UPDATE RDB$RELATION_FIELDS SET RDB$NULL_FLAG")
	{
		action = "ALTER";
		object_type = "TABLE";
		std::string::size_type pos = sql.find("RDB$RELATION_NAME = '");
		if (pos == std::string::npos)
			return true;
		pos += 21;
		std::string::size_type end = sql.find("'", pos);
		if (end == std::string::npos)
			return true;
		name = sql.substr(pos, end-pos);
	}

	std::string::size_type pbr = name.find("(");		// CREATE PROCEDURE NAME(    <- remove colon
	if (pbr != std::string::npos)
		name.erase(pbr);

	// process the action...
	NodeType t = getTypeByName(object_type);
	if (action == "CREATE" || action == "DECLARE")
	{
		if (addObject(t, name))		// inserts object into collection
			refreshByType(t);
		if (object_type == "TRIGGER")		// new trigger created, alert tables/views
		{									// to update their property pages
			std::string relation;
			strstrm >> relation;	// FOR
			strstrm >> relation;	// relation_name
			Relation *m = dynamic_cast<Relation *>(findByNameAndType(ntTable, relation));
			if (!m)
			{
				m = dynamic_cast<Relation *>(findByNameAndType(ntView, relation));
				if (!m)
					return true;
			}
			m->notify();
		}
	}
	else if (action == "DROP" || action == "ALTER")
	{
		MetadataItem *object = findByNameAndType(t, name);
		if (!object)
			return true;

		if (action == "DROP")
		{
			dropObject(object);
			if (t == ntTable || t == ntView)	// remove related triggers
			{
				while (true)
				{
					Trigger *todrop = 0;
					for (MetadataCollection<Trigger>::iterator it = triggersM.begin(); it != triggersM.end(); ++it)
					{
						std::string relname;			// trigger already gone => cannot fetch relation name
						if (!(*it).getRelation(relname)	|| relname == name)
							todrop = &(*it);
					}
					if (todrop)
						dropObject(todrop);
					else
						break;
				}
			}
		}
		else						// ALTER
		{
			if (t == ntTable)		// ALTER TABLE xyz ALTER field TYPE {domain or datatype}
			{
				std::string alter, field_name, maybe_type, domain_or_datatype;
				strstrm >> alter;
				strstrm >> field_name;
				if (field_name == "COLUMN")	// ALTER TABLE xyz ALTER COLUMN field TYPE {domain or datatype}
					strstrm >> field_name;
				strstrm >> maybe_type;
				if (maybe_type == "TYPE")		// domain is either created/modified/deleted or none
				{								// if we'd only know what was there before... life would be easier
					strstrm >> domain_or_datatype;
					std::string::size_type pos = domain_or_datatype.find("(");
					if (pos != std::string::npos)
						domain_or_datatype.erase(pos);		// remove if it has size/scale

					std::vector<std::string> typenames;				// I first tried a simple array of strings
					typenames.push_back("CHAR");					// but program kept crashing
					typenames.push_back("VARCHAR");
					typenames.push_back("INTEGER");
					typenames.push_back("SMALLINT");
					typenames.push_back("NUMERIC");
					typenames.push_back("DECIMAL");
					typenames.push_back("FLOAT");
					typenames.push_back("DOUBLE PRECISION");
					typenames.push_back("DATE");
					typenames.push_back("TIME");
					typenames.push_back("TIMESTAMP");
					typenames.push_back("ARRAY");
					typenames.push_back("BLOB");
					bool is_datatype = false;
					for (std::vector<std::string>::iterator it = typenames.begin(); it != typenames.end(); ++it)
						if ((*it) == domain_or_datatype)
							is_datatype = true;

					if (is_datatype)		// either existing domain is changing, or new is created
					{
						std::string domain_name = loadDomainNameForColumn(name, field_name);
						MetadataItem *m = domainsM.findByName(domain_name);
						if (m == 0)		// domain does not exist in DBH
						{
							m = domainsM.add();
							m->setName(domain_name);
							m->setParent(this);
							m->setType(ntDomain);	// just in case
						}
						((Domain *)m)->loadInfo();
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
				result = ((Relation *)object)->loadColumns();
			else if (t == ntProcedure)
				result = ((Procedure *)object)->checkAndLoadParameters(true);	// force reload
            else if (t == ntException)
                ((Exception *)object)->loadProperties(true);
			else if (t == ntTrigger)
			{
				((Trigger *)object)->loadInfo(true);
				std::string relation;					// alert table/view
				Trigger *tr = dynamic_cast<Trigger *>(findByNameAndType(ntTrigger, name));
				if (!tr || !tr->getRelation(relation))
					return true;
				Relation *m = dynamic_cast<Relation *>(findByNameAndType(ntTable, relation));
				if (!m)
				{
					m = dynamic_cast<Relation *>(findByNameAndType(ntView, relation));
					if (!m)
						return true;
				}
				m->notify();
			}
			else
				object->notify();

			if (!result)
				return false;
		}
	}

	// TODO: also, it would be great to record all DDL SQL statements into some log file, so user can
	//       review it later. Perhaps add a configuration options:
	//       - Log all successful DDL statements (Y/N)
	//       - Log file (path to file on disk)
	//		 - Log to db. (creates flamerobin_log table in database. That way the history of metadata changes
	//						goes together with database, and mulitple developes can work together easily)
	//       Log can contain stuff like: Server,DB,User,Role,Data&Time,SQL statement
	//       In short: History
	return true;
}
//------------------------------------------------------------------------------
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
		lastError().setMessage(e.ErrorMessage());
	}
	catch (...)
	{
		lastError().setMessage("System error.");
	}
	return false;
}
//------------------------------------------------------------------------------
// the caller of this function should check whether the database object has the
// password set, and if it does not, it should provide the password
//               and if it does, just provide that password
bool Database::connect(std::string password)
{
	if (connectedM)
		return true;

	try
	{
        databaseM = IBPP::DatabaseFactory("", getConnectionString(), getUsername(),
            password, getRole(), getCharset(), "");
		databaseM->Connect();
		connectedM = true;
		notify();

		tablesM.setParent(this);

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

	databaseM.clear();
	return false;
}
//------------------------------------------------------------------------------
bool Database::disconnect()
{
	if (!connectedM)
		return true;

	try
	{
		databaseM->Disconnect();
		resetCredentials();		// "forget" temporary username/password
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

		if (config().get("HideDisconnectedDatabases", false))
			getParent()->notify();
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
void Database::clear()
{
	setPath("");
	setCharset("");
	setUsername("");
	setPassword("");
	setRole("");
}
//------------------------------------------------------------------------------
bool Database::isConnected() const
{
	return connectedM;
}
//------------------------------------------------------------------------------
bool Database::getChildren(std::vector<MetadataItem *>& temp)
{
	if (!connectedM)
		return false;

	getCollections(temp);
	return true;
}
//------------------------------------------------------------------------------
// returns vector of all subitems
void Database::getCollections(std::vector<MetadataItem *>& temp)
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
//------------------------------------------------------------------------------
MetadataCollection<Generator>::const_iterator Database::generatorsBegin()
{
	return generatorsM.begin();
}
//------------------------------------------------------------------------------
MetadataCollection<Generator>::const_iterator Database::generatorsEnd()
{
	return generatorsM.end();
}
//------------------------------------------------------------------------------
MetadataCollection<Domain>::const_iterator Database::domainsBegin()
{
	return domainsM.begin();
}
//------------------------------------------------------------------------------
MetadataCollection<Domain>::const_iterator Database::domainsEnd()
{
	return domainsM.end();
}
//------------------------------------------------------------------------------
MetadataCollection<Table>::const_iterator Database::tablesBegin()
{
	return tablesM.begin();
}
//------------------------------------------------------------------------------
MetadataCollection<Table>::const_iterator Database::tablesEnd()
{
	return tablesM.end();
}
//------------------------------------------------------------------------------
std::string Database::getPath() const
{
	return pathM;
}
//------------------------------------------------------------------------------
std::string Database::getCharset() const
{
	if (connectionCredentials)
		return connectionCredentials->getCharset();
	else
		return credentials.getCharset();
}
//------------------------------------------------------------------------------
std::string Database::getUsername() const
{
	if (connectionCredentials)
		return connectionCredentials->getUsername();
	else
		return credentials.getUsername();
}
//------------------------------------------------------------------------------
std::string Database::getPassword() const
{
	if (connectionCredentials)
		return connectionCredentials->getPassword();
	else
		return credentials.getPassword();
}
//------------------------------------------------------------------------------
std::string Database::getRole() const
{
	if (connectionCredentials)
		return connectionCredentials->getRole();
	else
		return credentials.getRole();
}
//------------------------------------------------------------------------------
IBPP::Database& Database::getIBPPDatabase()
{
	return databaseM;
}
//------------------------------------------------------------------------------
void Database::setPath(std::string value)
{
	pathM = value;
}
//------------------------------------------------------------------------------
void Database::setCharset(std::string value)
{
	if (connectionCredentials)
		connectionCredentials->setCharset(value);
	else
		credentials.setCharset(value);
}
//------------------------------------------------------------------------------
void Database::setUsername(std::string value)
{
	if (connectionCredentials)
		connectionCredentials->setUsername(value);
	else
		credentials.setUsername(value);
}
//------------------------------------------------------------------------------
void Database::setPassword(std::string value)
{
	if (connectionCredentials)
		connectionCredentials->setPassword(value);
	else
		credentials.setPassword(value);
}
//------------------------------------------------------------------------------
void Database::setRole(std::string value)
{
	if (connectionCredentials)
		connectionCredentials->setRole(value);
	else
		credentials.setRole(value);
}
//------------------------------------------------------------------------------
const std::string Database::getTypeName() const
{
	return "DATABASE";
}
//------------------------------------------------------------------------------
void Database::accept(Visitor *v)
{
	v->visit(*this);
}
//------------------------------------------------------------------------------
Server *Database::getServer() const
{
    return dynamic_cast<Server *>(getParent());
}
//------------------------------------------------------------------------------
std::string Database::getConnectionString() const
{
    std::string serverConnStr = getServer()->getConnectionString();
    if (!serverConnStr.empty())
        return serverConnStr + ":" + pathM;
    else
        return pathM;
}
//------------------------------------------------------------------------------
std::string Database::extractNameFromConnectionString() const
{
    string name = pathM;
    string::size_type p = name.find_last_of("/\\:");
    if (p != string::npos)
        name.erase(0, p + 1);
    p = name.find_last_of(".");
    if (p != string::npos)
        name.erase(p, name.length());
    return name;
}
//------------------------------------------------------------------------------
const string Database::getId() const
{
    if (idM == 0)
        idM = getRoot()->getNextId();
    ostringstream oss;
    oss << idM;
    return oss.str();
}
//------------------------------------------------------------------------------
