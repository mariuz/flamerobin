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

  The Initial Developer of the Original Code is Michael Hieke.

  Portions created by the original developer
  are Copyright (C) 2005 Michael Hieke.

  All Rights Reserved.

  $Id$

  Contributor(s):
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

#include "gui/controls/TextControl.h"
//-----------------------------------------------------------------------------
TextControl::TextControl(wxWindow *parent, wxWindowID id, long style)
    : wxStyledTextCtrl(parent, id, wxDefaultPosition, wxDefaultSize, style)
{
    SetWrapMode(wxSTC_WRAP_WORD);
    // wxStyledTextCtrl uses black on white initially -> use system defaults
    resetStyles();
    // Hide margin area for line numbers and fold markers
	SetMarginWidth(1, 0); 
	SetMarginWidth(0, 0); 
}
//-----------------------------------------------------------------------------
void TextControl::resetStyles()
{
    StyleClearAll();
    // Use system default colours for selection
    SetSelBackground(true, 
        wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
    SetSelForeground(true, 
        wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
    // Use system default colours for text and background
    StyleSetBackground(wxSTC_STYLE_DEFAULT, 
        wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    StyleSetForeground(wxSTC_STYLE_DEFAULT, 
        wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    // Make all other styles use these default colours
    StyleClearAll();
}
//-----------------------------------------------------------------------------
//! event handling
BEGIN_EVENT_TABLE(TextControl, wxStyledTextCtrl)
    EVT_CONTEXT_MENU(TextControl::OnContextMenu)
    EVT_MENU(wxID_UNDO, TextControl::OnCommandUndo)
    EVT_MENU(wxID_REDO, TextControl::OnCommandRedo)
    EVT_MENU(wxID_CUT, TextControl::OnCommandCut)
    EVT_MENU(wxID_COPY, TextControl::OnCommandCopy)
    EVT_MENU(wxID_PASTE, TextControl::OnCommandPaste)
    EVT_MENU(wxID_CLEAR, TextControl::OnCommandDelete)
    EVT_MENU(wxID_SELECTALL, TextControl::OnCommandSelectAll)
    EVT_STC_START_DRAG(wxID_ANY, TextControl::OnStartDrag)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void TextControl::OnCommandUndo(wxCommandEvent& WXUNUSED(event))
{
    Undo();
}
//-----------------------------------------------------------------------------
void TextControl::OnCommandRedo(wxCommandEvent& WXUNUSED(event))
{
    Redo();
}
//-----------------------------------------------------------------------------
void TextControl::OnCommandCut(wxCommandEvent& WXUNUSED(event))
{
    Cut();
}
//-----------------------------------------------------------------------------
void TextControl::OnCommandCopy(wxCommandEvent& WXUNUSED(event))
{
    Copy();
}
//-----------------------------------------------------------------------------
void TextControl::OnCommandPaste(wxCommandEvent& WXUNUSED(event))
{
    Paste();
}
//-----------------------------------------------------------------------------
void TextControl::OnCommandDelete(wxCommandEvent& WXUNUSED(event))
{
    Clear();
}
//-----------------------------------------------------------------------------
void TextControl::OnCommandSelectAll(wxCommandEvent& WXUNUSED(event))
{
    SelectAll();
}
//-----------------------------------------------------------------------------
void TextControl::OnContextMenu(wxContextMenuEvent& WXUNUSED(event))
{
    wxMenu m(0);
    m.Append(wxID_UNDO, _("&Undo"));
    m.Append(wxID_REDO, _("&Redo"));
    m.AppendSeparator();
    m.Append(wxID_CUT, _("Cu&t"));
    m.Append(wxID_COPY, _("&Copy"));
    m.Append(wxID_PASTE, _("&Paste"));
    m.Append(wxID_CLEAR, _("&Delete"));
    m.AppendSeparator();
    m.Append(wxID_SELECTALL, _("Select &All"));

	// enable/disable commands according to control state
	m.Enable(wxID_UNDO, CanUndo());
	m.Enable(wxID_REDO, CanRedo());
    bool hasSelection = GetSelectionStart() != GetSelectionEnd();
	m.Enable(wxID_CUT, hasSelection);
	m.Enable(wxID_COPY, hasSelection);
    m.Enable(wxID_CLEAR, hasSelection);
    m.Enable(wxID_PASTE, CanPaste());

    PopupMenu(&m, ScreenToClient(::wxGetMousePosition()));
}
//-----------------------------------------------------------------------------
// TODO: Is this still necessary (?), because it works for me as-is
// Fix the annoying thing that you can not click inside the selection and 
// have it deselect the text and position the caret there
void TextControl::OnStartDrag(wxStyledTextEvent& event) // WXUNUSED(event))
{
    event.Skip();
/*
    wxPoint mp = wxGetMousePosition();
    int p = PositionFromPoint(ScreenToClient(mp));
	SetSelectionStart(p);
	SetSelectionEnd(p);
*/
}
//-----------------------------------------------------------------------------
