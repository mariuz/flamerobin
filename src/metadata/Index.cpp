/*
  Copyright (c) 2004-2015 The FlameRobin Development Team

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

#include "metadata/Index.h"
#include "metadata/MetadataItemVisitor.h"

Index::Index(bool unique, bool active, bool ascending, double statistics,
        bool system, wxString expression)
    : MetadataItem(ntIndex), isSystemM(system), uniqueFlagM(unique),
        activeM(active), indexTypeM(ascending ? itAscending : itDescending),
        statisticsM(statistics), segmentsM(), expressionM(expression)
{
}

bool Index::isSystem() const
{
    return isSystemM;
}

bool Index::isActive() const
{
    return activeM;
}

bool Index::isUnique() const
{
    return uniqueFlagM;
}

double Index::getStatistics()
{
    return statisticsM;
}

std::vector<wxString> *Index::getSegments()
{
    return &segmentsM;
}

wxString Index::getFieldsAsString()
{
    if (!expressionM.IsEmpty())
        return expressionM;
    else
    {
        wxString retval;
        for (std::vector<wxString>::iterator it = segmentsM.begin(); 
            it != segmentsM.end(); ++it)
        {
            if (!retval.empty())
                retval += ", ";
            retval += (*it);
        }
        return retval;
    }
}

Index::IndexType Index::getIndexType()
{
    return indexTypeM;
}

wxString Index::getExpression() const
{
    return expressionM;
}

void Index::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitIndex(*this);
}

