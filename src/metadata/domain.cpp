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

  $Id$

  Contributor(s): Nando Dessena
*/

//-----------------------------------------------------------------------------
// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include <sstream>

#include <ibpp.h>

#include "database.h"
#include "dberror.h"
#include "domain.h"
#include "MetadataItemVisitor.h"
//-----------------------------------------------------------------------------
Domain::Domain():
	MetadataItem()
{
	typeM = ntDomain;
	infoLoadedM = false;	// I had a 2 hour session with debugger to found out that this was missing
}
//-----------------------------------------------------------------------------
bool Domain::loadInfo()
{
	Database *d = getDatabase();
	if (!d)
	{
		//wxMessageBox(_("Domain::loadInfo, database = 0"), _("WARNING"), wxICON_WARNING|wxOK);
		return false;
	}
	IBPP::Database& db = d->getIBPPDatabase();

	try
	{
		IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
		tr1->Start();
		IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
		st1->Prepare(
			"select t.rdb$type, f.rdb$field_sub_type, f.rdb$field_length,"
			" f.rdb$field_precision, f.rdb$field_scale, c.rdb$character_set_name"
			" from rdb$fields f"
			" join rdb$types t on f.rdb$field_type=t.rdb$type"
			" left outer join rdb$character_sets c on c.rdb$character_set_id = f.rdb$character_set_id"
			" where f.rdb$field_name = ?"
			" and t.rdb$field_name='RDB$FIELD_TYPE'"
		);

		st1->Set(1, wx2std(getName()));
		st1->Execute();
		if (!st1->Fetch())
		{
			//wxMessageBox(_("Domain not found."), _("Warning."), wxICON_WARNING|wxOK);
			return false;
		}
		st1->Get(1, &datatypeM);
		if (st1->IsNull(2))
			subtypeM = 0;
		else
			st1->Get(2, &subtypeM);
		st1->Get(3, &lengthM);
		if (st1->IsNull(4))
			precisionM = 0;
		else
			st1->Get(4, &precisionM);
		if (st1->IsNull(5))
			scaleM = 0;
		else
			st1->Get(5, &scaleM);
		if (st1->IsNull(6))
			charsetM = wxT("");
		else
		{
			std::string charset;
			st1->Get(6, charset);
			charsetM = std2wx(charset);
			charsetM.erase(charsetM.find_last_not_of(wxT(" ")) + 1);
		}

		tr1->Commit();
		if (!isSystem())
			notifyObservers();
		infoLoadedM = true;
		return true;
	}
	catch (IBPP::Exception &e)
	{
		lastError().setMessage(std2wx(e.ErrorMessage()));
	}
	catch (...)
	{
		lastError().setMessage(_("System error."));
	}
	return false;
}
//-----------------------------------------------------------------------------
//! returns column's datatype as human readable wxString. It can also be used to construct DDL for tables
wxString Domain::getDatatypeAsString()
{
	if (!infoLoadedM)
		loadInfo();

	return datatype2string(datatypeM, scaleM, precisionM, subtypeM, lengthM);
}
//-----------------------------------------------------------------------------
wxString Domain::datatype2string(short datatype, short scale, short precision, short subtype, short length)
{
	std::ostringstream retval;		// this will be returned

	// special case (mess that some tools (ex. IBExpert) make by only setting scale and not changing type)
	if (datatype == 27 && scale < 0)
	{
		retval << "Numeric(15," << -scale << ")";
		return std2wx(retval.str());
	}

	// LONG&INT64: INT/SMALLINT (prec=0), DECIMAL(sub_type=2), NUMERIC(sub_type=1)
	if (datatype == 7 || datatype == 8 || datatype == 16)
	{
		if (scale == 0)
		{
			if (datatype == 7)
				return wxT("Smallint");
			else if (datatype == 8)
				return wxT("Integer");
			else
				return wxT("Numeric(18,0)");
		}
		else
		{
			retval << (subtype == 2 ? "Decimal(" : "Numeric(");
			if (precision <= 0 || precision > 18)
				retval << 18;
			else
				retval << precision;
			retval << "," << -scale << ")";
			return std2wx(retval.str());
		}
	}

	wxString names[] = {
		wxT("Char"),
		wxT("Float"),
		wxT("Double precision"),
		wxT("Timestamp"),
		wxT("Varchar"),
		wxT("Blob"),
		wxT("Date"),
		wxT("Time"),
		wxT("CSTRING")
	};
	short mapper[9] = { 14, 10, 27, 35, 37, 261, 12, 13, 40 };

	for (int i = 0; i < 9; ++i)
	{
		if (mapper[i] == datatype)
		{
			retval << wx2std(names[i]);
			break;
		}
	}

	if (datatype == 14 || datatype == 37 || datatype == 40)	// char, varchar & cstring, add (length)
		retval << "(" << length << ")";

	if (datatype == 261)	// blob
		retval << " sub_type " << subtype;

	return std2wx(retval.str());
}
//-----------------------------------------------------------------------------
void Domain::getDatatypeParts(wxString& type, wxString& size, wxString& scale)
{
	wxString datatype = getDatatypeAsString();
	wxString::size_type p1 = datatype.find(wxT("("));
	if (p1 != wxString::npos)
	{
		type = datatype.substr(0, p1);
		wxString::size_type p2 = datatype.find(wxT(","));
		if (p2 == wxString::npos)
			p2 = datatype.find(wxT(")"));
		else
		{
			wxString::size_type p3 = datatype.find(wxT(")"));
			scale = datatype.substr(p2 + 1, p3 - p2 - 1);
		}
		size = datatype.substr(p1 + 1, p2 - p1 - 1);
	}
	else
		type = datatype;
}
//-----------------------------------------------------------------------------
wxString Domain::getCharset()
{
	if (!infoLoadedM)
		loadInfo();

	return charsetM;
}
//-----------------------------------------------------------------------------
wxString Domain::getPrintableName()
{
	return getName() + wxT(" ") + getDatatypeAsString();
}
//-----------------------------------------------------------------------------
wxString Domain::getCreateSqlTemplate() const
{
	return	wxT("CREATE DOMAIN domain_name\n")
            wxT("AS datatype\n")
            wxT("DEFAULT {literal | NULL | USER}\n")
            wxT("[NOT NULL]\n")
            wxT("[CHECK (dom_search_condition)]\n")
            wxT("COLLATE collation;\n");
}
//-----------------------------------------------------------------------------
const wxString Domain::getTypeName() const
{
	return wxT("DOMAIN");
}
//-----------------------------------------------------------------------------
void Domain::acceptVisitor(MetadataItemVisitor* visitor)
{
	visitor->visit(*this);
}
//-----------------------------------------------------------------------------
