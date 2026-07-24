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
#include "core/JsonExpressionHelper.h"

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

    // Test 1: JSON validation
    wxString errorMsg;
    bool valid = JsonExpressionHelper::validateJson("{\"name\": \"FlameRobin\", \"version\": 26}", errorMsg);
    ok = check(valid, "validateJson valid syntax") && ok;

    bool invalid = JsonExpressionHelper::validateJson("{invalid json}", errorMsg);
    ok = check(!invalid, "validateJson invalid syntax") && ok;

    // Test 2: JSON formatting
    wxString formatted = JsonExpressionHelper::formatJson("{\"a\":1,\"b\":2}");
    ok = check(formatted.Find("\n") != wxNOT_FOUND, "formatJson formats multiline") && ok;

    // Test 3: JSON_VALUE generation
    wxString exprValue = JsonExpressionHelper::generateJsonValue("DOC_DATA", "$.user.id");
    ok = check(exprValue.Find("JSON_VALUE") != wxNOT_FOUND && exprValue.Find("$.user.id") != wxNOT_FOUND, "generateJsonValue expression") && ok;

    // Test 4: JSON_QUERY & JSON_EXISTS generation
    wxString exprQuery = JsonExpressionHelper::generateJsonQuery("DOC_DATA", "$.items");
    ok = check(exprQuery.Find("JSON_QUERY") != wxNOT_FOUND, "generateJsonQuery expression") && ok;

    wxString exprExists = JsonExpressionHelper::generateJsonExists("DOC_DATA", "$.active");
    ok = check(exprExists.Find("JSON_EXISTS") != wxNOT_FOUND, "generateJsonExists expression") && ok;

    return ok ? 0 : 1;
}
