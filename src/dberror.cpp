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
//-----------------------------------------------------------------------------
#include "ugly.h"
#include "dberror.h"
//-----------------------------------------------------------------------------
std::string YError::getMessage() const
{
	return messageM;
}
//-----------------------------------------------------------------------------
void YError::setMessage(std::string message)
{
	messageM = message;
}
//-----------------------------------------------------------------------------
#if (wxUSE_UNICODE)
void YError::setMessage(wxString message)	// makes things easier for unicode builds
{
	setMessage(wx2std(message));
}
#endif
//-----------------------------------------------------------------------------
YError& lastError()
{
	static YError e;
	return e;
}
//-----------------------------------------------------------------------------
