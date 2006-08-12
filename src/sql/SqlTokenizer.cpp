/*
Copyright (c) 2004, 2005, 2006 The FlameRobin Development Team

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
#include <map>

#include "sql/SqlTokenizer.h"
//-----------------------------------------------------------------------------
SqlTokenizer::SqlTokenizer()
    : termM(wxT(";"))
{
    init();
}
//-----------------------------------------------------------------------------
SqlTokenizer::SqlTokenizer(const wxString& statement)
    : sqlM(statement), termM(wxT(";"))
{
    init();
}
//-----------------------------------------------------------------------------
SqlTokenType SqlTokenizer::getKeywordTokenType(const wxString& possibleKeyword)
{
    if (possibleKeyword.IsEmpty())
        return tkIDENTIFIER;

    typedef std::map<wxString, SqlTokenType> KeywordMap;
    typedef std::map<wxString, SqlTokenType>::value_type KeywordEntry;

    static KeywordMap keywords;
    if (keywords.empty())
    {
        static const struct {wxChar* name; SqlTokenType type; } entries[] =
        {
            #include "keywordtokens.txt"
            // this element makes for simpler code: all lines in the file can
            // end with a comma, and it is used as the stop marker
            { wxT(""), tkUNKNOWN }
        };
        for (int i = 0; entries[i].type != tkUNKNOWN; i++)
        {
            keywords.insert(KeywordEntry(wxString(entries[i].name).Upper(),
                entries[i].type));
        }
    }
    KeywordMap::const_iterator pos = keywords.find(possibleKeyword);
    if (pos != keywords.end())
        return (*pos).second;
    return tkIDENTIFIER;
}
//-----------------------------------------------------------------------------
SqlTokenType SqlTokenizer::getCurrentToken()
{
    return sqlTokenTypeM;
}
//-----------------------------------------------------------------------------
wxString SqlTokenizer::getCurrentTokenString()
{
    if (sqlTokenStartM && sqlTokenEndM && sqlTokenEndM > sqlTokenStartM)
        return wxString(sqlTokenStartM, sqlTokenEndM);
    return wxEmptyString;
}
//-----------------------------------------------------------------------------
bool SqlTokenizer::isKeywordToken()
{
    return sqlTokenTypeM > tk_KEYWORDS_START_HERE;
}
//-----------------------------------------------------------------------------
void SqlTokenizer::init()
{
    sqlTokenEndM = sqlTokenStartM = sqlM.c_str();
    nextToken();
}
//-----------------------------------------------------------------------------
int SqlTokenizer::getCurrentTokenPosition()
{
    return (sqlTokenStartM - sqlM.c_str());
}
//-----------------------------------------------------------------------------
// same as nextToken, but skips whitespace, comments and parenthesis
bool SqlTokenizer::jumpToken()
{
    while (true)
    {
        if (!nextToken())
            return false;
        SqlTokenType stt = getCurrentToken();
        if (stt == tkPARENOPEN)
        {
            while (nextToken())
            {
                if (getCurrentToken() == tkPARENCLOSE)
                {
                    nextToken();
                    stt = getCurrentToken();
                    break;
                }
            }
        }
        if (stt != tkWHITESPACE && stt != tkCOMMENT)
            break;
    }
    return true;
}
//-----------------------------------------------------------------------------
bool SqlTokenizer::nextToken()
{
    sqlTokenStartM = sqlTokenEndM;
    if (sqlTokenEndM == 0 || *sqlTokenEndM == 0)
    {
        sqlTokenTypeM = tkEOF;
        return false;
    }
    // use wxChar* member to scan
    wxChar c = *sqlTokenEndM;
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
        keywordIdentifierToken();
    else if (c == '"')
        quotedIdentifierToken();
    else if (c == '\'')
        stringToken();
    else if (c == '(')
        symbolToken(tkPARENOPEN);
    else if (c == ')')
        symbolToken(tkPARENCLOSE);
    else if (c == '=')
        symbolToken(tkEQUALS);
    else if (c == '/' && *(sqlTokenEndM + 1) == '*')
        multilineCommentToken();
    else if (c == '-' && *(sqlTokenEndM + 1) == '-')
        singleLineCommentToken();
    else if (wxIsspace(c))
        whitespaceToken();
    else
        defaultToken();
    return true;
}
//-----------------------------------------------------------------------------
void SqlTokenizer::setStatement(const wxString& statement)
{
    sqlM = statement;
    init();
}
//-----------------------------------------------------------------------------
void SqlTokenizer::defaultToken()
{
    if (wxStricmp(sqlTokenStartM, termM.c_str()) == 0)
    {
        sqlTokenTypeM = tkTERM;
        sqlTokenEndM = sqlTokenStartM + termM.Length();
        return;
    }
    sqlTokenTypeM = tkUNKNOWN;
    sqlTokenEndM++;
}
//-----------------------------------------------------------------------------
void SqlTokenizer::keywordIdentifierToken()
{
    sqlTokenTypeM = tkIDENTIFIER;
    // identifier must start with letter 'a'..'z', and may continue with
    // those same letters, numbers and the characters '_' and '$'
    wxChar c = *sqlTokenEndM;
    wxASSERT((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
    do
    {
        sqlTokenEndM++;
        c = *sqlTokenEndM;
    }
    while (c != 0 && ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
        || (c >= '0' && c <= '9') || c == '_' || c == '$'));

    // check whether it's a keyword, and not an identifier
    wxString checkKW(sqlTokenStartM, sqlTokenEndM);
    checkKW.UpperCase();
    SqlTokenType keywordType = getKeywordTokenType(checkKW);
    if (keywordType != tkIDENTIFIER)
        sqlTokenTypeM = keywordType;
}
//-----------------------------------------------------------------------------
void SqlTokenizer::multilineCommentToken()
{
    sqlTokenTypeM = tkCOMMENT;
    wxASSERT(*sqlTokenEndM == '/' && *(sqlTokenEndM + 1) == '*');
    sqlTokenEndM += 2;
    // scan until end of comment, or until "\0" found
    while (*sqlTokenEndM != 0)
    {
        if (*sqlTokenEndM == '*' && *(sqlTokenEndM + 1) == '/')
        {
            sqlTokenEndM += 2;
            break;
        }
        sqlTokenEndM++;
    }
}
//-----------------------------------------------------------------------------
void SqlTokenizer::quotedIdentifierToken()
{
    sqlTokenTypeM = tkIDENTIFIER;
    wxASSERT(*sqlTokenEndM == '"');
    sqlTokenEndM++;
    // scan until after next double quote char, or until "\0" found
    while (*sqlTokenEndM != 0)
    {
        if (*sqlTokenEndM == '"')
        {
            sqlTokenEndM++;
            // scan over escaped quotes
            if (*sqlTokenEndM != '"')
                break;
        }
        sqlTokenEndM++;
    }
}
//-----------------------------------------------------------------------------
void SqlTokenizer::singleLineCommentToken()
{
    sqlTokenTypeM = tkCOMMENT;
    wxASSERT(*sqlTokenEndM == '-' && *(sqlTokenEndM + 1) == '-');
    sqlTokenEndM += 2;
    // scan until end of line, or until "\0" found
    while (*sqlTokenEndM != 0)
    {
        if (*sqlTokenEndM == '\n')
        {
            sqlTokenEndM++;
            if (*sqlTokenEndM == '\r')
                sqlTokenEndM++;
            break;
        }
        sqlTokenEndM++;
    }
}
//-----------------------------------------------------------------------------
void SqlTokenizer::stringToken()
{
    sqlTokenTypeM = tkSTRING;
    wxASSERT(*sqlTokenEndM == '\'');
    sqlTokenEndM++;
    // scan until after next quote char, or until "\0" found
    while (*sqlTokenEndM != 0)
    {
        if (*sqlTokenEndM == '\'')
        {
            sqlTokenEndM++;
            // scan over escaped quotes
            if (*sqlTokenEndM != '\'')
                break;
        }
        sqlTokenEndM++;
    }
}
//-----------------------------------------------------------------------------
inline void SqlTokenizer::symbolToken(SqlTokenType type)
{
    sqlTokenTypeM = type;
    sqlTokenEndM++;
}
//-----------------------------------------------------------------------------
void SqlTokenizer::whitespaceToken()
{
    sqlTokenTypeM = tkWHITESPACE;
    wxASSERT(wxIsspace(*sqlTokenEndM));
    sqlTokenEndM++;
    // scan until non-whitespace, or until "\0" found
    while (*sqlTokenEndM != 0 && wxIsspace(*sqlTokenEndM))
        sqlTokenEndM++;
}
//-----------------------------------------------------------------------------
