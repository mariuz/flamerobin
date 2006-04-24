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
void AdvancedMessageDialogButtons::addAlternateButton(int id,
    const wxString& caption)
{
    alternateButtonM.id = id;
    alternateButtonM.caption = caption;
}
//----------------------------------------------------------------------------
void AdvancedMessageDialogButtons::addNegativeButton(int id,
    const wxString& caption)
{
    negativeButtonM.id = id;
    negativeButtonM.caption = caption;
}
//----------------------------------------------------------------------------
wxButton* AdvancedMessageDialogButtons::createAffirmativeButton(
    wxWindow* parent)
{
    if (affirmativeButtonM.id == wxID_ANY)
        return 0;
    return new wxButton(parent, affirmativeButtonM.id,
        affirmativeButtonM.caption);
}
//----------------------------------------------------------------------------
wxButton* AdvancedMessageDialogButtons::createAlternateButton(
    wxWindow* parent)
{
    if (alternateButtonM.id == wxID_ANY)
        return 0;
    return new wxButton(parent, alternateButtonM.id,
        alternateButtonM.caption);
}
//----------------------------------------------------------------------------
wxButton* AdvancedMessageDialogButtons::createNegativeButton(
    wxWindow* parent)
{
    if (negativeButtonM.id == wxID_ANY)
        return 0;
    return new wxButton(parent, negativeButtonM.id, negativeButtonM.caption);
}
//----------------------------------------------------------------------------
int AdvancedMessageDialogButtons::getNumberOfButtons()
{
    int count = 0;
    if (affirmativeButtonM.id != wxID_ANY)
        ++count;
    if (alternateButtonM.id != wxID_ANY)
        ++count;
    if (negativeButtonM.id != wxID_ANY)
        ++count;
    return count;
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
AdvancedMessageDialog::AdvancedMessageDialog(wxWindow* parent, wxArtID iconId,
        const wxString& primaryText, const wxString& secondaryText,
        AdvancedMessageDialogButtons& buttons, bool showCheckBoxNeverAgain)
    : BaseDialog(parent, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
    checkBoxM = 0;
#ifdef __WXMSW__
    SetCaption(wxT("FlameRobin"));
#endif
    wxBoxSizer* controlsSizer = new wxBoxSizer(wxHORIZONTAL);

/* TODO
#ifdef __WXMAC__
    // application icon instead of message box icon
    wxStaticBitmap* iconBmp = new wxStaticBitmap(getControlsPanel(), wxID_ANY,
        
#else
*/
    // message box icon
    wxStaticBitmap* iconBmp = new wxStaticBitmap(getControlsPanel(), wxID_ANY,
        wxArtProvider::GetBitmap(iconId, wxART_MESSAGE_BOX));
// #endif
    controlsSizer->Add(iconBmp);
    controlsSizer->AddSpacer(styleguide().getMessageBoxIconMargin());

    wxBoxSizer* textSizer = new wxBoxSizer(wxVERTICAL);
    // primary and secondary texts
    wxStaticText* labelPrimary = new wxStaticText(getControlsPanel(),
        wxID_ANY, primaryText);
    wxFont primaryLabelFont(labelPrimary->GetFont());
    primaryLabelFont.SetWeight(wxBOLD);
#ifdef __WXGTK__
    primaryLabelFont.SetPointSize(primaryLabelFont.GetPointSize() * 4 / 3);
#endif
    labelPrimary->SetFont(primaryLabelFont);
    textSizer->Add(labelPrimary, 0, wxEXPAND);
    textSizer->AddSpacer(styleguide().getMessageBoxBetweenTextMargin());
    wxStaticText* labelSecondary = new wxStaticText(getControlsPanel(),
        wxID_ANY, secondaryText);
    textSizer->Add(labelSecondary, 0, wxEXPAND);
    
    // checkbox for "Don't show/ask again"
    if (showCheckBoxNeverAgain)
    {
        textSizer->AddSpacer(
            styleguide().getUnrelatedControlMargin(wxVERTICAL));
        bool ask = buttons.getNumberOfButtons() > 1;
        checkBoxM = new wxCheckBox(getControlsPanel(), wxID_ANY,
            ask ? _("Don't ask again") : _("Don't show again"));
        textSizer->Add(checkBoxM, 0, wxEXPAND);
    }

    controlsSizer->Add(textSizer);

    // command buttons
    wxButton* affirmativeButton = buttons.createAffirmativeButton(
        getControlsPanel());
    if (affirmativeButton)
    {
        Connect(affirmativeButton->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(AdvancedMessageDialog::OnButtonClick));
    }
    wxButton* alternateButton = buttons.createAlternateButton(
        getControlsPanel());
    if (alternateButton)
    {
        Connect(alternateButton->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(AdvancedMessageDialog::OnButtonClick));
    }
    wxButton* negativeButton = buttons.createNegativeButton(
        getControlsPanel());
    if (negativeButton)
    {
        Connect(negativeButton->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(AdvancedMessageDialog::OnButtonClick));
    }

    if (affirmativeButton)
    {
        affirmativeButton->SetDefault();
        affirmativeButton->SetFocus();
    }
    else if (alternateButton)
    {
        alternateButton->SetDefault();
        alternateButton->SetFocus();
    }
    else if (negativeButton)
    {
        negativeButton->SetDefault();
        negativeButton->SetFocus();
    }

    // create sizer for buttons -> styleguide class will align it correctly
    wxSizer* buttonSizer = styleguide().createButtonSizer(affirmativeButton,
        negativeButton, alternateButton);
    // use method in base class to set everything up
    layoutSizers(controlsSizer, buttonSizer);
}
//-----------------------------------------------------------------------------
bool AdvancedMessageDialog::getDontShowAgain() const
{
    return checkBoxM != 0 && checkBoxM->IsChecked();
}
//-----------------------------------------------------------------------------
void AdvancedMessageDialog::OnButtonClick(wxCommandEvent& event)
{
    switch (event.GetId())
    {
        case wxID_OK:
            EndModal(wxOK);
            break;
        case wxID_YES:
            EndModal(wxYES);
            break;
        case wxID_NO:
            EndModal(wxNO);
            break;
        case wxID_CANCEL:
            EndModal(wxCANCEL);
            break;
    }
}
//-----------------------------------------------------------------------------
int showAdvancedMessageDialog(wxWindow* parent, int style,
    const wxString& primaryText, const wxString& secondaryText,
    AdvancedMessageDialogButtons& buttons, bool* showNeverAgain = 0)
{
    if (!parent)
        parent = ::wxGetActiveWindow();
    if (showNeverAgain)
        *showNeverAgain = false;
    wxArtID iconId;
    switch (style)
    {
        case wxICON_INFORMATION:
            iconId = wxART_INFORMATION;
            break;
        case wxICON_ERROR:
            iconId = wxART_ERROR;
            break;
        default:
            // NOTE: wxART_QUESTION is deprecated in HIGs, so don't use it...
            iconId = wxART_WARNING;
            break;
    }

    AdvancedMessageDialog amd(parent, iconId, primaryText, secondaryText,
        buttons, showNeverAgain != 0);
    int res = amd.ShowModal();
    if (showNeverAgain)
        *showNeverAgain = amd.getDontShowAgain();
    return res;
}
//-----------------------------------------------------------------------------
int showAdvancedMessageDialog(wxWindow* parent, int style,
    const wxString& primaryText, const wxString& secondaryText,
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
int showInformationDialog(wxWindow* parent, const wxString& primaryText,
    const wxString& secondaryText, AdvancedMessageDialogButtons& buttons)
{
    return showAdvancedMessageDialog(parent, wxICON_INFORMATION, primaryText,
        secondaryText, buttons);
}
//-----------------------------------------------------------------------------
int showInformationDialog(wxWindow* parent, const wxString& primaryText,
    const wxString& secondaryText, AdvancedMessageDialogButtons buttons,
    Config& config, wxString configKey)
{
    return showAdvancedMessageDialog(parent, wxICON_INFORMATION, primaryText,
        secondaryText, buttons, config, configKey);
}
//-----------------------------------------------------------------------------
int showQuestionDialog(wxWindow* parent, const wxString& primaryText,
    const wxString& secondaryText, AdvancedMessageDialogButtons& buttons)
{
    return showAdvancedMessageDialog(parent, wxICON_QUESTION, primaryText,
        secondaryText, buttons);
}
//-----------------------------------------------------------------------------
int showQuestionDialog(wxWindow* parent, const wxString& primaryText,
    const wxString& secondaryText, AdvancedMessageDialogButtons buttons,
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
