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
#include "view.h"
#include "visitor.h"
#include "collection.h"
#include "relation.h"
//------------------------------------------------------------------------------
View::View()
	:Relation()
{
	typeM = ntView;
}
//------------------------------------------------------------------------------
//! returns false if an error occurs
bool View::getSource(std::string& source)
{
	Database *d = static_cast<Database *>(getParent());
	if (!d)
	{
		lastError().setMessage("Database not set.");
		return false;
	}

	IBPP::Database& db = d->getIBPPDatabase();

	try
	{
		IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
		tr1->Start();
		IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
		st1->Prepare("select rdb$view_source from rdb$relations where rdb$relation_name = ?");
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
std::string View::getAlterSql()
{
	if (!checkAndLoadColumns())
		return lastError().getMessage();
	std::string src;
	if (!getSource(src))
		return lastError().getMessage();

	std::string sql = "DROP VIEW " + getName() + ";\n";
	sql += "CREATE VIEW " + getName() + " (";

	bool first = true;
	for (MetadataCollection <Column>::const_iterator it = columnsM.begin(); it != columnsM.end(); ++it)
	{
		if (first)
			first = false;
		else
			sql += ", ";
		sql += (*it).getName();
	}
	sql += ")\nAS ";
	sql += src;
	return sql;
}
//------------------------------------------------------------------------------
std::string View::getCreateSqlTemplate() const
{
	std::string sql(
		"CREATE VIEW name ( view_column, ...)\n"
		"AS\n"
		"/* write select statement here */\n"
		"WITH CHECK OPTION;\n");
	return sql;
}
//------------------------------------------------------------------------------
const std::string View::getTypeName() const
{
	return "VIEW";
}
//------------------------------------------------------------------------------
void View::accept(Visitor *v)
{
	v->visit(*this);
}
//------------------------------------------------------------------------------

