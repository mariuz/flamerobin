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

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "sql/SqlFormatter.h"
#include "config/Config.h"

namespace
{

bool checkStr(const wxString& actual, const wxString& expected,
    const char* testName)
{
    if (actual == expected)
        return true;
    std::string exp(expected.mb_str()), act(actual.mb_str());
    std::cerr << testName << " failed.\n"
        << "  Expected:\n[" << exp << "]\n"
        << "  Actual:\n[" << act << "]\n";
    return false;
}

} // namespace

int main()
{
    // Override user home paths to avoid wxStandardPaths::Get() segfault
    config().setUserHomePath(wxGetCwd());
    config().setHomePath(wxGetCwd());

    bool ok = true;

    // Set configuration values for formatting
    config().setValue("FormatterIndentSpaces", 4);
    config().setValue("FormatterKeywordCase", 0); // UPPERCASE
    config().setValue("FormatterOneColumnPerLine", true);

    // Test 1: Simple SELECT statement formatting
    {
        wxString sql = "select col1, col2 from my_table where id = 5;";
        wxString expected = 
            "SELECT\n"
            "    col1,\n"
            "    col2\n"
            "FROM my_table\n"
            "WHERE id = 5;";
        wxString actual = SqlFormatter::format(sql, 2, 5);
        ok = checkStr(actual, expected, "Simple SELECT statement formatting") && ok;
    }

    // Test 2: Subquery formatting
    {
        wxString sql = "select a, b from t1 where x in (select y from t2 where z = 1);";
        wxString expected = 
            "SELECT\n"
            "    a,\n"
            "    b\n"
            "FROM t1\n"
            "WHERE x IN (\n"
            "    SELECT\n"
            "        y\n"
            "    FROM t2\n"
            "    WHERE z = 1\n"
            ");";
        wxString actual = SqlFormatter::format(sql, 2, 5);
        ok = checkStr(actual, expected, "Subquery formatting") && ok;
    }

    // Test 3: Formatting with different keyword cases (lowercase)
    {
        config().setValue("FormatterKeywordCase", 1); // lowercase
        wxString sql = "SELECT col1 FROM my_table WHERE id = 5;";
        wxString expected = 
            "select\n"
            "    col1\n"
            "from my_table\n"
            "where id = 5;";
        wxString actual = SqlFormatter::format(sql, 11, 2);
        ok = checkStr(actual, expected, "Lowercase keywords formatting") && ok;
    }

    // Test 4: One column per line is false
    {
        config().setValue("FormatterKeywordCase", 0); // UPPERCASE
        config().setValue("FormatterOneColumnPerLine", false);
        wxString sql = "select col1, col2 from my_table where id = 5;";
        wxString expected = 
            "SELECT col1, col2\n"
            "FROM my_table\n"
            "WHERE id = 5;";
        wxString actual = SqlFormatter::format(sql, 2, 5);
        ok = checkStr(actual, expected, "OneColumnPerLine = false formatting") && ok;
    }

    // Test 5: Firebird Version Detection / Version-specific keywords
    // In FB 3.0, "PACKAGE" is a keyword. In FB 2.5, it is not.
    {
        config().setValue("FormatterKeywordCase", 0); // UPPERCASE
        config().setValue("FormatterOneColumnPerLine", true);
        wxString sql = "select package from my_table;";
        
        // Under FB 2.5: package is not a keyword, so it shouldn't be capitalized if written in lower/mixed case.
        wxString expected25 = 
            "SELECT\n"
            "    package\n"
            "FROM my_table;";
        wxString actual25 = SqlFormatter::format(sql, 11, 2); // ODS 11.2 is FB 2.5
        ok = checkStr(actual25, expected25, "FB 2.5 package keyword detection") && ok;

        // Under FB 3.0: package is a keyword, so it should be capitalized.
        wxString expected30 = 
            "SELECT\n"
            "    PACKAGE\n"
            "FROM my_table;";
        wxString actual30 = SqlFormatter::format(sql, 12, 0); // ODS 12.0 is FB 3.0
        ok = checkStr(actual30, expected30, "FB 3.0 package keyword detection") && ok;
    }

    // Test 6: BETWEEN ... AND ... clause spacing/newlines
    {
        wxString sql = "select col1 from t where val between 1 and 10 and other = 2;";
        wxString expected = 
            "SELECT\n"
            "    col1\n"
            "FROM t\n"
            "WHERE val BETWEEN 1 AND 10\n"
            "AND other = 2;";
        wxString actual = SqlFormatter::format(sql, 2, 5);
        ok = checkStr(actual, expected, "BETWEEN ... AND ... clause formatting") && ok;
    }

    return ok ? 0 : 1;
}
