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
//----------------------------------------------------------------------------
#include <wx/artprov.h>
#include <vector>
#include <utility>
#include "config/Config.h"
#include "AdvancedMessageDialog.h"
//----------------------------------------------------------------------------
AdvancedMessageDialog::AdvancedMessageDialog(wxWindow* parent,
    const wxString& message, const wxString& caption, int style,
    AdvancedMessageDialogButtons* buttons, const wxString& name)
    :wxDialog(parent, -1, caption, wxDefaultPosition, wxDefaultSize,
     wxCAPTION)
{
    wxBoxSizer *bSizer1 = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *btnSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *topSizer = new wxBoxSizer(wxHORIZONTAL);

    // get the text size, so we can determine the minimal textctrl size
    wxScreenDC dc;
    wxCoord w, h, dw, dh;
    dc.GetMultiLineTextExtent(message, &w, &h);
    ::wxDisplaySize(&dw, &dh);      // I hate dialogs that go off the screen
    if (w > dw*0.8)                 // This may happen if we decide to
        w = dw*0.8;                 // print the backtrace in case of crash
    if (h > dh*0.8)                 // and similar stuff with a lot of text
        h = dh*0.8;

    wxTextCtrl *messageM = new wxTextCtrl(this, -1, message,
        wxDefaultPosition, wxSize(w, h), wxTE_MULTILINE|wxTE_READONLY|
        wxTE_RICH|wxTE_AUTO_URL|wxNO_BORDER|wxTE_NO_VSCROLL);
    messageM->SetOwnBackgroundColour(
        wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    messageM->SetInsertionPointEnd();

    // setup the icon
    wxArtID iconid = wxART_MISSING_IMAGE;
    if ((style & wxICON_QUESTION) == wxICON_QUESTION)
        iconid = wxART_QUESTION;
    else if ((style & wxICON_WARNING) == wxICON_WARNING)
        iconid = wxART_WARNING;
    else if ((style & wxICON_ERROR) == wxICON_ERROR)
        iconid = wxART_ERROR;
    else if ((style & wxICON_INFORMATION) == wxICON_INFORMATION)
        iconid = wxART_INFORMATION;
    if (iconid != wxART_MISSING_IMAGE)
    {
        wxStaticBitmap* sb = new wxStaticBitmap(this, -1,
            wxArtProvider::GetBitmap(iconid, wxART_MESSAGE_BOX));
        topSizer->Add(sb, 0, wxALL, 15);
    }

    topSizer->Add(messageM, 1, wxALL|wxEXPAND, 15);
    bSizer1->Add(topSizer, 0, wxEXPAND, 5);

    // add buttons set in "style" to the list
    AdvancedMessageDialogButtons temp;
    if (!buttons)
        buttons = &temp;
    if ((style & wxYES_NO) == wxYES_NO)
    {
        buttons->add(wxYES, _("Yes"));
        buttons->add(wxNO, _("No"));
    }
    if ((style & wxOK) == wxOK)
        buttons->add(wxOK, _("OK"));
    if ((style & wxCANCEL) == wxCANCEL)
        buttons->add(wxCANCEL, _("Cancel"));

    // create buttons
    wxButton* defaultBtn = 0;
    for (AdvancedMessageDialogButtons::const_iterator it = buttons->begin();
        it != buttons->end(); ++it)
    {
        wxButton *b = new wxButton(this, (*it).first, (*it).second);
        if (!defaultBtn)
            defaultBtn = b;
        btnSizer->Add(b, 0, wxALL, 5);
        Connect((*it).first, wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(AdvancedMessageDialog::OnButtonClick));
    }

    bSizer1->Add(btnSizer, 0, wxALIGN_RIGHT, 5);

    if (!name.IsEmpty())
    {
        checkBoxM = new wxCheckBox(this, -1, buttons->size() > 1 ?
            _("Don't ask again") : _("Don't show again"));
        bSizer1->Add(checkBoxM, 0, wxALL|wxEXPAND, 5);
    }

    SetSizer(bSizer1);
    bSizer1->Fit(this);
    SetAutoLayout(true);
    Layout();
    CentreOnParent();
    if (defaultBtn)
        defaultBtn->SetDefault();
}
//----------------------------------------------------------------------------
bool AdvancedMessageDialog::dontShowAgain() const
{
    if (!checkBoxM)
        return true;
    else
        return checkBoxM->IsChecked();
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
    AdvancedMessageDialog adm(parent, message, caption, style, buttons,
        keyname);
    value = adm.ShowModal();
    // Cancel means: cancel action, so it is not treated like a regular
    // "choice", but rather giving up on it (so, checkBox is ignored)
    if (!keyname.IsEmpty() && adm.dontShowAgain() && value != wxCANCEL)
        config().setValue(keyname, value);
    return value;
}
//----------------------------------------------------------------------------
