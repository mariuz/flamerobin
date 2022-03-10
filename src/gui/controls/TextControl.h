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


#ifndef TEXTCONTROL_H
#define TEXTCONTROL_H

#include <wx/wx.h>
#include <wx/stc/stc.h>

// Base class for multiline text controls in FlameRobin.  Based on
// wxStyledTextCtrl instead of wxTextCtrl, because that works better/faster
// on some systems, and provides popup menu on wxGTK.
class TextControl: public wxStyledTextCtrl
{
protected:
    void resetStyles();

protected:
    void OnCommandUndo(wxCommandEvent& event);
    void OnCommandRedo(wxCommandEvent& event);
    void OnCommandCut(wxCommandEvent& event);
    void OnCommandCopy(wxCommandEvent& event);
    void OnCommandPaste(wxCommandEvent& event);
    void OnCommandDelete(wxCommandEvent& event);
    void OnCommandSelectAll(wxCommandEvent& event);

    void OnCommandUpdateUndo(wxUpdateUIEvent& event);
    void OnCommandUpdateRedo(wxUpdateUIEvent& event);
    void OnCommandUpdateCut(wxUpdateUIEvent& event);
    void OnCommandUpdateCopy(wxUpdateUIEvent& event);
    void OnCommandUpdatePaste(wxUpdateUIEvent& event);
    void OnCommandUpdateDelete(wxUpdateUIEvent& event);
    void OnCommandUpdateSelectAll(wxUpdateUIEvent& event);

    void OnContextMenu(wxContextMenuEvent& event);

    DECLARE_EVENT_TABLE()
public:
    TextControl(wxWindow *parent, wxWindowID id = wxID_ANY);
};

#endif
