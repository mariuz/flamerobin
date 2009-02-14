/*
  Copyright (c) 2004-2009 The FlameRobin Development Team

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
//-----------------------------------------------------------------------------

#ifndef FR_PROGRESSINDICATOR_H
#define FR_PROGRESSINDICATOR_H

#include <wx/wx.h>
//-----------------------------------------------------------------------------
class ProgressIndicator
{
public:
    virtual ~ProgressIndicator() {};
    virtual bool isCanceled() = 0;
    virtual void initProgress(wxString progressMsg,
        unsigned int maxPosition = 0, unsigned int startingPosition = 0,
        unsigned int progressLevel = 1) = 0;
    virtual void initProgressIndeterminate(wxString progressMsg,
        unsigned int progressLevel = 1) = 0;
    virtual void setProgressMessage(wxString progressMsg,
        unsigned int progressLevel = 1) = 0;
    virtual void setProgressPosition(unsigned int currentPosition,
        unsigned int progressLevel = 1) = 0;
    virtual void stepProgress(int stepAmount = 1,
        unsigned int progressLevel = 1) = 0;
};
//-----------------------------------------------------------------------------
#endif // FR_PROGRESSINDICATOR_H
