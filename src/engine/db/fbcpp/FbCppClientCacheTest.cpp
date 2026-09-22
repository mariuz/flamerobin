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

// Regression test for GitHub issue #721:
// "Firebird embedded on Linux"
//
// A database can be registered with its own "Client library", which is how an
// embedded database is pointed at a Firebird installation that actually has an
// engine plugin.  That used to be stored in a single process wide static, and
// the client was only built the first time any database connected, so whichever
// database connected first decided the client library for the whole
// application.  Clients must be cached per library path instead.

#include <iostream>
#include <string>
#include <vector>

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/filename.h>

#include "engine/db/fbcpp/FbCppDatabase.h"

namespace
{

bool check(bool condition, const char* testName)
{
    std::cout << (condition ? "  PASSED: " : "  FAILED: ") << testName << "\n";
    return condition;
}

// A Firebird client library other than the one FlameRobin is linked against,
// so that the two can be told apart.  Any of these that exists will do.
std::string findOtherClientLibrary()
{
    const char* candidates[] = {
        "/opt/firebird/lib/libfbclient.so",
        "/usr/lib/x86_64-linux-gnu/libfbclient.so.2",
        "/usr/lib64/libfbclient.so.2",
        "/usr/lib/libfbclient.so.2"
    };
    for (const char* candidate : candidates)
    {
        if (wxFileName::FileExists(candidate))
            return candidate;
    }
    return std::string();
}

} // namespace

int main()
{
    std::cout << "Running Firebird client cache tests (issue #721)...\n";
    bool ok = true;

    // Test 1: the linked in client is created once and reused.
    fbcpp::Client& linked = fr::FbCppDatabase::getClient();
    ok = check(&linked == &fr::FbCppDatabase::getClient(),
        "The default client is cached") && ok;
    ok = check(fr::FbCppDatabase::isClientInitialized(),
        "Creating a client marks the client as initialized") && ok;

    // Test 2: a database that brings its own client library gets that library,
    // not whichever one happened to be loaded first.
    const std::string other = findOtherClientLibrary();
    if (other.empty())
    {
        std::cout << "  SKIPPED: no second Firebird client library on this machine\n";
    }
    else
    {
        std::cout << "  using " << other << " as the second client library\n";
        fbcpp::Client& custom = fr::FbCppDatabase::getClient(other);
        ok = check(&custom != &linked,
            "A configured client library is not served by the default client") && ok;
        ok = check(&custom == &fr::FbCppDatabase::getClient(other),
            "A configured client library is cached under its own path") && ok;
        ok = check(&linked == &fr::FbCppDatabase::getClient(),
            "The default client survives loading another one") && ok;
    }

    std::cout << "\nAll Firebird client cache tests " << (ok ? "PASSED" : "FAILED") << ".\n";
    return ok ? 0 : 1;
}
