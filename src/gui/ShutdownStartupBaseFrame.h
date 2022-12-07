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


#ifndef FR_SHUTDOWNSTARTUPBASEFRAME_H
#define FR_SHUTDOWNSTARTUPBASEFRAME_H

#include <wx/wx.h>
#include <wx/thread.h>

#include <memory>

#include "core/Observer.h"
#include "gui/ServiceBaseFrame.h"
#include "metadata/database.h"
#include "metadata/MetadataClasses.h"

class ShutdownStartupBaseFrame: public ServiceBaseFrame
{
public:

    // make sure that thread gets deleted
    virtual bool Destroy();
protected:

    void cancelShutdownStartup();

    virtual void doReadConfigSettings(const wxString& prefix);
    virtual void doWriteConfigSettings(const wxString& prefix) const;
    virtual const wxString getStorageName() const;

    virtual void createControls();
    virtual void layoutControls();
    virtual void updateControls();

    ShutdownStartupBaseFrame(wxWindow* parent, DatabasePtr db);
private:


    // observer stuff
    virtual void subjectRemoved(Subject* subject);
    virtual void update();

protected:
    enum {
        ID_radiobox_state = 101,
    };

    wxRadioBox* radiobox_state;
    IBPP::DSM getDatabaseMode();

private:
    // event handling

    void OnVerboseLogChange(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};

class ShutdownStartupThread : public ServiceThread
{
public:
    ShutdownStartupThread(ShutdownStartupBaseFrame* frame,  wxString server,
        wxString username, wxString password, wxString rolename, wxString charset,
        wxString dbfilename, IBPP::DSM flags
    );
protected:
    wxString dbfileM;
    IBPP::DSM dsmM;
};

#endif // FR_SHUTDOWNSTARTUPBASEFRAME_H
