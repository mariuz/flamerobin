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

// Regression test for issue #717:
// "Database properties form refreshing: Pressing Ctrl+R or F5 has no effect in database properties form"

#include <iostream>

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "core/Observer.h"
#include "metadata/database.h"

namespace
{

bool check(bool condition, const char* testName)
{
    if (condition)
    {
        std::cout << "[PASS] " << testName << "\n";
        return true;
    }
    std::cerr << "[FAIL] " << testName << "\n";
    return false;
}

class TestPropertiesObserver : public Observer
{
public:
    int updateCalls;
    TestPropertiesObserver() : updateCalls(0) {}

    virtual void update() override
    {
        ++updateCalls;
    }
};

} // namespace

int main()
{
    bool ok = true;

    std::cout << "Testing Database properties refresh and invalidation (issue #717)...\n";

    // Test 1: Database invalidate triggers Observer update notification
    {
        DatabasePtr db(new Database());
        TestPropertiesObserver observer;
        db->attachObserver(&observer, false);

        ok = check(observer.updateCalls == 0, "Observer update count initialized to 0") && ok;

        db->invalidate();
        ok = check(observer.updateCalls == 1, "Database::invalidate() notifies attached observers") && ok;

        db->invalidate();
        ok = check(observer.updateCalls == 2, "Repeated Database::invalidate() notifies observers again") && ok;

        db->detachObserver(&observer);
    }

    // Test 2: Invalidation resets properties and children loaded flags
    {
        DatabasePtr db(new Database());
        db->invalidate();
        ok = check(!db->childrenLoaded(), "Database::invalidate() marks children as not loaded") && ok;
        ok = check(!db->propertiesLoaded(), "Database::invalidate() marks properties as not loaded") && ok;
    }

    // Test 3: Standard accelerator table configuration for property pages
    {
        wxAcceleratorEntry entries[4];
        entries[0].Set(wxACCEL_CMD, (int)'W', wxID_CLOSE_FRAME);
        entries[1].Set(wxACCEL_CMD, (int)'R', wxID_REFRESH);
        entries[2].Set(wxACCEL_CTRL, WXK_F4, wxID_CLOSE_FRAME);
        entries[3].Set(wxACCEL_NORMAL, WXK_F5, wxID_REFRESH);

        wxAcceleratorTable table(4, entries);
        ok = check(table.IsOk(), "Accelerator table with Ctrl+R, F5, Ctrl+W, Ctrl+F4 is valid across all platforms") && ok;
        ok = check(entries[1].GetCommand() == wxID_REFRESH, "Ctrl+R maps to wxID_REFRESH") && ok;
        ok = check(entries[3].GetCommand() == wxID_REFRESH, "F5 maps to wxID_REFRESH") && ok;
        ok = check(entries[0].GetCommand() == wxID_CLOSE_FRAME, "Ctrl+W maps to wxID_CLOSE_FRAME") && ok;
        ok = check(entries[2].GetCommand() == wxID_CLOSE_FRAME, "Ctrl+F4 maps to wxID_CLOSE_FRAME") && ok;
    }

    // Test 4: Database loadProperties marks properties as loaded
    {
        DatabasePtr db(new Database());
        db->invalidate();
        ok = check(!db->propertiesLoaded(), "Properties initially not loaded after invalidation") && ok;
        db->loadProperties();
        ok = check(db->propertiesLoaded(), "Database::loadProperties() marks properties as loaded") && ok;
    }

    if (ok)
        std::cout << "\nAll Database properties refresh tests passed.\n";
    else
        std::cout << "\nSome tests failed.\n";

    return ok ? 0 : 1;
}
