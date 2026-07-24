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
#include <vector>
#include <wx/string.h>
#include "metadata/SchemaRefactoringHelper.h"

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

    // Test 1: qualifyObjectReferences
    wxString sql = "SELECT * FROM CUSTOMERS WHERE ID = 1;";
    std::vector<wxString> objects = {"CUSTOMERS"};
    wxString qualified = SchemaRefactoringHelper::qualifyObjectReferences(sql, "SALES", objects);
    ok = check(qualified.Find("SALES") != wxNOT_FOUND, "qualifyObjectReferences adds schema prefix") && ok;

    // Test 2: getDropSchemaStatement RESTRICT
    wxString dropRestrict = SchemaRefactoringHelper::getDropSchemaStatement("HR", false);
    ok = check(dropRestrict.Find("DROP SCHEMA") != wxNOT_FOUND && dropRestrict.Find("RESTRICT") != wxNOT_FOUND, "getDropSchemaStatement RESTRICT") && ok;

    // Test 3: getDropSchemaStatement CASCADE
    wxString dropCascade = SchemaRefactoringHelper::getDropSchemaStatement("HR", true);
    ok = check(dropCascade.Find("CASCADE") != wxNOT_FOUND, "getDropSchemaStatement CASCADE") && ok;

    return ok ? 0 : 1;
}
