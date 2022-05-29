/*
  Copyright (c) 2004-2022 The FlameRobin Development Team

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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/accel.h>
#include <wx/stockitem.h>

#include "gui/CommandIds.h"
#include "gui/CommandManager.h"

CommandManager::CommandManager()
{
    init();
}

bool CommandManager::findShortcutFor(int id, int& flags, int& keyCode)
{
    // our own commands
    ShortCutDataMap::const_iterator it;
    it = shortcutsM.find(id);
    if (it != shortcutsM.end())
    {
        ShortCutData scd(it->second);
        flags = scd.flags;
        keyCode = scd.keyCode;
        return true;
    }

    // standard commands
    wxAcceleratorEntry ae(wxGetStockAccelerator(id));
    if (ae.IsOk())
    {
        flags = ae.GetFlags();
        keyCode = ae.GetKeyCode();
        return true;
    }

    return false;
}

wxString CommandManager::getShortcutText(int id)
{
    int flags, keyCode;
    if (findShortcutFor(id, flags, keyCode))
    {
        // with different flags != wxACCEL_NORMAL ToString() will return stuff
        // like "Alt-Ctrl-X" -> fix this to read "Ctrl+Alt+X" instead
        wxString flagsText;
        if (flags & wxACCEL_SHIFT)
            flagsText += _("Shift+");
        if (flags & wxACCEL_CTRL)
            flagsText += _("Ctrl+");
        if (flags & wxACCEL_ALT)
            flagsText += _("Alt+");

        wxAcceleratorEntry ae(wxACCEL_NORMAL, keyCode, id);
        return flagsText + ae.ToString();
    }
    return wxEmptyString;
}

wxString CommandManager::getMainMenuItemText(const wxString& text, int id)
{
    wxString shortcut(getShortcutText(id));
    return (shortcut.empty() ? text : text + "\t" + shortcut);
}

wxString CommandManager::getPopupMenuItemText(const wxString& text, int id)
{
    bool appendShortcuts = false;
    // user interface guidelines state that popup menus should not
    // show keyboard shortcuts
    // could however be activated for wxGTK here...
    if (appendShortcuts)
    {
        wxString shortcut(getShortcutText(id));
        if (!shortcut.empty())
            return text + "\t" + shortcut;
    }
    return text;
}

wxString CommandManager::getToolbarHint(const wxString& text, int id)
{
    wxString shortcut(getShortcutText(id));
    return (shortcut.empty() ? text : text + " (" + shortcut + ")");
}

void CommandManager::init()
{
    ShortCutData scd;

    // missing from the stock command list: "Select All"
    scd.flags = wxACCEL_CTRL;
    scd.keyCode = 'A';
    shortcutsM.insert(ShortCutDataPair(wxID_SELECTALL, scd));
    // missing from the stock command list / wrong shortcut for Windows:
    // "Undo", "Redo"
    scd.flags = wxACCEL_CTRL;
    scd.keyCode = 'Z';
    shortcutsM.insert(ShortCutDataPair(wxID_UNDO, scd));
    if (wxPlatformInfo::Get().GetOperatingSystemId() & wxOS_WINDOWS)
        scd.keyCode = 'Y';
    else
        scd.flags = wxACCEL_CTRL | wxACCEL_SHIFT;
    shortcutsM.insert(ShortCutDataPair(wxID_REDO, scd));

    // statement execution commands
    scd.flags = wxACCEL_NORMAL;
    scd.keyCode = WXK_F4;
    shortcutsM.insert(ShortCutDataPair(Cmds::Query_Execute, scd));
    scd.keyCode = WXK_F5;
    shortcutsM.insert(ShortCutDataPair(Cmds::Query_Commit, scd));
    scd.keyCode = WXK_F8;
    shortcutsM.insert(ShortCutDataPair(Cmds::Query_Rollback, scd));

    // view commands
    scd.flags = wxACCEL_CTRL | wxACCEL_ALT;
    scd.keyCode = 'E';
    shortcutsM.insert(ShortCutDataPair(Cmds::View_Editor, scd));
    scd.keyCode = 'L';
    shortcutsM.insert(ShortCutDataPair(Cmds::View_Statistics, scd));
    scd.keyCode = 'D';
    shortcutsM.insert(ShortCutDataPair(Cmds::View_Data, scd));
    scd.keyCode = 'S';
    shortcutsM.insert(ShortCutDataPair(Cmds::View_SplitView, scd));
}

