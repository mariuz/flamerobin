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

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif
#include <iostream>
#include "gui/CommandManager.h"

namespace
{
static bool check(bool condition, const char* testName)
{
    if (condition)
        return true;
    std::cerr << testName << " failed.\n";
    return false;
}

static bool checkString(const wxString& actual, const wxString& expected, const char* testName)
{
    if (actual == expected)
        return true;
    std::cerr << testName << " failed.\n"
        << "Expected: " << (const char*)expected.mb_str() << "\n"
        << "Actual:   " << (const char*)actual.mb_str() << "\n";
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
    std::cout << "Starting CommandManager tests..." << std::endl;
    CommandManager& cm = CommandManager::get();

    // Test 1: getShortcutText is public and works for default commands
    std::cout << "Test 1: getShortcutText..." << std::endl;
    // wxID_SELECTALL is initialized to Ctrl+A in CommandManager::init()
    ok = checkString(cm.getShortcutText(wxID_SELECTALL), "Ctrl+A", "getShortcutText(wxID_SELECTALL)") && ok;

    // Test 2: setShortcut and getShortcutText
    std::cout << "Test 2: setShortcut..." << std::endl;
    cm.setShortcut(wxID_ABOUT, wxACCEL_CTRL | wxACCEL_SHIFT, 'H');
    ok = checkString(cm.getShortcutText(wxID_ABOUT), "Ctrl+Shift+H", "setShortcut/getShortcutText") && ok;

    // Test 3: getShortcutString static method
    std::cout << "Test 3: getShortcutString..." << std::endl;
    ok = checkString(CommandManager::getShortcutString(wxACCEL_ALT, WXK_F1), "Alt+F1", "getShortcutString(Alt+F1)") && ok;

    // Test 4: parseShortcutString static method
    std::cout << "Test 4: parseShortcutString..." << std::endl;
    int flags, keyCode;
    if (check(CommandManager::parseShortcutString("Ctrl+Alt+P", flags, keyCode), "parseShortcutString success"))
    {
        ok = check(flags == (wxACCEL_CTRL | wxACCEL_ALT), "parse flags") && ok;
        ok = check(keyCode == 'P', "parse keyCode") && ok;
    }
    else ok = false;

    // Test 5: Verify WXK_WINDOWS constants are available
    std::cout << "Test 5: WXK_WINDOWS constants..." << std::endl;
    int k1 = WXK_WINDOWS_LEFT;
    int k2 = WXK_WINDOWS_RIGHT;
    ok = check(k1 != 0 && k2 != 0, "WXK_WINDOWS constants available") && ok;

    if (ok)
        std::cout << "All CommandManager tests PASSED." << std::endl;
    return ok ? 0 : 1;
}
