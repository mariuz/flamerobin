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
#include "frutils.h"
#include "database.h"
#include "collection.h"
#include "relation.h"
//------------------------------------------------------------------------------
Relation::Relation()
{
	columnsM.setParent(this);
}
//------------------------------------------------------------------------------
YColumn *Relation::addColumn(YColumn &c)
{
	checkAndLoadColumns();
	YColumn *cc = columnsM.add(c);
	cc->setParent(this);
	return cc;
}
//------------------------------------------------------------------------------
bool Relation::checkAndLoadColumns()
{
	return (!columnsM.empty() || loadColumns());
}
//------------------------------------------------------------------------------
//! returns false if error occurs, and places the error text in error variable
bool Relation::loadColumns()
{
	columnsM.clear();
	YDatabase *d = static_cast<YDatabase *>(getParent());
	if (!d)
	{
		lastError().setMessage("database not set");
		return false;
	}

	IBPP::Database& db = d->getIBPPDatabase();

	try
	{
		IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
		tr1->Start();
		IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
		st1->Prepare(
			"select r.rdb$field_name, r.rdb$null_flag, r.rdb$field_source, l.rdb$collation_name, f.rdb$computed_blr, "
			" f.rdb$computed_source "
			" from rdb$fields f"
			" join rdb$relation_fields r on f.rdb$field_name=r.rdb$field_source"
			" left outer join rdb$collations l on l.rdb$collation_id = r.rdb$collation_id and l.rdb$character_set_id = f.rdb$character_set_id"
			" where r.rdb$relation_name = ?"
			" order by r.rdb$field_position"
		);

		st1->Set(1, getName());
		st1->Execute();
		while (st1->Fetch())
		{
			std::string name, source, collation, computedSrc;
			st1->Get(1, name);
			st1->Get(3, source);
			st1->Get(4, collation);
			readBlob(st1, 6, computedSrc);

			YColumn *cc = columnsM.add();
			cc->setName(name);
			cc->setParent(this);
			cc->Init(!st1->IsNull(2), source, !st1->IsNull(5), computedSrc, collation);
		}

		tr1->Commit();
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
//! load list of triggers for relation
//! link them to triggers in database's collection
bool Relation::getTriggers(std::vector<YTrigger *>& list, YTrigger::firingTimeType beforeOrAfter)
{
	YDatabase *d = getDatabase();
	if (!d)
	{
		lastError().setMessage("database not set");
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
bool Relation::getChildren(std::vector<YxMetadataItem *>& temp)
{
	return columnsM.getChildren(temp);
}
//------------------------------------------------------------------------------
