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

//! Support for dialog layout according to rules of different GUIs
//
//
#ifndef FR_STYLEGUIDE_H
#define FR_STYLEGUIDE_H

#include <wx/wx.h>


//! There will be exactly one static object of descendent class, returned by
//! styleguide() (see below).
class StyleGuide
{
public:
    /* creates a horizontal box sizer containing the button controls in the
       platform-correct order and correctly spaced */
    virtual wxSizer* createButtonSizer(wxButton* affirmativeButton,
        wxButton* negativeButton, wxButton* alternateButton = 0) = 0;
    /* returns the horizontal/vertical pixel margin between two buttons */
    virtual int getBetweenButtonsMargin(wxOrientation orientation) = 0;
    /* returns the horizontal pixel margin between a text control (e.g. for
       entering a file name) and the browse button attached to it */
    virtual int getBrowseButtonMargin() = 0;
    /* returns the vertical pixel margin between two stacked check boxes */
    virtual int getCheckboxSpacing() = 0;
    /* returns the horizontal pixel margin between a control and its label */
    virtual int getControlLabelMargin() = 0;
    /* returns the horizontal/vertical pixel border space of a dialog */
    virtual int getDialogMargin(wxDirection direction) = 0;
    /* returns the horizontal/vertical pixel border space of a frame */
    virtual int getFrameMargin(wxDirection direction) = 0;
    /* returns the horizontal pixel margin between the icon control and
       the text control(s) in a message box */
    virtual int getMessageBoxIconMargin() = 0;
    /* returns the vertical pixel margin between the primary and secondary
       text controls in a message box */
    virtual int getMessageBoxBetweenTextMargin() = 0;
    /* returns the horizontal/vertical pixel margin between two related
       controls (related vs. unrelated used for control grouping) */
    virtual int getRelatedControlMargin(wxOrientation orientation) = 0;
    /* returns the horizontal/vertical pixel margin between two unrelated
       controls (related vs. unrelated used for control grouping) */
    virtual int getUnrelatedControlMargin(wxOrientation orientation) = 0;

protected:
    /* class can't be instantiated, descendent classes will be */
    StyleGuide();
    virtual ~StyleGuide();
};

StyleGuide& styleguide();

#endif
