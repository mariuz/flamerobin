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
#include "core/URIProcessor.h"

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

int main(int argc, char** argv)
{
    wxInitializer initializer(argc, argv);
    if (!initializer.IsOk())
    {
        std::cerr << "Failed to initialize wxWidgets.\n";
        return 1;
    }

    bool ok = true;
    std::cout << "Starting URIProcessor tests..." << std::endl;

    // Test 1: Standard URI with params
    {
        std::cout << "Test 1: Standard URI with params..." << std::endl;
        URI uri("fr://page?type=dependencies&parent_window=12345");
        ok = check(uri.protocol == "fr", "protocol matches 'fr'") && ok;
        ok = check(uri.action == "page", "action matches 'page'") && ok;
        ok = check(uri.getParam("type") == "dependencies", "type param matches 'dependencies'") && ok;
        ok = check(uri.getParam("parent_window") == "12345", "parent_window param matches '12345'") && ok;
    }

    // Test 2: URI with trailing slash in action (normalized by MSHTML/IE)
    {
        std::cout << "Test 2: URI with trailing slash in action (MSHTML normalization)..." << std::endl;
        URI uri("fr://page/?type=dependencies&parent_window=12345");
        ok = check(uri.protocol == "fr", "protocol matches 'fr'") && ok;
        ok = check(uri.action == "page", "action matches 'page' with trailing slash stripped") && ok;
        ok = check(uri.getParam("type") == "dependencies", "type param matches 'dependencies'") && ok;
        ok = check(uri.getParam("parent_window") == "12345", "parent_window param matches '12345'") && ok;
    }

    // Test 3: Standard URI without params
    {
        std::cout << "Test 3: Standard URI without params..." << std::endl;
        URI uri("fr://refresh");
        ok = check(uri.protocol == "fr", "protocol matches 'fr'") && ok;
        ok = check(uri.action == "refresh", "action matches 'refresh'") && ok;
    }

    // Test 4: URI without params with trailing slash
    {
        std::cout << "Test 4: URI without params with trailing slash..." << std::endl;
        URI uri("fr://refresh/");
        ok = check(uri.protocol == "fr", "protocol matches 'fr'") && ok;
        ok = check(uri.action == "refresh", "action matches 'refresh' with trailing slash stripped") && ok;
    }

    // Test 5: URI with multiple trailing slashes
    {
        std::cout << "Test 5: URI with multiple trailing slashes..." << std::endl;
        URI uri("fr://close_frame///");
        ok = check(uri.protocol == "fr", "protocol matches 'fr'") && ok;
        ok = check(uri.action == "close_frame", "action matches 'close_frame' with multiple slashes stripped") && ok;
    }

    // Test 6: Invalid URI without scheme delimiter
    {
        std::cout << "Test 6: Invalid URI..." << std::endl;
        URI uri;
        bool res = uri.parseURI("invalid_uri_string");
        ok = check(!res, "parseURI returns false for invalid URI") && ok;
    }

    if (ok)
    {
        std::cout << "All URIProcessor tests PASSED." << std::endl;
        return 0;
    }
    else
    {
        std::cerr << "Some URIProcessor tests FAILED." << std::endl;
        return 1;
    }
}
