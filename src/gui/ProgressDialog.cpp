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

  The Initial Developer of the Original Code is Michael Hieke.

  Portions created by the original developer
  are Copyright (C) 2006 Michael Hieke.

  All Rights Reserved.

  $Id$

  Contributor(s):
*/
//-----------------------------------------------------------------------------

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

#include "gui/ProgressDialog.h"
//-----------------------------------------------------------------------------
ProgressDialog::ProgressDialog(wxWindow* parent, int id, const wxString& title,
        const wxPoint& pos, const wxSize& size, long style)
    : wxDialog(parent, id, title, pos, size, style)
{

}
//-----------------------------------------------------------------------------
ProgressDialog::~ProgressDialog()
{

}
//-----------------------------------------------------------------------------
// ProgressIndicator methods
bool ProgressDialog::isCancelled()
{
    return false;
}
//-----------------------------------------------------------------------------
void ProgressDialog::initProgress(wxString progressMsg, 
    unsigned int maxPosition, unsigned int startingPosition, 
    unsigned int progressLevel)
{
}
//-----------------------------------------------------------------------------
void ProgressDialog::initProgressIndeterminate(wxString progressMsg,
    unsigned int progressLevel)
{
}
//-----------------------------------------------------------------------------
void ProgressDialog::setProgressMessage(wxString progressMsg, 
    unsigned int progressLevel)
{
}
//-----------------------------------------------------------------------------
void ProgressDialog::setProgressPosition(unsigned int currentPosition,
    unsigned int progressLevel)
{
}
//-----------------------------------------------------------------------------
void ProgressDialog::stepProgress(int stepAmount, unsigned int progressLevel)
{
}
//-----------------------------------------------------------------------------
