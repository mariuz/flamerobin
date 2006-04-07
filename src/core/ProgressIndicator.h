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

  Contributor(s): Nando Dessena
*/
//-----------------------------------------------------------------------------

#ifndef FR_PROGRESSINDICATOR_H
#define FR_PROGRESSINDICATOR_H

#include <wx/wx.h>
//-----------------------------------------------------------------------------
class ProgressIndicator
{
public:
    virtual ~ProgressIndicator() {};
    virtual bool isCanceled() = 0;
    virtual void initProgress(wxString progressMsg, 
        unsigned int maxPosition = 0, unsigned int startingPosition = 0,
        unsigned int progressLevel = 1) = 0;
    virtual void initProgressIndeterminate(wxString progressMsg,
        unsigned int progressLevel = 1) = 0;
    virtual void setProgressMessage(wxString progressMsg,
        unsigned int progressLevel = 1) = 0;
    virtual void setProgressPosition(unsigned int currentPosition,
        unsigned int progressLevel = 1) = 0;
    virtual void stepProgress(int stepAmount = 1,
        unsigned int progressLevel = 1) = 0;
};
//-----------------------------------------------------------------------------
#endif // FR_PROGRESSINDICATOR_H
