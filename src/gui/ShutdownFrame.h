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


#ifndef FR_SHUTDOWNFRAME_H
#define FR_SHUTDOWNFRAME_H

#include <wx/wx.h>

#include "ShutdownStartupBaseFrame.h"

class ShutdownThread;

class ShutdownFrame: public ShutdownStartupBaseFrame {
    friend class ShutdownThread;
private:

    wxCheckBox* checkbox_attach;
    wxCheckBox* checkbox_tran;
    wxCheckBox* checkbox_force;

    wxStaticText* label_timeout;
    wxSpinCtrl* spinctrl_timeout;

    virtual void createControls();
    virtual void layoutControls();
    virtual void updateControls();

    static wxString getFrameId(DatabasePtr db);
protected:
    virtual void doReadConfigSettings(const wxString& prefix);
    virtual void doWriteConfigSettings(const wxString& prefix) const;
    virtual const wxString getName() const;
public:
    ShutdownFrame(wxWindow* parent, DatabasePtr db);

    static ShutdownFrame* findFrameFor(DatabasePtr db);
private:
    // event handling
    void OnStartButtonClick(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};

#endif // FR_SHUTDOWNFRAME_H
