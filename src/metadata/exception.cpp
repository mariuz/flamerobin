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

  The Initial Developer of the Original Code is Nando Dessena.

  Portions created by the original developer
  are Copyright (C) 2004 Nando Dessena.

  All Rights Reserved.

  Contributor(s):
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include <string>
#include "exception.h"
#include <ibpp.h>
#include "dberror.h"
#include "database.h"
//------------------------------------------------------------------------------
YException::YException()
{
    propertiesLoadedM = false;
}
//------------------------------------------------------------------------------
std::string YException::getCreateSqlTemplate() const
{
	return	"CREATE EXCEPTION name 'exception message';\n";
}
//------------------------------------------------------------------------------
const std::string YException::getTypeName() const
{
	return "EXCEPTION";
}
//------------------------------------------------------------------------------
std::string YException::getMessage()
{
    loadProperties();
    return messageM;
}
//------------------------------------------------------------------------------
int YException::getNumber()
{
    loadProperties();
    return numberM;
}
//------------------------------------------------------------------------------
void YException::loadProperties(bool force)
{
    if (!force && propertiesLoadedM)
        return;

	YDatabase *d = getDatabase();
	if (!d)
		return; // should signal an error here.

	messageM = "";
    numberM = 0;
	try
	{
		IBPP::Database& db = d->getDatabase();
		IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
		tr1->Start();
		IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
		st1->Prepare("select RDB$MESSAGE, RDB$EXCEPTION_NUMBER from RDB$EXCEPTIONS where RDB$EXCEPTION_NAME = ?");
		st1->Set(1, nameM);
		st1->Execute();
		st1->Fetch();
		st1->Get(1, messageM);
        st1->Get(2, numberM);
		tr1->Commit();
		propertiesLoadedM = true;
	}
	catch (IBPP::Exception &e)
	{
		lastError().setMessage(e.ErrorMessage());
	}
	catch (...)
	{
		lastError().setMessage("System error.");
	}
    notify();
}
//------------------------------------------------------------------------------
std::string YException::getAlterSql()
{
	return "ALTER EXCEPTION " + getName() + " '" + getMessage() + "';";
}
