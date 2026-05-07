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

    // Test for Firebird 5.0 keywords
    {
        SqlTokenizer t("TARGET SOURCE MATCHED");
        ok = checkToken(t.getCurrentToken(), kwTARGET, "TARGET: kwTARGET") && ok;
        t.nextToken();
        t.nextToken(); // skip whitespace
        ok = checkToken(t.getCurrentToken(), kwSOURCE, "SOURCE: kwSOURCE") && ok;
        t.nextToken();
        t.nextToken(); // skip whitespace
        ok = checkToken(t.getCurrentToken(), kwMATCHED, "MATCHED: kwMATCHED") && ok;
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

    // Test 29.5: SKIP LOCKED syntax and full statements
    {
        SqlTokenizer t1("SKIP LOCKED");
        ok = checkToken(t1.getCurrentToken(), kwSKIP, "SKIP: kwSKIP") && ok;
        t1.jumpToken(false);
        ok = checkToken(t1.getCurrentToken(), kwLOCKED, "LOCKED: kwLOCKED") && ok;

        SqlTokenizer t1_lower("skip locked");
        ok = checkToken(t1_lower.getCurrentToken(), kwSKIP, "SKIP (lower): kwSKIP") && ok;
        t1_lower.jumpToken(false);
        ok = checkToken(t1_lower.getCurrentToken(), kwLOCKED, "LOCKED (lower): kwLOCKED") && ok;

        SqlTokenizer t1_mixed("sKiP lOcKeD");
        ok = checkToken(t1_mixed.getCurrentToken(), kwSKIP, "SKIP (mixed): kwSKIP") && ok;
        t1_mixed.jumpToken(false);
        ok = checkToken(t1_mixed.getCurrentToken(), kwLOCKED, "LOCKED (mixed): kwLOCKED") && ok;

        SqlTokenizer t2("SELECT * FROM table WITH LOCK SKIP LOCKED");
        ok = checkToken(t2.getCurrentToken(), kwSELECT, "SELECT: kwSELECT") && ok;
        t2.jumpToken(false); // *
        t2.jumpToken(false);
        ok = checkToken(t2.getCurrentToken(), kwFROM, "FROM: kwFROM") && ok;
        t2.jumpToken(false); // table
        t2.jumpToken(false);
        ok = checkToken(t2.getCurrentToken(), kwWITH, "WITH: kwWITH") && ok;
        t2.jumpToken(false);
        ok = checkToken(t2.getCurrentToken(), kwLOCK, "LOCK: kwLOCK") && ok;
        t2.jumpToken(false);
        ok = checkToken(t2.getCurrentToken(), kwSKIP, "SKIP: kwSKIP") && ok;
        t2.jumpToken(false);
        ok = checkToken(t2.getCurrentToken(), kwLOCKED, "LOCKED: kwLOCKED") && ok;

        SqlTokenizer t3("UPDATE table SET col = 1 SKIP LOCKED");
        ok = checkToken(t3.getCurrentToken(), kwUPDATE, "UPDATE: kwUPDATE") && ok;
        t3.jumpToken(false); // table
        t3.jumpToken(false);
        ok = checkToken(t3.getCurrentToken(), kwSET, "SET: kwSET") && ok;
        t3.jumpToken(false); // col
        t3.jumpToken(false); // =
        t3.jumpToken(false); // 1
        t3.jumpToken(false);
        ok = checkToken(t3.getCurrentToken(), kwSKIP, "SKIP: kwSKIP") && ok;
        t3.jumpToken(false);
        ok = checkToken(t3.getCurrentToken(), kwLOCKED, "LOCKED: kwLOCKED") && ok;

        SqlTokenizer t4("DELETE FROM table SKIP LOCKED");
        ok = checkToken(t4.getCurrentToken(), kwDELETE, "DELETE: kwDELETE") && ok;
        t4.jumpToken(false);
        ok = checkToken(t4.getCurrentToken(), kwFROM, "FROM: kwFROM") && ok;
        t4.jumpToken(false); // table
        t4.jumpToken(false);
        ok = checkToken(t4.getCurrentToken(), kwSKIP, "SKIP: kwSKIP") && ok;
        t4.jumpToken(false);
        ok = checkToken(t4.getCurrentToken(), kwLOCKED, "LOCKED: kwLOCKED") && ok;
    }

    // Test 30: Version-based keyword strings
    {
        wxString fb25 = SqlTokenizer::getKeywordsString(SqlTokenizer::kwUpperCase, 11, 1); // ODS 11.1 (FB 2.5)
        wxString fb40 = SqlTokenizer::getKeywordsString(SqlTokenizer::kwUpperCase, 13, 0); // ODS 13.0 (FB 4.0)
        
        ok = check(fb25.Contains("SELECT"), "FB2.5 has SELECT") && ok;
        ok = check(!fb25.Contains("DECFLOAT"), "FB2.5 does not have DECFLOAT") && ok;
        ok = check(!fb25.Contains("PUBLICATION"), "FB2.5 does not have PUBLICATION") && ok;
        
        ok = check(fb40.Contains("SELECT"), "FB4.0 has SELECT") && ok;
        ok = check(fb40.Contains("DECFLOAT"), "FB4.0 has DECFLOAT") && ok;
        ok = check(fb40.Contains("PUBLICATION"), "FB4.0 has PUBLICATION") && ok;
        ok = check(!fb40.Contains("LOCKED"), "FB4.0 does not have LOCKED") && ok;

        wxString fb50 = SqlTokenizer::getKeywordsString(SqlTokenizer::kwUpperCase, 13, 1); // ODS 13.1 (FB 5.0)
        ok = check(fb50.Contains("SKIP"), "FB5.0 has SKIP") && ok;
        ok = check(fb50.Contains("LOCKED"), "FB5.0 has LOCKED") && ok;
        ok = check(fb50.Contains("GREATEST"), "FB5.0 has GREATEST") && ok;
        ok = check(fb50.Contains("LEAST"), "FB5.0 has LEAST") && ok;
        ok = check(fb50.Contains("JSON_VALUE"), "FB5.0 has JSON_VALUE") && ok;
        ok = check(fb50.Contains("JSON_TABLE"), "FB5.0 has JSON_TABLE") && ok;

        wxString fb60 = SqlTokenizer::getKeywordsString(SqlTokenizer::kwUpperCase, 14, 0); // ODS 14.0 (FB 6.0)
        ok = check(fb60.Contains("GREATEST"), "FB6.0 has GREATEST") && ok;
        ok = check(fb60.Contains("LEAST"), "FB6.0 has LEAST") && ok;
        ok = check(fb60.Contains("ANY_VALUE"), "FB6.0 has ANY_VALUE") && ok;
        ok = check(fb60.Contains("UNLIST"), "FB6.0 has UNLIST") && ok;
        ok = check(fb60.Contains("JSON_VALUE"), "FB6.0 has JSON_VALUE") && ok;
        ok = check(fb60.Contains("NAMED_ARG_ASSIGN"), "FB6.0 has NAMED_ARG_ASSIGN") && ok;

        // Verify reserved status in FB 6.0
        ok = check(SqlTokenizer::isReservedWord("ANY_VALUE", 14, 0), "ANY_VALUE is reserved in FB 6.0") && ok;
        ok = check(SqlTokenizer::isReservedWord("UNLIST", 14, 0), "UNLIST is reserved in FB 6.0") && ok;

        // Verify they are NOT in FB 5.0 (ODS 13.1)
        ok = check(!fb50.Contains("ANY_VALUE"), "FB5.0 does NOT have ANY_VALUE") && ok;
        ok = check(!fb50.Contains("UNLIST"), "FB5.0 does NOT have UNLIST") && ok;
        ok = check(!SqlTokenizer::isReservedWord("ANY_VALUE", 13, 1), "ANY_VALUE is NOT reserved in FB 5.0") && ok;
        ok = check(!SqlTokenizer::isReservedWord("UNLIST", 13, 1), "UNLIST is NOT reserved in FB 5.0") && ok;
    }

    // Test: JSON functions (FB 5.0+)
    {
        SqlTokenizer t("JSON_VALUE(col, '$.path') JSON_QUERY(col, '$.path') JSON_OBJECT(KEY 'a' VALUE 1)");
        ok = checkToken(t.getCurrentToken(), kwJSON_VALUE, "JSON_VALUE: kwJSON_VALUE") && ok;
        t.jumpToken(false); // (
        t.jumpToken(false); // col
        t.jumpToken(false); // ,
        t.jumpToken(false); // '$.path'
        t.jumpToken(false); // )
        t.jumpToken(false); // JSON_QUERY
        ok = checkToken(t.getCurrentToken(), kwJSON_QUERY, "JSON_QUERY: kwJSON_QUERY") && ok;
        t.jumpToken(false); // (
        t.jumpToken(false); // col
        t.jumpToken(false); // ,
        t.jumpToken(false); // '$.path'
        t.jumpToken(false); // )
        t.jumpToken(false); // JSON_OBJECT
        ok = checkToken(t.getCurrentToken(), kwJSON_OBJECT, "JSON_OBJECT: kwJSON_OBJECT") && ok;
    }

    // Test: GREATEST / LEAST (FB 5.0+)
    {
        SqlTokenizer t("GREATEST(1, 2) LEAST(1, 2)");
        ok = checkToken(t.getCurrentToken(), kwGREATEST, "GREATEST: kwGREATEST") && ok;
        t.jumpToken(false); // (
        t.jumpToken(false); // 1
        t.jumpToken(false); // ,
        t.jumpToken(false); // 2
        t.jumpToken(false); // )
        t.jumpToken(false); // LEAST
        ok = checkToken(t.getCurrentToken(), kwLEAST, "LEAST: kwLEAST") && ok;
    }

    // Test: ANY_VALUE / UNLIST (FB 6.0+)
    {
        SqlTokenizer t("ANY_VALUE(col) UNLIST(str)");
        ok = checkToken(t.getCurrentToken(), kwANY_VALUE, "ANY_VALUE: kwANY_VALUE") && ok;
        t.jumpToken(false); // (
        t.jumpToken(false); // col
        t.jumpToken(false); // )
        t.jumpToken(false); // UNLIST
        ok = checkToken(t.getCurrentToken(), kwUNLIST, "UNLIST: kwUNLIST") && ok;
    }

    // Test: Firebird 6.0 Enhanced Security keywords (OWNER, INITIAL)
    {
        SqlTokenizer t("OWNER INITIAL");
        ok = checkToken(t.getCurrentToken(), kwOWNER, "OWNER: kwOWNER") && ok;
        t.jumpToken(false);
        ok = checkToken(t.getCurrentToken(), kwINITIAL, "INITIAL: kwINITIAL") && ok;
    }

    // Test: CREATE DATABASE with OWNER and INITIAL USER
    {
        SqlTokenizer t("CREATE DATABASE 'test.fdb' OWNER 'admin' INITIAL USER 'user'");
        ok = checkToken(t.getCurrentToken(), kwCREATE, "CREATE: kwCREATE") && ok;
        t.jumpToken(false);
        ok = checkToken(t.getCurrentToken(), kwDATABASE, "DATABASE: kwDATABASE") && ok;
        t.jumpToken(false);
        ok = checkToken(t.getCurrentToken(), tkSTRING, "string: 'test.fdb'") && ok;
        t.jumpToken(false);
        ok = checkToken(t.getCurrentToken(), kwOWNER, "OWNER: kwOWNER") && ok;
        t.jumpToken(false);
        ok = checkToken(t.getCurrentToken(), tkSTRING, "string: 'admin'") && ok;
        t.jumpToken(false);
        ok = checkToken(t.getCurrentToken(), kwINITIAL, "INITIAL: kwINITIAL") && ok;
        t.jumpToken(false);
        ok = checkToken(t.getCurrentToken(), kwUSER, "USER: kwUSER") && ok;
        t.jumpToken(false);
        ok = checkToken(t.getCurrentToken(), tkSTRING, "string: 'user'") && ok;
    }

    // Test: LATERAL join
    {
        wxString sql = "SELECT * FROM RDB$DATABASE JOIN LATERAL (SELECT 1 FROM RDB$DATABASE) ON 1=1";
        SqlTokenizer tk(sql);
        ok = check(tk.getCurrentToken() == kwSELECT, "LATERAL test: Start with SELECT") && ok;
        ok = check(tk.jumpToken(false) && tk.getCurrentToken() == tkUNKNOWN, "LATERAL test: Jump to *") && ok;
        ok = check(tk.jumpToken(false) && tk.getCurrentToken() == kwFROM, "LATERAL test: Jump to FROM") && ok;
        ok = check(tk.jumpToken(false) && tk.getCurrentToken() == tkIDENTIFIER, "LATERAL test: Jump to RDB$DATABASE") && ok;
        ok = check(tk.jumpToken(false) && tk.getCurrentToken() == kwJOIN, "LATERAL test: Jump to JOIN") && ok;
        ok = check(tk.jumpToken(false) && tk.getCurrentToken() == kwLATERAL, "LATERAL test: Jump to LATERAL") && ok;
        ok = check(tk.jumpToken(true) && tk.getCurrentToken() == kwON, "LATERAL test: Jump past LATERAL subquery to ON") && ok;
    }

    // Test: Operators
    {
        SqlTokenizer t1(">=");
        ok = checkToken(t1.getCurrentToken(), kwGEQ, ">=: kwGEQ") && ok;
        SqlTokenizer t2("||");
        ok = checkToken(t2.getCurrentToken(), kwCONCATENATE, "||: kwCONCATENATE") && ok;
        SqlTokenizer t3("=>");
        ok = checkToken(t3.getCurrentToken(), kwNAMED_ARG_ASSIGN, "=>: kwNAMED_ARG_ASSIGN") && ok;
        SqlTokenizer t4("NAMED_ARG_ASSIGN");
        ok = checkToken(t4.getCurrentToken(), kwNAMED_ARG_ASSIGN, "NAMED_ARG_ASSIGN: kwNAMED_ARG_ASSIGN") && ok;
        SqlTokenizer t5(":=");
        ok = checkToken(t5.getCurrentToken(), kwBIND_PARAM, ":=: kwBIND_PARAM") && ok;
        SqlTokenizer t6("<>");
        ok = checkToken(t6.getCurrentToken(), kwNEQ, "<>: kwNEQ") && ok;
        SqlTokenizer t7("!=");
        ok = checkToken(t7.getCurrentToken(), kwNEQ, "!=: kwNEQ") && ok;
        SqlTokenizer t8("<=");
        ok = checkToken(t8.getCurrentToken(), kwLEQ, "<=: kwLEQ") && ok;
        SqlTokenizer t9("!>");
        ok = checkToken(t9.getCurrentToken(), kwNOT_GTR, "!>: kwNOT_GTR") && ok;
    }

    return ok ? 0 : 1;
}
