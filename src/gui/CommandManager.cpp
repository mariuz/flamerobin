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
#include <wx/tokenzr.h>

#include "config/Config.h"
#include "gui/CommandIds.h"
#include "gui/CommandManager.h"

CommandManager::CommandManager()
{
    init();
    load();
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
        return getShortcutString(flags, keyCode);
    }
    return wxEmptyString;
}

wxString CommandManager::getShortcutString(int flags, int keyCode)
{
    wxString flagsText;
    if (flags & wxACCEL_SHIFT)
        flagsText += _("Shift+");
    if (flags & wxACCEL_CTRL)
        flagsText += _("Ctrl+");
    if (flags & wxACCEL_ALT)
        flagsText += _("Alt+");

    wxAcceleratorEntry ae(wxACCEL_NORMAL, keyCode);
    return flagsText + ae.ToString();
}

bool CommandManager::parseShortcutString(const wxString& s, int& flags, int& keyCode)
{
    flags = wxACCEL_NORMAL;
    wxStringTokenizer tkz(s, "+-");
    wxString lastToken;
    while (tkz.HasMoreTokens())
    {
        lastToken = tkz.GetNextToken().Trim(false).Trim(true);
        if (lastToken.IsSameAs("Ctrl", false))
            flags |= wxACCEL_CTRL;
        else if (lastToken.IsSameAs("Alt", false))
            flags |= wxACCEL_ALT;
        else if (lastToken.IsSameAs("Shift", false))
            flags |= wxACCEL_SHIFT;
        else
            break;
    }
    
    if (lastToken.empty())
        return false;

    // Use wxAcceleratorEntry to parse the key part if possible, 
    // or handle it manually for simple chars
    wxAcceleratorEntry ae;
    if (ae.FromString("Ctrl+" + lastToken))
    {
        keyCode = ae.GetKeyCode();
        return true;
    }
    return false;
}

void CommandManager::setShortcut(int id, int flags, int keyCode)
{
    shortcutsM.erase(id);
    ShortCutData scd;
    scd.flags = flags;
    scd.keyCode = keyCode;
    shortcutsM.insert(ShortCutDataPair(id, scd));
}

void CommandManager::getShortcut(int id, int& flags, int& keyCode)
{
    if (!findShortcutFor(id, flags, keyCode))
    {
        flags = 0;
        keyCode = 0;
    }
}

static wxString getCommandConfigKey(int id)
{
    // Ideally we would have symbolic names, but for now we use ID
    // Better: use a map of ID -> Name
    // For now, let's use "Shortcuts/ID_%d"
    return wxString::Format("Shortcuts/ID_%d", id);
}

void CommandManager::load()
{
    // Load from config
    // We need a list of all IDs we care about
    // For now, we'll just try to load anything that was previously saved
    // But we don't know the keys. 
    // Actually, we should probably iterate over all commands in shortcutsM
    for (ShortCutDataMap::iterator it = shortcutsM.begin(); it != shortcutsM.end(); ++it)
    {
        wxString key = getCommandConfigKey(it->first);
        wxString val;
        if (config().getValue(key, val))
        {
            int f, k;
            if (parseShortcutString(val, f, k))
            {
                it->second.flags = f;
                it->second.keyCode = k;
            }
        }
    }
}

void CommandManager::save()
{
    for (ShortCutDataMap::const_iterator it = shortcutsM.begin(); it != shortcutsM.end(); ++it)
    {
        wxString key = getCommandConfigKey(it->first);
        wxString val = getShortcutString(it->second.flags, it->second.keyCode);
        config().setValue(key, val);
    }
}

/* static */
void CommandManager::getCustomizableCommands(CommandInfoVector& commands)
{
    CommandInfo ci;

    // Standard commands
    ci.id = wxID_NEW; ci.name = _("New"); commands.push_back(ci);
    ci.id = wxID_OPEN; ci.name = _("Open"); commands.push_back(ci);
    ci.id = wxID_SAVE; ci.name = _("Save"); commands.push_back(ci);
    ci.id = wxID_SAVEAS; ci.name = _("Save As"); commands.push_back(ci);
    ci.id = wxID_CLOSE; ci.name = _("Close"); commands.push_back(ci);
    ci.id = wxID_UNDO; ci.name = _("Undo"); commands.push_back(ci);
    ci.id = wxID_REDO; ci.name = _("Redo"); commands.push_back(ci);
    ci.id = wxID_CUT; ci.name = _("Cut"); commands.push_back(ci);
    ci.id = wxID_COPY; ci.name = _("Copy"); commands.push_back(ci);
    ci.id = wxID_PASTE; ci.name = _("Paste"); commands.push_back(ci);
    ci.id = wxID_DELETE; ci.name = _("Delete"); commands.push_back(ci);
    ci.id = wxID_SELECTALL; ci.name = _("Select All"); commands.push_back(ci);
    ci.id = wxID_FIND; ci.name = _("Find"); commands.push_back(ci);

    // Query commands
    ci.id = Cmds::Query_Execute; ci.name = _("Execute Query"); commands.push_back(ci);
    ci.id = Cmds::Query_Commit; ci.name = _("Commit Transaction"); commands.push_back(ci);
    ci.id = Cmds::Query_Rollback; ci.name = _("Rollback Transaction"); commands.push_back(ci);
    ci.id = Cmds::Query_Show_plan; ci.name = _("Show Execution Plan"); commands.push_back(ci);

    // View commands
    ci.id = Cmds::View_Editor; ci.name = _("View Editor"); commands.push_back(ci);
    ci.id = Cmds::View_Statistics; ci.name = _("View Statistics"); commands.push_back(ci);
    ci.id = Cmds::View_Data; ci.name = _("View Data"); commands.push_back(ci);
    ci.id = Cmds::View_SplitView; ci.name = _("Toggle Split View"); commands.push_back(ci);
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

