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
are Copyright (C) 2004 Michael Hieke.

All Rights Reserved.

$Id$

Contributor(s): Nando Dessena
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

#include <string>
#include "BaseDialog.h"
#include "config.h"
#include "styleguide.h"
//-----------------------------------------------------------------------------
BaseDialog::BaseDialog(wxWindow* parent, int id, const wxString& title, 
        const wxPoint& pos, const wxSize& size, long style)
    : wxDialog(parent, id, title, pos, size, style|wxNO_FULL_REPAINT_ON_RESIZE)
{
    panel_controls = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL|wxCLIP_CHILDREN|wxNO_BORDER|wxNO_FULL_REPAINT_ON_RESIZE);
}
//-----------------------------------------------------------------------------
void BaseDialog::layoutSizers(wxSizer* controls, wxSizer* buttons, bool expandControls)
{
    wxBoxSizer* sizerVert = new wxBoxSizer(wxVERTICAL);
    sizerVert->Add(0, styleguide().getDialogMargin(wxTOP));
    sizerVert->Add(controls, expandControls ? 1 : 0, wxEXPAND);
    // make buttons align to bottom of dialog
    sizerVert->Add(0, styleguide().getDialogMargin(wxBOTTOM), expandControls ? 0 : 1, wxEXPAND);
    sizerVert->Add(buttons, 0, wxEXPAND);
    sizerVert->Add(0, styleguide().getDialogMargin(wxBOTTOM));

    wxBoxSizer* sizerHorz = new wxBoxSizer(wxHORIZONTAL);
    sizerHorz->Add(styleguide().getDialogMargin(wxLEFT), 0);
    sizerHorz->Add(sizerVert, 1, wxEXPAND);
    sizerHorz->Add(styleguide().getDialogMargin(wxRIGHT), 0);

    wxBoxSizer* sizerAll = new wxBoxSizer(wxHORIZONTAL);
    sizerAll->Add(sizerHorz, 1, wxEXPAND);

    panel_controls->SetSizer(sizerAll);
    sizerAll->Fit(this);
    sizerAll->SetSizeHints(this);
}
//-----------------------------------------------------------------------------
bool BaseDialog::Show(bool show)
{
    if (show)
        readConfigSettings();
    else
        writeConfigSettings();
    return wxDialog::Show(show);
}
//-----------------------------------------------------------------------------
BaseDialog::~BaseDialog()
{
}
//-----------------------------------------------------------------------------
void BaseDialog::readConfigSettings()
{
    bool enabled = false;
    if (config().getValue("FrameStorage", enabled) && enabled)
    {
        std::string itemPrefix = getStorageName();
        if (!itemPrefix.empty())
        {
            wxRect r = getDefaultRect();
            config().getValue(itemPrefix + "::width", r.width);
            config().getValue(itemPrefix + "::height", r.height);
            doReadConfigSettings(itemPrefix);
            if (r.width > 0 && r.height > 0)
                SetSize(r.width, r.height);
            // default to centered dialogs, set to 0 to disable
            bool centered = true;
            config().getValue(itemPrefix + "::centerOnParent", centered);
            if (centered)
                CenterOnParent();
            else            // if we don't want it centered, let's restore the position
            {
                config().getValue(itemPrefix + "::x", r.x);
                config().getValue(itemPrefix + "::y", r.y);
                SetSize(r);
            }
        }
    }
}
//-----------------------------------------------------------------------------
void BaseDialog::doReadConfigSettings(const std::string& WXUNUSED(prefix))
{
}
//-----------------------------------------------------------------------------
void BaseDialog::writeConfigSettings() const
{
    bool enabled = false;
    if (config().getValue("FrameStorage", enabled) && enabled)
    {
        // save window size to config.
        std::string itemPrefix = getStorageName();
        if (!itemPrefix.empty())
        {
            wxRect r = GetRect();
            config().setValue(itemPrefix + "::width", r.width);
            config().setValue(itemPrefix + "::height", r.height);

            bool centered = true;
            config().getValue(itemPrefix + "::centerOnParent", centered);
            if (!centered)
            {
                config().setValue(itemPrefix + "::x", r.x);
                config().setValue(itemPrefix + "::y", r.y);
            }
            doWriteConfigSettings(itemPrefix);
        }
    }
}
//-----------------------------------------------------------------------------
void BaseDialog::doWriteConfigSettings(const std::string& WXUNUSED(prefix)) const
{
}
//-----------------------------------------------------------------------------
const std::string BaseDialog::getName() const
{
    // Couldn't find a reliable (meaning supportable and cross-platform) way
    // to use the class name here, so every derived frame needs to override getName()
    // if it needs to use features that depend on it.
    return "";
}
//-----------------------------------------------------------------------------
const std::string BaseDialog::getStorageName() const
{
    return getName();
}
//-----------------------------------------------------------------------------
const wxRect BaseDialog::getDefaultRect() const
{
    return wxRect(-1, -1, -1, -1);
}
//-----------------------------------------------------------------------------
