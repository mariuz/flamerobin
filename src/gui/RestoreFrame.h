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


#ifndef RESTOREFRAME_H
#define RESTOREFRAME_H

#include <wx/wx.h>

#include "BackupRestoreBaseFrame.h"

class RestoreThread;

class RestoreFrame: public BackupRestoreBaseFrame {
    friend class RestoreThread;
private:
    wxCheckBox* checkbox_replace;
    wxCheckBox* checkbox_deactivate;
    wxCheckBox* checkbox_noshadow;
    wxCheckBox* checkbox_validity;
    wxCheckBox* checkbox_commit;
    wxCheckBox* checkbox_space;
    wxStaticText* label_pagesize;
    wxChoice* choice_pagesize;
    void createControls();
    void layoutControls();
    virtual void updateControls();

    static wxString getFrameId(DatabasePtr db);
protected:
    virtual void doReadConfigSettings(const wxString& prefix);
    virtual void doWriteConfigSettings(const wxString& prefix) const;
    virtual const wxString getName() const;
public:
    RestoreFrame(wxWindow* parent, DatabasePtr db);

    static RestoreFrame* findFrameFor(DatabasePtr db);
private:
    // event handling
    void OnBrowseButtonClick(wxCommandEvent& event);
    void OnStartButtonClick(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};

#endif // RESTOREFRAME_H
