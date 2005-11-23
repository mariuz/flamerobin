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

#include <vector>

#include "constraints.h"
#include "MetadataItemVisitor.h"
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
    for (std::vector<wxString>::const_iterator it = referencedColumnsM.begin(); it != referencedColumnsM.end(); ++it)
    {
        if (it != referencedColumnsM.begin())
            result += wxT(",");
        result += (*it);
    }
    return result;
};
//-----------------------------------------------------------------------------
void Constraint::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visit(*this);
}
//-----------------------------------------------------------------------------
Table* Constraint::getTable() const
{
    return dynamic_cast<Table *>(getParentObjectOfType(ntTable));
}
//-----------------------------------------------------------------------------
