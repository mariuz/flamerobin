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
#include "sql/MultiStatement.h"
#include "sql/SqlTokenizer.h"
//-----------------------------------------------------------------------------
SingleStatement::SingleStatement(const wxString& sql, bool valid)
    :sqlM(sql), isValidM(valid), typeM(stOther), thirdStringM(wxEmptyString)
{
    SqlTokenType tkn[3] = { tkEOF, tkEOF, tkEOF };
    SqlTokenizer tk(sql);
    for (int i = 0; i < 3; tk.nextToken())
    {
        SqlTokenType stt = tk.getCurrentToken();
        if (stt == tkWHITESPACE || stt == tkCOMMENT)
            continue;
        if (stt == tkEOF)
            break;
        if (i == 2)
            thirdStringM = tk.getCurrentTokenString();
        tkn[i++] = stt;
    }
    if (tkn[0] == kwCOMMIT)
        typeM = stCommit;
    else if (tkn[0] == kwROLLBACK)
        typeM = stRollback;
    else if (tkn[0] == kwSET && tkn[1] == kwTERMINATOR)
        typeM = stSetTerm;
    else if (tkn[0] == kwSET && (tkn[1] == kwAUTO || tkn[1] == kwAUTODDL))
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
    return true;
}
//-----------------------------------------------------------------------------
bool SingleStatement::isValid() const
{
    return isValidM && !sqlM.Strip().IsEmpty();
}
//-----------------------------------------------------------------------------
wxString SingleStatement::getSql() const
{
    if (!isValidM)
        return wxEmptyString;
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
// optionally place the statement offset (start) into "offset" variable
SingleStatement MultiStatement::getStatementAt(int position, int* offset)
{
    oldPosM = searchPosM = 0;
    while (true)
    {
        SingleStatement s = getNextStatement();
        if (!s.isValid() || (int)lastPosM >= position)   // found or at end
        {
            if (offset)
                *offset = oldPosM;
            return s;
        }
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

