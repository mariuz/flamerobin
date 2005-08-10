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
  are Copyright (C) 2005 Milan Babuskov.

  All Rights Reserved.

  Contributor(s):
*/

#ifndef TRIGGERWIZARDDIALOG_H
#define TRIGGERWIZARDDIALOG_H

#include <wx/wx.h>
#include <wx/image.h>
#include <wx/spinctrl.h>
#include "BaseDialog.h"
class MetadataItem;
//-----------------------------------------------------------------------------
class TriggerWizardDialog: public BaseDialog
{
public:
    TriggerWizardDialog(wxWindow* parent, MetadataItem *item);
    void OnOkButtonClick(wxCommandEvent& event);

private:
	MetadataItem *relationM;
    void set_properties();
    void do_layout();

protected:
    wxStaticText* label_1;
    wxTextCtrl* text_ctrl_1;
    wxCheckBox* checkbox_1_copy;
    wxRadioBox* radio_box_1_copy;
    wxCheckBox* checkbox_insert;
    wxCheckBox* checkbox_update;
    wxCheckBox* checkbox_delete;
    wxStaticText* label_2;
    wxSpinCtrl* spin_ctrl_1;
    wxButton* button_ok;
    wxButton* button_cancel;
    virtual const std::string getName() const;

    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif // TRIGGERWIZARDDIALOG_H
