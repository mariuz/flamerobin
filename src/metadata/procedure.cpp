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
#include "visitor.h"
#include "collection.h"
#include "parameter.h"
#include "procedure.h"
//------------------------------------------------------------------------------
YProcedure::YProcedure()
{
	typeM = ntProcedure;
	parametersM.setParent(this);
	parametersLoadedM = false;
}
//------------------------------------------------------------------------------
YParameter *YProcedure::addParameter(YParameter &c)
{
	if (!parametersLoadedM)
		loadParameters();
	YParameter *cc = parametersM.add(c);
	cc->setParent(this);
	return cc;
}
//------------------------------------------------------------------------------
bool YProcedure::getChildren(std::vector<YxMetadataItem *>& temp)
{
	return parametersM.getChildren(temp);
}
//------------------------------------------------------------------------------
bool YProcedure::isSelectable()
{
	if (!parametersLoadedM)
		loadParameters();
	for (YMetadataCollection <YParameter>::const_iterator it = parametersM.begin(); it != parametersM.end(); ++it)
		if ((*it).getParameterType() == ptOutput)
			return true;
	return false;
}
//------------------------------------------------------------------------------
std::string YProcedure::getSelectStatement(bool withColumns)
{
	if (!parametersLoadedM)
		loadParameters();
	std::string collist, parlist;
	for (YMetadataCollection <YParameter>::const_iterator it = parametersM.begin(); it != parametersM.end(); ++it)
	{
		if ((*it).getParameterType() == ptInput)
		{
			if (!parlist.empty())
				parlist += ", ";
			parlist += (*it).getName();
		}
		else
		{
			if (!collist.empty())
				collist += ", ";
			collist += (*it).getName();
		}
	}

	std::string sql = "SELECT ";
	if (withColumns)
		sql += collist;
	else
		sql += "* ";
	sql += "\nFROM " + nameM;
	if (!parlist.empty())
		sql += "(" + parlist + ")";
	return sql;
}
//------------------------------------------------------------------------------
bool YProcedure::checkAndLoadParameters(bool force)
{
	if (force || !parametersLoadedM)
	{
		loadParameters();
		notify();
	}
	return parametersLoadedM;
}
//------------------------------------------------------------------------------
//! returns false if error occurs, and places the error text in error variable
bool YProcedure::loadParameters()
{
	parametersM.clear();
	YDatabase *d = static_cast<YDatabase *>(getParent());
	if (!d)
	{
		lastError().setMessage("database not set");
		parametersLoadedM = false;
		return false;
	}

	IBPP::Database& db = d->getDatabase();

	try
	{
		IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
		tr1->Start();
		IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
		st1->Prepare(
			"select p.rdb$parameter_name, p.rdb$field_source, p.rdb$parameter_type"
			" from rdb$procedure_parameters p"
			" where p.rdb$PROCEDURE_name = ? "
			" order by p.rdb$parameter_type, rdb$PARAMETER_number "
		);
		st1->Set(1, getName());
		st1->Execute();

		while (st1->Fetch())
		{
			std::string column_name, source;
			short partype;
			st1->Get(1, column_name);
			st1->Get(2, source);
			st1->Get(3, &partype);

			YParameter p(source, partype);
			p.setName(column_name);
			YParameter *pp = parametersM.add(p);
			pp->setParent(this);
		}

		tr1->Commit();
		parametersLoadedM = true;
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

	parametersLoadedM = false;
	return false;
}
//------------------------------------------------------------------------------
//! returns false if an error occurs
bool YProcedure::getSource(std::string& source)
{
	YDatabase *d = static_cast<YDatabase *>(getParent());
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
		st1->Prepare("select rdb$procedure_source from rdb$procedures where rdb$procedure_name = ?");
		st1->Set(1, getName());
		st1->Execute();
		st1->Fetch();
		readBlob(st1, 1, source);
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
std::string YProcedure::getDefinition()
{
	checkAndLoadParameters();
	std::string collist, parlist;
	YMetadataCollection <YParameter>::const_iterator lastInput, lastOutput;
	for (YMetadataCollection <YParameter>::const_iterator it = parametersM.begin(); it != parametersM.end(); ++it)
	{
		if ((*it).getParameterType() == ptInput)
			lastInput = it;
		else
			lastOutput = it;
	}
	for (YMetadataCollection <YParameter>::const_iterator it = parametersM.begin(); it != parametersM.end(); ++it)
	{
		if ((*it).getParameterType() == ptInput)
		{
			parlist += "    " + (*it).getName() + " " + (*it).getDomain()->getDatatypeAsString();
			if (it != lastInput)
				parlist += ",";
			parlist += "\n";
		}
		else
		{
			collist += "    " + (*it).getName() + " " + (*it).getDomain()->getDatatypeAsString();
			if (it != lastOutput)
				collist += ",";
			collist += "\n";
		}
	}
	std::string retval = nameM;
	if (!parlist.empty())
		retval += "(\n" + parlist + ")";
	retval += "\n";
	if (!collist.empty())
		retval += "returns:\n" + collist;
	return retval;
}
//------------------------------------------------------------------------------
std::string YProcedure::getAlterSql()
{
	if (!parametersLoadedM)
		if (loadParameters())
			return lastError().getMessage();

	std::string source;
	if (!getSource(source))
		return lastError().getMessage();

	std::string sql = "SET TERM ^ ;\nALTER PROCEDURE " + nameM;
	if (!parametersM.empty())
	{
		std::string input, output;
		for (YMetadataCollection <YParameter>::const_iterator it = parametersM.begin(); it != parametersM.end(); ++it)
		{
			if ((*it).getParameterType() == ptInput)
			{
				if (input.empty())
					input += " (\n    ";
				else
					input += ",\n    ";
				input += (*it).getName() + " " + (*it).getDomain()->getDatatypeAsString();
			}
			else
			{
				if (output.empty())
					output += "\nRETURNS (\n    ";
				else
					output += ",\n    ";
				output += (*it).getName() + " " + (*it).getDomain()->getDatatypeAsString();
			}
		}

		if (!input.empty())
			sql += input + " )";
		if (!output.empty())
			sql += output + " )";
	}
	sql += "\nAS";
	sql += source;
	sql += "^\nSET TERM ; ^";
	return sql;
}
//------------------------------------------------------------------------------
std::string YProcedure::getCreateSqlTemplate() const
{
	std::string s("SET TERM ^ ;\n\n"
			"CREATE PROCEDURE name \n"
			" ( input_parameter_name < datatype>, ... ) \n"
			"RETURNS \n"
			" ( output_parameter_name < datatype>, ... )\n"
			"AS \n"
			"DECLARE VARIABLE variable_name < datatype>; \n"
			"BEGIN\n"
			"  /* write your code here */ \n"
			"END^\n\n"
			"SET TERM ; ^\n");
	return s;
}
//------------------------------------------------------------------------------
const std::string YProcedure::getTypeName() const
{
	return "PROCEDURE";
}
//------------------------------------------------------------------------------
void YProcedure::accept(Visitor *v)
{
	v->visit(*this);
}
//------------------------------------------------------------------------------

