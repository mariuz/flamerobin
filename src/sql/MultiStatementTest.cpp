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

#include "sql/MultiStatement.h"

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

} // namespace

int main()
{
    bool ok = true;

    // Test 1: empty SQL produces one empty statement then invalid
    {
        MultiStatement ms("");
        SingleStatement s = ms.getNextStatement();
        ok = check(s.isValid(), "empty SQL: first statement valid") && ok;
        ok = check(s.isEmptyStatement(), "empty SQL: statement is empty") && ok;

        SingleStatement s2 = ms.getNextStatement();
        ok = check(!s2.isValid(), "empty SQL: second call invalid") && ok;
    }

    // Test 2: single statement without trailing terminator
    {
        MultiStatement ms("SELECT 1 FROM RDB$DATABASE");
        SingleStatement s = ms.getNextStatement();
        ok = check(s.isValid(), "single stmt: valid") && ok;
        ok = check(!s.isEmptyStatement(), "single stmt: not empty") && ok;
        ok = checkStr(s.getSql(), "SELECT 1 FROM RDB$DATABASE",
            "single stmt: SQL") && ok;

        SingleStatement s2 = ms.getNextStatement();
        ok = check(!s2.isValid(), "single stmt: no more") && ok;
    }

    // Test 3: single statement with trailing terminator
    {
        MultiStatement ms("SELECT 1 FROM RDB$DATABASE;");
        SingleStatement s = ms.getNextStatement();
        ok = check(s.isValid(), "stmt+term: valid") && ok;
        ok = checkStr(s.getSql(), "SELECT 1 FROM RDB$DATABASE",
            "stmt+term: SQL without terminator") && ok;
    }

    // Test 4: two statements separated by terminator
    {
        MultiStatement ms("SELECT 1; SELECT 2");
        SingleStatement s1 = ms.getNextStatement();
        ok = check(s1.isValid(), "two stmts: first valid") && ok;
        ok = checkStr(s1.getSql(), "SELECT 1", "two stmts: first SQL") && ok;

        SingleStatement s2 = ms.getNextStatement();
        ok = check(s2.isValid(), "two stmts: second valid") && ok;
        // Text after the terminator (leading space is preserved)
        ok = checkStr(s2.getSql(), " SELECT 2", "two stmts: second SQL") && ok;

        SingleStatement s3 = ms.getNextStatement();
        ok = check(!s3.isValid(), "two stmts: no more") && ok;
    }

    // Test 5: embedded single-quoted string containing terminator
    {
        MultiStatement ms("SELECT 'hello; world' FROM T");
        SingleStatement s = ms.getNextStatement();
        ok = check(s.isValid(), "embedded string: valid") && ok;
        ok = checkStr(s.getSql(), "SELECT 'hello; world' FROM T",
            "embedded string: SQL") && ok;

        SingleStatement s2 = ms.getNextStatement();
        ok = check(!s2.isValid(), "embedded string: no more") && ok;
    }

    // Test 6: COMMIT statement
    {
        SingleStatement s("COMMIT");
        ok = check(s.isValid(), "COMMIT: valid") && ok;
        ok = check(s.isCommitStatement(), "COMMIT: isCommit") && ok;
        ok = check(!s.isRollbackStatement(), "COMMIT: not rollback") && ok;
        ok = check(!s.isEmptyStatement(), "COMMIT: not empty") && ok;
    }

    // Test 7: COMMIT WORK statement
    {
        SingleStatement s("COMMIT WORK");
        ok = check(s.isCommitStatement(), "COMMIT WORK: isCommit") && ok;
    }

    // Test 8: ROLLBACK statement
    {
        SingleStatement s("ROLLBACK");
        ok = check(s.isValid(), "ROLLBACK: valid") && ok;
        ok = check(s.isRollbackStatement(), "ROLLBACK: isRollback") && ok;
        ok = check(!s.isCommitStatement(), "ROLLBACK: not commit") && ok;
    }

    // Test 9: ROLLBACK WORK statement
    {
        SingleStatement s("ROLLBACK WORK");
        ok = check(s.isRollbackStatement(), "ROLLBACK WORK: isRollback") && ok;
    }

    // Test 10: ROLLBACK with savepoint is not recognised as simple rollback
    {
        SingleStatement s("ROLLBACK TO SAVEPOINT SP1");
        ok = check(s.isValid(), "ROLLBACK TO SAVEPOINT: valid") && ok;
        ok = check(!s.isRollbackStatement(),
            "ROLLBACK TO SAVEPOINT: not simple rollback") && ok;
    }

    // Test 11: line comment containing terminator does not split statement
    {
        MultiStatement ms("SELECT 1 -- comment with ; inside\nFROM T");
        SingleStatement s = ms.getNextStatement();
        ok = check(s.isValid(), "line comment: valid") && ok;
        ok = checkStr(s.getSql(),
            "SELECT 1 -- comment with ; inside\nFROM T",
            "line comment: full SQL") && ok;

        SingleStatement s2 = ms.getNextStatement();
        ok = check(!s2.isValid(), "line comment: no more") && ok;
    }

    // Test 12: block comment containing terminator does not split statement
    {
        MultiStatement ms("SELECT /* ; not a terminator */ 1");
        SingleStatement s = ms.getNextStatement();
        ok = check(s.isValid(), "block comment: valid") && ok;
        ok = checkStr(s.getSql(), "SELECT /* ; not a terminator */ 1",
            "block comment: full SQL") && ok;
    }

    // Test 13: SET AUTODDL recognised
    {
        SingleStatement s("SET AUTODDL ON");
        ok = check(s.isValid(), "SET AUTODDL: valid") && ok;
        wxString setting;
        ok = check(s.isSetAutoDDLStatement(setting),
            "SET AUTODDL: recognised") && ok;
        ok = checkStr(setting, "ON", "SET AUTODDL: setting value") && ok;
    }

    // Test 14: SET AUTODDL with lowercase value
    {
        SingleStatement s("SET AUTO off");
        wxString setting;
        ok = check(s.isSetAutoDDLStatement(setting),
            "SET AUTO: recognised") && ok;
        ok = checkStr(setting, "off", "SET AUTO: setting value") && ok;
    }

    // Test 15: SET TERM changes the active terminator
    {
        MultiStatement ms("SET TERM ^^\nSELECT 1^^\nSET TERM ;;");
        // SET TERM ^^ is consumed internally; first returned statement is
        // the body between the new terminator occurrences.
        SingleStatement s1 = ms.getNextStatement();
        ok = check(s1.isValid(), "SET TERM: first stmt valid") && ok;

        SingleStatement s2 = ms.getNextStatement();
        ok = check(s2.isValid(), "SET TERM: body stmt valid") && ok;

        // After SET TERM ;; the active terminator should be ";"
        ok = checkStr(ms.getTerminator(), ";",
            "SET TERM: terminator restored") && ok;
    }

    // Test 16: getStatementAt retrieves statement by cursor position
    {
        MultiStatement ms("SELECT 1; SELECT 2");
        int offset = 0;
        SingleStatement s = ms.getStatementAt(5, offset);
        ok = check(s.isValid(), "getStatementAt pos 5: valid") && ok;
        ok = checkStr(s.getSql(), "SELECT 1",
            "getStatementAt pos 5: SQL") && ok;
    }

    // Test 17: getStart / getEnd positions
    {
        MultiStatement ms("SELECT 1; SELECT 2");
        ms.getNextStatement();
        ok = check(ms.getStart() == 0, "getStart: first stmt at 0") && ok;
        // getEnd should point to the ';' separator
        ok = check(ms.getEnd() == 8, "getEnd: first stmt ends at 8") && ok;
    }

    // Test 18: three statements
    {
        MultiStatement ms("A; B; C");
        SingleStatement s1 = ms.getNextStatement();
        ok = check(s1.isValid(), "three stmts: s1 valid") && ok;
        ok = checkStr(s1.getSql(), "A", "three stmts: s1") && ok;

        SingleStatement s2 = ms.getNextStatement();
        ok = check(s2.isValid(), "three stmts: s2 valid") && ok;
        ok = checkStr(s2.getSql(), " B", "three stmts: s2") && ok;

        SingleStatement s3 = ms.getNextStatement();
        ok = check(s3.isValid(), "three stmts: s3 valid") && ok;
        ok = checkStr(s3.getSql(), " C", "three stmts: s3") && ok;

        SingleStatement s4 = ms.getNextStatement();
        ok = check(!s4.isValid(), "three stmts: no more") && ok;
    }

    // Test 19: invalid (default-constructed) SingleStatement
    {
        SingleStatement s;
        ok = check(!s.isValid(), "default SingleStatement: invalid") && ok;
    }

    // Tests 20-23: issue #338 - timezone-related SET statements must parse
    // as ordinary (stOther) statements, not as commit/rollback/set-term/etc.

    // Test 20: SET TIME ZONE 'America/Sao_Paulo'
    {
        SingleStatement s("SET TIME ZONE 'America/Sao_Paulo'");
        ok = check(s.isValid(), "SET TIME ZONE: valid") && ok;
        ok = check(!s.isEmptyStatement(), "SET TIME ZONE: not empty") && ok;
        ok = check(!s.isCommitStatement(), "SET TIME ZONE: not commit") && ok;
        ok = check(!s.isRollbackStatement(), "SET TIME ZONE: not rollback") && ok;
        wxString dummy;
        ok = check(!s.isSetTermStatement(dummy),
            "SET TIME ZONE: not set term") && ok;
        ok = check(!s.isSetAutoDDLStatement(dummy),
            "SET TIME ZONE: not set autoddl") && ok;
        ok = checkStr(s.getSql(), "SET TIME ZONE 'America/Sao_Paulo'",
            "SET TIME ZONE: SQL preserved") && ok;
    }

    // Test 21: SET BIND OF TIME WITH TIME ZONE TO LEGACY
    {
        SingleStatement s("SET BIND OF TIME WITH TIME ZONE TO LEGACY");
        ok = check(s.isValid(), "SET BIND TIME TZ: valid") && ok;
        ok = check(!s.isEmptyStatement(), "SET BIND TIME TZ: not empty") && ok;
        ok = check(!s.isCommitStatement(), "SET BIND TIME TZ: not commit") && ok;
        ok = check(!s.isRollbackStatement(),
            "SET BIND TIME TZ: not rollback") && ok;
        wxString dummy;
        ok = check(!s.isSetTermStatement(dummy),
            "SET BIND TIME TZ: not set term") && ok;
        ok = check(!s.isSetAutoDDLStatement(dummy),
            "SET BIND TIME TZ: not set autoddl") && ok;
        ok = checkStr(s.getSql(),
            "SET BIND OF TIME WITH TIME ZONE TO LEGACY",
            "SET BIND TIME TZ: SQL preserved") && ok;
    }

    // Test 22: SET BIND OF TIMESTAMP WITH TIME ZONE TO LEGACY
    {
        SingleStatement s("SET BIND OF TIMESTAMP WITH TIME ZONE TO LEGACY");
        ok = check(s.isValid(), "SET BIND TIMESTAMP TZ: valid") && ok;
        ok = check(!s.isEmptyStatement(),
            "SET BIND TIMESTAMP TZ: not empty") && ok;
        ok = check(!s.isCommitStatement(),
            "SET BIND TIMESTAMP TZ: not commit") && ok;
        ok = check(!s.isRollbackStatement(),
            "SET BIND TIMESTAMP TZ: not rollback") && ok;
        wxString dummy;
        ok = check(!s.isSetTermStatement(dummy),
            "SET BIND TIMESTAMP TZ: not set term") && ok;
        ok = check(!s.isSetAutoDDLStatement(dummy),
            "SET BIND TIMESTAMP TZ: not set autoddl") && ok;
        ok = checkStr(s.getSql(),
            "SET BIND OF TIMESTAMP WITH TIME ZONE TO LEGACY",
            "SET BIND TIMESTAMP TZ: SQL preserved") && ok;
    }

    // Test 23: issue #338 - full four-statement sequence splits correctly
    {
        MultiStatement ms(
            "SET TIME ZONE 'America/Sao_Paulo';\n"
            "SET BIND OF TIME WITH TIME ZONE TO LEGACY;\n"
            "SET BIND OF TIMESTAMP WITH TIME ZONE TO LEGACY;\n"
            "SELECT localtime, current_time, CURRENT_TIMESTAMP"
            " FROM rdb$database");

        SingleStatement s1 = ms.getNextStatement();
        ok = check(s1.isValid(), "issue#338 seq: s1 valid") && ok;
        ok = checkStr(s1.getSql(), "SET TIME ZONE 'America/Sao_Paulo'",
            "issue#338 seq: s1 SQL") && ok;

        SingleStatement s2 = ms.getNextStatement();
        ok = check(s2.isValid(), "issue#338 seq: s2 valid") && ok;
        ok = checkStr(s2.getSql(),
            "\nSET BIND OF TIME WITH TIME ZONE TO LEGACY",
            "issue#338 seq: s2 SQL") && ok;

        SingleStatement s3 = ms.getNextStatement();
        ok = check(s3.isValid(), "issue#338 seq: s3 valid") && ok;
        ok = checkStr(s3.getSql(),
            "\nSET BIND OF TIMESTAMP WITH TIME ZONE TO LEGACY",
            "issue#338 seq: s3 SQL") && ok;

        SingleStatement s4 = ms.getNextStatement();
        ok = check(s4.isValid(), "issue#338 seq: s4 valid") && ok;
        ok = check(!s4.isEmptyStatement(),
            "issue#338 seq: s4 not empty") && ok;

        SingleStatement s5 = ms.getNextStatement();
        ok = check(!s5.isValid(), "issue#338 seq: no more") && ok;
    }

    // Test 24: LATERAL join
    {
        MultiStatement ms("SELECT * FROM T1 JOIN LATERAL (SELECT 1 FROM T2) ON 1=1; SELECT * FROM T3");
        SingleStatement s1 = ms.getNextStatement();
        ok = check(s1.isValid(), "LATERAL: first valid") && ok;
        ok = checkStr(s1.getSql(), "SELECT * FROM T1 JOIN LATERAL (SELECT 1 FROM T2) ON 1=1",
            "LATERAL: first SQL") && ok;

        SingleStatement s2 = ms.getNextStatement();
        ok = check(s2.isValid(), "LATERAL: second valid") && ok;
        ok = checkStr(s2.getSql(), " SELECT * FROM T3", "LATERAL: second SQL") && ok;
    }

    return ok ? 0 : 1;
}
