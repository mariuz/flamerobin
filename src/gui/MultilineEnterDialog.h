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


#ifndef MULTILINEENTERDIALOG_H
#define MULTILINEENTERDIALOG_H

#include <wx/wx.h>

#include "gui/BaseDialog.h"

class TextControl;

bool GetMultilineTextFromUser(wxWindow* parent, const wxString& title,
    wxString& value, const wxString& caption = wxEmptyString,
    const wxString& buttonLabel = wxEmptyString);

//! normally you shouldn't need to create objects of this class, just use
//  the GetMultilineTextFromUser() function
class MultilineEnterDialog: public BaseDialog {
private:
    TextControl* text_ctrl_value;
    wxStaticText* static_caption;
    wxButton* button_ok;
    wxButton* button_cancel;
    void layoutControls();
protected:
    virtual const wxString getName() const;
public:
    MultilineEnterDialog(wxWindow* parent, const wxString& title,
        const wxString& caption = wxEmptyString);

    wxString getText() const;
    void setText(const wxString& text);

    void setOkButtonLabel(const wxString& label);
};

#endif
