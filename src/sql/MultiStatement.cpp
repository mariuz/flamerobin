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

#include <sstream>
#include "core/StringUtils.h"
#include "sql/SimpleParser.h"
#include "MultiStatement.h"
//-----------------------------------------------------------------------------
SingleStatement::SingleStatement(const wxString& sql, bool valid)
    :sqlM(sql), isValidM(valid), typeM(stOther), thirdStringM(wxEmptyString)
{
    // copied from ExecuteSqlFrame
    wxString sqlc(sql);
    sqlc.erase(sqlc.find_last_not_of(wxT("\n\r\t ")) + 1);    // right-trim
    std::stringstream strstrm;              // Search and intercept
    std::string first, second, third;       // SET TERM and COMMIT statements
    wxString strippedSql(sqlc);
    SimpleParser::removeComments(strippedSql, wxT("/*"), wxT("*/"));
    SimpleParser::removeComments(strippedSql, wxT("--"), wxT("\n"));
    strstrm << wx2std(strippedSql.Upper());
    strstrm >> first;
    strstrm >> second;
    strstrm >> third;
    thirdStringM = std2wx(third);
    if (first == "COMMIT")
        typeM = stCommit;
    else if (first == "ROLLBACK")
        typeM = stRollback;
    else if (first == "SET" && (second == "TERM" || second == "TERMINATOR"))
        typeM = stSetTerm;
    else if (first == "SET" && (second == "AUTO" || second == "AUTODDL"))
        typeM = stSetAutoDDL;
    else
        typeM = stOther;
}
//-----------------------------------------------------------------------------
bool SingleStatement::isCommitStatement() const
{
    return typeM == stCommit;
}
//-----------------------------------------------------------------------------
bool SingleStatement::isRollbackStatement() const
{
    return typeM == stRollback;
}
//-----------------------------------------------------------------------------
bool SingleStatement::isSetTermStatement(wxString& newTerm) const
{
    if (typeM != stSetTerm)
        return false;

    newTerm = thirdStringM;
    return true;
}
//-----------------------------------------------------------------------------
bool SingleStatement::isSetAutoDDLStatement(wxString& newSetting) const
{
    if (typeM != stSetAutoDDL)
        return false;

    newSetting = thirdStringM;
}
//-----------------------------------------------------------------------------
bool SingleStatement::isValid() const
{
    return isValidM && !sqlM.Strip().IsEmpty();
}
//-----------------------------------------------------------------------------
wxString SingleStatement::getSql() const
{
    return sqlM;
}
//-----------------------------------------------------------------------------
MultiStatement::MultiStatement(const wxString& sql, const wxString& terminator)
    :sqlM(sql), terminatorM(terminator), atEndM(false)
{
    oldPosM = searchPosM = 0;
}
//-----------------------------------------------------------------------------
SingleStatement MultiStatement::getNextStatement()
{
    if (atEndM)    // end marked in previous iteration
    {
        SingleStatement is(wxEmptyString, false);   // false = invalid
        return is;
    }

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
        SingleStatement ss(sqlM.Mid(oldPosM, lastPosM - oldPosM));
        searchPosM = lastPosM + terminatorM.length();
        if (pos == wxString::npos)      // last statement
            atEndM = true;              // mark the end (for next call)

        wxString newTerm;                   // change terminator
        if (ss.isSetTermStatement(newTerm))
        {
            terminatorM = newTerm;
            if (newTerm.IsEmpty())  // the caller should decide what to do as
                return ss;          // we don't want to popup msgbox from here
            if (atEndM)             // terminator is the last statement
            {
                SingleStatement is(wxEmptyString, false);   // false = invalid
                return is;
            }
            oldPosM = searchPosM;
            continue;
        }
        return ss;
    }
}
//-----------------------------------------------------------------------------
SingleStatement MultiStatement::getStatementAt(int position)
{
    oldPosM = searchPosM = 0;
    while (true)
    {
        SingleStatement s = getNextStatement();
        if (!s.isValid() || lastPosM >= position)   // found or at end
            return s;
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

