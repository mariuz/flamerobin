/*
Copyright (c) 2004, 2005, 2006 The FlameRobin Development Team

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


  $Id$

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

#include "gui/controls/LogTextControl.h"
//-----------------------------------------------------------------------------
LogTextControl::LogTextControl(wxWindow *parent, wxWindowID id, long style)
    : TextControl(parent, id, style)
{
    setDefaultStyles();
}
//-----------------------------------------------------------------------------
void LogTextControl::addStyledText(const wxString& message, LogStyle style)
{
    // This implements the typical behaviour for log text controls:
    // When the caret is at the end of the text it will be kept there, keeping
    // the last logged text visible.
    // Otherwise the caret position is not altered, so user can navigate
    // in the already logged text.
    int lenBefore = GetLength();
    bool atEnd = lenBefore == GetCurrentPos();
    AppendText(message);
    int len = GetLength();
    StartStyling(lenBefore, 255);
    SetStyling(len - lenBefore - 1, int(style));
    if (atEnd)
        GotoPos(len);
}
//-----------------------------------------------------------------------------
void LogTextControl::logErrorMsg(const wxString& message)
{
    addStyledText(message, logStyleError);
}
//-----------------------------------------------------------------------------
void LogTextControl::logImportantMsg(const wxString& message)
{
    addStyledText(message, logStyleImportant);
}
//-----------------------------------------------------------------------------
void LogTextControl::logMsg(const wxString& message)
{
    addStyledText(message, logStyleDefault);
}
//-----------------------------------------------------------------------------
void LogTextControl::setDefaultStyles()
{
    StyleSetForeground(int(logStyleImportant), *wxBLUE);
    StyleSetForeground(int(logStyleError), *wxRED);
}
//-----------------------------------------------------------------------------
