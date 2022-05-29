/*
  Copyright (c) 2004-2022 The FlameRobin Development Team

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
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <vector>

#include "metadata/constraints.h"
#include "metadata/database.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/table.h"

bool Constraint::isSystem() const
{
    Table* t = getTable();
    if (t)
        return t->isSystem();
    else
        return false;
}

const wxString Constraint::getTypeName() const
{
    return "CONSTRAINT";
}

wxString ColumnConstraint::getColumnList(const wxString& separator,
    const wxString& suffix) const
{
    wxString result;
    for (std::vector<wxString>::const_iterator it = columnsM.begin(); it != columnsM.end(); ++it)
    {
        if (it != columnsM.begin())
            result += separator;
        result += (*it) + suffix;
    }
    return result;
};

bool ColumnConstraint::hasColumn(const wxString& column) const
{
    return columnsM.end() != std::find(columnsM.begin(), columnsM.end(),
        column);
}

wxString ForeignKey::getReferencedColumnList() const
{
    wxString result;
    for (std::vector<wxString>::const_iterator it = referencedColumnsM.begin();
         it != referencedColumnsM.end(); ++it)
    {
        if (it != referencedColumnsM.begin())
            result += ", ";
        result += (*it);
    }
    return result;
};

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
            result += " AND ";
        Identifier col1(*im);
        Identifier col2(*it);
        wxString c1 = (quoted ? col1.getQuoted() : col1.get());
        wxString c2 = (quoted ? col2.getQuoted() : col2.get());
        result += table + "." + c1 + " = " + rtab + "." + c2;
    }
    return result;
}

void ForeignKey::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitForeignKey(*this);
}

void UniqueConstraint::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitUniqueConstraint(*this);
}

void PrimaryKeyConstraint::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitPrimaryKeyConstraint(*this);
}

Table* Constraint::getTable() const
{
    MetadataItem* m = getParent();
    while (m)
    {
        if (Table* t = dynamic_cast<Table*>(m))
            return t;
        m = m->getParent();
    }
    return 0;
}

