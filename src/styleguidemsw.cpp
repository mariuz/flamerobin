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

  Contributor(s): Nando Dessena
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

#if !defined(__WXMSW__)
#error Include this only in project for Windows!!!
#endif

#include "styleguide.h"
#ifdef __GNUWIN32__
#include "windows.h"
#endif
//------------------------------------------------------------------------------
class YStyleGuideMSW: public YxStyleGuide
{
private:
    int dbuHorzM;
    int dbuVertM;
    bool dbuValidM;
    void dbuNeeded();
    int dbuToPixelHorz(int dbu);
    int dbuToPixelVert(int dbu);
public:
    YStyleGuideMSW();
    virtual wxSizer* createButtonSizer(wxButton* button_ok, wxButton* button_cancel);
    virtual int getBetweenButtonsMargin(wxOrientation orientation);
    virtual int getBrowseButtonMargin();
    virtual int getCheckboxSpacing();
    virtual int getControlLabelMargin();
    virtual int getDialogMargin(wxDirection direction);
    virtual int getFrameMargin(wxDirection direction);
    virtual int getRelatedControlMargin(wxOrientation orientation);
    virtual int getUnrelatedControlMargin(wxOrientation orientation);
	virtual int getEditorFontSize();
};
//------------------------------------------------------------------------------
YStyleGuideMSW::YStyleGuideMSW()
{
    dbuHorzM = 0;
    dbuVertM = 0;
    dbuValidM = false;
}
//------------------------------------------------------------------------------
void YStyleGuideMSW::dbuNeeded()
{
    if (!dbuValidM)
    {
        HDC dc = GetDC(0);
        HFONT fnt = (HFONT)SelectObject(dc, (HFONT)GetStockObject(DEFAULT_GUI_FONT));

        wxString s(wxT("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"));
        int len = s.Length();
        SIZE sz;
        GetTextExtentPoint32(dc, s.c_str(), len, &sz);
        SelectObject(dc, fnt);
        ReleaseDC(0, dc);

        dbuHorzM = (sz.cx + len / 2) / len;
        dbuVertM = sz.cy;
        dbuValidM = true;
    }
}
//------------------------------------------------------------------------------
int YStyleGuideMSW::dbuToPixelHorz(int dbu)
{
    dbuNeeded();
    return dbu * dbuHorzM / 4;
}
//------------------------------------------------------------------------------
int YStyleGuideMSW::dbuToPixelVert(int dbu)
{
    dbuNeeded();
    return dbu * dbuVertM / 8;
}
//------------------------------------------------------------------------------
wxSizer* YStyleGuideMSW::createButtonSizer(wxButton* button_ok, wxButton* button_cancel)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    // right-align
    sizer->Add(0, 0, 1, wxEXPAND);
    if (button_ok != 0)
        sizer->Add(button_ok);
    if (button_ok != 0 && button_cancel != 0)
        sizer->Add(getBetweenButtonsMargin(wxHORIZONTAL), 0);
    if (button_cancel != 0)
        sizer->Add(button_cancel);
    return sizer;
}
//------------------------------------------------------------------------------
int YStyleGuideMSW::getBetweenButtonsMargin(wxOrientation orientation)
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
//------------------------------------------------------------------------------
int YStyleGuideMSW::getBrowseButtonMargin()
{
	return dbuToPixelHorz(1);
}
//------------------------------------------------------------------------------
int YStyleGuideMSW::getCheckboxSpacing()
{
    return dbuToPixelVert(3);
}
//------------------------------------------------------------------------------
int YStyleGuideMSW::getControlLabelMargin()
{
    return dbuToPixelHorz(3);
}
//------------------------------------------------------------------------------
int YStyleGuideMSW::getDialogMargin(wxDirection direction)
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
//------------------------------------------------------------------------------
int YStyleGuideMSW::getFrameMargin(wxDirection direction)
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
//------------------------------------------------------------------------------
int YStyleGuideMSW::getRelatedControlMargin(wxOrientation orientation)
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
//------------------------------------------------------------------------------
int YStyleGuideMSW::getUnrelatedControlMargin(wxOrientation orientation)
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
//------------------------------------------------------------------------------
int YStyleGuideMSW::getEditorFontSize()
{
	return 10;
}
//------------------------------------------------------------------------------
YxStyleGuide& styleguide()
{
    static YStyleGuideMSW guide;
    return guide;
}
//------------------------------------------------------------------------------
