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

  $Id:  $

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
#include <vector>
#include <utility>
#include "config/Config.h"
#include "AdvancedMessageDialog.h"
//----------------------------------------------------------------------------
AdvancedMessageDialog::AdvancedMessageDialog(wxWindow* parent,
    const wxString& message, const wxString& caption,
    const AdvancedMessageDialogButtons& buttons, const wxString& name)
    :wxDialog(parent, -1, caption)
{
    wxBoxSizer *bSizer1 = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *bSizer2 = new wxBoxSizer(wxHORIZONTAL);
    messageM = new wxStaticText(this, -1, message, wxDefaultPosition,
        wxDefaultSize, wxALIGN_CENTRE);
    bSizer1->Add(messageM, 1, wxALL|wxEXPAND, 20);

    checkBoxM = new wxCheckBox(this, -1, _("Don't ask again"));
    if (!name.IsEmpty())
        bSizer1->Add(checkBoxM, 0, wxALL|wxEXPAND, 5);

    for (AdvancedMessageDialogButtons::const_iterator it = buttons.begin();
        it != buttons.end(); ++it)
    {
        wxButton *b = new wxButton(this, (*it).first, (*it).second);
        bSizer2->Add(b, 0, wxALL, 5);
        Connect((*it).first, wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(AdvancedMessageDialog::OnButtonClick));
    }
    bSizer1->Add(bSizer2, 0, wxEXPAND, 5);
    SetSizer(bSizer1);
    bSizer1->Fit(this);
    SetAutoLayout(true);
    Layout();
}
//----------------------------------------------------------------------------
bool AdvancedMessageDialog::dontShowAgain() const
{
    return checkBoxM->IsChecked();
}
//----------------------------------------------------------------------------
void AdvancedMessageDialog::OnButtonClick(wxCommandEvent& event)
{
    EndModal(event.GetId());
}
//----------------------------------------------------------------------------
int AdvancedMessageBox(const wxString& message,  const wxString& caption,
    const AdvancedMessageDialogButtons& buttons, const wxString& keyname)
{
    int value;
    if (config().getValue(keyname, value))
        return value;
    AdvancedMessageDialog adm(0, message, caption, buttons, keyname);
    value = adm.ShowModal();
    // Cancel means: cancel action, so it is not treated like a regular
    // "choice", but rather giving up on it (so, checkBox is ignored)
    if (!keyname.IsEmpty() && adm.dontShowAgain() && value != wxCANCEL)
        config().setValue(keyname, value);
    return value;
}
//----------------------------------------------------------------------------
