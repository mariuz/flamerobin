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
#include "collection.h"
#include "metadataitemwithcolumns.h"
//------------------------------------------------------------------------------
YxMetadataItemWithColumns::YxMetadataItemWithColumns()
{
	columnsM.setParent(this);
}
//------------------------------------------------------------------------------
YColumn *YxMetadataItemWithColumns::addColumn(YColumn &c)
{
	checkAndLoadColumns();
	YColumn *cc = columnsM.add(c);
	cc->setParent(this);
	return cc;
}
//------------------------------------------------------------------------------
bool YxMetadataItemWithColumns::checkAndLoadColumns()
{
	return (!columnsM.empty() || loadColumns());
}
//------------------------------------------------------------------------------
//! returns false if error occurs, and places the error text in error variable
bool YxMetadataItemWithColumns::loadColumns()
{
	columnsM.clear();
	YDatabase *d = static_cast<YDatabase *>(getParent());
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
			"select r.rdb$field_name, r.rdb$null_flag, r.rdb$field_source, l.rdb$collation_name"
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
			std::string name, source, collation;
			st1->Get(1, name);
			st1->Get(3, source);
			st1->Get(4, collation);

			YColumn *cc = columnsM.add();
			cc->setName(name);
			cc->setParent(this);
			cc->Init(!st1->IsNull(2), source, collation);
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
bool YxMetadataItemWithColumns::getChildren(std::vector<YxMetadataItem *>& temp)
{
	return columnsM.getChildren(temp);
}
//------------------------------------------------------------------------------
