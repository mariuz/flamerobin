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

//! Support for dialog layout according to rules of different GUIs
//
//
#ifndef FR_STYLEGUIDE_H
#define FR_STYLEGUIDE_H

#include <wx/wx.h>

//--------------------------------------------------------------------------------------
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
//--------------------------------------------------------------------------------------
StyleGuide& styleguide();
//--------------------------------------------------------------------------------------
#endif
