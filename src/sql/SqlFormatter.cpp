/*
  Copyright (c) 2004-2026 The FlameRobin Development Team

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

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "sql/SqlFormatter.h"
#include "sql/SqlTokenizer.h"
#include "config/Config.h"
#include <vector>
#include <algorithm>

namespace {

struct ParenInfo {
    bool isSubquery;
    int indentLevelAtStart;
};

SqlTokenType peekNextToken(SqlTokenizer tokenizer, wxString& outTokenText)
{
    while (tokenizer.nextToken())
    {
        SqlTokenType type = tokenizer.getCurrentToken();
        if (type != tkWHITESPACE && type != tkCOMMENT)
        {
            outTokenText = tokenizer.getCurrentTokenString();
            return type;
        }
    }
    outTokenText = wxEmptyString;
    return tkEOF;
}

bool isOperatorToken(SqlTokenType type, const wxString& text)
{
    if (type == tkEQUALS || type == kwNAMED_ARG_ASSIGN || type == kwBIND_PARAM || type == kwCONCATENATE)
        return true;
    if (text == L"+" || text == L"-" || text == L"*" || text == L"/" || text == L"<" || text == L">" || text == L"<=" || text == L">=" || text == L"<>" || text == L"!=")
        return true;
    return false;
}

} // namespace

/*static*/
wxString SqlFormatter::format(const wxString& sql, int odsMajor, int odsMinor)
{
    int indentSpaces = config().get("FormatterIndentSpaces", 4);
    int keywordCase = config().get("FormatterKeywordCase", 0);
    bool oneColumnPerLine = config().get("FormatterOneColumnPerLine", true);

    SqlTokenizer tokenizer(sql);
    wxString formattedSql;

    int indentLevel = 0;
    bool startOfLine = true;
    bool spaceNeeded = false;
    bool inSelectList = false;
    int selectParenLevel = 0;
    bool inBetween = false;

    std::vector<ParenInfo> parenStack;
    
    SqlTokenType lastToken = tkEOF;
    wxString lastTokenText = wxEmptyString;

    auto addNewLine = [&]() {
        if (!formattedSql.IsEmpty() && formattedSql.Last() != '\n')
        {
            formattedSql += L"\n";
            startOfLine = true;
            spaceNeeded = false;
        }
    };

    auto indent = [&]() {
        if (startOfLine)
        {
            formattedSql += wxString(L' ', static_cast<size_t>(indentSpaces) * static_cast<size_t>(indentLevel));
            startOfLine = false;
        }
    };

    bool hasToken = (tokenizer.getCurrentToken() != tkEOF);
    while (hasToken)
    {
        SqlTokenType type = tokenizer.getCurrentToken();
        wxString text = tokenizer.getCurrentTokenString();

        if (type == tkWHITESPACE)
        {
            hasToken = tokenizer.nextToken();
            continue;
        }

        if (type == tkCOMMENT)
        {
            if (!startOfLine && spaceNeeded)
            {
                formattedSql += L" ";
            }
            indent();
            formattedSql += text;
            startOfLine = false;
            
            if (text.StartsWith(L"--"))
            {
                addNewLine();
            }
            else
            {
                spaceNeeded = true;
            }
            hasToken = tokenizer.nextToken();
            continue;
        }

        bool isKwd = SqlTokenizer::isKeyword(text, odsMajor, odsMinor);

        wxString formattedText = text;
        if (isKwd)
        {
            if (keywordCase == 0)
                formattedText = text.Upper();
            else if (keywordCase == 1)
                formattedText = text.Lower();
        }

        wxString nextTokenText;
        SqlTokenType nextType = peekNextToken(tokenizer, nextTokenText);
        wxString nextTokenTextUpper = nextTokenText.Upper();
        wxString formattedTextUpper = formattedText.Upper();

        bool shouldStartNewLine = false;

        if (isKwd)
        {
            if (formattedTextUpper == L"SELECT" || formattedTextUpper == L"FROM" || 
                formattedTextUpper == L"WHERE" || formattedTextUpper == L"GROUP" || 
                formattedTextUpper == L"HAVING" || formattedTextUpper == L"ORDER" || 
                formattedTextUpper == L"UNION" || formattedTextUpper == L"JOIN" || 
                formattedTextUpper == L"LEFT" || formattedTextUpper == L"RIGHT" || 
                formattedTextUpper == L"INNER" || formattedTextUpper == L"OUTER" || 
                formattedTextUpper == L"CROSS" || formattedTextUpper == L"INSERT" || 
                formattedTextUpper == L"UPDATE" || formattedTextUpper == L"DELETE" || 
                formattedTextUpper == L"SET" || formattedTextUpper == L"VALUES" || 
                formattedTextUpper == L"INTO" || formattedTextUpper == L"CREATE" || 
                formattedTextUpper == L"ALTER" || formattedTextUpper == L"DROP" || 
                formattedTextUpper == L"RECREATE")
            {
                shouldStartNewLine = true;

                wxString lastUpper = lastTokenText.Upper();
                if (formattedTextUpper == L"INTO" && lastUpper == L"INSERT")
                    shouldStartNewLine = false;
                else if (formattedTextUpper == L"FROM" && lastUpper == L"DELETE")
                    shouldStartNewLine = false;
                else if (formattedTextUpper == L"BY" && (lastUpper == L"GROUP" || lastUpper == L"ORDER"))
                    shouldStartNewLine = false;
                else if (formattedTextUpper == L"JOIN" && (lastUpper == L"LEFT" || lastUpper == L"RIGHT" || 
                         lastUpper == L"INNER" || lastUpper == L"OUTER" || lastUpper == L"CROSS" || lastUpper == L"NATURAL"))
                    shouldStartNewLine = false;
                else if (formattedTextUpper == L"OUTER" && (lastUpper == L"LEFT" || lastUpper == L"RIGHT"))
                    shouldStartNewLine = false;
            }
            else if ((formattedTextUpper == L"AND" || formattedTextUpper == L"OR") && !inBetween)
            {
                if (parenStack.empty() || parenStack.back().isSubquery)
                {
                    shouldStartNewLine = true;
                }
            }
        }

        if (shouldStartNewLine)
        {
            if (inSelectList && (formattedTextUpper == L"FROM" || formattedTextUpper == L"INTO" || 
                                 formattedTextUpper == L"WHERE" || formattedTextUpper == L"GROUP" || 
                                 formattedTextUpper == L"HAVING" || formattedTextUpper == L"ORDER" || 
                                 formattedTextUpper == L"UNION"))
            {
                if (oneColumnPerLine)
                {
                    indentLevel = std::max(0, indentLevel - 1);
                }
                inSelectList = false;
            }

            addNewLine();
        }

        if (!startOfLine && spaceNeeded)
        {
            bool noSpaceBeforeThis = (type == tkCOMMA || type == tkPARENCLOSE || text == L";" || text == L"." || text == L":");
            if (type == tkPARENOPEN)
            {
                if (lastToken == tkIDENTIFIER)
                {
                    noSpaceBeforeThis = true;
                }
                else if (lastToken > tk_KEYWORDS_START_HERE)
                {
                    wxString lastUpper = lastTokenText.Upper();
                    if (lastUpper != L"IN" && lastUpper != L"VALUES" && lastUpper != L"USING" && 
                        lastUpper != L"ON" && lastUpper != L"OVER" && lastUpper != L"WITH" && 
                        lastUpper != L"AS" && lastUpper != L"AND" && lastUpper != L"OR" && 
                        lastUpper != L"NOT" && lastUpper != L"FROM" && lastUpper != L"WHERE")
                    {
                        noSpaceBeforeThis = true;
                    }
                }
            }

            if (!noSpaceBeforeThis)
            {
                formattedSql += L" ";
            }
        }

        indent();

        formattedSql += formattedText;

        if (formattedTextUpper == L"BETWEEN")
        {
            inBetween = true;
        }
        else if (formattedTextUpper == L"AND" && inBetween)
        {
            inBetween = false;
        }

        if (formattedTextUpper == L"SELECT")
        {
            inSelectList = true;
            selectParenLevel = 0;
            if (oneColumnPerLine)
            {
                indentLevel++;
                addNewLine();
            }
        }

        if (type == tkPARENOPEN)
        {
            if (inSelectList)
            {
                selectParenLevel++;
            }
            bool isSubquery = (nextTokenTextUpper == L"SELECT" || nextTokenTextUpper == L"WITH");
            parenStack.push_back(ParenInfo{ isSubquery, indentLevel });
            
            if (isSubquery)
            {
                addNewLine();
                indentLevel++;
            }
        }
        else if (type == tkPARENCLOSE)
        {
            if (inSelectList)
            {
                selectParenLevel = std::max(0, selectParenLevel - 1);
            }
            if (!parenStack.empty())
            {
                ParenInfo info = parenStack.back();
                parenStack.pop_back();
                if (info.isSubquery)
                {
                    indentLevel = info.indentLevelAtStart;
                    if (formattedSql.Last() == ')')
                    {
                        formattedSql.RemoveLast();
                    }
                    addNewLine();
                    indent();
                    formattedSql += L")";
                }
            }
        }

        if (type == tkCOMMA && inSelectList && selectParenLevel == 0)
        {
            if (oneColumnPerLine)
            {
                addNewLine();
            }
        }

        spaceNeeded = (type != tkPARENOPEN && text != L"." && text != L":");

        if ((text == L"+" || text == L"-") && 
            (lastToken == tkPARENOPEN || lastToken == tkCOMMA || lastToken == tkEQUALS || 
             isOperatorToken(lastToken, lastTokenText) || lastTokenText.IsEmpty()))
        {
            spaceNeeded = false;
        }

        if (type == tkTERM || text == L";")
        {
            addNewLine();
        }

        lastToken = type;
        lastTokenText = text;

        hasToken = tokenizer.nextToken();
    }

    while (!formattedSql.IsEmpty() && (formattedSql.Last() == ' ' || formattedSql.Last() == '\n' || formattedSql.Last() == '\r'))
    {
        formattedSql.RemoveLast();
    }

    return formattedSql;
}
