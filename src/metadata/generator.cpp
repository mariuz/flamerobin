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

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif
//------------------------------------------------------------------------------
#include <sstream>
#include "ibpp.h"
#include "dberror.h"
#include "database.h"
#include "metadataitem.h"
#include "generator.h"
//------------------------------------------------------------------------------
YGenerator::YGenerator():
	YxMetadataItem()
{
	typeM = ntGenerator;
	valueLoadedM = false;
}
//------------------------------------------------------------------------------
int YGenerator::getValue()
{
	loadValue();
	return valueM;
}
//------------------------------------------------------------------------------
bool YGenerator::loadValue(bool force)
{
	if (!force && valueLoadedM)
		return true;

	YDatabase *d = getDatabase();
	if (!d)
	{
		lastError().setMessage("Database not set.");
		return false;
	}

	IBPP::Database& db = d->getDatabase();

	try
	{
		IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
		tr1->Start();
		IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
		st1->Prepare("select gen_id(" + nameM + ", 0) from rdb$database");
		st1->Execute();
		st1->Fetch();
		st1->Get(1, &valueM);
		tr1->Commit();
		valueLoadedM = true;
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
std::string YGenerator::getPrintableName() const
{
	if (!valueLoadedM)
		return nameM;

	std::ostringstream s;
	s << nameM;
	s << " = ";
	s << valueM;
	return s.str();
}
//------------------------------------------------------------------------------
std::string YGenerator::getCreateSqlTemplate() const
{
	return "CREATE GENERATOR name;\nSET GENERATOR name TO value;\n";
}
//------------------------------------------------------------------------------
const std::string YGenerator::getTypeName() const
{
	return "GENERATOR";
}
//------------------------------------------------------------------------------

