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

// Regression tests for GitHub issue #675:
// "DATE is not being shown correctly all dates in preview showing them as 00.00.0000"

#include <iostream>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "gui/controls/DataGridRows.h"
#include "gui/controls/DataGridRowBuffer.h"

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
    std::cout << "Running DataGrid Date/Time/Timestamp Regression Tests (Issue #675)...\n";

    // Test 1: IBPP::Date constructor with components and getter roundtrip
    {
        IBPP::Date d(2023, 5, 20);
        int y = 0, m = 0, day = 0;
        d.GetDate(y, m, day);
        ok = check(y == 2023 && m == 5 && day == 20, "IBPP::Date(2023, 5, 20) GetDate components") && ok;
        ok = check(d.Year() == 2023 && d.Month() == 5 && d.Day() == 20, "IBPP::Date Year/Month/Day accessors") && ok;

        int raw = d.GetDate();
        ok = check(raw != 0, "IBPP::Date GetDate() integer representation is non-zero") && ok;

        IBPP::Date d2(raw);
        int y2 = 0, m2 = 0, day2 = 0;
        d2.GetDate(y2, m2, day2);
        ok = check(y2 == 2023 && m2 == 5 && day2 == 20, "IBPP::Date(raw) decodes correctly") && ok;
    }

    // Test 2: IBPP::Date SetDate and default constructor
    {
        IBPP::Date d;
        int y = 0, m = 0, day = 0;
        d.GetDate(y, m, day);
        ok = check(y == 0 && m == 0 && day == 0 && d.GetDate() == 0, "IBPP::Date() default is 0") && ok;

        d.SetDate(2024, 2, 29);
        d.GetDate(y, m, day);
        ok = check(y == 2024 && m == 2 && day == 29, "IBPP::Date SetDate(2024, 2, 29)") && ok;
    }

    // Test 3: IBPP::Date Today and Add
    {
        IBPP::Date d;
        d.SetDate(2024, 2, 28);
        d.Add(1);
        int y = 0, m = 0, day = 0;
        d.GetDate(y, m, day);
        ok = check(y == 2024 && m == 2 && day == 29, "IBPP::Date Add(1) on leap year") && ok;

        d.Add(1);
        d.GetDate(y, m, day);
        ok = check(y == 2024 && m == 3 && day == 1, "IBPP::Date Add(1) roll to March 1") && ok;
    }

    // Test 4: IBPP::Time encoding and decoding
    {
        IBPP::Time t(14, 30, 45, 1234);
        int h = 0, mi = 0, s = 0, f = 0;
        t.GetTime(h, mi, s, f);
        ok = check(h == 14 && mi == 30 && s == 45 && f == 1234, "IBPP::Time GetTime components") && ok;

        int rawTime = t.GetTime();
        ok = check(rawTime != 0, "IBPP::Time GetTime() integer representation is non-zero") && ok;

        IBPP::Time t2(rawTime);
        int h2 = 0, mi2 = 0, s2 = 0, f2 = 0;
        t2.GetTime(h2, mi2, s2, f2);
        ok = check(h2 == 14 && mi2 == 30 && s2 == 45 && f2 == 1234, "IBPP::Time(raw) decodes correctly") && ok;
    }

    // Test 5: IBPP::Timestamp encoding, decoding, and buffer roundtrip
    {
        IBPP::Timestamp ts(2026, 8, 14, 18, 27, 30, 5000);
        int y = 0, mo = 0, d = 0, h = 0, mi = 0, s = 0, f = 0;
        ts.GetDate(y, mo, d);
        ts.GetTime(h, mi, s, f);
        ok = check(y == 2026 && mo == 8 && d == 14, "IBPP::Timestamp GetDate components") && ok;
        ok = check(h == 18 && mi == 27 && s == 30 && f == 5000, "IBPP::Timestamp GetTime components") && ok;

        IBPP::Date dt = ts.GetDate();
        IBPP::Time tm = ts.GetTime();
        ok = check(dt.GetDate() != 0, "IBPP::Timestamp GetDate() returns valid Date with non-zero raw") && ok;
        ok = check(tm.GetTime() != 0, "IBPP::Timestamp GetTime() returns valid Time with non-zero raw") && ok;
    }

    // Test 6: DataGridRowBuffer Date storage and retrieval
    {
        DataGridRowBuffer buffer(1);
        IBPP::Date d(2023, 11, 25);
        buffer.setValue(0, d.GetDate());

        int retrievedVal = 0;
        bool hasVal = buffer.getValue(0, retrievedVal);
        ok = check(hasVal, "DataGridRowBuffer getValue for date offset") && ok;

        IBPP::Date retrievedDate(retrievedVal);
        int y = 0, m = 0, day = 0;
        retrievedDate.GetDate(y, m, day);
        ok = check(y == 2023 && m == 11 && day == 25, "DataGridRowBuffer retrieved date matches 2023-11-25") && ok;

        wxString formatted = wxString::Format("%02d.%02d.%04d", day, m, y);
        ok = check(formatted == "25.11.2023",
            "Date formatted string matches 25.11.2023") && ok;
    }

    // Test 7: DataGridRowBuffer Time and Timestamp storage and retrieval
    {
        DataGridRowBuffer buffer(3);
        IBPP::Timestamp ts(2025, 12, 31, 23, 59, 58, 9990);
        buffer.setValue(0, ts.GetDate().GetDate());
        buffer.setValue(sizeof(int), ts.GetTime().GetTime());

        int vDate = 0, vTime = 0;
        buffer.getValue(0, vDate);
        buffer.getValue(sizeof(int), vTime);

        IBPP::Timestamp tsRead;
        tsRead.SetDate(vDate);
        tsRead.SetTime(IBPP::Time::tmNone, vTime, 0);

        int y = 0, mo = 0, d = 0, h = 0, mi = 0, s = 0, f = 0;
        tsRead.GetDate(y, mo, d);
        tsRead.GetTime(h, mi, s, f);

        ok = check(y == 2025 && mo == 12 && d == 31, "Timestamp buffer read date components") && ok;
        ok = check(h == 23 && mi == 59 && s == 58 && f == 9990, "Timestamp buffer read time components") && ok;
    }

    if (ok)
    {
        std::cout << "\nALL DATAGRID DATE TESTS PASSED!\n";
        return 0;
    }
    else
    {
        std::cerr << "\nSOME DATAGRID DATE TESTS FAILED!\n";
        return 1;
    }
}
