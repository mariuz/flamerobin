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

#include <iostream>
#include <wx/string.h>
#include "metadata/TemporalTableHelper.h"

int main()
{
    bool ok = true;

    auto check = [](bool condition, const char* name) {
        if (!condition) {
            std::cerr << "FAIL: " << name << "\n";
            return false;
        }
        std::cout << "PASS: " << name << "\n";
        return true;
    };

    // Test 1: Temporal Table DDL
    wxString ddl = TemporalTableHelper::generateTemporalTableDDL("EMPLOYEES");
    ok = check(ddl.Find("PERIOD FOR") != wxNOT_FOUND && ddl.Find("ROW START") != wxNOT_FOUND, "generateTemporalTableDDL syntax") && ok;

    // Test 2: Historical AS OF Query
    wxString asOf = TemporalTableHelper::generateHistoricalAsOfQuery("EMPLOYEES", "2026-06-01 12:00:00");
    ok = check(asOf.Find("FOR SYSTEM_TIME AS OF") != wxNOT_FOUND, "generateHistoricalAsOfQuery syntax") && ok;

    // Test 3: Historical BETWEEN Query
    wxString between = TemporalTableHelper::generateHistoricalBetweenQuery("EMPLOYEES", "2026-01-01 00:00:00", "2026-12-31 23:59:59");
    ok = check(between.Find("FOR SYSTEM_TIME BETWEEN") != wxNOT_FOUND, "generateHistoricalBetweenQuery syntax") && ok;

    return ok ? 0 : 1;
}
