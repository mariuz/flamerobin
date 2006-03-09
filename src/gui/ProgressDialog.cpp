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

#include "gui/ProgressDialog.h"
#include "styleguide.h"
//-----------------------------------------------------------------------------
void recursiveLayout(wxSizer* sizer)
{
    if (sizer)
    {
        wxSizerItemList::compatibility_iterator 
            node = sizer->GetChildren().GetFirst();
        while (node)
        {
            wxSizerItem *item = node->GetData();
            if (item->GetSizer())
                item->GetSizer()->Layout();
            node = node->GetNext();
        }
        sizer->Layout();
    }
}
//-----------------------------------------------------------------------------
// ProgressDialog
ProgressDialog::ProgressDialog(wxWindow* parent, const wxString& title, 
        unsigned int levelCount, const wxPoint& pos, const wxSize& size)
    : BaseDialog(parent, wxID_ANY, title, pos, size, wxDEFAULT_DIALOG_STYLE)
{
    canceledM = false;
    levelCountM = levelCount;
    SetExtraStyle(GetExtraStyle() | wxWS_EX_TRANSIENT);

    createControls();
    layoutControls();

    winDisablerM = 0;
    enableOtherWindows(false);
    Show();
    Enable();
    Update();
}
//-----------------------------------------------------------------------------
ProgressDialog::~ProgressDialog()
{
    enableOtherWindows(true);
}
//-----------------------------------------------------------------------------
void ProgressDialog::createControls()
{
    int gaugeHeight = wxSystemSettings::GetMetric(wxSYS_HSCROLL_Y);
    for (unsigned int i = 1; i <= levelCountM; i++)
    {
        wxStaticText* label = new wxStaticText(getControlsPanel(), wxID_ANY, 
            wxEmptyString);
        labelsM.push_back(label);
        wxGauge* gauge = new wxGauge(getControlsPanel(), wxID_ANY, 100, 
            wxDefaultPosition, wxSize(300, gaugeHeight), 
            wxGA_HORIZONTAL | wxGA_SMOOTH);
        gaugesM.push_back(gauge);
    }
    button_cancel = new wxButton(getControlsPanel(), wxID_CANCEL,
        _("Cancel"));
}
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
void ProgressDialog::layoutControls()
{
    wxSizer* sizerControls = new wxBoxSizer(wxVERTICAL);
    int dyLarge = styleguide().getUnrelatedControlMargin(wxVERTICAL);
    int dySmall = styleguide().getRelatedControlMargin(wxVERTICAL);
    for (unsigned int i = 0; i < levelCountM; i++)
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
//-----------------------------------------------------------------------------
wxGauge* ProgressDialog::getGaugeForLevel(unsigned int progressLevel)
{
    if (progressLevel > 0 && progressLevel <= gaugesM.size())
        return gaugesM[progressLevel - 1];
    return 0;
}
//-----------------------------------------------------------------------------
wxStaticText* ProgressDialog::getLabelForLevel(unsigned int progressLevel)
{
    if (progressLevel > 0 && progressLevel <= labelsM.size())
        return labelsM[progressLevel - 1];
    return 0;
}
//-----------------------------------------------------------------------------
inline bool ProgressDialog::isValidProgressLevel(unsigned int progressLevel)
{
    return progressLevel > 0 && progressLevel <= levelCountM;
}
//-----------------------------------------------------------------------------
void ProgressDialog::setCanceled()
{
    if (!canceledM)
    {
        canceledM = true;
        button_cancel->Enable(false);
        setProgressMessage(_("Cancelling..."));
        Update();
    }
}
//-----------------------------------------------------------------------------
void ProgressDialog::setGaugeIndeterminate(wxGauge* gauge, 
    bool /* indeterminate */ )
{
    if (gauge)
    {
    // TODO
    }
}
//-----------------------------------------------------------------------------
bool ProgressDialog::Show(bool show)
{
    if (!show)
        enableOtherWindows(true);
    return BaseDialog::Show(show);
}
//-----------------------------------------------------------------------------
// ProgressIndicator methods
bool ProgressDialog::isCanceled()
{
    wxYieldIfNeeded();
    return canceledM;
}
//-----------------------------------------------------------------------------
void ProgressDialog::initProgress(wxString progressMsg, 
    unsigned int maxPosition, unsigned int startingPosition, 
    unsigned int progressLevel)
{
    wxASSERT(isValidProgressLevel(progressLevel));

    wxStaticText* label = getLabelForLevel(progressLevel);
    wxGauge* gauge = getGaugeForLevel(progressLevel);
    if (label && gauge)
    {
        label->SetLabel(progressMsg);
        setGaugeIndeterminate(gauge, false);
        gauge->SetRange(maxPosition);
        gauge->SetValue(startingPosition);
        Update();
    }
}
//-----------------------------------------------------------------------------
void ProgressDialog::initProgressIndeterminate(wxString progressMsg,
    unsigned int progressLevel)
{
    wxASSERT(isValidProgressLevel(progressLevel));

    wxStaticText* label = getLabelForLevel(progressLevel);
    wxGauge* gauge = getGaugeForLevel(progressLevel);
    if (label && gauge)
    {
        label->SetLabel(progressMsg);
        setGaugeIndeterminate(gauge, true);
        Update();
    }
}
//-----------------------------------------------------------------------------
void ProgressDialog::setProgressMessage(wxString progressMsg, 
    unsigned int progressLevel)
{
    wxASSERT(isValidProgressLevel(progressLevel));

    wxStaticText* label = getLabelForLevel(progressLevel);
    if (label)
        label->SetLabel(progressMsg);
    Update();
}
//-----------------------------------------------------------------------------
void ProgressDialog::setProgressPosition(unsigned int currentPosition,
    unsigned int progressLevel)
{
    wxASSERT(isValidProgressLevel(progressLevel));

    wxGauge* gauge = getGaugeForLevel(progressLevel);
    if (gauge)
        gauge->SetValue(currentPosition);
    Update();
}
//-----------------------------------------------------------------------------
void ProgressDialog::stepProgress(int stepAmount, unsigned int progressLevel)
{
    wxASSERT(isValidProgressLevel(progressLevel));

    wxGauge* gauge = getGaugeForLevel(progressLevel);
    if (gauge)
    {
        int pos = gauge->GetValue() + stepAmount;
        int maxPos = gauge->GetRange();
        gauge->SetValue((pos < 0) ? 0 : (pos > maxPos ? maxPos : pos));
        Update();
    }
}
//-----------------------------------------------------------------------------
//! event handling
BEGIN_EVENT_TABLE(ProgressDialog, BaseDialog)
    EVT_BUTTON(wxID_CANCEL, ProgressDialog::OnCancelButtonClick)
    EVT_CLOSE(ProgressDialog::OnClose)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void ProgressDialog::OnCancelButtonClick(wxCommandEvent& WXUNUSED(event))
{
    setCanceled();
}
//-----------------------------------------------------------------------------
void ProgressDialog::OnClose(wxCloseEvent& event)
{
    setCanceled();
    event.Veto();
}
//-----------------------------------------------------------------------------
