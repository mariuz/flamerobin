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

#include "gui/ShortcutCustomizationDialog.h"
#include "gui/StyleGuide.h"

enum
{
    ID_list_ctrl = 1000,
    ID_button_change
};

BEGIN_EVENT_TABLE(ShortcutCustomizationDialog, BaseDialog)
    EVT_BUTTON(ID_button_change, ShortcutCustomizationDialog::OnChangeShortcut)
    EVT_LIST_ITEM_ACTIVATED(ID_list_ctrl, ShortcutCustomizationDialog::OnListActivated)
END_EVENT_TABLE()

ShortcutCustomizationDialog::ShortcutCustomizationDialog(wxWindow* parent)
    : BaseDialog(parent, wxID_ANY, _("Customize Keyboard Shortcuts"))
{
    wxBoxSizer* sizerMain = new wxBoxSizer(wxVERTICAL);

    listCtrlM = new wxListCtrl(this, ID_list_ctrl, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    listCtrlM->InsertColumn(0, _("Command"), wxLIST_FORMAT_LEFT, 250);
    listCtrlM->InsertColumn(1, _("Shortcut"), wxLIST_FORMAT_LEFT, 150);

    sizerMain->Add(listCtrlM, 1, wxEXPAND | wxALL, styleguide().getDialogMargin(wxTOP));

    wxBoxSizer* sizerButtons = new wxBoxSizer(wxHORIZONTAL);
    wxButton* btnChange = new wxButton(this, ID_button_change, _("Change Shortcut..."));
    sizerButtons->Add(btnChange, 0, wxALL, styleguide().getRelatedControlMargin(wxHORIZONTAL));
    
    sizerButtons->AddStretchSpacer();
    
    sizerButtons->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxALL, styleguide().getRelatedControlMargin(wxHORIZONTAL));

    sizerMain->Add(sizerButtons, 0, wxEXPAND | wxALL, styleguide().getDialogMargin(wxTOP));

    SetSizerAndFit(sizerMain);
    SetSize(500, 400);

    CommandManager::getCustomizableCommands(commandsM);
    populateList();
}

void ShortcutCustomizationDialog::populateList()
{
    listCtrlM->DeleteAllItems();
    for (size_t i = 0; i < commandsM.size(); ++i)
    {
        long index = listCtrlM->InsertItem((long)i, commandsM[i].name);
        listCtrlM->SetItem(index, 1, commandManagerM.getShortcutText(commandsM[i].id));
        listCtrlM->SetItemData(index, (long)i);
    }
}

void ShortcutCustomizationDialog::OnChangeShortcut(wxCommandEvent& WXUNUSED(event))
{
    long selected = listCtrlM->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (selected == -1)
        return;

    size_t cmdIndex = (size_t)listCtrlM->GetItemData(selected);
    ShortcutCaptureDialog dlg(this, commandsM[cmdIndex].name);
    if (dlg.ShowModal() == wxID_OK)
    {
        commandManagerM.setShortcut(commandsM[cmdIndex].id, dlg.getFlags(), dlg.getKeyCode());
        commandManagerM.save();
        populateList();
    }
}

void ShortcutCustomizationDialog::OnListActivated(wxListEvent& WXUNUSED(event))
{
    wxCommandEvent evt;
    OnChangeShortcut(evt);
}

// ShortcutCaptureDialog implementation

BEGIN_EVENT_TABLE(ShortcutCaptureDialog, BaseDialog)
    EVT_KEY_DOWN(ShortcutCaptureDialog::OnKeyDown)
END_EVENT_TABLE()

ShortcutCaptureDialog::ShortcutCaptureDialog(wxWindow* parent, const wxString& commandName)
    : BaseDialog(parent, wxID_ANY, _("Capture Shortcut")), flagsM(0), keyCodeM(0)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(new wxStaticText(this, wxID_ANY, wxString::Format(_("Press the new shortcut for:\n%s"), commandName)), 0, wxALL | wxALIGN_CENTER, 10);

    shortcutTextM = new wxStaticText(this, wxID_ANY, _("Press a key combination..."), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER | wxST_NO_AUTORESIZE);
    shortcutTextM->SetFont(shortcutTextM->GetFont().MakeBold().MakeLarger());
    sizer->Add(shortcutTextM, 0, wxEXPAND | wxALL, 20);

    sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxALL | wxALIGN_CENTER, 10);

    SetSizerAndFit(sizer);
}

void ShortcutCaptureDialog::OnKeyDown(wxKeyEvent& event)
{
    int keyCode = event.GetKeyCode();
    
    // Ignore pure modifier keys
    if (keyCode == WXK_CONTROL || keyCode == WXK_SHIFT || keyCode == WXK_ALT 
        || keyCode == WXK_WINDOWS_LEFT || keyCode == WXK_WINDOWS_RIGHT)
    {
        event.Skip();
        return;
    }

    flagsM = 0;
    if (event.ControlDown()) flagsM |= wxACCEL_CTRL;
    if (event.AltDown()) flagsM |= wxACCEL_ALT;
    if (event.ShiftDown()) flagsM |= wxACCEL_SHIFT;
    
    keyCodeM = keyCode;

    shortcutTextM->SetLabel(CommandManager::getShortcutString(flagsM, keyCodeM));
}
