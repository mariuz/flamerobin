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
#include "gui/SchemaHtmlGenerator.h"

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
    std::cout << "Starting SchemaVisualization tests..." << std::endl;

    wxString dummyJson = "{\"databaseName\": \"test_db\", \"isDark\": true, \"tables\": []}";
    wxString html = fr::getSchemaHtmlTemplate(dummyJson);

    // Test 1: HTML template is not empty
    std::cout << "Test 1: HTML template is not empty..." << std::endl;
    ok = check(!html.IsEmpty(), "HTML template is not empty") && ok;

    // Test 2: Contains correct script type
    std::cout << "Test 2: Contains text/babel..." << std::endl;
    ok = check(html.Contains("type=\"text/babel\""), "HTML contains type=\"text/babel\"") && ok;

    // Test 3: Does NOT contain data-type="module" (prevents file:// CORS security errors)
    std::cout << "Test 3: Does NOT contain data-type=\"module\"..." << std::endl;
    ok = check(!html.Contains("data-type=\"module\""), "HTML does not contain data-type=\"module\"") && ok;

    // Test 4: Contains classic runtime preset registration and data-presets attribute
    std::cout << "Test 4: Contains classic runtime preset registration..." << std::endl;
    ok = check(html.Contains("Babel.registerPreset(\"react-classic\""), "HTML contains Babel.registerPreset(\"react-classic\")") && ok;
    ok = check(html.Contains("data-presets=\"react-classic\""), "HTML contains data-presets=\"react-classic\"") && ok;

    // Test 5: Contains comment directives for classic runtime
    std::cout << "Test 5: Contains jsxRuntime classic comments..." << std::endl;
    ok = check(html.Contains("@jsxRuntime classic"), "HTML contains @jsxRuntime classic comment") && ok;
    ok = check(html.Contains("@jsx React.createElement"), "HTML contains @jsx React.createElement comment") && ok;

    // Test 6: Contains the embedded dummy JSON
    std::cout << "Test 6: Contains embedded JSON schema..." << std::endl;
    ok = check(html.Contains(dummyJson), "HTML contains embedded JSON schema") && ok;

    if (ok)
    {
        std::cout << "All SchemaVisualization tests PASSED." << std::endl;
        return 0;
    }
    else
    {
        std::cerr << "Some SchemaVisualization tests FAILED." << std::endl;
        return 1;
    }
}
