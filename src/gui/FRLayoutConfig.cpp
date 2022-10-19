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

#include <algorithm>
#include "gui/FRLayoutConfig.h"

class RgbHsvConversion // used by StyleGuidFR::getReadonlyColour()
{
private:
    float redM, greenM, blueM;
    float hueM, satM, valM;
    bool rgbValidM, hsvValidM;
    void calcHsvFromRgb();
    void calcRgbFromHsv();
public:
    RgbHsvConversion(const wxColour& colour);
    wxColour getColour();
    float getValue();
    void setValue(float value);
};

RgbHsvConversion::RgbHsvConversion(const wxColour& colour)
{
    redM = colour.Red() / 255.0;
    greenM = colour.Green() / 255.0;
    blueM = colour.Blue() / 255.0;
    rgbValidM = colour.IsOk();
    hueM = 0.0;
    satM = 0.0;
    valM = 0.0;
    hsvValidM = false;
}

// Adapted from public domain code by Zack Booth Simpson
// http://www.mine-control.com/zack/code/zrgbhsv.cpp
void RgbHsvConversion::calcHsvFromRgb()
{
    float rgbMin = std::min(redM, std::min(greenM, blueM));
    float rgbMax = std::max(redM, std::max(greenM, blueM));
    valM = rgbMax;
    float delta = rgbMax - rgbMin;
    if (delta == 0.0)
    {
        // gray value, saturation is 0, hue is undefined
        hueM = -1.0;
        satM = 0.0;
        hsvValidM = true;
        return;
    }

    satM = delta / rgbMax;
    if (redM == rgbMax) // between yellow & magenta
        hueM = (greenM - blueM) / delta;
    else if (greenM == rgbMax) // between cyan & yellow
        hueM = 2 + (blueM - redM) / delta;
    else // between magenta & cyan
        hueM = 4 + (redM - greenM) / delta;
    hueM *= 60; // degrees
    if (hueM < 0.0)
        hueM += 360.0;
    hueM /= 360.0;
    hsvValidM = true;
}

// Adapted from public domain code by Zack Booth Simpson
// http://www.mine-control.com/zack/code/zrgbhsv.cpp
void RgbHsvConversion::calcRgbFromHsv()
{
    if (satM == 0.0) // gray value, saturation is 0, hue is undefined
    {
        redM = greenM = blueM = valM;
        rgbValidM = true;
        return;
    }

    float f, p, q, t;
    float h = 6.0 * hueM;
    int sector = (int) floor(h);
    f = h - sector;
    p = valM * (1.0 - satM);
    q = valM * (1.0 - satM * f);
    t = valM * (1.0 - satM * (1.0 - f));

    switch (sector)
    {
        case 0:
            redM = valM;
            greenM = t;
            blueM = p;
            break;
        case 1:
            redM = q;
            greenM = valM;
            blueM = p;
            break;
        case 2:
            redM = p;
            greenM = valM;
            blueM = t;
            break;
        case 3:
            redM = p;
            greenM = q;
            blueM = valM;
            break;
        case 4:
            redM = t;
            greenM = p;
            blueM = valM;
            break;
        default:
            redM = valM;
            greenM = p;
            blueM = q;
            break;
    }
    rgbValidM = true;
}

wxColour RgbHsvConversion::getColour()
{
    if (!rgbValidM)
    {
        wxASSERT(hsvValidM);
        calcRgbFromHsv();
    }
    return wxColour((unsigned char)(255.0 * redM),
        (unsigned char)(255.0 * greenM), (unsigned char)(255.0 * blueM));
}

float RgbHsvConversion::getValue()
{
    if (!hsvValidM)
    {
        wxASSERT(rgbValidM);
        calcHsvFromRgb();
    }
    return valM;
}

void RgbHsvConversion::setValue(float value)
{
    if (value < 0.0 || value > 1.0)
        return;
    if (rgbValidM && !hsvValidM)
        calcHsvFromRgb();
    if (valM != value)
    {
        valM = value;
        rgbValidM = false;
    }
}

FRLayoutConfig::FRLayoutConfig()
{
}

FRLayoutConfig::~FRLayoutConfig()
{
}

wxColour FRLayoutConfig::getReadonlyColour()
{
    static wxColour colourReadOnly;
    if (!colourReadOnly.IsOk())
    {
        // first try to compute a colour that is between "white" and "gray"
        // (but use the actual system colours instead of hard-coded values)
        //wxColour clWnd(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
        wxColour clWnd(stylerManager().getDefaultStyle()->getbgColor());
        int r1 = clWnd.Red(), g1 = clWnd.Green(), b1 = clWnd.Blue();
        wxColour clBtn = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
        int r2 = clBtn.Red(), g2 = clBtn.Green(), b2 = clBtn.Blue();
        int distance = abs(r1 - r2) + abs(g1 - g2) + abs(b1 - b2);
        if (distance >= 72)
        {
            // start at 50 %, and use progressively lighter colours for larger
            // distances between white and gray
            int scale = (distance >= 192) ? distance / 64 : 2;
            colourReadOnly.Set(r1 + (r2 - r1) / scale,
                g1 + (g2 - g1) / scale, b1 + (b2 - b1) / scale);
        }
        else
        {
            // wxSYS_COLOUR_WINDOW and wxSYS_COLOUR_BTNFACE are too similar
            // compute a darker shade of wxSYS_COLOUR_WINDOW
            RgbHsvConversion rgbhsv(clWnd);
            rgbhsv.setValue(std::max(0.0, rgbhsv.getValue() - 0.05));
            colourReadOnly = rgbhsv.getColour();
        }
    }
    return colourReadOnly;
}

int FRLayoutConfig::getEditorFontSize()
{
#ifdef __WINDOWS__
    return 10; // WIN
#else
    return 12; // MAC, GTK
#endif
}

FRLayoutConfig& frlayoutconfig()
{
    static FRLayoutConfig layout;
    return layout;
}

