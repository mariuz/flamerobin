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

  Contributor(s): Milan Babuskov.
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include <string>
#include "ibpp.h"
#include "function.h"
#include "database.h"
#include "domain.h"
//------------------------------------------------------------------------------
YFunction::YFunction()
{
	typeM = ntFunction;
}
//------------------------------------------------------------------------------
std::string YFunction::getCreateSqlTemplate() const
{
	return "DECLARE EXTERNAL FUNCTION name [datatype | CSTRING (int) [, datatype | CSTRING (int) ...]]\n"
           "RETURNS {datatype [BY VALUE] | CSTRING (int)} [FREE_IT]\n"
           "ENTRY_POINT 'entryname'\n"
           "MODULE_NAME 'modulename';\n";
}
//------------------------------------------------------------------------------
const std::string YFunction::getTypeName() const
{
	return "FUNCTION";
}
//------------------------------------------------------------------------------
std::string YFunction::getDropSqlStatement() const
{
    return "DROP EXTERNAL FUNCTION " + getName() + ";";
}
//------------------------------------------------------------------------------
std::string YFunction::getDefinition()
{
	YDatabase *d = getDatabase();
	if (!d)
		return "Error";

	IBPP::Database& db = d->getDatabase();
	std::string retval = nameM + "(\n";
	try
	{
		IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
		tr1->Start();
		IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
		st1->Prepare(
			"SELECT f.RDB$RETURN_ARGUMENT, a.RDB$MECHANISM, a.RDB$ARGUMENT_POSITION, "
			" a.RDB$FIELD_TYPE, a.RDB$FIELD_SCALE, a.RDB$FIELD_LENGTH, a.RDB$FIELD_SUB_TYPE, a.RDB$FIELD_PRECISION"
			" FROM RDB$FUNCTIONS f"
			" LEFT OUTER JOIN RDB$FUNCTION_ARGUMENTS a ON f.RDB$FUNCTION_NAME = a.RDB$FUNCTION_NAME"
			" WHERE f.RDB$FUNCTION_NAME = ?"
			" ORDER BY a.RDB$ARGUMENT_POSITION"
		);
		st1->Set(1, nameM);
		st1->Execute();
		std::string retstr;
		while (st1->Fetch())
		{
			short returnarg, mechanism, type, scale, length, subtype, precision, retpos;
			st1->Get(1, returnarg);
			st1->Get(2, mechanism);
			st1->Get(3, retpos);
			st1->Get(4, type);
			st1->Get(5, scale);
			st1->Get(6, length);
			st1->Get(7, subtype);
			st1->Get(8, precision);
			std::string param = "    " + YDomain::datatype2string(type, scale, precision, subtype, length)
				+ " by " + (mechanism ? "value":"reference");
			if (returnarg == retpos)	// output
				retstr = param;
			else
				retval += param + ",\n";
		}
		retval += ")\nreturns:\n" + retstr;
	}
	catch (IBPP::Exception &e)
	{
		return e.ErrorMessage();
	}
	catch (...)
	{
		return "System error.";
	}
	return retval;
}
//------------------------------------------------------------------------------
