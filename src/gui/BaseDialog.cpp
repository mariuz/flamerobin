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

#include "config/Config.h"
#include "gui/BaseDialog.h"
#include "gui/StyleGuide.h"

BaseDialog::BaseDialog(wxWindow* parent, int id, const wxString& title,
        const wxPoint& pos, const wxSize& size, long style)
    : wxDialog(parent, id, title, pos, size, style)
{
    panel_controls = new wxPanel(this, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxTAB_TRAVERSAL | wxCLIP_CHILDREN | wxNO_BORDER);
}

wxPanel* BaseDialog::getControlsPanel()
{
    return panel_controls;
}

void BaseDialog::layoutSizers(wxSizer* controls, wxSizer* buttons,
    bool expandControls)
{
    wxBoxSizer* sizerVert = new wxBoxSizer(wxVERTICAL);
    sizerVert->AddSpacer(styleguide().getDialogMargin(wxTOP));
    if (controls)
    {
        sizerVert->Add(controls, expandControls ? 1 : 0, wxEXPAND);
        // make buttons align to bottom of dialog
        sizerVert->Add(0, styleguide().getDialogMargin(wxBOTTOM),
            expandControls ? 0 : 1, wxEXPAND);
    }
    sizerVert->Add(buttons, 0, wxEXPAND);
    sizerVert->AddSpacer(styleguide().getDialogMargin(wxBOTTOM));

    wxBoxSizer* sizerHorz = new wxBoxSizer(wxHORIZONTAL);
    sizerHorz->AddSpacer(styleguide().getDialogMargin(wxLEFT));
    sizerHorz->Add(sizerVert, 1, wxEXPAND);
    sizerHorz->AddSpacer(styleguide().getDialogMargin(wxRIGHT));

    wxBoxSizer* sizerAll = new wxBoxSizer(wxHORIZONTAL);
    sizerAll->Add(sizerHorz, 1, wxEXPAND);

    panel_controls->SetSizer(sizerAll);
    sizerAll->Fit(this);
    sizerAll->SetSizeHints(this);
}

bool BaseDialog::Show(bool show)
{
    if (show)
        readConfigSettings();
    else
        writeConfigSettings();
    return wxDialog::Show(show);
}

BaseDialog::~BaseDialog()
{
}

//! updates colors of controls
void BaseDialog::updateColors(wxWindow *parent)
{
    if (parent == 0)
        parent = this;
    const wxColour silver = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
    wxWindowList& l = parent->GetChildren();
    for (wxWindowList::const_iterator it = l.begin(); it != l.end(); ++it)
    {
        if (dynamic_cast<wxPanel *>(*it))
        {
            updateColors(dynamic_cast<wxWindow *>(*it));
            continue;
        }

        // wxNullColour = reset to default. We must use that instead of
        // white or wxSYS_COLOUR_WINDOW or whatever since it only wxNullColour
        // works properly with wxGTK
        wxTextCtrl *tc = dynamic_cast<wxTextCtrl *>(*it);
        if (tc)
            tc->SetBackgroundColour(tc->IsEditable() ? wxNullColour : silver);
    }

}

void BaseDialog::readConfigSettings()
{
    // default to centered dialogs
    bool centered = config().get("centerDialogOnParent", true);
    if (config().get("FrameStorage", false))
    {
        wxString itemPrefix = getStorageName();
        if (!itemPrefix.empty())
        {
            wxRect r = getDefaultRect();
            if (getConfigStoresWidth())
                config().getValue(itemPrefix + Config::pathSeparator + "width", r.width);
            if (getConfigStoresHeight())
                config().getValue(itemPrefix + Config::pathSeparator + "height", r.height);
            doReadConfigSettings(itemPrefix);
            if (r.width > 0 || r.height > 0)
                SetSize(r.width, r.height);
            // default to global setting, set to 0 to disable
            // restore the position if we don't want it centered
            config().getValue(itemPrefix + Config::pathSeparator + "centerDialogOnParent", centered);
            if (!centered)
            {
                config().getValue(itemPrefix + Config::pathSeparator + "x", r.x);
                config().getValue(itemPrefix + Config::pathSeparator + "y", r.y);
                SetSize(r);
            }
        }
    }
    if (centered)
        Centre(wxCENTER_FRAME | wxBOTH);
}

void BaseDialog::doReadConfigSettings(const wxString& WXUNUSED(prefix))
{
}

void BaseDialog::writeConfigSettings() const
{
    if (config().get("FrameStorage", false) && !IsIconized())
    {
        // wxFileConfig::Flush() should only be called once
        SubjectLocker locker(&config());

        // save window size to config.
        wxString itemPrefix = getStorageName();
        if (!itemPrefix.empty())
        {
            wxRect r = GetRect();
            config().setValue(itemPrefix + Config::pathSeparator + "width", r.width);
            config().setValue(itemPrefix + Config::pathSeparator + "height", r.height);

            bool centered = true;
            config().getValue(itemPrefix + Config::pathSeparator + "centerDialogOnParent", centered);
            if (!centered)
            {
                config().setValue(itemPrefix + Config::pathSeparator + "x", r.x);
                config().setValue(itemPrefix + Config::pathSeparator + "y", r.y);
            }
            doWriteConfigSettings(itemPrefix);
        }
    }
}

void BaseDialog::doWriteConfigSettings(const wxString& WXUNUSED(prefix)) const
{
}

const wxString BaseDialog::getName() const
{
    // Couldn't find a reliable (meaning supportable and cross-platform) way
    // to use the class name here, so every derived frame needs to override getName()
    // if it needs to use features that depend on it.
    return "";
}

const wxString BaseDialog::getStorageName() const
{
    return getName();
}

const wxRect BaseDialog::getDefaultRect() const
{
    return wxRect(-1, -1, -1, -1);
}

bool BaseDialog::getConfigStoresWidth() const
{
    return true;
}

bool BaseDialog::getConfigStoresHeight() const
{
    return true;
}

