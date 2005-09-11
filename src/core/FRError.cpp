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

  The Initial Developer of the Original Code is Nando Dessena.

  Portions created by the original developer
  are Copyright (C) 2005 Nando Dessena.

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

#include "FRError.h"
#include "ugly.h"
//-----------------------------------------------------------------------------
void handleException(std::exception& e)
{
    wxMessageBox(std2wx(e.what()), _("Error"), wxOK | wxICON_ERROR);
}
//-----------------------------------------------------------------------------
FRError::FRError(const wxString& message)
    : messageM(0)
{
    std::string s = wx2std(message);
    messageM = new char[s.length() + 1];
    for (unsigned i=0; i < s.length(); ++i) // copy the string
        messageM[i] = s[i];
    messageM[s.length()] = '\0';
}
//-----------------------------------------------------------------------------
// needed so that each object has it's own copy of message
FRError::FRError(const FRError& source)
{
    std::string s(source.what());   // source.what() is always non-zero!
    messageM = new char[s.length() + 1];
    for (unsigned i=0; i < s.length(); ++i) // copy the string
        messageM[i] = s[i];
    messageM[s.length()] = '\0';
}
//-----------------------------------------------------------------------------
const char* FRError::what() const throw()
{
    return messageM;
}
//-----------------------------------------------------------------------------
FRError::~FRError() throw()
{
    if (messageM)
        delete messageM;
}
//-----------------------------------------------------------------------------
