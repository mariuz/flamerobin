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


#ifndef MAINTENANCEFRAME_H
#define MAINTENANCEFRAME_H

#include <wx/wx.h>
#include <wx/spinctrl.h>

#include "ServiceBaseFrame.h"
#include "engine/db/DatabaseBackend.h"

class MaintenanceThread;

class MaintenanceFrame: public ServiceBaseFrame {
    friend class MaintenanceThread;
private:
    wxCheckBox* checkbox_sweep;
    wxCheckBox* checkbox_validate;
    wxCheckBox* checkbox_full;
    wxCheckBox* checkbox_mend;
    wxCheckBox* checkbox_readonly;
    wxCheckBox* checkbox_ignore_checksums;
    wxCheckBox* checkbox_kill_shadows;
    wxCheckBox* checkbox_upgrade;

    wxSpinCtrl* spinctrl_parallelworkers;

    virtual void createControls();
    virtual void layoutControls();
    virtual void updateControls();

    static wxString getFrameId(DatabasePtr db);
protected:
    virtual void doReadConfigSettings(const wxString& prefix);
    virtual void doWriteConfigSettings(const wxString& prefix) const;
    virtual const wxString getName() const;
public:
    MaintenanceFrame(wxWindow* parent, DatabasePtr db);

    static MaintenanceFrame* findFrameFor(DatabasePtr db);
private:
    // event handling
    void OnStartButtonClick(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};

class MaintenanceThread : public ServiceThread
{
public:
    MaintenanceThread(MaintenanceFrame* frame, wxString server,
        wxString username, wxString password, wxString rolename, wxString charset,
        const fr::MaintenanceConfig& config
    );
protected:
    virtual void Execute(fr::IServicePtr);
    virtual wxString getOperationName() const;

    fr::MaintenanceConfig configM;

};
#endif // MAINTENANCEFRAME_H
