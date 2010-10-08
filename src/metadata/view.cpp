/*
  Copyright (c) 2004-2010 The FlameRobin Development Team

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

#include "metadata/database.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/view.h"
#include "sql/StatementBuilder.h"
//-----------------------------------------------------------------------------
View::View()
    : Relation()
{
    setType(ntView);
}
//-----------------------------------------------------------------------------
wxString View::getSource()
{
    ensurePropertiesLoaded();
    return sourceM;
}
//-----------------------------------------------------------------------------
void View::setSource(const wxString& value)
{
    sourceM = value;
}
//-----------------------------------------------------------------------------
wxString View::getCreateSql()
{
    ensureChildrenLoaded();

    StatementBuilder sb;
    sb << kwCREATE << ' ' << kwVIEW << ' ' << getQuotedName() << wxT(" (")
        << StatementBuilder::IncIndent;

    // make sure that line breaking occurs after comma, not before
    ColumnPtrs::const_iterator it = columnsM.begin();
    wxString colName = (*it)->getQuotedName();
    for (++it; it != columnsM.end(); ++it)
    {
        sb << colName + wxT(", ");
        colName = (*it)->getQuotedName();
    }
    sb << colName;

    sb << ')' << StatementBuilder::DecIndent << StatementBuilder::NewLine
        << kwAS << ' ' << StatementBuilder::DisableLineWrapping
        << getSource() << ';' << StatementBuilder::NewLine;
    return sb;
}
//-----------------------------------------------------------------------------
wxString View::getCreateSqlTemplate() const
{
    StatementBuilder sb;
    sb << kwCREATE << ' ' << kwVIEW << wxT(" name ( view_column, ...)")
        << StatementBuilder::NewLine << kwAS << StatementBuilder::NewLine
        << wxT("/* write select statement here */")
        << StatementBuilder::NewLine
        << kwWITH << ' ' << kwCHECK << ' ' << kwOPTION << ';'
        << StatementBuilder::NewLine;
    return sb;
}
//-----------------------------------------------------------------------------
const wxString View::getTypeName() const
{
    return wxT("VIEW");
}
//-----------------------------------------------------------------------------
void View::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitView(*this);
}
//-----------------------------------------------------------------------------
// Views collection
void Views::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitViews(*this);
}
//-----------------------------------------------------------------------------
void Views::load(ProgressIndicator* progressIndicator)
{
    Database* db = getDatabase(wxT("Views::load"));

    std::string stmt = "select rdb$relation_name from rdb$relations"
        " where (rdb$system_flag = 0 or rdb$system_flag is null)"
        " and rdb$view_source is not null order by 1";
    setItems(db, ntView, db->loadIdentifiers(progressIndicator, stmt));
}
//-----------------------------------------------------------------------------
void Views::loadChildren()
{
    load(0);
}
//-----------------------------------------------------------------------------
