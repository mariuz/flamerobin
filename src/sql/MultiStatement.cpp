/*
  Copyright (c) 2004-2012 The FlameRobin Development Team

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

#include <algorithm>

#include "sql/MultiStatement.h"
#include "sql/SqlTokenizer.h"
//-----------------------------------------------------------------------------
SingleStatement::SingleStatement()
    : isValidM(false), typeM(stOther)
{
}
//-----------------------------------------------------------------------------
SingleStatement::SingleStatement(const wxString& sql)
    : sqlM(sql), isValidM(true), typeM(stOther)
{
    SqlTokenizer tk(sql);
    for (int i = 0; i < 3; tk.nextToken())
    {
        SqlTokenType stt = tk.getCurrentToken();
        if (stt == tkWHITESPACE || stt == tkCOMMENT)
            continue;
        if (stt == tkEOF)
            break;

        switch (i)
        {
            case 0:
                // exit as soon as possible if token type not recognized
                if (stt != kwSET)
                {
                    if (stt == kwCOMMIT)
                        typeM = stCommit;
                    else if (stt == kwROLLBACK)
                        typeM = stRollback;
                    return;
                }
                break;
            case 1:
                if (stt == kwTERMINATOR)
                    typeM = stSetTerm;
                else if (stt == kwAUTO || stt == kwAUTODDL)
                    typeM = stSetAutoDDL;
                else
                    return;
                break;
            case 2:
                thirdStringM = tk.getCurrentTokenString();
                break;
            default:
                break;
        }
        ++i;
    }
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
//! MultiStatement class
MultiStatement::MultiStatement(const wxString& sql, const wxString& terminator)
    : sqlM(sql), terminatorM(terminator), atEndM(false)
{
    oldPosM = searchPosM = sqlM.begin();
}
//-----------------------------------------------------------------------------
SingleStatement MultiStatement::getNextStatement()
{
    if (atEndM)    // end marked in previous iteration
        return SingleStatement();

    wxString interesting(wxT("'/-"));
    if (!terminatorM.empty())
        interesting += *terminatorM.begin();
    wxString::const_iterator searchEnd = sqlM.end();

    oldPosM = searchPosM;
    while (true)
    {
        lastPosM = searchEnd;
        wxString::const_iterator p = std::find_first_of(searchPosM, searchEnd,
            interesting.begin(), interesting.end());
        if (p == searchEnd)
        {
            atEndM = true;
        }
        else
        {
            // scan over embedded quotes
            if (*p == '\'')
            {
                searchPosM = std::find(p + 1, searchEnd, '\'');
                if (searchPosM != searchEnd)
                {
                    ++searchPosM;
                    continue;
                }
            }
            // scan over single-line comment
            else if (*p == '-' && p + 1 != searchEnd && *(p + 1) == '-')
            {
                searchPosM = std::find(p + 2, searchEnd, '\n');
                if (searchPosM != searchEnd)
                {
                    ++searchPosM;
                    continue;
                }
            }
            // scan over multi-line comment
            else if (*p == '/' && p + 1 != searchEnd && *(p + 1) == '*')
            {
                wxString commentEnd(wxT("*/"));
                searchPosM = std::search(p + 2, searchEnd, commentEnd.begin(),
                    commentEnd.end());
                if (searchPosM != searchEnd)
                {
                    searchPosM += commentEnd.size();
                    continue;
                }
            }
            // ignore partial matches with terminator
            else
            {
                wxString::const_iterator p1 = terminatorM.begin();
                wxString::const_iterator p2 = p;
                while (p1 != terminatorM.end() && p2 != interesting.end())
                {
                    if (*p1 != *p2)
                        break;
                    p1++;
                    p2++;
                }
                if (p1 != terminatorM.end())
                {
                    searchPosM = p + 1;
                    continue;
                }
                lastPosM = p;
                searchPosM = p + terminatorM.size();
            }
            if (searchPosM == searchEnd)
                atEndM = true;
        }

        wxString sql(oldPosM, lastPosM);
        SingleStatement ss(sql);

        wxString newTerm;                   // change terminator
        if (ss.isSetTermStatement(newTerm))
        {
            terminatorM = newTerm;
            if (newTerm.empty())    // the caller should decide what to do as
                return ss;          // we don't want to popup msgbox from here
            if (atEndM)             // terminator is the last statement
                return SingleStatement();

            interesting = wxT("'/-");
            interesting += *terminatorM.begin();
            oldPosM = searchPosM;
            continue;
        }
        return ss;
    }
}
//-----------------------------------------------------------------------------
SingleStatement MultiStatement::getStatementAt(int position, int& offset)
{
    oldPosM = searchPosM = sqlM.begin();
    while (true)
    {
        SingleStatement s = getNextStatement();
        if (!s.isValid() || lastPosM - sqlM.begin() >= position)
        {
            offset = oldPosM - sqlM.begin();
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
    return oldPosM - sqlM.begin();
}
//-----------------------------------------------------------------------------
int MultiStatement::getEnd() const
{
    return lastPosM - sqlM.begin();
}
//-----------------------------------------------------------------------------
