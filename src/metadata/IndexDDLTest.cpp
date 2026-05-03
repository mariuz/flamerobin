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
#include <vector>

#include "metadata/IndexDDL.h"

namespace
{
bool check(const wxString& actual, const wxString& expected, const char* testName)
{
    if (actual == expected)
        return true;
    std::string expectedText(expected.mb_str());
    std::string actualText(actual.mb_str());
    std::cerr << testName << " failed.\n"
        << "Expected: " << expectedText << "\n"
        << "Actual:   " << actualText << "\n";
    return false;
}
}

int main()
{
    bool ok = true;

    std::vector<wxString> segments;
    segments.push_back("\"COL_A\"");
    segments.push_back("\"COL_B\"");

    ok = check(
        buildIndexBodySql(wxEmptyString, segments, wxEmptyString),
        " (\"COL_A\",\"COL_B\")",
        "regular index body")
        && ok;

    ok = check(
        buildIndexBodySql(wxEmptyString, segments, "COL_A IS NOT NULL"),
        " (\"COL_A\",\"COL_B\") WHERE COL_A IS NOT NULL",
        "partial index body")
        && ok;

    ok = check(
        buildIndexBodySql("UPPER(COL_A)", std::vector<wxString>(), "COL_A IS NOT NULL"),
        " COMPUTED BY UPPER(COL_A) WHERE COL_A IS NOT NULL",
        "computed partial index body")
        && ok;

    ok = check(
        buildIndexBodySql("EXTRACT(YEAR FROM COL_C)", std::vector<wxString>(), "COL_A > 0 AND COL_B < 10"),
        " COMPUTED BY EXTRACT(YEAR FROM COL_C) WHERE COL_A > 0 AND COL_B < 10",
        "complex computed partial index body")
        && ok;

    ok = check(
        buildIndexBodySql(wxEmptyString, segments, "COL_A IN (1, 2, 3)"),
        " (\"COL_A\",\"COL_B\") WHERE COL_A IN (1, 2, 3)",
        "partial index body with IN clause")
        && ok;

    return ok ? 0 : 1;
}
