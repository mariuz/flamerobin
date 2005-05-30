/*
The contents of this file are subject to the Initial Developer's Public
License Version 1.0 (the "License"); you may not use this file except in
compliance with the License. You may obtain a copy of the License here:
http://www.flamerobin.org/license.html.

Software distributed under the License is distributed on an "AS IS"
basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
License for the specific language governing rights and limitations under
the License.

The Original Code is FlameRobin (TM).

The Initial Developer of the Original Code is Milan Babuskov.

Portions created by the original developer
are Copyright (C) 2004 Milan Babuskov.

All Rights Reserved.

$Id$

Contributor(s): Michael Hieke, Nando Dessena
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <wx/clipbrd.h>
#include <string>
#include "MultilineEnterDialog.h"
#include "styleguide.h"
//-----------------------------------------------------------------------------
bool GetMultilineTextFromUser(const wxString& caption, wxString& value, wxWindow *parent)
{
    wxString result(value);
    MultilineEnterDialog med(parent, caption, result);
    if (wxID_OK != med.ShowModal())
        return false;
    value = med.getText();
    return true;
}
//-----------------------------------------------------------------------------
MultilineEnterDialog::MultilineEnterDialog(wxWindow* parent, const wxString& title, const wxString& initialText)
    : BaseDialog(parent, -1, title)
{
	text_ctrl_value = new TextCtrlWithContextMenu(getControlsPanel(), initialText);
    button_ok = new wxButton(getControlsPanel(), ID_button_ok, _("Save"));
    button_cancel = new wxButton(getControlsPanel(), ID_button_cancel, _("Cancel"));
    do_layout();
    button_ok->SetDefault();
}
//-----------------------------------------------------------------------------
void MultilineEnterDialog::do_layout()
{
    wxBoxSizer* sizerControls = new wxBoxSizer(wxHORIZONTAL);
    text_ctrl_value->SetSizeHints(200, 100);
    text_ctrl_value->SetSize(200, 100);
    sizerControls->Add(text_ctrl_value, 1, wxEXPAND);
    wxSizer* sizerButtons = styleguide().createButtonSizer(button_ok, button_cancel);
    layoutSizers(sizerControls, sizerButtons, true);
}
//-----------------------------------------------------------------------------
const std::string MultilineEnterDialog::getName() const
{
    return "MultilineEnterDialog";
}
//-----------------------------------------------------------------------------
wxString MultilineEnterDialog::getText() const
{
    return text_ctrl_value->GetText();
}
//-----------------------------------------------------------------------------
TextCtrlWithContextMenu::TextCtrlWithContextMenu(wxWindow* parent, const wxString& initialText)
	:wxStyledTextCtrl(parent, myId, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER)
{
	SetMarginWidth(1, 0);			// turn off the margins
	SetMarginWidth(0, 0);
    SetWrapMode(wxSTC_WRAP_WORD);
	
	SetText(initialText);
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(TextCtrlWithContextMenu, wxStyledTextCtrl)
    EVT_CONTEXT_MENU(TextCtrlWithContextMenu::OnContextMenu)
	EVT_STC_START_DRAG(TextCtrlWithContextMenu::myId, TextCtrlWithContextMenu::OnStartDrag)
    EVT_MENU(TextCtrlWithContextMenu::ID_MENU_UNDO,             TextCtrlWithContextMenu::OnMenuUndo)
    EVT_MENU(TextCtrlWithContextMenu::ID_MENU_REDO,             TextCtrlWithContextMenu::OnMenuRedo)
    EVT_MENU(TextCtrlWithContextMenu::ID_MENU_CUT,              TextCtrlWithContextMenu::OnMenuCut)
    EVT_MENU(TextCtrlWithContextMenu::ID_MENU_COPY,             TextCtrlWithContextMenu::OnMenuCopy)
    EVT_MENU(TextCtrlWithContextMenu::ID_MENU_PASTE,            TextCtrlWithContextMenu::OnMenuPaste)
    EVT_MENU(TextCtrlWithContextMenu::ID_MENU_DELETE,           TextCtrlWithContextMenu::OnMenuDelete)
    EVT_MENU(TextCtrlWithContextMenu::ID_MENU_SELECT_ALL,       TextCtrlWithContextMenu::OnMenuSelectAll)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
// Avoiding the annoying thing that you cannot click inside the selection and have it deselected and have caret there
void TextCtrlWithContextMenu::OnStartDrag(wxStyledTextEvent& WXUNUSED(event))
{
	wxPoint mp = ::wxGetMousePosition();
    int p = PositionFromPoint(ScreenToClient(mp));
	SetSelectionStart(p);	// deselect text
	SetSelectionEnd(p);
}
//-----------------------------------------------------------------------------
void TextCtrlWithContextMenu::OnContextMenu(wxContextMenuEvent& WXUNUSED(event))
{
    wxMenu m(0);
    m.Append(ID_MENU_UNDO, _("Undo"));
    m.Append(ID_MENU_REDO, _("Redo"));
    m.AppendSeparator();
    m.Append(ID_MENU_CUT,    _("Cut"));
    m.Append(ID_MENU_COPY,   _("Copy"));
    m.Append(ID_MENU_PASTE,  _("Paste"));
    m.Append(ID_MENU_DELETE, _("Delete"));
    m.AppendSeparator();
    m.Append(ID_MENU_SELECT_ALL,       _("Select All"));

	// disable stuff
	m.Enable(ID_MENU_UNDO, CanUndo());
	m.Enable(ID_MENU_REDO, CanRedo());
	if (GetSelectionStart() == GetSelectionEnd())		// nothing is selected
	{
		m.Enable(ID_MENU_CUT,              false);
		m.Enable(ID_MENU_COPY,             false);
		m.Enable(ID_MENU_DELETE,           false);
	}

    PopupMenu(&m, ScreenToClient(::wxGetMousePosition()));
}
//-----------------------------------------------------------------------------
void TextCtrlWithContextMenu::OnMenuUndo(wxCommandEvent& WXUNUSED(event))
{
	Undo();
}
//-----------------------------------------------------------------------------
void TextCtrlWithContextMenu::OnMenuRedo(wxCommandEvent& WXUNUSED(event))
{
	Redo();
}
//-----------------------------------------------------------------------------
void TextCtrlWithContextMenu::OnMenuCut(wxCommandEvent& WXUNUSED(event))
{
	Cut();
}
//-----------------------------------------------------------------------------
void TextCtrlWithContextMenu::OnMenuCopy(wxCommandEvent& WXUNUSED(event))
{
	Copy();
}
//-----------------------------------------------------------------------------
void TextCtrlWithContextMenu::OnMenuPaste(wxCommandEvent& WXUNUSED(event))
{
	Paste();
}
//-----------------------------------------------------------------------------
void TextCtrlWithContextMenu::OnMenuDelete(wxCommandEvent& WXUNUSED(event))
{
	Clear();
}
//-----------------------------------------------------------------------------
void TextCtrlWithContextMenu::OnMenuSelectAll(wxCommandEvent& WXUNUSED(event))
{
	SelectAll();
}
//-----------------------------------------------------------------------------
