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

#if !defined(__WXMSW__)
#error Include this only in project for Windows!!!
#endif

#include "gui/StyleGuide.h"
#ifdef __GNUWIN32__
#include "wx/msw/wrapwin.h" // for "windows.h"
#endif

class StyleGuideMSW: public StyleGuide
{
private:
    int dbuHorzM;
    int dbuVertM;
    bool dbuValidM;
    void dbuNeeded();
    int dbuToPixelHorz(int dbu);
    int dbuToPixelVert(int dbu);
public:
    StyleGuideMSW();
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

StyleGuideMSW::StyleGuideMSW()
{
    dbuHorzM = 0;
    dbuVertM = 0;
    dbuValidM = false;
}

void StyleGuideMSW::dbuNeeded()
{
    if (!dbuValidM)
    {
        HDC dc = GetDC(0);
        HFONT fnt = (HFONT)SelectObject(dc, 
            (HFONT)GetStockObject(DEFAULT_GUI_FONT));

        wxString s("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
        int len = int(s.Length());
        SIZE sz;
        GetTextExtentPoint32(dc, s.c_str(), len, &sz);
        SelectObject(dc, fnt);
        ReleaseDC(0, dc);

        dbuHorzM = (sz.cx + len / 2) / len;
        dbuVertM = sz.cy;
        dbuValidM = true;
    }
}

int StyleGuideMSW::dbuToPixelHorz(int dbu)
{
    dbuNeeded();
    return dbu * dbuHorzM / 4;
}

int StyleGuideMSW::dbuToPixelVert(int dbu)
{
    dbuNeeded();
    return dbu * dbuVertM / 8;
}

wxSizer* StyleGuideMSW::createButtonSizer(wxButton* affirmativeButton,
    wxButton* negativeButton, wxButton* alternateButton)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    // right-align
    sizer->AddStretchSpacer(1);
    if (affirmativeButton)
        sizer->Add(affirmativeButton);
    if (affirmativeButton && (alternateButton || negativeButton))
        sizer->AddSpacer(getBetweenButtonsMargin(wxHORIZONTAL));
    if (alternateButton)
        sizer->Add(alternateButton);
    if (alternateButton && negativeButton)
        sizer->AddSpacer(getBetweenButtonsMargin(wxHORIZONTAL));
    if (negativeButton)
        sizer->Add(negativeButton);
    return sizer;
}

int StyleGuideMSW::getBetweenButtonsMargin(wxOrientation orientation)
{
    switch (orientation)
    {
        case wxHORIZONTAL:
            return dbuToPixelHorz(4);
        case wxVERTICAL:
            return dbuToPixelVert(4);
        default:
            return 0;
    }
}

int StyleGuideMSW::getBrowseButtonMargin()
{
    return dbuToPixelHorz(1);
}

int StyleGuideMSW::getCheckboxSpacing()
{
    return dbuToPixelVert(3);
}

int StyleGuideMSW::getControlLabelMargin()
{
    return dbuToPixelHorz(3);
}

int StyleGuideMSW::getDialogMargin(wxDirection direction)
{
    switch (direction)
    {
        case wxLEFT:
        case wxRIGHT:
            return dbuToPixelHorz(7);
        case wxTOP:
        case wxBOTTOM:
            return dbuToPixelVert(7);
        default:
            return 0;
    }
}

int StyleGuideMSW::getFrameMargin(wxDirection direction)
{
    switch (direction)
    {
        case wxLEFT:
        case wxRIGHT:
            return dbuToPixelHorz(4);
        case wxTOP:
        case wxBOTTOM:
            return dbuToPixelVert(4);
        default:
            return 0;
    }
}

int StyleGuideMSW::getMessageBoxIconMargin()
{
    return dbuToPixelHorz(12);
}

int StyleGuideMSW::getMessageBoxBetweenTextMargin()
{
    return dbuToPixelVert(8);
}

int StyleGuideMSW::getRelatedControlMargin(wxOrientation orientation)
{
    switch (orientation)
    {
        case wxHORIZONTAL:
            return dbuToPixelHorz(4);
        case wxVERTICAL:
            return dbuToPixelVert(4);
        default:
            return 0;
    }
}

int StyleGuideMSW::getUnrelatedControlMargin(wxOrientation orientation)
{
    switch (orientation)
    {
        case wxHORIZONTAL:
            return dbuToPixelHorz(7);
        case wxVERTICAL:
            return dbuToPixelVert(7);
        default:
            return 0;
    }
}

StyleGuide& styleguide()
{
    static StyleGuideMSW guide;
    return guide;
}

