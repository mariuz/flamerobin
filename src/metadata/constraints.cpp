/*
  Copyright (c) 2004-2008 The FlameRobin Development Team

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

#include <vector>

#include "metadata/constraints.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/table.h"
//-----------------------------------------------------------------------------
bool Constraint::isSystem() const
{
    return (getName_().StartsWith(wxT("RDB$")) 
        || getName_().StartsWith(wxT("INTEG_")));
}
//-----------------------------------------------------------------------------
wxString ColumnConstraint::getColumnList() const
{
    wxString result;
    for (std::vector<wxString>::const_iterator it = columnsM.begin(); it != columnsM.end(); ++it)
    {
        if (it != columnsM.begin())
            result += wxT(",");
        result += (*it);
    }
    return result;
};
//-----------------------------------------------------------------------------
wxString ForeignKey::getReferencedColumnList() const
{
    wxString result;
    for (std::vector<wxString>::const_iterator it = referencedColumnsM.begin();
         it != referencedColumnsM.end(); ++it)
    {
        if (it != referencedColumnsM.begin())
            result += wxT(",");
        result += (*it);
    }
    return result;
};
//-----------------------------------------------------------------------------
wxString ForeignKey::getJoin(bool quoted) const
{
    Identifier reftab(referencedTableM);
    wxString rtab = (quoted ? reftab.getQuoted() : reftab.get());
    wxString table = (quoted ? getTable()->getQuotedName() : getTable()->getName_());
    wxString result;
    std::vector<wxString>::const_iterator im = columnsM.begin();
    for (std::vector<wxString>::const_iterator it = referencedColumnsM.begin();
         it != referencedColumnsM.end(); ++it, ++im)
    {
        if (!result.IsEmpty())
            result += wxT(" AND ");
        Identifier col1(*im);
        Identifier col2(*it);
        wxString c1 = (quoted ? col1.getQuoted() : col1.get());
        wxString c2 = (quoted ? col2.getQuoted() : col2.get());
        result += table + wxT(".") + c1 + wxT(" = ") + rtab + wxT(".") + c2;
    }
    return result;
}
//-----------------------------------------------------------------------------
void ForeignKey::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitForeignKey(*this);
}
//-----------------------------------------------------------------------------
void UniqueConstraint::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitUniqueConstraint(*this);
}
//-----------------------------------------------------------------------------
void PrimaryKeyConstraint::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitPrimaryKeyConstraint(*this);
}
//-----------------------------------------------------------------------------
Table* Constraint::getTable() const
{
    return dynamic_cast<Table *>(getParentObjectOfType(ntTable));
}
//-----------------------------------------------------------------------------
