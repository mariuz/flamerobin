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

  $Id$

  Contributor(s): Milan Babuskov.
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include <string>

#include <ibpp.h>

#include "core/Visitor.h"
#include "database.h"
#include "domain.h"
#include "function.h"
//-----------------------------------------------------------------------------
Function::Function()
{
	typeM = ntFunction;
	infoLoadedM = false;
}
//-----------------------------------------------------------------------------
std::string Function::getCreateSqlTemplate() const
{
	return "DECLARE EXTERNAL FUNCTION name [datatype | CSTRING (int) [, datatype | CSTRING (int) ...]]\n"
           "RETURNS {datatype [BY VALUE] | CSTRING (int)} [FREE_IT]\n"
           "ENTRY_POINT 'entryname'\n"
           "MODULE_NAME 'modulename';\n";
}
//-----------------------------------------------------------------------------
const std::string Function::getTypeName() const
{
	return "FUNCTION";
}
//-----------------------------------------------------------------------------
std::string Function::getDropSqlStatement() const
{
    return "DROP EXTERNAL FUNCTION " + getName() + ";";
}
//-----------------------------------------------------------------------------
std::string Function::getDefinition()
{
	loadInfo();
	return definitionM;
}
//-----------------------------------------------------------------------------
void Function::loadInfo(bool force)
{
	if (infoLoadedM && !force)
		return;

	Database *d = getDatabase();
	if (!d)
	{
		definitionM = "Error";
		return;
	}

	IBPP::Database& db = d->getIBPPDatabase();
	definitionM = getName() + "(\n";
	try
	{
		IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
		tr1->Start();
		IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
		st1->Prepare(
			"SELECT f.RDB$RETURN_ARGUMENT, a.RDB$MECHANISM, a.RDB$ARGUMENT_POSITION, "
			" a.RDB$FIELD_TYPE, a.RDB$FIELD_SCALE, a.RDB$FIELD_LENGTH, a.RDB$FIELD_SUB_TYPE, a.RDB$FIELD_PRECISION,"
			" f.RDB$MODULE_NAME, f.RDB$ENTRYPOINT "
			" FROM RDB$FUNCTIONS f"
			" LEFT OUTER JOIN RDB$FUNCTION_ARGUMENTS a ON f.RDB$FUNCTION_NAME = a.RDB$FUNCTION_NAME"
			" WHERE f.RDB$FUNCTION_NAME = ?"
			" ORDER BY a.RDB$ARGUMENT_POSITION"
		);
		st1->Set(1, getName());
		st1->Execute();
		std::string retstr;
		bool first = true;
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
			st1->Get(9, libraryNameM);
			st1->Get(10, entryPointM);
			std::string param = "    " + Domain::datatype2string(type, scale, precision, subtype, length)
				+ " by " + (mechanism == 0 ? "value":"reference");
			if (mechanism == -1)
				param += " [FREE_IT]";
			if (returnarg == retpos)	// output
				retstr = param;
			else
			{
				if (first)
					first = false;
				else
					definitionM += ",\n";
				definitionM += param;
			}
		}
		definitionM += "\n)\nreturns:\n" + retstr;
		infoLoadedM = true;
		tr1->Commit();
	}
	catch (IBPP::Exception &e)
	{
		definitionM = e.ErrorMessage();
	}
	catch (...)
	{
		definitionM = "System error.";
	}
}
//-----------------------------------------------------------------------------
std::string Function::getHtmlHeader()
{
	loadInfo();
	return "<B>Library name:</B> " + libraryNameM + "<BR><B>Entry point:</B>  " + entryPointM + "<BR><BR>";
}
//-----------------------------------------------------------------------------
void Function::accept(Visitor *v)
{
	v->visit(*this);
}
//-----------------------------------------------------------------------------
