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
  are Copyright (C) 2004 Nando Dessena.

  All Rights Reserved.

  $Id$

  Contributor(s): Michael Hieke
*/

#ifndef FRUTILS_H
#define FRUTILS_H

#include <string>
#include <list>
#include <wx/wx.h>
//-----------------------------------------------------------------------------
//! Returns the absolute application path, if possible, or an empty string in case of failure.
std::string getApplicationPath();
//-----------------------------------------------------------------------------
//! sets all controls to width of widest control
void adjustControlsMinWidth(std::list<wxWindow*> controls);
//-----------------------------------------------------------------------------
#endif // FRUTILS_H

