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

#if !defined(__WXMAC__)
#error Include this only in project for Mac OS X!!!
#endif

#include "styleguide.h"
//------------------------------------------------------------------------------
class YStyleGuideMAC: public YxStyleGuide
{
public:
    YStyleGuideMAC();
    virtual wxSizer* createButtonSizer(wxButton* button_ok, wxButton* button_cancel);
    virtual int getBetweenButtonsMargin(wxOrientation orientation);
    virtual int getBrowseButtonMargin();
    virtual int getCheckboxSpacing();
    virtual int getControlLabelMargin();
    virtual int getDialogMargin(wxDirection direction);
    virtual int getEditorFontSize();
    virtual int getFrameMargin(wxDirection direction);
    virtual int getRelatedControlMargin(wxOrientation orientation);
    virtual int getUnrelatedControlMargin(wxOrientation orientation);
};
//------------------------------------------------------------------------------
YStyleGuideMAC::YStyleGuideMAC()
{
}
//------------------------------------------------------------------------------
wxSizer* YStyleGuideMAC::createButtonSizer(wxButton* button_ok, wxButton* button_cancel)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    // right-align
    sizer->Add(0, 0, 1, wxEXPAND);
    if (button_cancel != 0)
        sizer->Add(button_cancel);
    if (button_ok != 0 && button_cancel != 0)
        sizer->Add(getBetweenButtonsMargin(wxHORIZONTAL), 0);
    if (button_ok != 0)
        sizer->Add(button_ok);
    return sizer;
}
//------------------------------------------------------------------------------
int YStyleGuideMAC::getBetweenButtonsMargin(wxOrientation WXUNUSED(orientation))
{
    return 12;
}
//------------------------------------------------------------------------------
int YStyleGuideMAC::getBrowseButtonMargin()
{
    return 10;
}
//------------------------------------------------------------------------------
int YStyleGuideMAC::getCheckboxSpacing()
{
    return 7;
}
//------------------------------------------------------------------------------
int YStyleGuideMAC::getControlLabelMargin()
{
    return 5;
}
//------------------------------------------------------------------------------
int YStyleGuideMAC::getDialogMargin(wxDirection direction)
{
    switch (direction) 
    {
        case wxTOP:
            return 16;
        case wxLEFT:
        case wxRIGHT:
        case wxBOTTOM:
            return 20;
        default:
            return 0;
    }
}
//------------------------------------------------------------------------------
int YStyleGuideMAC::getEditorFontSize()
{
    return 12;
}
//------------------------------------------------------------------------------
int YStyleGuideMAC::getFrameMargin(wxDirection direction)
{
    return 16;
}
//------------------------------------------------------------------------------
int YStyleGuideMAC::getRelatedControlMargin(wxOrientation WXUNUSED(orientation))
{
    return 12;
}
//------------------------------------------------------------------------------
int YStyleGuideMAC::getUnrelatedControlMargin(wxOrientation WXUNUSED(orientation))
{
    return 16;
}
//------------------------------------------------------------------------------
YxStyleGuide& styleguide()
{
    static YStyleGuideMAC guide;
    return guide;
}
