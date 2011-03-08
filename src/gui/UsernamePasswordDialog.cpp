/*
  Copyright (c) 2004-2011 The FlameRobin Development Team

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

#include "gui/StyleGuide.h"
#include "gui/UsernamePasswordDialog.h"
//-----------------------------------------------------------------------------
UsernamePasswordDialog::UsernamePasswordDialog(wxWindow* parent,
        const wxString& title, const wxString& username, bool usernameIsFixed,
        const wxString& description)
    : BaseDialog(parent, wxID_ANY, title)
{
    wxWindow* panel = getControlsPanel();
    // create controls
    if (description.size())
        labelDescriptionM = new wxStaticText(panel, wxID_ANY, description);
    else
        labelDescriptionM = 0;
    labelUsernameM = new wxStaticText(panel, wxID_ANY, _("&Username:"));
    textUsernameM = new wxTextCtrl(panel, wxID_ANY, username);
    textUsernameM->SetEditable(!usernameIsFixed);
    labelPasswordM = new wxStaticText(panel, wxID_ANY, _("&Password:"));
    textPasswordM = new wxTextCtrl(panel, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    buttonOkM = new wxButton(panel, wxID_OK, _("Connect"));
    buttonCancelM = new wxButton(panel, wxID_CANCEL, _("Cancel"));
    layoutControls();
    if (!usernameIsFixed && username.size() == 0)
        textUsernameM->SetFocus();
    else
        textPasswordM->SetFocus();
    updateColors();
    updateButton();
    Connect(wxEVT_COMMAND_TEXT_UPDATED,
        wxCommandEventHandler(UsernamePasswordDialog::OnSettingsChange));
}
//-----------------------------------------------------------------------------
wxString UsernamePasswordDialog::getUsername()
{
    return textUsernameM->GetValue();
}
//-----------------------------------------------------------------------------
wxString UsernamePasswordDialog::getPassword()
{
    return textPasswordM->GetValue();
}
//-----------------------------------------------------------------------------
void UsernamePasswordDialog::layoutControls()
{
    // create sizer for controls
    wxBoxSizer* sizerControls = new wxBoxSizer(wxVERTICAL);
    if (labelDescriptionM)
    {
        sizerControls->Add(labelDescriptionM, 0, wxEXPAND);
        sizerControls->AddSpacer(
            styleguide().getUnrelatedControlMargin(wxVERTICAL));
    }

    wxFlexGridSizer* sizerGrid = new wxFlexGridSizer(2, 2,
        styleguide().getRelatedControlMargin(wxVERTICAL),
        styleguide().getControlLabelMargin());
    sizerGrid->AddGrowableCol(1);

    sizerGrid->Add(labelUsernameM, 0, wxALIGN_CENTER_VERTICAL);
    sizerGrid->Add(textUsernameM, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);
    sizerGrid->Add(labelPasswordM, 0, wxALIGN_CENTER_VERTICAL);
    sizerGrid->Add(textPasswordM, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);
    sizerControls->Add(sizerGrid, 0, wxEXPAND);

    // create sizer for buttons -> styleguide class will align it correctly
    wxSizer* sizerButtons = styleguide().createButtonSizer(buttonOkM,
        buttonCancelM);
    // use method in base class to set everything up
    layoutSizers(sizerControls, sizerButtons);
}
//-----------------------------------------------------------------------------
void UsernamePasswordDialog::updateButton()
{
    bool oldEnabled = buttonOkM->IsEnabled();
    bool newEnabled = (textUsernameM->GetValue().size() > 0)
        && (textPasswordM->GetValue().size() > 0);
    buttonOkM->Enable(newEnabled);
    if (!oldEnabled && newEnabled)
        buttonOkM->SetDefault();
}
//-----------------------------------------------------------------------------
void UsernamePasswordDialog::OnSettingsChange(wxCommandEvent& WXUNUSED(event))
{
    updateButton();
}
//-----------------------------------------------------------------------------
