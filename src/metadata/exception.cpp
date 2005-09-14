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

#include "database.h"
#include "dberror.h"
#include "exception.h"
#include "MetadataItemVisitor.h"
//-----------------------------------------------------------------------------
Exception::Exception()
{
    propertiesLoadedM = false;
}
//-----------------------------------------------------------------------------
wxString Exception::getCreateSqlTemplate() const
{
	return	wxT("CREATE EXCEPTION name 'exception message';\n");
}
//-----------------------------------------------------------------------------
const wxString Exception::getTypeName() const
{
	return wxT("EXCEPTION");
}
//-----------------------------------------------------------------------------
wxString Exception::getMessage()
{
    loadProperties();
    return messageM;
}
//-----------------------------------------------------------------------------
int Exception::getNumber()
{
    loadProperties();
    return numberM;
}
//-----------------------------------------------------------------------------
void Exception::loadProperties(bool force)
{
    if (!force && propertiesLoadedM)
        return;

	Database* d = getDatabase();
	if (!d)
		return; // should signal an error here.

	messageM = wxT("");
    numberM = 0;
	try
	{
		IBPP::Database& db = d->getIBPPDatabase();
		IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
		tr1->Start();
		IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
		st1->Prepare("select RDB$MESSAGE, RDB$EXCEPTION_NUMBER from RDB$EXCEPTIONS where RDB$EXCEPTION_NAME = ?");
		st1->Set(1, wx2std(getName()));
		st1->Execute();
		st1->Fetch();
		std::string message;
		st1->Get(1, message);
		messageM = std2wx(message);
        st1->Get(2, numberM);
		tr1->Commit();
		propertiesLoadedM = true;
	}
	catch (IBPP::Exception &e)
	{
		lastError().setMessage(std2wx(e.ErrorMessage()));
	}
	catch (...)
	{
		lastError().setMessage(_("System error."));
	}
    notifyObservers();
}
//-----------------------------------------------------------------------------
wxString Exception::getAlterSql()
{
	return wxT("ALTER EXCEPTION ") + getName() + wxT(" '") + getMessage() + wxT("';");
}
//-----------------------------------------------------------------------------
void Exception::acceptVisitor(MetadataItemVisitor* visitor)
{
	visitor->visit(*this);
}
//-----------------------------------------------------------------------------
