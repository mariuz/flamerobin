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
#include <string>
#include "engine/db/DatabaseBackend.h"

namespace
{
static bool check(bool condition, const char* testName)
{
    if (condition)
        return true;
    std::cerr << testName << " failed.\n";
    return false;
}
}

int main()
{
    bool ok = true;
    std::cout << "Starting MaintenanceConfig tests..." << std::endl;

    fr::MaintenanceConfig config;
    config.dbPath = "test.fdb";
    config.parallel = 4;
    config.flags = fr::MaintenanceFlags::Sweep;

    ok = check(config.dbPath == "test.fdb", "dbPath initialization") && ok;
    ok = check(config.parallel == 4, "parallel workers initialization") && ok;
    ok = check(config.flags == fr::MaintenanceFlags::Sweep, "flags initialization") && ok;

    // Test bitwise operations (if they were supported, but they are enum class)
    // Actually they are handled as int in IbppService.cpp
    
    int flags = (int)fr::MaintenanceFlags::Validate | (int)fr::MaintenanceFlags::Full;
    ok = check(flags == 6, "bitwise OR of flags") && ok;

    if (ok)
        std::cout << "All MaintenanceConfig tests PASSED." << std::endl;
    return ok ? 0 : 1;
}
