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
  are Copyright (C) 2004 Michael Hieke.

  All Rights Reserved.

  $Id$

  Contributor(s):
*/

//! Support for dialog layout according to rules of different GUIs
//
//
#ifndef FR_STYLEGUIDE_H
#define FR_STYLEGUIDE_H

#include <wx/wx.h>

//---------------------------------------------------------------------------------------
//! There will be exactly one static object of descendent class, returned by
//! styleguide() (see below).
class StyleGuide
{
public:
    virtual wxSizer* createButtonSizer(wxButton* button_ok, wxButton* button_cancel) = 0;
    virtual int getBetweenButtonsMargin(wxOrientation orientation) = 0;
    virtual int getBrowseButtonMargin() = 0;
    virtual int getCheckboxSpacing() = 0;
    virtual int getControlLabelMargin() = 0;
    virtual int getDialogMargin(wxDirection direction) = 0;
    virtual int getFrameMargin(wxDirection direction) = 0;
    virtual int getRelatedControlMargin(wxOrientation orientation) = 0;
    virtual int getUnrelatedControlMargin(wxOrientation orientation) = 0;
	virtual int getEditorFontSize() = 0;
protected:
    StyleGuide();
    virtual ~StyleGuide();
};
//---------------------------------------------------------------------------------------
StyleGuide& styleguide();
//---------------------------------------------------------------------------------------
#endif
