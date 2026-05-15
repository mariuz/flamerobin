/*
  Copyright (c) 2004-2026 The FlameRobin Development Team

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

#ifndef FR_REPLICATION_STATUS_FRAME_H
#define FR_REPLICATION_STATUS_FRAME_H

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/timer.h>
#include "gui/BaseFrame.h"
#include "metadata/database.h"

namespace fr
{

class ReplicationStatusFrame : public BaseFrame
{
public:
    ReplicationStatusFrame(wxWindow* parent, DatabasePtr db);
    virtual ~ReplicationStatusFrame();

    static ReplicationStatusFrame* findFrameFor(DatabasePtr db);

private:
    static wxString getFrameId(DatabasePtr db);
    DatabaseWeakPtr databaseM;
    wxListCtrl* listCtrlM;
    wxTimer timerM;

    void createControls();
    void layoutControls();
    void refreshStatus();

    void OnTimer(wxTimerEvent& event);
    void OnRefresh(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};

} // namespace fr

#endif // FR_REPLICATION_STATUS_FRAME_H
