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

#include <wx/textbuf.h>

#include "sql/SelectStatement.h"
#include "sql/SqlTokenizer.h"

SelectStatement::SelectStatement(const wxString& sql)
{
    setStatement(sql);
}

bool SelectStatement::isValidSelectStatement()
{
    return (posSelectM != -1 && posFromM != -1);
}

void SelectStatement::setStatement(const wxString& sql)
{
    sqlM = sql;
    posSelectM = posFromM = posFromEndM = -1;
    tokenizerM.setStatement(sql);

    // find SELECT and FROM position
    int paren = 0;
    do
    {
        SqlTokenType stt = tokenizerM.getCurrentToken();
        if (stt == tkEOF)
            break;

        // check parenthesis to ignore nested select statements
        if (stt == tkPARENOPEN)
            paren++;
        if (stt == tkPARENCLOSE && paren > 0)
            paren--;

        if (paren == 0)
        {
            if (stt == kwSELECT)
                posSelectM = tokenizerM.getCurrentTokenPosition();
            if (posSelectM != -1 && stt == kwFROM)
                posFromM = tokenizerM.getCurrentTokenPosition();
            if (posFromM != -1 && (stt == kwWHERE || stt == kwGROUP
                || stt == kwORDER || stt == kwPLAN || stt == kwROWS))
            {
                posFromEndM = tokenizerM.getCurrentTokenPosition();
            }
        }
    }
    while (tokenizerM.nextToken());

    if (posSelectM != -1 && posFromM != -1 && posFromEndM == -1)
        posFromEndM = tokenizerM.getCurrentTokenPosition();
}

wxString SelectStatement::getStatement()
{
    return sqlM;
}

// start from SELECT position and look for COMMA
void SelectStatement::getColumns(std::vector<wxString>& columns)
{
    // reset to tokenizer to SELECT position (inefficient, but what to do)
    tokenizerM.setStatement(sqlM);
    while (posSelectM != tokenizerM.getCurrentTokenPosition())
        if (!tokenizerM.nextToken())
            return; // throw?

    wxString columnName;
    while (tokenizerM.jumpToken(true /* skip parenthesis */))
    {
        SqlTokenType stt = tokenizerM.getCurrentToken();
        if (stt == kwFROM)
            break;  // we're done here, no more tables
        if (columnName.IsEmpty() && stt == tkIDENTIFIER)
        {
            columnName = tokenizerM.getCurrentTokenString();
            columns.push_back(columnName);
            continue;
        }
        if (stt == tkCOMMA)
            columnName.Clear();
    }
}

/*
SELECT ...
FROM t1,t2 alias,t3
JOIN t4 ON x=y and z=g and ()
LEFT JOIN t5 alias2 ON ...
*/
// start from FROM position and look for either COMMA or JOIN
// FIXME: support table aliases should be added
void SelectStatement::getTables(std::vector<wxString>& tables)
{
    // reset to tokenizer to FROM position (inefficient, but what to do)
    tokenizerM.setStatement(sqlM);
    while (posFromM != tokenizerM.getCurrentTokenPosition())
        if (!tokenizerM.nextToken())
            return; // throw?

    // find SELECT and FROM position
    wxString tableName;
    while (tokenizerM.jumpToken(true /* skip parenthesis */))
    {
        SqlTokenType stt = tokenizerM.getCurrentToken();
        if (stt == kwWHERE || stt == kwGROUP || stt == kwORDER
            || stt == kwPLAN || stt == kwROWS)
        {
            break;  // we're done here, no more tables
        }
        if (tableName.IsEmpty() && stt == tkIDENTIFIER)
        {
            tableName = tokenizerM.getCurrentTokenString();
            tables.push_back(tableName);
            continue;
        }
        if (stt == tkCOMMA || stt == kwJOIN)
            tableName.Clear();
    }
}

void SelectStatement::add(const wxString& toAdd, int position)
{
    wxString s(sqlM.Left(position));

    // always add extra space in case we're adding to the end of the
    // statement
    s += wxTextBuffer::GetEOL() + toAdd;

    s += sqlM.Mid(position);
    setStatement(s);
}

void SelectStatement::addTable(const wxString& name, const wxString& joinType,
    const wxString& joinList)
{
    if (joinType == "CARTESIAN")
    {
        std::vector<wxString> s;
        getTables(s);
        if (s.empty())
            add(name + " ", posFromM + 5); // 5 = strlen("FROM ");
        else
            add(name + ", ", posFromM + 5);    // 5 = strlen("FROM ");
    }
    else
    {
        add(joinType + " " + name + " ON " + joinList + " ",
            posFromEndM);
    }
}

void SelectStatement::addColumn(const wxString& columnList)
{
    std::vector<wxString> s;
    getColumns(s);
    if (s.empty())
        add(columnList + " ", posSelectM + 7); // 7 = strlen("SELECT ");
    else
        add(columnList + ", ", posSelectM + 7);
}

// covers only the most basic cases
void SelectStatement::orderBy(int column)
{
    // look for ORDER BY.
    tokenizerM.setStatement(sqlM);
    bool hasOrder = false;
    int pos = -1;
    while (tokenizerM.jumpToken(true /* skip parenthesis */))
    {
        SqlTokenType stt = tokenizerM.getCurrentToken();
        if (stt == kwORDER)
            hasOrder = true;
        if (hasOrder && stt == kwBY)
        {
            pos = tokenizerM.getCurrentTokenPosition() + 2;
            break;
        }
    }
    wxString coltoadd;
    coltoadd.Printf("%d", column);
    // if !found, add ORDER BY at the end
    if (pos == -1)
    {
        sqlM += wxTextBuffer::GetEOL() + wxString("ORDER BY ")
            + coltoadd;
    }
    else
    {
        // Are we already using that column?
        tokenizerM.jumpToken(true);
        if (coltoadd != tokenizerM.getCurrentTokenString())
        {
            add(coltoadd + ", ", pos); // No. Add it
            return;
        }

        // Yes. Remove ASC or DESC and add the opposite
        tokenizerM.jumpToken(true);
        SqlTokenType stt = tokenizerM.getCurrentToken();
        int p = tokenizerM.getCurrentTokenPosition();
        wxString s = tokenizerM.getCurrentTokenString();
        if (stt == kwDESCENDING || stt == kwASCENDING) // remove
            sqlM.Remove(p, s.Length());
        if (stt != kwDESCENDING)    // add desc if there wasn't
            add("DESC ", p);
    }
}

