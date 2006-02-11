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

  Contributor(s): Milan Babuskov, Nando Dessena, Michael Hieke
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
#include "domain.h"
#include "function.h"
#include "MetadataItemVisitor.h"
//-----------------------------------------------------------------------------
Function::Function()
{
    typeM = ntFunction;
    infoLoadedM = false;
}
//-----------------------------------------------------------------------------
wxString Function::getCreateSqlTemplate() const
{
    return wxT("DECLARE EXTERNAL FUNCTION name [datatype | CSTRING (int) [, datatype | CSTRING (int) ...]]\n")
           wxT("RETURNS {datatype [BY VALUE] | CSTRING (int)} [FREE_IT]\n")
           wxT("ENTRY_POINT 'entryname'\n")
           wxT("MODULE_NAME 'modulename';\n");
}
//-----------------------------------------------------------------------------
const wxString Function::getTypeName() const
{
    return wxT("FUNCTION");
}
//-----------------------------------------------------------------------------
wxString Function::getDropSqlStatement() const
{
    return wxT("DROP EXTERNAL FUNCTION ") + getQuotedName() + wxT(";");
}
//-----------------------------------------------------------------------------
wxString Function::getDefinition()
{
    loadInfo();
    return definitionM;
}
//-----------------------------------------------------------------------------
void Function::loadInfo(bool force)
{
    if (infoLoadedM && !force)
        return;

    Database* d = getDatabase();
    if (!d)
    {
        definitionM = wxT("Error");
        return;
    }

    IBPP::Database& db = d->getIBPPDatabase();
    definitionM = getName_() + wxT("(\n");
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
        st1->Set(1, wx2std(getName_()));
        st1->Execute();
        wxString retstr;
        bool first = true;
        while (st1->Fetch())
        {
            short returnarg, mechanism, type, scale, length, subtype, precision, retpos;
            std::string libraryName, entryPoint;
            st1->Get(1, returnarg);
            st1->Get(2, mechanism);
            st1->Get(3, retpos);
            st1->Get(4, type);
            st1->Get(5, scale);
            st1->Get(6, length);
            st1->Get(7, subtype);
            st1->Get(8, precision);
            st1->Get(9, libraryName);
            libraryNameM = std2wx(libraryName);
            st1->Get(10, entryPoint);
            entryPointM = std2wx(entryPoint);
            wxString param = wxT("    ") + Domain::datatype2string(
                type, scale, precision, subtype, length, true) + wxT(" by ")
                + (mechanism == 0 ? wxT("value") : wxT("reference"));
            if (mechanism == -1)
                param += wxT(" [FREE_IT]");
            if (returnarg == retpos)    // output
                retstr = param;
            else
            {
                if (first)
                    first = false;
                else
                    definitionM += wxT(",\n");
                definitionM += param;
            }
        }
        definitionM += wxT("\n)\nreturns:\n") + retstr;
        infoLoadedM = true;
        tr1->Commit();
    }
    catch (IBPP::Exception &e)
    {
        definitionM = std2wx(e.ErrorMessage());
    }
    catch (...)
    {
        definitionM = _("System error.");
    }
}
//-----------------------------------------------------------------------------
wxString Function::getHtmlHeader()
{
    loadInfo();
    return wxT("<B>Library name:</B> ") + libraryNameM + wxT("<BR><B>Entry point:</B>  ")
        + entryPointM + wxT("<BR><BR>");
}
//-----------------------------------------------------------------------------
void Function::loadDescription()
{
    MetadataItem::loadDescription(
        wxT("select RDB$DESCRIPTION from RDB$FUNCTIONS ")
        wxT("where RDB$FUNCTION_NAME = ?"));
}
//-----------------------------------------------------------------------------
void Function::saveDescription(wxString description)
{
    MetadataItem::saveDescription(
        wxT("update RDB$FUNCTIONS set rdb$description = ? ")
        wxT("where RDB$FUNCTION_NAME = ?"),
        description);
}
//-----------------------------------------------------------------------------
void Function::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visit(*this);
}
//-----------------------------------------------------------------------------
