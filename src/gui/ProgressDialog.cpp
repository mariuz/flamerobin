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


// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "gui/ProgressDialog.h"
#include "gui/StyleGuide.h"

// ProgressDialog
ProgressDialog::ProgressDialog(wxWindow* parent, const wxString& title,
        size_t levelCount)
    : BaseDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize,
        wxDEFAULT_DIALOG_STYLE), canceledM(false), winDisablerM(0)
{
    SetExtraStyle(GetExtraStyle() | wxWS_EX_TRANSIENT);
    setProgressLevelCount(levelCount);
}

ProgressDialog::~ProgressDialog()
{
    enableOtherWindows(true);
}

void ProgressDialog::destroyControls()
{
    labelsM.clear();
    gaugesM.clear();
    getControlsPanel()->DestroyChildren();
}

void ProgressDialog::createControls()
{
    int gaugeHeight = wxSystemSettings::GetMetric(wxSYS_HSCROLL_Y);
    for (size_t i = 1; i <= levelCountM; i++)
    {
        wxStaticText* label = new wxStaticText(getControlsPanel(), wxID_ANY,
            wxEmptyString, wxDefaultPosition, wxDefaultSize,
            // don't resize the label, keep the same width as the gauge
            wxST_NO_AUTORESIZE);
        labelsM.push_back(label);
        wxGauge* gauge = new wxGauge(getControlsPanel(), wxID_ANY, 100,
            wxDefaultPosition, wxSize(300, gaugeHeight),
            wxGA_HORIZONTAL | wxGA_SMOOTH);
        gaugesM.push_back(gauge);
    }
    button_cancel = new wxButton(getControlsPanel(), wxID_CANCEL, _("Cancel"));
    button_cancel->SetDefault();
    button_cancel->SetFocus();
}

void ProgressDialog::doUpdate()
{
    // update all changed controls, and make sure they paint themselves...

#ifndef __WXMAC__ // it is too expensive (window compositing)
    Update();
#endif
#ifndef __WXMSW__ // not necessary, painting is not done at idle time
    wxYieldIfNeeded(); 
#endif
}

void ProgressDialog::enableOtherWindows(bool enable)
{
     if (!enable && (0 == winDisablerM))
        winDisablerM = new wxWindowDisabler(this);
    else if (enable)
    {
        delete winDisablerM;
        winDisablerM = 0;
    }
}

void ProgressDialog::layoutControls()
{
    wxSizer* sizerControls = new wxBoxSizer(wxVERTICAL);
    int dyLarge = styleguide().getUnrelatedControlMargin(wxVERTICAL);
    int dySmall = styleguide().getRelatedControlMargin(wxVERTICAL);
    for (size_t i = 0; i < levelCountM; i++)
    {
        if (i > 0)
            sizerControls->AddSpacer(dyLarge);
        sizerControls->Add(labelsM[i], 0, wxEXPAND);
        sizerControls->AddSpacer(dySmall);
        sizerControls->Add(gaugesM[i], 0, wxEXPAND);
    }

    wxSizer* sizerButtons = new wxBoxSizer(wxHORIZONTAL);
    sizerButtons->Add(0, 0, 1, wxEXPAND);
    sizerButtons->Add(button_cancel);
    sizerButtons->Add(0, 0, 1, wxEXPAND);

    // use method in base class to set everything up
    layoutSizers(sizerControls, sizerButtons);
}

wxGauge* ProgressDialog::getGaugeForLevel(size_t progressLevel)
{
    if (progressLevel > 0 && progressLevel <= gaugesM.size())
        return gaugesM[progressLevel - 1];
    return 0;
}

wxStaticText* ProgressDialog::getLabelForLevel(size_t progressLevel)
{
    if (progressLevel > 0 && progressLevel <= labelsM.size())
        return labelsM[progressLevel - 1];
    return 0;
}

inline bool ProgressDialog::isValidProgressLevel(size_t progressLevel)
{
    return progressLevel > 0 && progressLevel <= levelCountM;
}

void ProgressDialog::setCanceled()
{
    if (!canceledM)
    {
        canceledM = true;
        button_cancel->Enable(false);
        setProgressMessage(_("Cancelling..."));
        doUpdate();
    }
}

bool ProgressDialog::Show(bool show)
{
    if (!show)
        enableOtherWindows(true);
    return BaseDialog::Show(show);
}

// ProgressIndicator methods
bool ProgressDialog::isCanceled()
{
    wxYieldIfNeeded();
    return canceledM;
}

void ProgressDialog::initProgress(wxString progressMsg, size_t maxPosition,
    size_t startingPosition, size_t progressLevel)
{
    wxASSERT(isValidProgressLevel(progressLevel));

    wxStaticText* label = getLabelForLevel(progressLevel);
    wxGauge* gauge = getGaugeForLevel(progressLevel);
    if (label && gauge)
    {
        indeterminateM[progressLevel - 1] = false;
        label->SetLabel(progressMsg);
        gauge->SetRange(static_cast<int>(maxPosition));
        gauge->SetValue(static_cast<int>(startingPosition));
        doUpdate();
    }
}

void ProgressDialog::initProgressIndeterminate(wxString progressMsg,
    size_t progressLevel)
{
    wxASSERT(isValidProgressLevel(progressLevel));

    wxStaticText* label = getLabelForLevel(progressLevel);
    wxGauge* gauge = getGaugeForLevel(progressLevel);
    if (label && gauge)
    {
        indeterminateM[progressLevel - 1] = true;
        label->SetLabel(progressMsg);
        gauge->Pulse();
        doUpdate();
    }
}

void ProgressDialog::setProgressMessage(wxString progressMsg,
    size_t progressLevel)
{
    wxASSERT(isValidProgressLevel(progressLevel));

    wxStaticText* label = getLabelForLevel(progressLevel);
    if (label)
    {
        label->SetLabel(progressMsg);
        doUpdate();
    }
}

void ProgressDialog::setProgressPosition(size_t currentPosition,
    size_t progressLevel)
{
    wxASSERT(isValidProgressLevel(progressLevel));

    wxGauge* gauge = getGaugeForLevel(progressLevel);
    if (gauge)
    {
        indeterminateM[progressLevel - 1] = false;
        gauge->SetValue(static_cast<int>(currentPosition));
        doUpdate();
    }
}

void ProgressDialog::stepProgress(int stepAmount, size_t progressLevel)
{
    wxASSERT(isValidProgressLevel(progressLevel));

    wxGauge* gauge = getGaugeForLevel(progressLevel);
    if (gauge)
    {
        if (indeterminateM[progressLevel - 1])
            gauge->Pulse();
        else
        {
            int pos = gauge->GetValue() + stepAmount;
            int maxPos = gauge->GetRange();
            gauge->SetValue((pos < 0) ? 0 : (pos > maxPos ? maxPos : pos));
        }
        doUpdate();
    }
}

void ProgressDialog::doShow()
{
    enableOtherWindows(false);
    Show();
    Enable();
    doUpdate();
}

void ProgressDialog::doHide()
{
    Hide();
    enableOtherWindows(true);
}

void ProgressDialog::setProgressLevelCount(size_t levelCount)
{
    if (levelCountM != levelCount)
    {
        levelCountM = levelCount;
        indeterminateM.resize(levelCount);
        destroyControls();
        createControls();
        layoutControls();
    }
}

//! event handling
BEGIN_EVENT_TABLE(ProgressDialog, BaseDialog)
    EVT_BUTTON(wxID_CANCEL, ProgressDialog::OnCancelButtonClick)
    EVT_CLOSE(ProgressDialog::OnClose)
END_EVENT_TABLE()

void ProgressDialog::OnCancelButtonClick(wxCommandEvent& WXUNUSED(event))
{
    setCanceled();
}

void ProgressDialog::OnClose(wxCloseEvent& event)
{
    setCanceled();
    event.Veto();
}

