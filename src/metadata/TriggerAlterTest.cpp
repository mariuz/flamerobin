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

// Regression test for issue #625: "Alter Trigger show double AS"

#include <iostream>

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

namespace
{

bool needsASToAlterTrigger(const wxString& source)
{
    wxString trimmedSource = source;
    trimmedSource.Trim(false);
    wxString upperSource = trimmedSource.Upper();
    bool needAS = true;
    if (upperSource.StartsWith("AS"))
    {
        if (trimmedSource.length() == 2)
            needAS = false;
        else
        {
            wxChar c = trimmedSource[2];
            bool isIdentifierChar = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
            if (!isIdentifierChar)
                needAS = false;
        }
    }
    else if (upperSource.StartsWith("EXTERNAL"))
    {
        if (trimmedSource.length() == 8)
            needAS = false;
        else
        {
            wxChar c = trimmedSource[8];
            bool isIdentifierChar = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
            if (!isIdentifierChar)
                needAS = false;
        }
    }
    return needAS;
}

bool check(bool actual, bool expected, const char* testName)
{
    if (actual == expected)
        return true;
    std::cerr << testName << " failed. Expected: " << (expected ? "true" : "false")
              << ", Got: " << (actual ? "true" : "false") << "\n";
    return false;
}

} // namespace

int main()
{
    bool ok = true;

    // Normal triggers that already contain AS in their RDB$TRIGGER_SOURCE
    ok = check(needsASToAlterTrigger("AS\nBEGIN\nEND"), false, "AS with newline") && ok;
    ok = check(needsASToAlterTrigger("as\nBEGIN\nEND"), false, "as lowercase with newline") && ok;
    ok = check(needsASToAlterTrigger("AS BEGIN\nEND"), false, "AS with space") && ok;
    ok = check(needsASToAlterTrigger("  AS BEGIN\nEND"), false, "AS with leading spaces") && ok;
    ok = check(needsASToAlterTrigger("AS"), false, "exact AS") && ok;

    // Triggers that do not have AS (e.g. legacy/manually manipulated)
    ok = check(needsASToAlterTrigger("BEGIN\nEND"), true, "directly starts with BEGIN") && ok;
    ok = check(needsASToAlterTrigger("DECLARE VARIABLE X INT;\nBEGIN\nEND"), true, "starts with DECLARE") && ok;

    // External triggers (must not prepend AS)
    ok = check(needsASToAlterTrigger("EXTERNAL NAME 'my_ext' ENGINE 'UDR'"), false, "EXTERNAL trigger") && ok;
    ok = check(needsASToAlterTrigger("external name 'my_ext' engine 'udr'"), false, "external lowercase trigger") && ok;
    ok = check(needsASToAlterTrigger("  EXTERNAL NAME 'my_ext'"), false, "EXTERNAL trigger with leading spaces") && ok;

    // Edge cases: identifier that starts with "AS" but is not "AS" word
    ok = check(needsASToAlterTrigger("ASCENDING"), true, "starts with ASCENDING") && ok;
    ok = check(needsASToAlterTrigger("ASSOCIATED_FIELD = 1"), true, "starts with ASSOCIATED") && ok;

    return ok ? 0 : 1;
}
