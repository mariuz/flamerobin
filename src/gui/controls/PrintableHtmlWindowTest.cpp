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
#include "gui/controls/PrintableHtmlWindow.h"
#include "config/Config.h"

namespace
{
static bool check(bool condition, const char* testName)
{
    if (condition)
        return true;
    std::cerr << "FAILED: " << testName << "\n";
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
    std::cout << "Starting PrintableHtmlWindow SVG icon tests..." << std::endl;

    // Point config to repo source directory so html-templates/ can be found during tests
    wxFileName exeDir(wxStandardPaths::Get().GetExecutablePath());
    wxString repoDir = exeDir.GetPath(); // build dir
    wxFileName repoPath(repoDir, "");
    repoPath.RemoveLastDir(); // root of flamerobin repo
    config().setHomePath(repoPath.GetFullPath());

    // Test 1: embedSvgIconsInHtml with SVG references
    {
        std::cout << "Test 1: embedSvgIconsInHtml replaces SVG icon paths with data:image/svg+xml;base64..." << std::endl;
        wxString templatesPath = config().getHtmlTemplatesPath();
        wxString html = wxString::Format(
            "<html><head><title>Test</title></head><body>"
            "<img src=\"%sok2.svg\" height=\"16\" width=\"16\">"
            "<img src=\"file:///%sredx.svg\" height=\"16\" width=\"16\">"
            "</body></html>",
            templatesPath, templatesPath);

        wxString embedded = PrintableHtmlWindow::embedSvgIconsInHtml(html);
        ok = check(embedded.Contains("data:image/svg+xml;base64,"), "contains base64 data URI") && ok;
        ok = check(!embedded.Contains("ok2.svg"), "ok2.svg path replaced") && ok;
        ok = check(!embedded.Contains("redx.svg"), "redx.svg path replaced") && ok;
    }

    // Test 2: prepareHtmlForDisplay injects X-UA-Compatible and CSS
    {
        std::cout << "Test 2: prepareHtmlForDisplay injects X-UA-Compatible IE=edge and styling..." << std::endl;
        wxString html = "<html><head><title>Properties</title></head><body><h1>Content</h1></body></html>";
        wxString processed = PrintableHtmlWindow::prepareHtmlForDisplay(html);

        ok = check(processed.Contains("X-UA-Compatible"), "contains X-UA-Compatible meta tag") && ok;
        ok = check(processed.Contains("IE=edge"), "contains IE=edge") && ok;
        ok = check(processed.Contains("<style>"), "contains injected stylesheet") && ok;
    }

    // Test 3: prepareHtmlForDisplay handles templates with SVG icons
    {
        std::cout << "Test 3: prepareHtmlForDisplay handles full metadata page with SVG..." << std::endl;
        wxString templatesPath = config().getHtmlTemplatesPath();
        wxString html = wxString::Format(
            "<html><head><title>Privileges</title></head><body>"
            "<img src=\"%sok2.svg\" height=\"16\" width=\"16\">"
            "</body></html>",
            templatesPath);
        wxString processed = PrintableHtmlWindow::prepareHtmlForDisplay(html);

        ok = check(processed.Contains("data:image/svg+xml;base64,"), "SVG converted to base64 data URI in full pipeline") && ok;
        ok = check(processed.Contains("X-UA-Compatible"), "X-UA-Compatible present") && ok;
        ok = check(!processed.Contains("ok2.svg"), "ok2.svg path replaced") && ok;
    }

    if (ok)
    {
        std::cout << "All PrintableHtmlWindow SVG icon tests PASSED." << std::endl;
        return 0;
    }
    else
    {
        std::cerr << "Some PrintableHtmlWindow SVG icon tests FAILED." << std::endl;
        return 1;
    }
}
