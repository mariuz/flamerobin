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

  Contributor(s):
*/

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

#include <fstream>
#include <sstream>
#include <string>

#include "simpleparser.h"
//-----------------------------------------------------------------------------
// returns false if errors occur
bool Parser::stripSql(std::string &sql)
{
    while (true)    // strip quotes and brackets
    {
        std::string::size_type p1, p2;
        p1 = sql.find("/*");
        p2 = sql.find("'");
        if (p1 == std::string::npos && p2 == std::string::npos)    // no more
            break;

        if (p1 != std::string::npos && (p1 < p2 || p2 == std::string::npos))
        {
            p2 = sql.find("*/", p1);
            if (p2 == std::string::npos)
            {
                wxMessageBox(_("Cannot parse sql, please close the comments."), _("Error."), wxOK|wxICON_WARNING);
                return false;
            }
            sql.erase(p1, p2-p1+1);
            continue;
        }

        if (p2 != std::string::npos && (p2 < p1 || p1 == std::string::npos))
        {
            p1 = p2;
            p2 = sql.find("'", p1+1);
            if (p2 == std::string::npos)
            {
                wxMessageBox(_("Cannot parse sql, please close the quotes."), _("Error."), wxOK|wxICON_WARNING);
                return false;
            }
            sql.erase(p1, p2-p1+1);
            continue;
        }
    }
    return true;
}
//-----------------------------------------------------------------------------
// get next token from sql string
// returns number of characters removed
std::string::size_type Parser::nextToken(std::string& in, std::string& out)
{
    if (in.empty())
        return 0;

    std::string::size_type retval = 0;

    const std::string spaces("\n\r\t ");
    const std::string delims(";,");
    const std::string all = spaces + delims;

    std::string::size_type pos, endp;
    if (in.find_first_of(spaces) == 0)        // strip starting spaces
    {
        pos = in.find_first_not_of(spaces);
        if (pos == std::string::npos)
            return 0;
        retval += pos;
        in.erase(0, pos);
    }

    pos = in.find_first_of(delims);            // starts with delims?
    if (pos == 0)
    {
        out = in.substr(0, 1);
        in.erase(0, 1);
        retval += 1;
        return retval;
    }

    endp = in.find_first_of(all);            // starts with other characters
    out = in.substr(0, endp);
    in.erase(0, endp);
    retval += endp;
    return retval;
}
//-----------------------------------------------------------------------------
// gets table names from SELECT sql script
// input: sql statement without SELECT clause, i.e. should start with: "FROM"
//
std::string::size_type Parser::getTableNames(std::vector<std::string>& list, std::string sql)
{
    sql += " ";     // parser needs blank space at end

    std::string::size_type retval = 0;
    std::string keywords[] = {"LEFT", "RIGHT", "INNER", "OUTER", "FULL", "JOIN", "WHERE", "GROUP", "ORDER"};
    std::string ss(sql);
    std::string s;
    retval += nextToken(ss, s);            // remove FROM
    while (true)
    {
        std::string::size_type c = nextToken(ss, s);        // a table: push it to the list
        if (c == 0)
            return retval;
        retval += c;
        list.push_back(s);

        while (true)
        {
            c = nextToken(ss, s);        // alias, comma, keyword (left, outer, inner, right, join, where, group, order), ;
            if (c == 0)
                return retval;
            if (s == ";")
                return retval;
            if (s == ",")    // next is a table
            {
                retval += c;
                break;
            }

            bool is_join = false;
            for (int i=0; i<sizeof(keywords)/sizeof(std::string); ++i)
            {
                if (s == keywords[i])
                {
                    if (i < 5)
                    {
                        int c2 = nextToken(ss, s);
                        if (c2 == 0)
                            break;
                        retval += c2;
                        i=-1;        // start from beginning
                        continue;
                    }
                    else if (s == "JOIN")    // 5
                    {
                        is_join = true;
                        break;
                    }
                    else
                        return retval;    // no more tables
                }
            }
            retval += c;
            if (is_join)
                break;
        }
    }

    // never gets here
    //return retval;
}
//-----------------------------------------------------------------------------
//! removes comments from sql statements, with taking care of single quotes
void Parser::removeComments(std::string& sql, const std::string startComment, const std::string endComment)
{
    using namespace std;
    string::size_type oldpos = 0;
    while (true)
    {
        string::size_type pos = sql.find(startComment, oldpos);
        if (pos == string::npos)
            break;

        string::size_type quote = sql.find("'", oldpos);
        if (quote != string::npos && quote < pos)    // move to the next quote
        {
            oldpos = 1 + sql.find("'", quote+1);    // end quote
            continue;
        }

        oldpos = sql.find(endComment, pos+startComment.length());
        if (oldpos == string::npos)    // unclosed comment
            break;

        sql.erase(pos, oldpos-pos+endComment.length());
        oldpos = pos;
    }
}
//-----------------------------------------------------------------------------
