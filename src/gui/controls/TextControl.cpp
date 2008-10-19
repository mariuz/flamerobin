/*
  Copyright (c) 2004-2008 The FlameRobin Development Team

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


  $Id$

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
    EVT_MENU(wxID_DELETE, TextControl::OnCommandDelete)
    EVT_MENU(wxID_SELECTALL, TextControl::OnCommandSelectAll)

    EVT_UPDATE_UI(wxID_UNDO, TextControl::OnCommandUpdateUndo)
    EVT_UPDATE_UI(wxID_REDO, TextControl::OnCommandUpdateRedo)
    EVT_UPDATE_UI(wxID_CUT, TextControl::OnCommandUpdateCut)
    EVT_UPDATE_UI(wxID_COPY, TextControl::OnCommandUpdateCopy)
    EVT_UPDATE_UI(wxID_PASTE, TextControl::OnCommandUpdatePaste)
    EVT_UPDATE_UI(wxID_DELETE, TextControl::OnCommandUpdateDelete)
    EVT_UPDATE_UI(wxID_SELECTALL, TextControl::OnCommandUpdateSelectAll)
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
void TextControl::OnCommandUpdateUndo(wxUpdateUIEvent& event)
{
    event.Enable(CanUndo());
}
//-----------------------------------------------------------------------------
void TextControl::OnCommandUpdateRedo(wxUpdateUIEvent& event)
{
    event.Enable(CanRedo());
}
//-----------------------------------------------------------------------------
void TextControl::OnCommandUpdateCut(wxUpdateUIEvent& event)
{
    event.Enable(!GetReadOnly() && GetSelectionStart() != GetSelectionEnd());
}
//-----------------------------------------------------------------------------
void TextControl::OnCommandUpdateCopy(wxUpdateUIEvent& event)
{
    event.Enable(GetSelectionStart() != GetSelectionEnd());
}
//-----------------------------------------------------------------------------
void TextControl::OnCommandUpdatePaste(wxUpdateUIEvent& event)
{
    event.Enable(!GetReadOnly());
}
//-----------------------------------------------------------------------------
void TextControl::OnCommandUpdateDelete(wxUpdateUIEvent& event)
{
    event.Enable(!GetReadOnly() && GetSelectionStart() != GetSelectionEnd());
}
//-----------------------------------------------------------------------------
void TextControl::OnCommandUpdateSelectAll(wxUpdateUIEvent& event)
{
    event.Enable(CanUndo());
}
//-----------------------------------------------------------------------------
void TextControl::OnContextMenu(wxContextMenuEvent& WXUNUSED(event))
{
    SetFocus();

    wxMenu m(0);
    m.Append(wxID_UNDO, _("&Undo"));
    m.Append(wxID_REDO, _("&Redo"));
    m.AppendSeparator();
    m.Append(wxID_CUT, _("Cu&t"));
    m.Append(wxID_COPY, _("&Copy"));
    m.Append(wxID_PASTE, _("&Paste"));
    m.Append(wxID_DELETE, _("&Delete"));
    m.AppendSeparator();
    m.Append(wxID_SELECTALL, _("Select &All"));

    PopupMenu(&m, ScreenToClient(::wxGetMousePosition()));
}
//-----------------------------------------------------------------------------
