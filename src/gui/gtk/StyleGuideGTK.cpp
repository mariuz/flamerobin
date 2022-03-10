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

#if defined(__WXMSW__) || defined(__WXMAC__)
#error Must not be included in project for Windows or OS X!!!
#endif

#include "gui/StyleGuide.h"

class StyleGuideGTK: public StyleGuide
{
public:
    StyleGuideGTK();
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

StyleGuideGTK::StyleGuideGTK()
{
}

wxSizer* StyleGuideGTK::createButtonSizer(wxButton* affirmativeButton,
    wxButton* negativeButton, wxButton* alternateButton)
{

    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    // right-align as per HIG
    sizer->AddStretchSpacer(1);
    if (alternateButton)
        sizer->Add(alternateButton);
    if (alternateButton && (negativeButton || affirmativeButton))
        sizer->AddSpacer(getBetweenButtonsMargin(wxHORIZONTAL));
    if (negativeButton)
        sizer->Add(negativeButton);
    if (negativeButton && affirmativeButton)
        sizer->AddSpacer(getBetweenButtonsMargin(wxHORIZONTAL));
    if (affirmativeButton)
        sizer->Add(affirmativeButton);
    return sizer;
}

int StyleGuideGTK::getBetweenButtonsMargin(wxOrientation /*orientation*/)
{
    return 6;
}

int StyleGuideGTK::getBrowseButtonMargin()
{
    return 6;
}

int StyleGuideGTK::getCheckboxSpacing()
{
    return 6;
}

int StyleGuideGTK::getControlLabelMargin()
{
    return 12;
}

int StyleGuideGTK::getDialogMargin(wxDirection /*direction*/)
{
    return 12;
}

int StyleGuideGTK::getFrameMargin(wxDirection /*direction*/)
{
    return 12;
}

int StyleGuideGTK::getMessageBoxIconMargin()
{
    return 12;
}

int StyleGuideGTK::getMessageBoxBetweenTextMargin()
{
    return 24;
}

int StyleGuideGTK::getRelatedControlMargin(wxOrientation orientation)
{
    switch (orientation)
    {
        case wxHORIZONTAL:
            return 12;
        case wxVERTICAL:
            return 6;
        default:
            return 0;
    }
}

int StyleGuideGTK::getUnrelatedControlMargin(wxOrientation /*orientation*/)
{
    return 18;
}

StyleGuide& styleguide()
{
    static StyleGuideGTK guide;
    return guide;
}
