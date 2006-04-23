/*
Copyright (c) 2004, 2005, 2006 The FlameRobin Development Team

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
//----------------------------------------------------------------------------
#include <wx/artprov.h>
#include <wx/display.h>

#include "config/Config.h"
#include "gui/AdvancedMessageDialog.h"
#include "gui/StyleGuide.h"
//----------------------------------------------------------------------------
#ifdef FR_NEWADVANCEDMESSAGEDIALOG
//----------------------------------------------------------------------------
AdvancedMessageDialogButtons::AdvancedMessageDialogButtons()
{
    affirmativeButtonM.id = wxID_ANY;
    alternateButtonM.id = wxID_ANY;
    negativeButtonM.id = wxID_ANY;
}
//----------------------------------------------------------------------------
void AdvancedMessageDialogButtons::addAffirmativeButton(int id,
    const wxString& caption)
{
    affirmativeButtonM.id = id;
    affirmativeButtonM.caption = caption;
}
//----------------------------------------------------------------------------
wxString& AdvancedMessageDialogButtons::getAffirmativeButtonCaption()
{
    return affirmativeButtonM.caption;
}
//----------------------------------------------------------------------------
int AdvancedMessageDialogButtons::getAffirmativeButtonId()
{
    return affirmativeButtonM.id;
}
//----------------------------------------------------------------------------
bool AdvancedMessageDialogButtons::getAffirmativeButtonUsed()
{
    return affirmativeButtonM.id != wxID_ANY;
}
//----------------------------------------------------------------------------
void AdvancedMessageDialogButtons::addAlternateButton(int id,
    const wxString& caption)
{
    alternateButtonM.id = id;
    alternateButtonM.caption = caption;
}
//----------------------------------------------------------------------------
wxString& AdvancedMessageDialogButtons::getAlternateButtonCaption()
{
    return alternateButtonM.caption;
}
//----------------------------------------------------------------------------
int AdvancedMessageDialogButtons::getAlternateButtonId()
{
    return alternateButtonM.id;
}
//----------------------------------------------------------------------------
bool AdvancedMessageDialogButtons::getAlternateButtonUsed()
{
    return alternateButtonM.id != wxID_ANY;
}
//----------------------------------------------------------------------------
void AdvancedMessageDialogButtons::addNegativeButton(int id,
    const wxString& caption)
{
    negativeButtonM.id = id;
    negativeButtonM.caption = caption;
}
//----------------------------------------------------------------------------
wxString& AdvancedMessageDialogButtons::getNegativeButtonCaption()
{
    return negativeButtonM.caption;
}
//----------------------------------------------------------------------------
int AdvancedMessageDialogButtons::getNegativeButtonId()
{
    return negativeButtonM.id;
}
//----------------------------------------------------------------------------
bool AdvancedMessageDialogButtons::getNegativeButtonUsed()
{
    return negativeButtonM.id != wxID_ANY;
}
//----------------------------------------------------------------------------
AdvancedMessageDialogButtonsOk::AdvancedMessageDialogButtonsOk(
    const wxString buttonOkCaption)
{
    addAffirmativeButton(wxID_OK, buttonOkCaption);
}
//-----------------------------------------------------------------------------
AdvancedMessageDialogButtonsOkCancel::AdvancedMessageDialogButtonsOkCancel(
    const wxString buttonOkCaption, const wxString buttonCancelCaption)
{
    addAffirmativeButton(wxID_OK, buttonOkCaption);
    addNegativeButton(wxID_CANCEL, buttonCancelCaption);
}
//-----------------------------------------------------------------------------
int showAdvancedMessageDialog(wxWindow* parent, int style,
    wxString primaryText, wxString secondaryText,
    AdvancedMessageDialogButtons& buttons, bool* showNeverAgain = 0)
{
    if (!parent)
        parent = ::wxGetActiveWindow();
    if (showNeverAgain)
        *showNeverAgain = false;
// TODO: show AdvancedMessageDialog
    return wxCANCEL;
}
//-----------------------------------------------------------------------------
int showAdvancedMessageDialog(wxWindow* parent, int style,
    wxString primaryText, wxString secondaryText,
    AdvancedMessageDialogButtons buttons, Config& config, wxString configKey)
{
    int value;
    if (config.getValue(configKey, value))
        return value;

    bool showNeverAgain = false;
    value = showAdvancedMessageDialog(parent, style, primaryText,
        secondaryText, buttons, &showNeverAgain);
    // wxID_CANCEL means: cancel action, so it is not treated like a regular
    // "choice"; the checkbox ("Don't show/ask again") is ignored even if set
    if (value != wxCANCEL && !configKey.empty() && showNeverAgain)
        config.setValue(configKey, value);
    return value;
}
//-----------------------------------------------------------------------------
int showInformationDialog(wxWindow* parent, wxString primaryText,
    wxString secondaryText, AdvancedMessageDialogButtons& buttons)
{
    return showAdvancedMessageDialog(parent, wxICON_INFORMATION, primaryText,
        secondaryText, buttons);
}
//-----------------------------------------------------------------------------
int showInformationDialog(wxWindow* parent, wxString primaryText,
    wxString secondaryText, AdvancedMessageDialogButtons buttons,
    Config& config, wxString configKey)
{
    return showAdvancedMessageDialog(parent, wxICON_INFORMATION, primaryText,
        secondaryText, buttons, config, configKey);
}
//-----------------------------------------------------------------------------
int showQuestionDialog(wxWindow* parent, wxString primaryText,
    wxString secondaryText, AdvancedMessageDialogButtons& buttons)
{
    return showAdvancedMessageDialog(parent, wxICON_QUESTION, primaryText,
        secondaryText, buttons);
}
//-----------------------------------------------------------------------------
int showQuestionDialog(wxWindow* parent, wxString primaryText,
    wxString secondaryText, AdvancedMessageDialogButtons buttons,
    Config& config, wxString configKey)
{
    return showAdvancedMessageDialog(parent, wxICON_QUESTION, primaryText,
        secondaryText, buttons, config, configKey);
}
//-----------------------------------------------------------------------------


#else // FR_NEWADVANCEDMESSAGEDIALOG
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
#endif // FR_NEWADVANCEDMESSAGEDIALOG
//----------------------------------------------------------------------------
