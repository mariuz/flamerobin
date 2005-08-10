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
Procedure::Procedure()
{
	typeM = ntProcedure;
	parametersM.setParent(this);
	parametersLoadedM = false;
}
//------------------------------------------------------------------------------
Parameter *Procedure::addParameter(Parameter &c)
{
	if (!parametersLoadedM)
		loadParameters();
	Parameter *cc = parametersM.add(c);
	cc->setParent(this);
	return cc;
}
//------------------------------------------------------------------------------
bool Procedure::getChildren(std::vector<MetadataItem *>& temp)
{
	return parametersM.getChildren(temp);
}
//------------------------------------------------------------------------------
bool Procedure::isSelectable()
{
	if (!parametersLoadedM)
		loadParameters();
	for (MetadataCollection <Parameter>::const_iterator it = parametersM.begin(); it != parametersM.end(); ++it)
		if ((*it).getParameterType() == ptOutput)
			return true;
	return false;
}
//------------------------------------------------------------------------------
std::string Procedure::getSelectStatement(bool withColumns)
{
	if (!parametersLoadedM)
		loadParameters();
	std::string collist, parlist;
	for (MetadataCollection <Parameter>::const_iterator it = parametersM.begin(); it != parametersM.end(); ++it)
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
	sql += "\nFROM " + getName();
	if (!parlist.empty())
		sql += "(" + parlist + ")";
	return sql;
}
//------------------------------------------------------------------------------
bool Procedure::checkAndLoadParameters(bool force)
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
bool Procedure::loadParameters()
{
	parametersM.clear();
	Database *d = static_cast<Database *>(getParent());
	if (!d)
	{
		lastError().setMessage("database not set");
		parametersLoadedM = false;
		return false;
	}

	IBPP::Database& db = d->getIBPPDatabase();

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

			Parameter p(source, partype);
			p.setName(column_name);
			Parameter *pp = parametersM.add(p);
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
bool Procedure::getSource(std::string& source)
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
std::string Procedure::getDefinition()
{
	checkAndLoadParameters();
	std::string collist, parlist;
	MetadataCollection <Parameter>::const_iterator lastInput, lastOutput;
	for (MetadataCollection <Parameter>::const_iterator it = parametersM.begin(); it != parametersM.end(); ++it)
	{
		if ((*it).getParameterType() == ptInput)
			lastInput = it;
		else
			lastOutput = it;
	}
	for (MetadataCollection <Parameter>::const_iterator it = parametersM.begin(); it != parametersM.end(); ++it)
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
	std::string retval = getName();
	if (!parlist.empty())
		retval += "(\n" + parlist + ")";
	retval += "\n";
	if (!collist.empty())
		retval += "returns:\n" + collist;
	return retval;
}
//------------------------------------------------------------------------------
std::string Procedure::getAlterSql()
{
	if (!parametersLoadedM)
		if (loadParameters())
			return lastError().getMessage();

	std::string source;
	if (!getSource(source))
		return lastError().getMessage();

	std::string sql = "SET TERM ^ ;\nALTER PROCEDURE " + getName();
	if (!parametersM.empty())
	{
		std::string input, output;
		for (MetadataCollection <Parameter>::const_iterator it = parametersM.begin(); it != parametersM.end(); ++it)
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
std::string Procedure::getCreateSqlTemplate() const
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
const std::string Procedure::getTypeName() const
{
	return "PROCEDURE";
}
//------------------------------------------------------------------------------
void Procedure::accept(Visitor *v)
{
	v->visit(*this);
}
//------------------------------------------------------------------------------

