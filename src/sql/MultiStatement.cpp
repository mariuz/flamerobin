/*
Copyright (c) 2006 The FlameRobin Development Team

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
//-----------------------------------------------------------------------------
// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "MultiStatement.h"
//-----------------------------------------------------------------------------
MultiStatement::MultiStatement(const wxString& sql, const wxString& terminator)
    :sqlM(source), terminatorM(terminator), atEndM(false)
{
    oldPosM = searchPosM = 0;
}
//-----------------------------------------------------------------------------
bool MultiStatement::getNextStatement(wxString& sql)
{
    if (atEndM)    // end marked in previous iteration
        return false;

    oldPosM = searchPosM;
    while (true)
    {
        wxString::size_type pos = sqlM.find(terminatorM, searchPosM);
        wxString::size_type quote = sqlM.find(wxT("'"), searchPosM);
        wxString::size_type comment1 = sqlM.find(wxT("/*"), searchPosM);
        wxString::size_type comment2 = sqlM.find(wxT("--"), searchPosM);

        // check if terminator is maybe inside quotes or comments
        if (pos != wxString::npos)            // terminator found
        {
            // find the closest (check for quotes first)
            if (quote != wxString::npos && quote < pos &&
                (comment1 == wxString::npos || quote < comment1) &&
                (comment2 == wxString::npos || quote < comment2))
            {
                searchPosM = sqlM.find(wxT("'"), quote+1);     // end quote
                if (searchPosM++ != wxString::npos)
                    continue;
                pos = wxString::npos;
            }

            // check for comment1
            if (pos != wxString::npos &&
                comment1 != wxString::npos && comment1 < pos &&
                (comment2 == wxString::npos || comment1 < comment2))
            {
                searchPosM = sqlM.find(wxT("*/"), comment1 + 1); // end comment
                if (searchPosM++ != wxString::npos)
                    continue;
                pos = wxString::npos;
            }

            // check for comment2
            if (pos != wxString::npos &&
                comment2 != wxString::npos && comment2 < pos)
            {
                searchPosM = sqlM.find(wxT("\n"), comment2 + 1); // end comment
                if (searchPosM++ != wxString::npos)
                    continue;
                pos = wxString::npos;
            }
        }

        lastPosM = (pos == wxString::npos ? sqlM.length() : pos);
        sql = sqlM.substr(oldPosM, lastPosM - oldPosM);

        searchPosM = lastPosM + terminatorM.length();
        if (pos == wxString::npos)      // last statement
            atEndM = true;               // mark the end (for next call)
        return true;
    }
}
//-----------------------------------------------------------------------------
bool MultiStatement::getStatementAt(wxString& sql, int position)
{
    oldPosM = searchPosM = 0;
    while (true)
    {
        if (!getNextStatement(sql))
            return false;
        if (lastPosM >= position)
            return true;
    }
}
//-----------------------------------------------------------------------------
void MultiStatement::setTerminator(const wxString& newTerm)
{
    terminatorM = newTerm;
}
//-----------------------------------------------------------------------------
wxString MultiStatement::getTerminator() const
{
    return terminatorM;
}
//-----------------------------------------------------------------------------
int MultiStatement::getStart() const
{
    return oldPosM;
}
//-----------------------------------------------------------------------------
int MultiStatement::getEnd() const
{
    return lastPosM;
}
//-----------------------------------------------------------------------------

