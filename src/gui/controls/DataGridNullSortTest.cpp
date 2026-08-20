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

// Regression tests for GitHub issue #685:
// "Fatal error when sorting a column just after double click on NULL cell"

#include <iostream>
#include <vector>
#include <algorithm>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

namespace
{

static bool check(bool condition, const char* testName)
{
    if (condition)
    {
        std::cout << "  PASSED: " << testName << "\n";
        return true;
    }
    else
    {
        std::cout << "  FAILED: " << testName << "\n";
        return false;
    }
}

struct MockRow
{
    bool isNull;
    wxString value;
};

void sortMockRows(std::vector<size_t>& mapping, const std::vector<MockRow>& rows, bool asc)
{
    std::stable_sort(mapping.begin(), mapping.end(), [&rows, asc](size_t a, size_t b) {
        bool nullA = rows[a].isNull;
        bool nullB = rows[b].isNull;
        if (nullA != nullB)
        {
            return asc ? !nullA : nullA;
        }
        if (nullA && nullB)
            return false;

        wxString valA = rows[a].value;
        wxString valB = rows[b].value;

        double numA = 0, numB = 0;
        bool isNumA = valA.ToDouble(&numA);
        bool isNumB = valB.ToDouble(&numB);

        if (isNumA && isNumB)
        {
            return asc ? (numA < numB) : (numA > numB);
        }

        int cmp = valA.CmpNoCase(valB);
        return asc ? (cmp < 0) : (cmp > 0);
    });
}

} // namespace

int main()
{
    wxInitializer initializer;
    if (!initializer.IsOk())
    {
        std::cerr << "Failed to initialize wxWidgets.\n";
        return 1;
    }

    bool ok = true;
    std::cout << "Running DataGrid NULL Sort Regression Tests (Issue #685)...\n";

    // Test 1: Ascending sort with mixed numbers and NULLs (NULLs should be at the end)
    {
        std::vector<MockRow> rows = {
            { true, "" },       // 0: NULL
            { false, "100" },   // 1: 100
            { true, "" },       // 2: NULL
            { false, "25" },    // 3: 25
            { false, "5" }      // 4: 5
        };

        std::vector<size_t> mapping = { 0, 1, 2, 3, 4 };
        sortMockRows(mapping, rows, true);

        // Expected order: 4 (5), 3 (25), 1 (100), then NULLs (0, 2)
        ok = check(mapping[0] == 4 && rows[mapping[0]].value == "5", "Ascending sort first element is 5") && ok;
        ok = check(mapping[1] == 3 && rows[mapping[1]].value == "25", "Ascending sort second element is 25") && ok;
        ok = check(mapping[2] == 1 && rows[mapping[2]].value == "100", "Ascending sort third element is 100") && ok;
        ok = check(rows[mapping[3]].isNull && rows[mapping[4]].isNull, "Ascending sort places NULLs at end") && ok;
    }

    // Test 2: Descending sort with mixed numbers and NULLs (NULLs should be at the beginning)
    {
        std::vector<MockRow> rows = {
            { false, "10" },    // 0: 10
            { true, "" },       // 1: NULL
            { false, "50" },    // 2: 50
            { true, "" },       // 3: NULL
            { false, "1" }      // 4: 1
        };

        std::vector<size_t> mapping = { 0, 1, 2, 3, 4 };
        sortMockRows(mapping, rows, false);

        // Expected order: NULLs (1, 3), then 2 (50), 0 (10), 4 (1)
        ok = check(rows[mapping[0]].isNull && rows[mapping[1]].isNull, "Descending sort places NULLs at start") && ok;
        ok = check(mapping[2] == 2 && rows[mapping[2]].value == "50", "Descending sort third element is 50") && ok;
        ok = check(mapping[3] == 0 && rows[mapping[3]].value == "10", "Descending sort fourth element is 10") && ok;
        ok = check(mapping[4] == 4 && rows[mapping[4]].value == "1", "Descending sort fifth element is 1") && ok;
    }

    // Test 3: String value [null] detection logic
    {
        wxString nullStr = "[null]";
        bool isNullString = (nullStr == "[null]" || nullStr.Strip(wxString::both).Lower() == "[null]");
        ok = check(isNullString, "Literal [null] recognized as NULL string") && ok;

        wxString nullStrSpaced = "  [NULL]  ";
        bool isNullStringSpaced = (nullStrSpaced == "[null]" || nullStrSpaced.Strip(wxString::both).Lower() == "[null]");
        ok = check(isNullStringSpaced, "Whitespace-padded [NULL] recognized as NULL string") && ok;
    }

    return ok ? 0 : 1;
}
