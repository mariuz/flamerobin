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

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <sstream>
#include "parameter.h"
#include "database.h"
#include "ugly.h"
#include "dberror.h"
//------------------------------------------------------------------------------
YDatabase::YDatabase()
{
	typeM = ntDatabase;
	connectedM = false;

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
void YDatabase::getIdentifiers(std::vector<std::string>& temp)
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
			"where t.rdb$field_name='RDB$FIELD_TYPE' order by 1";
		case ntException:   return "select RDB$EXCEPTION_NAME from RDB$EXCEPTIONS ORDER BY 1";
		default:			return "";
	};
}
//------------------------------------------------------------------------------
// This could be moved to YColumn class
std::string YDatabase::loadDomainNameForColumn(std::string table, std::string field)
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
std::vector<std::string> YDatabase::getCollations(std::string charset)
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
//! load charset-collation pairs if needed
void YDatabase::loadCollations()
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
//! load list of objects of type "type" from database, and fill the DBH
bool YDatabase::loadObjects(NodeType type)
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
void YDatabase::refreshByType(NodeType type)
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
YxMetadataItem *YDatabase::findByNameAndType(NodeType nt, std::string name)
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
void YDatabase::dropObject(YxMetadataItem *object)
{
	object->drop();		// alert the children if any

	// find the collection that contains it, and remove it
	NodeType nt = object->getType();
	switch (nt)
	{
		case ntTable:		tablesM.remove((YTable *)object);			break;
		case ntView:		viewsM.remove((YView *)object);				break;
		case ntTrigger:		triggersM.remove((YTrigger *)object);		break;
		case ntProcedure:	proceduresM.remove((YProcedure *)object);	break;
		case ntFunction:	functionsM.remove((YFunction *)object);		break;
		case ntGenerator:	generatorsM.remove((YGenerator *)object);	break;
		case ntRole:		rolesM.remove((YRole *)object);				break;
		case ntDomain:		domainsM.remove((YDomain *)object);			break;
        case ntException:   exceptionsM.remove((YException *)object);   break;
		default:
			return;
	};
}
//------------------------------------------------------------------------------
bool YDatabase::addObject(NodeType type, std::string name)
{
	YxMetadataItem *m;
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
bool YDatabase::parseCommitedSql(std::string sql)
{
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
		YGenerator *g = dynamic_cast<YGenerator *>(findByNameAndType(ntGenerator, name));
		if (!g)
			return true;
		g->loadValue(true);		// force (re)load of generator value
		action = "ALTER";		// behaves like "alter"
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

	// process the action...
	NodeType t = getTypeByName(object_type);
	if (action == "CREATE" || action == "DECLARE")
	{
		if (addObject(t, name))		// inserts object into collection
			refreshByType(t);
	}
	else if (action == "DROP" || action == "ALTER")
	{
		YxMetadataItem *object = findByNameAndType(t, name);
		if (!object)
			return true;

		if (action == "DROP")
		{
			dropObject(object);
		}
		else						// ALTER
		{
			if (t == ntTable)		// ALTER TABLE xyz ALTER field TYPE {domain or datatype}
			{
				std::string alter, field_name, maybe_type, domain_or_datatype;
				strstrm >> alter;
				strstrm >> field_name;
				strstrm >> maybe_type;
				if (maybe_type == "TYPE")		// domain is either created/modified/deleted or none
				{								// if we'd only know what was there before... life would be easier
					strstrm >> domain_or_datatype;
					domain_or_datatype.erase(domain_or_datatype.find("("));	// remove if it has size/scale
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
						YxMetadataItem *m = domainsM.findByName(domain_name);
						if (m == 0)		// domain does not exist in DBH
						{
							m = domainsM.add();
							m->setName(domain_name);
							m->setParent(this);
							m->setType(ntDomain);	// just in case
						}
						((YDomain *)m)->loadInfo();
					}
					else	// there is an extra RDB$domain in domainsM, reload domain info
					{							// TODO: this is very expensive operation (esp. on large databases)
						loadObjects(ntDomain);	// we could rewrite it to select domain for given column of table
					}							// and only load its info and add it
				}
			}

			// TODO: this is a place where we would simply call virtual invalidate() function
			// and object would do wherever it needs to
			bool result = true;
			if (t == ntTable || t == ntView)
				result = ((YxMetadataItemWithColumns *)object)->loadColumns();
			else if (t == ntProcedure)
				result = ((YProcedure *)object)->checkAndLoadParameters();
            else if (t == ntException)
                ((YException *)object)->loadProperties(true);
			else if (t == ntTrigger)
				((YTrigger *)object)->loadInfo(true);
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
bool YDatabase::reconnect() const
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
bool YDatabase::connect(std::string password)
{
	if (connectedM)
		return true;

	try
	{
		if (databaseM.intf() == 0)		// database object needs to be created
		{
			std::string hostname = getParent()->getName();
			databaseM = IBPP::DatabaseFactory(hostname, pathM,	usernameM, password);
		}

		databaseM->Connect();
		connectedM = true;
		validPasswordM = password;	// successful
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

	databaseM.clear();
	return false;
}
//------------------------------------------------------------------------------
bool YDatabase::disconnect()
{
	if (!connectedM)
		return true;

	try
	{
		databaseM->Disconnect();
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

		// this a special case for YDatabase only since it doesn't destroy its subitems
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
void YDatabase::clear()
{
	pathM = "";
	charsetM = "";
	usernameM = "";
	passwordM = "";
	validPasswordM = "";
	roleM = "";
}
//------------------------------------------------------------------------------
bool YDatabase::isConnected() const
{
	return connectedM;
}
//------------------------------------------------------------------------------
// retuns vector of all subitems
bool YDatabase::getChildren(std::vector<YxMetadataItem *>& temp)
{
	if (!connectedM)
		return false;

	temp.push_back(&domainsM);
    temp.push_back(&exceptionsM);
	temp.push_back(&functionsM);
	temp.push_back(&generatorsM);
	temp.push_back(&proceduresM);
	temp.push_back(&rolesM);
	temp.push_back(&tablesM);
	temp.push_back(&triggersM);
	temp.push_back(&viewsM);
	return true;
}
//------------------------------------------------------------------------------
YMetadataCollection<YGenerator>::const_iterator YDatabase::generatorsBegin()
{
	return generatorsM.begin();
}
//------------------------------------------------------------------------------
YMetadataCollection<YGenerator>::const_iterator YDatabase::generatorsEnd()
{
	return generatorsM.end();
}
//------------------------------------------------------------------------------
YMetadataCollection<YDomain>::const_iterator YDatabase::domainsBegin()
{
	return domainsM.begin();
}
//------------------------------------------------------------------------------
YMetadataCollection<YDomain>::const_iterator YDatabase::domainsEnd()
{
	return domainsM.end();
}
//------------------------------------------------------------------------------
YMetadataCollection<YTable>::const_iterator YDatabase::tablesBegin()
{
	return tablesM.begin();
}
//------------------------------------------------------------------------------
YMetadataCollection<YTable>::const_iterator YDatabase::tablesEnd()
{
	return tablesM.end();
}
//------------------------------------------------------------------------------
std::string YDatabase::getPath() const
{
	return pathM;
}
//------------------------------------------------------------------------------
std::string YDatabase::getCharset() const
{
	return charsetM;
}
//------------------------------------------------------------------------------
std::string YDatabase::getUsername() const
{
	return usernameM;
}
//------------------------------------------------------------------------------
std::string YDatabase::getPassword() const
{
	return passwordM;
}
//------------------------------------------------------------------------------
std::string YDatabase::getRole() const
{
	return roleM;
}
//------------------------------------------------------------------------------
IBPP::Database& YDatabase::getDatabase()
{
	return databaseM;
}
//------------------------------------------------------------------------------
void YDatabase::setPath(std::string value)
{
	pathM = value;
	setName(value);
}
//------------------------------------------------------------------------------
void YDatabase::setCharset(std::string value)
{
	charsetM = value;
}
//------------------------------------------------------------------------------
void YDatabase::setUsername(std::string value)
{
	usernameM = value;
}
//------------------------------------------------------------------------------
void YDatabase::setPassword(std::string value)
{
	passwordM = value;
}
//------------------------------------------------------------------------------
void YDatabase::setRole(std::string value)
{
	roleM = value;
}
//------------------------------------------------------------------------------
const std::string YDatabase::getTypeName() const
{
	return "DATABASE";
}
//------------------------------------------------------------------------------
