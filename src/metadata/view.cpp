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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include <ibpp.h>

#include "collection.h"
#include "core/Visitor.h"
#include "database.h"
#include "dberror.h"
#include "frutils.h"
#include "MetadataItemVisitor.h"
#include "relation.h"
#include "ugly.h"
#include "view.h"
//-----------------------------------------------------------------------------
View::View()
	: Relation()
{
	typeM = ntView;
}
//-----------------------------------------------------------------------------
//! returns false if an error occurs
bool View::getSource(wxString& source)
{
	source = wxT("");
	Database *d = static_cast<Database *>(getParent());
	if (!d)
	{
		lastError().setMessage(wxT("Database not set."));
		return false;
	}

	IBPP::Database& db = d->getIBPPDatabase();

	try
	{
		IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
		tr1->Start();
		IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
		st1->Prepare("select rdb$view_source from rdb$relations where rdb$relation_name = ?");
		st1->Set(1, wx2std(getName()));
		st1->Execute();
		st1->Fetch();
        readBlob(st1, 1, source);
		tr1->Commit();
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
wxString View::getAlterSql()
{
	if (!checkAndLoadColumns())
		return lastError().getMessage();
	wxString src;
	if (!getSource(src))
		return lastError().getMessage();

	wxString sql = wxT("DROP VIEW ") + getName() + wxT(";\n");
	sql += wxT("CREATE VIEW ") + getName() + wxT(" (");

	bool first = true;
	for (MetadataCollection <Column>::const_iterator it = columnsM.begin(); it != columnsM.end(); ++it)
	{
		if (first)
			first = false;
		else
			sql += wxT(", ");
		sql += (*it).getName();
	}
	sql += wxT(")\nAS ");
	sql += src;
	return sql;
}
//-----------------------------------------------------------------------------
wxString View::getCreateSqlTemplate() const
{
	wxString sql(
		wxT("CREATE VIEW name ( view_column, ...)\n")
		wxT("AS\n")
		wxT("/* write select statement here */\n")
		wxT("WITH CHECK OPTION;\n"));
	return sql;
}
//-----------------------------------------------------------------------------
const wxString View::getTypeName() const
{
	return wxT("VIEW");
}
//-----------------------------------------------------------------------------
void View::acceptVisitor(MetadataItemVisitor* visitor)
{
	visitor->visit(*this);
}
//-----------------------------------------------------------------------------
