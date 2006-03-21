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
  are Copyright (C) 2005 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s): Nando Dessena, Michael Hieke
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

#include "Index.h"
#include "MetadataItemVisitor.h"
//-----------------------------------------------------------------------------
Index::Index(bool unique, bool active, bool ascending, double statistics,
    bool system)
{
    typeM = ntIndex;
    uniqueFlagM = unique;
    activeM = active;
    indexTypeM = (ascending ? itAscending : itDescending);
    statisticsM = statistics;
    isSystemM = system;
}
//-----------------------------------------------------------------------------
bool Index::isSystem() const
{
    return isSystemM;
}
//-----------------------------------------------------------------------------
bool Index::isActive()
{
    return activeM;
}
//-----------------------------------------------------------------------------
bool Index::isUnique()
{
    return uniqueFlagM;
}
//-----------------------------------------------------------------------------
double Index::getStatistics()
{
    return statisticsM;
}
//-----------------------------------------------------------------------------
std::vector<wxString> *Index::getSegments()
{
    return &segmentsM;
}
//-----------------------------------------------------------------------------
wxString Index::getFieldsAsString()
{
    wxString retval;
    for (std::vector<wxString>::iterator it = segmentsM.begin(); it != segmentsM.end(); ++it)
    {
        if (!retval.empty())
            retval += wxT(",");
        retval += (*it);
    }
    return retval;
}
//-----------------------------------------------------------------------------
Index::IndexType Index::getIndexType()
{
    return indexTypeM;
}
//-----------------------------------------------------------------------------
void Index::loadDescription()
{
    MetadataItem::loadDescription(
        wxT("select RDB$DESCRIPTION from RDB$INDICES ")
        wxT("where RDB$INDEX_NAME = ?"));
}
//-----------------------------------------------------------------------------
void Index::saveDescription(wxString description)
{
    MetadataItem::saveDescription(
        wxT("update RDB$INDICES set RDB$DESCRIPTION = ? ")
        wxT("where RDB$INDEX_NAME = ?"),
        description);
}
//-----------------------------------------------------------------------------
void Index::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visit(*this);
}
//-----------------------------------------------------------------------------
