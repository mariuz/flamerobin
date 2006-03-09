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
  are Copyright (C) 2006 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s): Michael Hieke
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
//----------------------------------------------------------------------------
#include <wx/artprov.h>
#include <wx/display.h>

#include "config/Config.h"
#include "gui/AdvancedMessageDialog.h"
#include "styleguide.h"
//----------------------------------------------------------------------------
AdvancedMessageDialog::AdvancedMessageDialog(wxWindow* parent,
        const wxString& message, const wxString& caption, int style,
        AdvancedMessageDialogButtons* buttons, const wxString& name)
    : BaseDialog(parent, wxID_ANY, caption, wxDefaultPosition, wxDefaultSize,
        wxDEFAULT_DIALOG_STYLE) // wxCAPTION)
{
    // setup the icon, default to wxICON_INFORMATION if no other was given
    wxArtID iconid = wxART_INFORMATION;
    if ((style & wxICON_QUESTION) == wxICON_QUESTION)
        iconid = wxART_QUESTION;
    else if ((style & wxICON_WARNING) == wxICON_WARNING)
        iconid = wxART_WARNING;
    else if ((style & wxICON_ERROR) == wxICON_ERROR)
        iconid = wxART_ERROR;

    // setup dialog buttons: add buttons set in "style" to list
    AdvancedMessageDialogButtons tempButtons;
    if (!buttons)
        buttons = &tempButtons;
    if ((style & wxYES_NO) == wxYES_NO)
    {
        buttons->add(wxYES, _("Yes"));
        buttons->add(wxNO, _("No"));
    }
    if ((style & wxOK) == wxOK)
        buttons->add(wxOK, _("OK"));
    if ((style & wxCANCEL) == wxCANCEL)
        buttons->add(wxCANCEL, _("Cancel"));
    if (buttons->size() == 0)
        buttons->add(wxOK, _("OK"));

    // create controls and sizers
    wxSizer* sizerControls = new wxBoxSizer(wxHORIZONTAL);
    wxStaticBitmap* bmp = new wxStaticBitmap(getControlsPanel(), wxID_ANY,
        wxArtProvider::GetBitmap(iconid, wxART_MESSAGE_BOX));
    sizerControls->Add(bmp, 0, wxALIGN_TOP);
    // TODO: get the spacing right for all toolkits
    sizerControls->AddSpacer(
        2 * styleguide().getUnrelatedControlMargin(wxHORIZONTAL));

    wxSizer* sizerText = new wxBoxSizer(wxVERTICAL);
    wxStaticText* text = new wxStaticText(getControlsPanel(), wxID_ANY,
        message);
    sizerText->Add(text);

    if (!name.IsEmpty())
    {
        checkBoxM = new wxCheckBox(getControlsPanel(), wxID_ANY,
            (buttons->size() > 1) ? _("Don't ask again") : _("Don't show again"));
        sizerText->AddSpacer(styleguide().getUnrelatedControlMargin(wxVERTICAL));
        sizerText->Add(checkBoxM, 0, wxEXPAND);
    }
    sizerControls->Add(sizerText);

    // create buttons and button sizer -- BIG TODO
    wxSizer* sizerButtons = new wxBoxSizer(wxHORIZONTAL);
    sizerButtons->AddStretchSpacer(1);
    wxButton* defaultBtn = 0;
    for (AdvancedMessageDialogButtons::const_iterator it = buttons->begin();
        it != buttons->end(); ++it)
    {
        wxButton* btn = new wxButton(getControlsPanel(), (*it).first, (*it).second);
        if (defaultBtn)
            sizerButtons->AddSpacer(styleguide().getBetweenButtonsMargin(wxHORIZONTAL));
        else
            defaultBtn = btn;
        sizerButtons->Add(btn);
        Connect((*it).first, wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(AdvancedMessageDialog::OnButtonClick));
    }

    layoutSizers(sizerControls, sizerButtons);

    if (defaultBtn)
    {
        defaultBtn->SetDefault();
        defaultBtn->SetFocus();
    }
}
//----------------------------------------------------------------------------
bool AdvancedMessageDialog::getDontShowAgain() const
{
    if (checkBoxM)
        return checkBoxM->IsChecked();
    return true;
}
//----------------------------------------------------------------------------
void AdvancedMessageDialog::OnButtonClick(wxCommandEvent& event)
{
    EndModal(event.GetId());
}
//----------------------------------------------------------------------------
int AdvancedMessageBox(const wxString& message,  const wxString& caption,
    int style, AdvancedMessageDialogButtons* buttons, wxWindow* parent,
    const wxString& keyname)
{
    int value;
    if (config().getValue(keyname, value))
        return value;

    if (!parent)
        parent = wxTheApp->GetTopWindow();
    AdvancedMessageDialog adm(parent, message, caption, style, buttons,
        keyname);
    value = adm.ShowModal();
    // Cancel means: cancel action, so it is not treated like a regular
    // "choice", but rather giving up on it (so, checkBox is ignored)
    if (!keyname.IsEmpty() && adm.getDontShowAgain() && value != wxCANCEL)
        config().setValue(keyname, value);
    return value;
}
//----------------------------------------------------------------------------
