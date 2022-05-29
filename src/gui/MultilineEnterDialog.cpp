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

#include "gui/controls/TextControl.h"
#include "gui/MultilineEnterDialog.h"
#include "gui/StyleGuide.h"

bool GetMultilineTextFromUser(wxWindow* parent, const wxString& title,
    wxString& value, const wxString& caption, const wxString& buttonLabel)
{
    MultilineEnterDialog med(parent, title, caption);
    med.setText(value);
    if (!buttonLabel.IsEmpty())
        med.setOkButtonLabel(buttonLabel);
    if (wxID_OK != med.ShowModal())
        return false;
    value = med.getText();
    return true;
}

MultilineEnterDialog::MultilineEnterDialog(wxWindow* parent,
        const wxString& title, const wxString& caption)
    : BaseDialog(parent, wxID_ANY, title)
{
    if (!caption.IsEmpty())
        static_caption = new wxStaticText(getControlsPanel(), -1, caption);
    else
        static_caption = 0;
    text_ctrl_value = new TextControl(getControlsPanel());
    button_ok = new wxButton(getControlsPanel(), wxID_OK, _("Save"));
    button_cancel = new wxButton(getControlsPanel(), wxID_CANCEL, _("Cancel"));
    layoutControls();
    button_ok->SetDefault();
}

wxString MultilineEnterDialog::getText() const
{
    return text_ctrl_value->GetText();
}

void MultilineEnterDialog::setText(const wxString& text)
{
    text_ctrl_value->SetText(text);
}

void MultilineEnterDialog::setOkButtonLabel(const wxString& label)
{
    button_ok->SetLabel(label);
}

const wxString MultilineEnterDialog::getName() const
{
    return "MultilineEnterDialog";
}

void MultilineEnterDialog::layoutControls()
{
    wxBoxSizer* sizerControls = new wxBoxSizer(wxVERTICAL);
    if (static_caption)
    {
        sizerControls->Add(static_caption, 0, wxEXPAND);
        // styleguide().getControlLabelMargin() doesn't look good
        sizerControls->AddSpacer(
            styleguide().getRelatedControlMargin(wxVERTICAL));
    }
    text_ctrl_value->SetSizeHints(200, 100);
    text_ctrl_value->SetSize(200, 100);
    sizerControls->Add(text_ctrl_value, 1, wxEXPAND);
    wxSizer* sizerButtons = styleguide().createButtonSizer(button_ok,
        button_cancel);
    layoutSizers(sizerControls, sizerButtons, true);
}

