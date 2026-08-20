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
#include <wx/wx.h>
#include "gui/ExecuteRoutineDialog.h"

namespace
{
bool check(bool condition, const char* testName)
{
    if (condition)
    {
        std::cout << "[PASS] " << testName << std::endl;
        return true;
    }
    std::cerr << "[FAIL] " << testName << std::endl;
    return false;
}
}

int main()
{
    bool ok = true;

    // Test 1: Ensure ExecuteRoutineDialog control IDs do not collide with wxID_CLOSE (5001) or standard stock IDs
    ok = check(static_cast<int>(ExecuteRoutineDialog::ID_button_execute_routine) != static_cast<int>(wxID_CLOSE),
        "ExecuteRoutineDialog::ID_button_execute_routine does not equal wxID_CLOSE") && ok;
    ok = check(static_cast<int>(ExecuteRoutineDialog::ID_button_execute_routine) != static_cast<int>(wxID_OPEN),
        "ExecuteRoutineDialog::ID_button_execute_routine does not equal wxID_OPEN") && ok;
    ok = check(static_cast<int>(ExecuteRoutineDialog::ID_button_open_in_editor) != static_cast<int>(wxID_NEW),
        "ExecuteRoutineDialog::ID_button_open_in_editor does not equal wxID_NEW") && ok;
    ok = check(static_cast<int>(ExecuteRoutineDialog::ID_button_open_in_editor) != static_cast<int>(wxID_CLOSE),
        "ExecuteRoutineDialog::ID_button_open_in_editor does not equal wxID_CLOSE") && ok;

    // Ensure IDs are outside wx standard stock range [wxID_LOWEST .. wxID_HIGHEST]
    ok = check(static_cast<int>(ExecuteRoutineDialog::ID_button_execute_routine) < static_cast<int>(wxID_LOWEST) ||
               static_cast<int>(ExecuteRoutineDialog::ID_button_execute_routine) > static_cast<int>(wxID_HIGHEST),
        "ExecuteRoutineDialog::ID_button_execute_routine is outside standard wx stock ID range") && ok;
    ok = check(static_cast<int>(ExecuteRoutineDialog::ID_button_open_in_editor) < static_cast<int>(wxID_LOWEST) ||
               static_cast<int>(ExecuteRoutineDialog::ID_button_open_in_editor) > static_cast<int>(wxID_HIGHEST),
        "ExecuteRoutineDialog::ID_button_open_in_editor is outside standard wx stock ID range") && ok;

    return ok ? 0 : 1;
}
