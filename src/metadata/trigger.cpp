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
  The function getTriggerType is adapted from Firebird projects isql code

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

//------------------------------------------------------------------------------
#include <sstream>
#include <ibpp.h>
#include "metadataitem.h"
#include "dberror.h"
#include "database.h"
#include "trigger.h"
//------------------------------------------------------------------------------
YTrigger::YTrigger():
	YxMetadataItem()
{
	typeM = ntTrigger;
	infoIsLoadedM = false;
}
//------------------------------------------------------------------------------
bool YTrigger::getTriggerInfo(std::string& object, bool& active, int& position, std::string& type)
{
	if (!infoIsLoadedM && !loadInfo())
		return false;
	object = objectM;
	active = activeM;
	position = positionM;
	type = triggerTypeM;
	return true;
}
//------------------------------------------------------------------------------
bool YTrigger::loadInfo(bool force)
{
	infoIsLoadedM = false;
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
			"select t.rdb$relation_name, t.rdb$trigger_sequence, t.rdb$trigger_inactive, t.rdb$trigger_type "
			"from rdb$triggers t where rdb$trigger_name = ? "
		);

		st1->Set(1, getName());
		st1->Execute();
		st1->Fetch();
		st1->Get(1, objectM);
		st1->Get(2, &positionM);

		short temp;
		if (st1->IsNull(3))
			temp = 0;
		else
			st1->Get(3, &temp);
		activeM = (temp == 0);

		int ttype;
		st1->Get(4, &ttype);
		triggerTypeM = getTriggerType(ttype);
		tr1->Commit();
		infoIsLoadedM = true;
		if (force)
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
bool YTrigger::getSource(std::string& source) const
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
		st1->Prepare("select rdb$trigger_source from rdb$triggers where rdb$trigger_name = ?");
		st1->Set(1, getName());
		st1->Execute();
		st1->Fetch();
		IBPP::Blob b = IBPP::BlobFactory(st1->Database(), st1->Transaction());
		st1->Get(1, b);

		try				// if blob is empty the exception is thrown
		{				// I tried to check st1->IsNull(1) but it doesn't work
			b->Open();	// to this hack is the only way (for the time being)
		}
		catch (...)
		{
			source = "";
			return true;
		}

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
std::string YTrigger::getTriggerType(int type)
{
	std::string res;

	std::vector<std::string> prefix_types, suffix_types;
	prefix_types.push_back("BEFORE");
	prefix_types.push_back("AFTER");

	suffix_types.push_back("");
	suffix_types.push_back("INSERT");
	suffix_types.push_back("UPDATE");
	suffix_types.push_back("DELETE");

	#define TRIGGER_ACTION_PREFIX(value) ((value + 1) & 1)
	#define TRIGGER_ACTION_SUFFIX(value, slot) (((value + 1) >> (slot * 2 - 1)) & 3)

	std::string result;
	int prefix = TRIGGER_ACTION_PREFIX(type);
	result = prefix_types[prefix];

	int suffix = TRIGGER_ACTION_SUFFIX(type, 1);
	result += " " + suffix_types[suffix];
	suffix = TRIGGER_ACTION_SUFFIX(type, 2);
	if (suffix)
		result += " OR " + suffix_types[suffix];
	suffix = TRIGGER_ACTION_SUFFIX(type, 3);
	if (suffix)
		result += " OR " + suffix_types[suffix];
	return result;
}
//------------------------------------------------------------------------------
YTrigger::firingTimeType YTrigger::getFiringTime()
{
	if (!infoIsLoadedM)
		loadInfo();
	if (triggerTypeM.substr(0, 6) == "BEFORE")
		return beforeTrigger;
	else
		return afterTrigger;
}
//------------------------------------------------------------------------------
std::string YTrigger::getAlterSql()
{
	std::string object, source, type;
	bool active;
	int position;

	if (!getTriggerInfo(object, active, position, type) || !getSource(source))
		return lastError().getMessage();

	std::ostringstream sql;
	sql << "SET TERM ^ ;\nALTER TRIGGER " << nameM;
	if (active)
		sql << " ACTIVE\n";
	else
		sql << " INACTIVE\n";
	sql << type;
	sql << " POSITION ";
	sql << position << "\n";
	//sql << " AS ";
	sql << source;
	sql << "^\nSET TERM ; ^";
	return sql.str();
}
//------------------------------------------------------------------------------
std::string YTrigger::getCreateSqlTemplate() const
{
	return	"SET TERM ^ ;\n\n"
			"CREATE TRIGGER name FOR table/view \n"
			" [IN]ACTIVE \n"
			" {BEFORE | AFTER} INSERT OR UPDATE OR DELETE \n"
			" POSITION number \n"
			"AS \n"
			"BEGIN \n"
			"    /* enter trigger code here */ \n"
			"END^\n\n"
			"SET TERM ; ^\n";
}
//------------------------------------------------------------------------------
const std::string YTrigger::getTypeName() const
{
	return "TRIGGER";
}
//------------------------------------------------------------------------------

