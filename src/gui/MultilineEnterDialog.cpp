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

#include "gui/controls/TextControl.h"
#include "gui/MultilineEnterDialog.h"
#include "styleguide.h"
//-----------------------------------------------------------------------------
bool GetMultilineTextFromUser(const wxString& caption, wxString& value, 
    wxWindow* parent)
{
    MultilineEnterDialog med(parent, caption);
    med.setText(value);
    if (wxID_OK != med.ShowModal())
        return false;
    value = med.getText();
    return true;
}
//-----------------------------------------------------------------------------
MultilineEnterDialog::MultilineEnterDialog(wxWindow* parent, 
        const wxString& title)
    : BaseDialog(parent, wxID_ANY, title)
{
	text_ctrl_value = new TextControl(getControlsPanel());
    button_ok = new wxButton(getControlsPanel(), wxID_OK, _("Save"));
    button_cancel = new wxButton(getControlsPanel(), wxID_CANCEL, _("Cancel"));
    layoutControls();
    button_ok->SetDefault();
}
//-----------------------------------------------------------------------------
wxString MultilineEnterDialog::getText() const
{
    return text_ctrl_value->GetText();
}
//-----------------------------------------------------------------------------
void MultilineEnterDialog::setText(const wxString& text)
{
    text_ctrl_value->SetText(text);
}
//-----------------------------------------------------------------------------
const wxString MultilineEnterDialog::getName() const
{
    return wxT("MultilineEnterDialog");
}
//-----------------------------------------------------------------------------
void MultilineEnterDialog::layoutControls()
{
    wxBoxSizer* sizerControls = new wxBoxSizer(wxHORIZONTAL);
    text_ctrl_value->SetSizeHints(200, 100);
    text_ctrl_value->SetSize(200, 100);
    sizerControls->Add(text_ctrl_value, 1, wxEXPAND);
    wxSizer* sizerButtons = styleguide().createButtonSizer(button_ok, 
        button_cancel);
    layoutSizers(sizerControls, sizerButtons, true);
}
//-----------------------------------------------------------------------------
