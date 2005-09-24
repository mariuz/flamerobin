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
  are Copyright (C) 2005 Michael Hieke.

  All Rights Reserved.

  $Id$

  Contributor(s):
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
