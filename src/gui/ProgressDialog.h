//-----------------------------------------------------------------------------
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

  The Initial Developer of the Original Code is Michael Hieke.

  Portions created by the original developer
  are Copyright (C) 2006 Michael Hieke.

  All Rights Reserved.

  $Id$

  Contributor(s):
*/
//-----------------------------------------------------------------------------

#ifndef FR_PROGRESSDIALOG_H
#define FR_PROGRESSDIALOG_H

#include <wx/wx.h>

#include <vector>

#include "core/ProgressIndicator.h"
#include "gui/BaseDialog.h"
//-----------------------------------------------------------------------------
class ProgressDialog: public BaseDialog, public ProgressIndicator
{
private:
    bool canceledM;
    unsigned int levelCountM;
    std::vector<wxStaticText*> labelsM;
    std::vector<wxGauge*> gaugesM;
    wxButton* button_cancel;

    wxWindowDisabler* winDisablerM;

    void createControls();
    void layoutControls();

    void enableOtherWindows(bool enable);
    void setCanceled();

    wxGauge* getGaugeForLevel(unsigned int progressLevel);
    wxStaticText* getLabelForLevel(unsigned int progressLevel);
    bool isValidProgressLevel(unsigned int progressLevel);
    void setGaugeIndeterminate(wxGauge* gauge, bool indeterminate);

public:
    ProgressDialog(const wxString& title, unsigned int levelCount = 1,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize);
    ~ProgressDialog();

   virtual bool Show(bool show = true);

    // ProgressIndicator methods
    virtual bool isCanceled();
    virtual void initProgress(wxString progressMsg, 
        unsigned int maxPosition = 0, unsigned int startingPosition = 0,
        unsigned int progressLevel = 1);
    virtual void initProgressIndeterminate(wxString progressMsg,
        unsigned int progressLevel = 1);
    virtual void setProgressMessage(wxString progressMsg,
        unsigned int progressLevel = 1);
    virtual void setProgressPosition(unsigned int currentPosition,
        unsigned int progressLevel = 1);
    virtual void stepProgress(int stepAmount = 1,
        unsigned int progressLevel = 1);
private:
    // event handling
    void OnCancelButtonClick(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif // FR_PROGRESSDIALOG_H
