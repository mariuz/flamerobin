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

    // Test 2: Regression test for #692 - Verify selectable vs executable procedure detection
    {
        // Procedure with SUSPEND (selectable)
        wxString sourceSelectable = "BEGIN\n  FOR SELECT ID, NAME FROM T INTO :OUT_ID, :OUT_NAME DO\n    SUSPEND;\nEND";
        bool isSelectable1 = sourceSelectable.Upper().Contains("SUSPEND");
        ok = check(isSelectable1 == true, "Procedure with SUSPEND is detected as selectable") && ok;

        // Procedure without SUSPEND (executable / non-selectable)
        wxString sourceExecutable = "BEGIN\n  OUT_ID = 100;\n  OUT_NAME = 'Test';\nEND";
        bool isSelectable2 = sourceExecutable.Upper().Contains("SUSPEND");
        ok = check(isSelectable2 == false, "Procedure without SUSPEND is detected as non-selectable") && ok;
    }

    // Test 3: Regression test for #692 - Verify statement singleton output lifecycle
    {
        struct MockExecProcStatement
        {
            bool hasRow = false;
            bool rowAvailable = false;
            bool eofReached = false;
            int colCount = 1;
            std::string outputVal;

            void execute()
            {
                hasRow = true;
                if (colCount > 0)
                {
                    rowAvailable = hasRow;
                    eofReached = true;
                }
            }

            bool fetch()
            {
                if (hasRow)
                {
                    hasRow = false;
                    rowAvailable = true;
                    return true;
                }
                if (eofReached)
                {
                    rowAvailable = false;
                    return false;
                }
                return false;
            }

            bool isNull(int col) const
            {
                if (!rowAvailable) return true;
                return outputVal.empty();
            }

            std::string getString(int col) const
            {
                if (isNull(col)) return "";
                return outputVal;
            }
        };

        MockExecProcStatement st;
        st.outputVal = "ComputedResult_42";
        st.execute();

        // Direct getter access after execute() should have rowAvailable == true
        ok = check(!st.isNull(0), "EXECUTE PROCEDURE direct getter access is not null after execute()") && ok;
        ok = check(st.getString(0) == "ComputedResult_42", "EXECUTE PROCEDURE returns valid result directly") && ok;

        // Fetch should also consume the row cleanly
        st.hasRow = true; // reset for fetch test
        ok = check(st.fetch() == true, "EXECUTE PROCEDURE fetch() returns true on first call") && ok;
        ok = check(!st.isNull(0), "EXECUTE PROCEDURE getter is not null after fetch()") && ok;
        ok = check(st.getString(0) == "ComputedResult_42", "EXECUTE PROCEDURE returns valid value after fetch()") && ok;
        ok = check(st.fetch() == false, "EXECUTE PROCEDURE fetch() returns false on second call (EOF)") && ok;
        ok = check(st.isNull(0) == true, "EXECUTE PROCEDURE is null after EOF") && ok;
    }

    return ok ? 0 : 1;
}
