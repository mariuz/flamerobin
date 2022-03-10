/*
  Copyright (c) 2004-2022 The FlameRobin Development Team

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
*/

#ifndef FR_ADVANCEDMESSAGEDIALOG_H
#define FR_ADVANCEDMESSAGEDIALOG_H

#include <wx/wx.h>
#include <wx/artprov.h>

#include "config/Config.h"
#include "gui/BaseDialog.h"

class AdvancedMessageDialogButtons
{
private:
    struct AdvancedMessageDialogButtonData
    {
        int id;
        wxString caption;
    };
    AdvancedMessageDialogButtonData affirmativeButtonM;
    AdvancedMessageDialogButtonData alternateButtonM;
    AdvancedMessageDialogButtonData negativeButtonM;
protected:
    AdvancedMessageDialogButtons();

    void addAffirmativeButton(int id, const wxString& caption);
    void addAlternateButton(int id, const wxString& caption);
    void addNegativeButton(int id, const wxString& caption);
public:
    wxButton* createAffirmativeButton(wxWindow* parent);
    wxButton* createAlternateButton(wxWindow* parent);
    wxButton* createNegativeButton(wxWindow* parent);

    int getNumberOfButtons();
};

class AdvancedMessageDialogButtonsOk: public AdvancedMessageDialogButtons
{
public:
    AdvancedMessageDialogButtonsOk(const wxString buttonOkCaption = _("OK"));
};

class AdvancedMessageDialogButtonsOkCancel: public AdvancedMessageDialogButtons
{
public:
    AdvancedMessageDialogButtonsOkCancel(const wxString buttonOkCaption,
        const wxString buttonCancelCaption = _("&Cancel"));
};

class AdvancedMessageDialogButtonsYesNoCancel:
    public AdvancedMessageDialogButtons
{
public:
    AdvancedMessageDialogButtonsYesNoCancel(const wxString buttonYesCaption,
        const wxString buttonNoCaption = _("&No"),
        const wxString buttonCancelCaption = _("&Cancel"));
};

class AdvancedMessageDialog: public BaseDialog
{
private:
    wxControl* controlPrimaryTextM;
    wxControl* controlSecondaryTextM;
    wxCheckBox* checkBoxM;
public:
    AdvancedMessageDialog(wxWindow* parent, wxArtID iconId,
        const wxString& primaryText, const wxString& secondaryText,
        AdvancedMessageDialogButtons& buttons,
        const wxString& dontShowAgainText);

    bool getDontShowAgain() const;
private:
    // event handling
    void OnButtonClick(wxCommandEvent& event);
};

int showInformationDialog(wxWindow* parent, const wxString& primaryText,
    const wxString& secondaryText, AdvancedMessageDialogButtons buttons);
int showInformationDialog(wxWindow* parent, const wxString& primaryText,
    const wxString& secondaryText, AdvancedMessageDialogButtons buttons,
    Config& config, const wxString& configKey,
    const wxString& dontShowAgainText);

int showQuestionDialog(wxWindow* parent, const wxString& primaryText,
    const wxString& secondaryText, AdvancedMessageDialogButtons buttons);
int showQuestionDialog(wxWindow* parent, const wxString& primaryText,
    const wxString& secondaryText, AdvancedMessageDialogButtons buttons,
    Config& config, const wxString& configKey,
    const wxString& dontShowAgainText);

int showWarningDialog(wxWindow* parent, const wxString& primaryText,
    const wxString& secondaryText, AdvancedMessageDialogButtons buttons);
int showWarningDialog(wxWindow* parent, const wxString& primaryText,
    const wxString& secondaryText, AdvancedMessageDialogButtons buttons,
    Config& config, const wxString& configKey,
    const wxString& dontShowAgainText);

int showErrorDialog(wxWindow* parent, const wxString& primaryText,
    const wxString& secondaryText, AdvancedMessageDialogButtons buttons);
int showErrorDialog(wxWindow* parent, const wxString& primaryText,
    const wxString& secondaryText, AdvancedMessageDialogButtons buttons,
    Config& config, const wxString& configKey,
    const wxString& dontShowAgainText);

#endif // FR_ADVANCEDMESSAGEDIALOG_H
