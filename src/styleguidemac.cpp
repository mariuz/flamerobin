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

#if !defined(__WXMAC__)
#error Include this only in project for Mac OS X!!!
#endif

#include "styleguide.h"
//------------------------------------------------------------------------------
class StyleGuideMAC: public StyleGuide
{
public:
    StyleGuideMAC();
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
StyleGuideMAC::StyleGuideMAC()
{
}
//------------------------------------------------------------------------------
wxSizer* StyleGuideMAC::createButtonSizer(wxButton* button_ok, wxButton* button_cancel)
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
int StyleGuideMAC::getBetweenButtonsMargin(wxOrientation WXUNUSED(orientation))
{
    return 12;
}
//------------------------------------------------------------------------------
int StyleGuideMAC::getBrowseButtonMargin()
{
    return 10;
}
//------------------------------------------------------------------------------
int StyleGuideMAC::getCheckboxSpacing()
{
    return 7;
}
//------------------------------------------------------------------------------
int StyleGuideMAC::getControlLabelMargin()
{
    return 5;
}
//------------------------------------------------------------------------------
int StyleGuideMAC::getDialogMargin(wxDirection direction)
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
int StyleGuideMAC::getEditorFontSize()
{
    return 12;
}
//------------------------------------------------------------------------------
int StyleGuideMAC::getFrameMargin(wxDirection direction)
{
    return 16;
}
//------------------------------------------------------------------------------
int StyleGuideMAC::getRelatedControlMargin(wxOrientation WXUNUSED(orientation))
{
    return 12;
}
//------------------------------------------------------------------------------
int StyleGuideMAC::getUnrelatedControlMargin(wxOrientation WXUNUSED(orientation))
{
    return 16;
}
//------------------------------------------------------------------------------
StyleGuide& styleguide()
{
    static StyleGuideMAC guide;
    return guide;
}
