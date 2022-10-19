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


#ifndef FR_PROGRESSDIALOG_H
#define FR_PROGRESSDIALOG_H

#include <wx/wx.h>

#include <vector>

#include "core/ProgressIndicator.h"
#include "gui/BaseDialog.h"

class ProgressDialog: public BaseDialog, public ProgressIndicator
{
private:
    bool canceledM;
    std::vector<bool> indeterminateM;
    size_t levelCountM;
    std::vector<wxStaticText*> labelsM;
    std::vector<wxGauge*> gaugesM;
    wxButton* button_cancel;

    wxWindowDisabler* winDisablerM;

    void destroyControls();
    void createControls();
    void layoutControls();

    void doUpdate();
    void enableOtherWindows(bool enable);
    void setCanceled();

    wxGauge* getGaugeForLevel(size_t progressLevel);
    wxStaticText* getLabelForLevel(size_t progressLevel);
    bool isValidProgressLevel(size_t progressLevel);

public:
    ProgressDialog(wxWindow* parent, const wxString& title,
        size_t levelCount = 1);
    ~ProgressDialog();

   virtual bool Show(bool show = true);

    // ProgressIndicator methods
    virtual bool isCanceled();
    virtual void initProgress(wxString progressMsg,
        size_t maxPosition = 0, size_t startingPosition = 0,
        size_t progressLevel = 1);
    virtual void initProgressIndeterminate(wxString progressMsg,
        size_t progressLevel = 1);
    virtual void setProgressMessage(wxString progressMsg,
        size_t progressLevel = 1);
    virtual void setProgressPosition(size_t currentPosition,
        size_t progressLevel = 1);
    virtual void stepProgress(int stepAmount = 1,
        size_t progressLevel = 1);
    virtual void doShow();
    virtual void doHide();
    virtual void setProgressLevelCount(size_t levelCount = 1);
private:
    // event handling
    void OnCancelButtonClick(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    DECLARE_EVENT_TABLE()
};

#endif // FR_PROGRESSDIALOG_H
