/*
Copyright (c) 2004, 2005, 2006 The FlameRobin Development Team

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


  $Id$

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

#include "core/Visitor.h"
#include "dberror.h"
#include "frutils.h"
#include "metadata/collection.h"
#include "metadata/database.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/view.h"
#include "ugly.h"
//-----------------------------------------------------------------------------
View::View()
{
    typeM = ntView;
}
//-----------------------------------------------------------------------------
//! returns false if an error occurs
bool View::getSource(wxString& source)
{
    source = wxT("");
    Database *d = getDatabase();
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
        st1->Set(1, wx2std(getName_()));
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
wxString View::getCreateSql()
{
    wxString src;
    if (!checkAndLoadColumns())
        return lastError().getMessage();
    if (!getSource(src))
        return lastError().getMessage();

    wxString sql;
    sql += wxT("CREATE VIEW ") + getQuotedName() + wxT(" (");

    bool first = true;
    for (MetadataCollection <Column>::const_iterator it = columnsM.begin(); it != columnsM.end(); ++it)
    {
        if (first)
            first = false;
        else
            sql += wxT(", ");
        sql += (*it).getQuotedName();
    }
    sql += wxT(")\nAS ");
    sql += src;
    sql += wxT(";\n");
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
