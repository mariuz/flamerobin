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

// Regression test for issue #715:
// "Global Temporary Table: new index not showing up in UI"

#include <iostream>

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "sql/SqlStatement.h"
#include "metadata/table.h"
#include "metadata/Index.h"

namespace
{

bool check(bool condition, const char* testName)
{
    if (condition)
    {
        std::cout << "[PASS] " << testName << "\n";
        return true;
    }
    std::cerr << "[FAIL] " << testName << "\n";
    return false;
}

} // namespace

int main()
{
    bool ok = true;

    std::cout << "Testing Global Temporary Table index recognition and handling (issue #715)...\n";

    // Test 1: CREATE INDEX on GTT recognized as actCREATE ntIndex
    {
        wxString sql = "CREATE INDEX IDX_GTT_COL ON GTT_TEST (COL1);";
        SqlStatement stm(sql, nullptr);
        ok = check(stm.getName() == "IDX_GTT_COL", "CREATE INDEX on GTT: index name parsed") && ok;
        ok = check(stm.getAction() == actCREATE, "CREATE INDEX on GTT: identified as actCREATE") && ok;
        ok = check(stm.getObjectType() == ntIndex, "CREATE INDEX on GTT: identified as ntIndex") && ok;
        ok = check(stm.isDDL(), "CREATE INDEX on GTT: identified as DDL") && ok;
    }

    // Test 2: CREATE UNIQUE DESCENDING INDEX on GTT
    {
        wxString sql = "CREATE UNIQUE DESCENDING INDEX IDX_GTT_U_DESC ON GTT_TEST (COL2);";
        SqlStatement stm(sql, nullptr);
        ok = check(stm.getName() == "IDX_GTT_U_DESC", "CREATE UNIQUE DESCENDING INDEX: name parsed") && ok;
        ok = check(stm.getAction() == actCREATE, "CREATE UNIQUE DESCENDING INDEX: identified as actCREATE") && ok;
        ok = check(stm.getObjectType() == ntIndex, "CREATE UNIQUE DESCENDING INDEX: identified as ntIndex") && ok;
    }

    // Test 3: DROP INDEX recognition
    {
        wxString sql = "DROP INDEX IDX_GTT_COL;";
        SqlStatement stm(sql, nullptr);
        ok = check(stm.getName() == "IDX_GTT_COL", "DROP INDEX: index name parsed") && ok;
        ok = check(stm.getAction() == actDROP, "DROP INDEX: identified as actDROP") && ok;
        ok = check(stm.getObjectType() == ntIndex, "DROP INDEX: identified as ntIndex") && ok;
        ok = check(stm.isDDL(), "DROP INDEX: identified as DDL") && ok;
    }

    // Test 4: GTTable dynamic_cast to Table and Relation
    {
        GTTable gtt(DatabasePtr(), "GTT_TEST");
        Relation* rel = &gtt;
        Table* tbl = dynamic_cast<Table*>(rel);
        ok = check(tbl != nullptr, "GTTable dynamic_cast to Table* succeeds") && ok;
        ok = check(gtt.getType() == ntGTT, "GTTable type is ntGTT") && ok;
    }

    // Test 5: CREATE GLOBAL TEMPORARY TABLE recognized as ntGTT
    {
        wxString sql = "CREATE GLOBAL TEMPORARY TABLE GTT_DOC (ID INT, NAME VARCHAR(50)) ON COMMIT PRESERVE ROWS;";
        SqlStatement stm(sql, nullptr);
        ok = check(stm.getName() == "GTT_DOC", "CREATE GTT: table name parsed") && ok;
        ok = check(stm.getAction() == actCREATE, "CREATE GTT: identified as actCREATE") && ok;
        ok = check(stm.getObjectType() == ntGTT, "CREATE GTT: identified as ntGTT") && ok;
    }

    // Test 6: ALTER TABLE on GTT recognized as actALTER
    {
        wxString sql = "ALTER TABLE GTT_DOC ADD AGE INT;";
        SqlStatement stm(sql, nullptr);
        ok = check(stm.getName() == "GTT_DOC", "ALTER TABLE on GTT: table name parsed") && ok;
        ok = check(stm.getAction() == actALTER, "ALTER TABLE on GTT: identified as actALTER") && ok;
    }

    if (ok)
        std::cout << "\nAll GTT index tests passed.\n";
    else
        std::cout << "\nSome tests failed.\n";

    return ok ? 0 : 1;
}
