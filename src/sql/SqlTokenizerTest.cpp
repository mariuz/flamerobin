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

#include <iostream>

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "sql/SqlTokenizer.h"

namespace
{

bool check(bool condition, const char* testName)
{
    if (condition)
        return true;
    std::cerr << testName << " failed.\n";
    return false;
}

bool checkStr(const wxString& actual, const wxString& expected,
    const char* testName)
{
    if (actual == expected)
        return true;
    std::string exp(expected.mb_str()), act(actual.mb_str());
    std::cerr << testName << " failed.\n"
        << "  Expected: [" << exp << "]\n"
        << "  Actual:   [" << act << "]\n";
    return false;
}

bool checkToken(SqlTokenType actual, SqlTokenType expected,
    const char* testName)
{
    if (actual == expected)
        return true;
    std::cerr << testName << " failed: got token " << (int)actual
        << " but expected " << (int)expected << ".\n";
    return false;
}

} // namespace

int main()
{
    bool ok = true;

    // Test 1: empty string -> tkEOF
    {
        SqlTokenizer t("");
        ok = checkToken(t.getCurrentToken(), tkEOF, "empty: tkEOF") && ok;
        ok = check(!t.isKeywordToken(), "empty: not keyword") && ok;
    }

    // Test 2: uppercase SELECT keyword
    {
        SqlTokenizer t("SELECT");
        ok = checkToken(t.getCurrentToken(), kwSELECT,
            "SELECT: kwSELECT") && ok;
        ok = check(t.isKeywordToken(), "SELECT: isKeywordToken") && ok;
        ok = checkStr(t.getCurrentTokenString(), "SELECT",
            "SELECT: tokenString") && ok;
    }

    // Test 3: lowercase select keyword (case-insensitive matching)
    {
        SqlTokenizer t("select");
        ok = checkToken(t.getCurrentToken(), kwSELECT,
            "select lowercase: kwSELECT") && ok;
    }

    // Test 4: identifier that is not a keyword
    {
        SqlTokenizer t("MYTABLE");
        ok = checkToken(t.getCurrentToken(), tkIDENTIFIER,
            "MYTABLE: tkIDENTIFIER") && ok;
        ok = check(!t.isKeywordToken(), "MYTABLE: not keyword") && ok;
        ok = checkStr(t.getCurrentTokenString(), "MYTABLE",
            "MYTABLE: tokenString") && ok;
    }

    // Test 5: quoted identifier
    {
        SqlTokenizer t("\"MY TABLE\"");
        ok = checkToken(t.getCurrentToken(), tkIDENTIFIER,
            "quoted id: tkIDENTIFIER") && ok;
    }

    // Test 6: string literal
    {
        SqlTokenizer t("'hello world'");
        ok = checkToken(t.getCurrentToken(), tkSTRING,
            "string literal: tkSTRING") && ok;
    }

    // Test 7: whitespace
    {
        SqlTokenizer t("   ");
        ok = checkToken(t.getCurrentToken(), tkWHITESPACE,
            "whitespace: tkWHITESPACE") && ok;
    }

    // Test 8: single-line comment
    {
        SqlTokenizer t("-- this is a comment\n");
        ok = checkToken(t.getCurrentToken(), tkCOMMENT,
            "line comment: tkCOMMENT") && ok;
    }

    // Test 9: block comment
    {
        SqlTokenizer t("/* block comment */");
        ok = checkToken(t.getCurrentToken(), tkCOMMENT,
            "block comment: tkCOMMENT") && ok;
    }

    // Test 10: CREATE keyword
    {
        SqlTokenizer t("CREATE");
        ok = checkToken(t.getCurrentToken(), kwCREATE,
            "CREATE: kwCREATE") && ok;
    }

    // Test 11: ALTER keyword
    {
        SqlTokenizer t("ALTER");
        ok = checkToken(t.getCurrentToken(), kwALTER,
            "ALTER: kwALTER") && ok;
    }

    // Test 12: DROP keyword
    {
        SqlTokenizer t("DROP");
        ok = checkToken(t.getCurrentToken(), kwDROP, "DROP: kwDROP") && ok;
    }

    // Test 13: TABLE keyword
    {
        SqlTokenizer t("TABLE");
        ok = checkToken(t.getCurrentToken(), kwTABLE,
            "TABLE: kwTABLE") && ok;
    }

    // Test 14: INSERT, UPDATE, DELETE keywords
    {
        SqlTokenizer t("INSERT");
        ok = checkToken(t.getCurrentToken(), kwINSERT,
            "INSERT: kwINSERT") && ok;
    }
    {
        SqlTokenizer t("UPDATE");
        ok = checkToken(t.getCurrentToken(), kwUPDATE,
            "UPDATE: kwUPDATE") && ok;
    }
    {
        SqlTokenizer t("DELETE");
        ok = checkToken(t.getCurrentToken(), kwDELETE,
            "DELETE: kwDELETE") && ok;
    }

    // Test 15: WHERE keyword
    {
        SqlTokenizer t("WHERE");
        ok = checkToken(t.getCurrentToken(), kwWHERE,
            "WHERE: kwWHERE") && ok;
    }

    // Test 16: parentheses
    {
        SqlTokenizer t("(");
        ok = checkToken(t.getCurrentToken(), tkPARENOPEN,
            "open paren: tkPARENOPEN") && ok;
    }
    {
        SqlTokenizer t(")");
        ok = checkToken(t.getCurrentToken(), tkPARENCLOSE,
            "close paren: tkPARENCLOSE") && ok;
    }

    // Test 17: comma and equals
    {
        SqlTokenizer t(",");
        ok = checkToken(t.getCurrentToken(), tkCOMMA,
            "comma: tkCOMMA") && ok;
    }
    {
        SqlTokenizer t("=");
        ok = checkToken(t.getCurrentToken(), tkEQUALS,
            "equals: tkEQUALS") && ok;
    }

    // Test 18: multiple tokens - jumpToken skips whitespace
    {
        SqlTokenizer t("SELECT MYTABLE");
        ok = checkToken(t.getCurrentToken(), kwSELECT,
            "multi tokens: first SELECT") && ok;
        t.jumpToken(false);
        ok = checkToken(t.getCurrentToken(), tkIDENTIFIER,
            "multi tokens: MYTABLE after jump") && ok;
        ok = checkStr(t.getCurrentTokenString(), "MYTABLE",
            "multi tokens: MYTABLE string") && ok;
    }

    // Test 19: token positions (getCurrentTokenPosition)
    {
        SqlTokenizer t("SELECT 1");
        ok = check(t.getCurrentTokenPosition() == 0,
            "position: SELECT at 0") && ok;
        t.nextToken(); // whitespace
        ok = check(t.getCurrentTokenPosition() == 6,
            "position: space at 6") && ok;
        t.nextToken(); // '1'
        ok = check(t.getCurrentTokenPosition() == 7,
            "position: 1 at 7") && ok;
    }

    // Test 20: isReservedWord (static method)
    ok = check(SqlTokenizer::isReservedWord("SELECT"),
        "isReservedWord SELECT") && ok;
    ok = check(SqlTokenizer::isReservedWord("FROM"),
        "isReservedWord FROM") && ok;
    ok = check(SqlTokenizer::isReservedWord("WHERE"),
        "isReservedWord WHERE") && ok;
    ok = check(!SqlTokenizer::isReservedWord("MYTABLE"),
        "isReservedWord MYTABLE false") && ok;
    ok = check(!SqlTokenizer::isReservedWord(""),
        "isReservedWord empty false") && ok;

    // Test 21: getKeyword with explicit case (no config dependency)
    ok = checkStr(SqlTokenizer::getKeyword(kwSELECT, true), "SELECT",
        "getKeyword SELECT upper") && ok;
    ok = checkStr(SqlTokenizer::getKeyword(kwSELECT, false), "select",
        "getKeyword SELECT lower") && ok;
    ok = checkStr(SqlTokenizer::getKeyword(kwCREATE, true), "CREATE",
        "getKeyword CREATE upper") && ok;
    ok = checkStr(SqlTokenizer::getKeyword(kwCREATE, false), "create",
        "getKeyword CREATE lower") && ok;
    ok = checkStr(SqlTokenizer::getKeyword(kwINSERT, true), "INSERT",
        "getKeyword INSERT upper") && ok;
    ok = checkStr(SqlTokenizer::getKeyword(kwINSERT, false), "insert",
        "getKeyword INSERT lower") && ok;

    // Test 22: getKeywordTokenType (case-insensitive)
    ok = checkToken(SqlTokenizer::getKeywordTokenType("SELECT"), kwSELECT,
        "getKeywordTokenType SELECT") && ok;
    ok = checkToken(SqlTokenizer::getKeywordTokenType("select"), kwSELECT,
        "getKeywordTokenType select lowercase") && ok;
    ok = checkToken(SqlTokenizer::getKeywordTokenType("CREATE"), kwCREATE,
        "getKeywordTokenType CREATE") && ok;
    ok = checkToken(SqlTokenizer::getKeywordTokenType("MYTABLE"), tkIDENTIFIER,
        "getKeywordTokenType MYTABLE") && ok;
    ok = checkToken(SqlTokenizer::getKeywordTokenType(""), tkIDENTIFIER,
        "getKeywordTokenType empty") && ok;

    // Test 23: setStatement re-initialises the tokenizer
    {
        SqlTokenizer t("SELECT");
        ok = checkToken(t.getCurrentToken(), kwSELECT,
            "setStatement: initial SELECT") && ok;
        t.setStatement("INSERT");
        ok = checkToken(t.getCurrentToken(), kwINSERT,
            "setStatement: after setStatement INSERT") && ok;
    }

    // Test 24: string with embedded escaped quote
    {
        SqlTokenizer t("'it''s'");
        ok = checkToken(t.getCurrentToken(), tkSTRING,
            "escaped quote string: tkSTRING") && ok;
        ok = checkStr(t.getCurrentTokenString(), "'it''s'",
            "escaped quote string: full token") && ok;
    }

    // Test 25: quoted identifier with embedded double quote
    {
        SqlTokenizer t("\"MY\"\"TABLE\"");
        ok = checkToken(t.getCurrentToken(), tkIDENTIFIER,
            "quoted id with escape: tkIDENTIFIER") && ok;
    }

    // Test 26: COMMIT and ROLLBACK keywords
    {
        SqlTokenizer t("COMMIT");
        ok = checkToken(t.getCurrentToken(), kwCOMMIT,
            "COMMIT: kwCOMMIT") && ok;
    }
    {
        SqlTokenizer t("ROLLBACK");
        ok = checkToken(t.getCurrentToken(), kwROLLBACK,
            "ROLLBACK: kwROLLBACK") && ok;
    }

    // Test 27: SET keyword
    {
        SqlTokenizer t("SET");
        ok = checkToken(t.getCurrentToken(), kwSET, "SET: kwSET") && ok;
    }

    // Test 28: Modern Firebird keywords (FB4+)
    {
        SqlTokenizer t("DECFLOAT");
        ok = checkToken(t.getCurrentToken(), kwDECFLOAT, "DECFLOAT: kwDECFLOAT") && ok;
    }
    {
        SqlTokenizer t("INT128");
        ok = checkToken(t.getCurrentToken(), kwINT128, "INT128: kwINT128") && ok;
    }
    {
        SqlTokenizer t("ZONE");
        ok = checkToken(t.getCurrentToken(), kwZONE, "ZONE: kwZONE") && ok;
    }

    {
        SqlTokenizer t("SCROLL");
        ok = checkToken(t.getCurrentToken(), kwSCROLL, "SCROLL: kwSCROLL") && ok;
    }

    // Test 29: Scrollable cursor syntax (FB 4.0)
    {
        SqlTokenizer t("DECLARE SCROLL CURSOR C1 FOR SELECT * FROM T");
        ok = checkToken(t.getCurrentToken(), kwDECLARE, "DECLARE: kwDECLARE") && ok;
        t.jumpToken(false);
        ok = checkToken(t.getCurrentToken(), kwSCROLL, "SCROLL: kwSCROLL") && ok;
        t.jumpToken(false);
        ok = checkToken(t.getCurrentToken(), kwCURSOR, "CURSOR: kwCURSOR") && ok;
    }

    return ok ? 0 : 1;
}
