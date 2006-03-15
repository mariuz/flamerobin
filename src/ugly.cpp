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

  The Initial Developer of the Original Code is Milan Babuskov.

  Portions created by the original developer
  are Copyright (C) 2004 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s): Nando Dessena, Michael Hieke
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

#include <wx/encconv.h>

#include "ugly.h"
//-----------------------------------------------------------------------------
//! ugly.cpp contains all ugly stuff with ifdefs and similar
//! things that would otherwise make code ugly and unreadable
//-----------------------------------------------------------------------------
//! converts wxString to std::string
std::string wx2std(const wxString& input)
{
    if (input.empty())
        return "";
    return std::string(input.mb_str(*wxConvCurrent));
}
//-----------------------------------------------------------------------------
//! converts std:string to wxString
wxString std2wx(std::string input)
{
    if (input.empty())
        return wxEmptyString;
    return wxString(input.c_str(), *wxConvCurrent);
}
//-----------------------------------------------------------------------------
//! return wxString for comparison, used to limit features to certain platforms
wxString getPlatformName()
{
#ifdef __WINDOWS__
    return wxT("win");
#elif defined(__MAC__) || defined(__APPLE__)
    return wxT("mac");
#elif defined(__UNIX__)
    return wxT("unix");
#elif 
    return wxT("undefined");
#endif
}
//-----------------------------------------------------------------------------
