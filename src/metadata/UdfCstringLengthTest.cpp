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

namespace
{

short getAdjustedLength(short type, short length, short bpc)
{
    if (bpc > 1 && (type == 14 || type == 37 || type == 40))
        return length / bpc;
    return length;
}

bool check(short actual, short expected, const char* testName)
{
    if (actual == expected)
        return true;
    std::cerr << testName << " failed. Expected: " << expected
              << ", Got: " << actual << "\n";
    return false;
}

} // namespace

int main()
{
    bool ok = true;

    // Type 40 = CSTRING
    ok = check(getAdjustedLength(40, 40, 4), 10, "CSTRING length 40 in UTF-8 (bpc=4)") && ok;
    ok = check(getAdjustedLength(40, 10, 1), 10, "CSTRING length 10 in ASCII (bpc=1)") && ok;
    ok = check(getAdjustedLength(40, 30, 3), 10, "CSTRING length 30 with 3 bytes per char") && ok;

    // Type 14 = CHAR
    ok = check(getAdjustedLength(14, 40, 4), 10, "CHAR length 40 in UTF-8 (bpc=4)") && ok;

    // Type 37 = VARCHAR
    ok = check(getAdjustedLength(37, 40, 4), 10, "VARCHAR length 40 in UTF-8 (bpc=4)") && ok;

    // Other types should not be adjusted (e.g. integer = 8, float = 10, etc.)
    ok = check(getAdjustedLength(8, 4, 4), 4, "INTEGER length 4 in UTF-8 (should not change)") && ok;
    ok = check(getAdjustedLength(27, 8, 4), 8, "DOUBLE PRECISION length 8 in UTF-8 (should not change)") && ok;

    return ok ? 0 : 1;
}
