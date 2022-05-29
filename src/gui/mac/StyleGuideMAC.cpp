/*
  Copyright (c) 2004-2022 The FlameRobin Development Team

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
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#if !defined(__WXMAC__)
#error Include this only in project for Mac OS X!!!
#endif

#include "gui/StyleGuide.h"

class StyleGuideMAC: public StyleGuide
{
public:
    StyleGuideMAC();
    virtual wxSizer* createButtonSizer(wxButton* affirmativeButton,
        wxButton* negativeButton, wxButton* alternateButton = 0);
    virtual int getBetweenButtonsMargin(wxOrientation orientation);
    virtual int getBrowseButtonMargin();
    virtual int getCheckboxSpacing();
    virtual int getControlLabelMargin();
    virtual int getDialogMargin(wxDirection direction);
    virtual int getFrameMargin(wxDirection direction);
    virtual int getMessageBoxIconMargin();
    virtual int getMessageBoxBetweenTextMargin();
    virtual int getRelatedControlMargin(wxOrientation orientation);
    virtual int getUnrelatedControlMargin(wxOrientation orientation);
};

StyleGuideMAC::StyleGuideMAC()
{
}

wxSizer* StyleGuideMAC::createButtonSizer(wxButton* affirmativeButton,
    wxButton* negativeButton, wxButton* alternateButton)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    // right-align
    sizer->AddStretchSpacer(1);
    if (alternateButton)
        sizer->Add(alternateButton);
    if (alternateButton && (negativeButton || affirmativeButton))
        sizer->AddSpacer(2 * getBetweenButtonsMargin(wxHORIZONTAL));
    if (negativeButton)
        sizer->Add(negativeButton);
    if (negativeButton && affirmativeButton)
        sizer->AddSpacer(getBetweenButtonsMargin(wxHORIZONTAL));
    if (affirmativeButton)
        sizer->Add(affirmativeButton);
    return sizer;
}

int StyleGuideMAC::getBetweenButtonsMargin(wxOrientation /*orientation*/)
{
    return 12;
}

int StyleGuideMAC::getBrowseButtonMargin()
{
    return 10;
}

int StyleGuideMAC::getCheckboxSpacing()
{
    return 7;
}

int StyleGuideMAC::getControlLabelMargin()
{
    return 5;
}

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

int StyleGuideMAC::getFrameMargin(wxDirection /*direction*/)
{
    return 16;
}

int StyleGuideMAC::getMessageBoxIconMargin()
{
    return 16;
}

int StyleGuideMAC::getMessageBoxBetweenTextMargin()
{
    return 8;
}

int StyleGuideMAC::getRelatedControlMargin(wxOrientation /*orientation*/)
{
    return 12;
}

int StyleGuideMAC::getUnrelatedControlMargin(wxOrientation /*orientation*/)
{
    return 16;
}

StyleGuide& styleguide()
{
    static StyleGuideMAC guide;
    return guide;
}
