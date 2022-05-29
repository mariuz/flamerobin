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

#ifndef LOGTEXTCONTROL_H
#define LOGTEXTCONTROL_H

#include "gui/controls/TextControl.h"

class LogTextControl: public TextControl
{
private:
    void setDefaultStyles();
protected:
    enum LogStyle { logStyleDefault, logStyleImportant, logStyleError };
    void addStyledText(const wxString& message, LogStyle style);
public:
    LogTextControl(wxWindow *parent, wxWindowID id = wxID_ANY);

    void ClearAll();

    void logErrorMsg(const wxString& message);
    void logImportantMsg(const wxString& message);
    void logMsg(const wxString& message);
protected:
    void OnCommandClearAll(wxCommandEvent& event);
    void OnCommandUpdate(wxUpdateUIEvent& event);

    void OnContextMenu(wxContextMenuEvent& event);

    DECLARE_EVENT_TABLE()
};

#endif
