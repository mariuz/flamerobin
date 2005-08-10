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

Contributor(s):
*/

#ifndef RESTOREFRAME_H
#define RESTOREFRAME_H

#include <wx/wx.h>
#include "BackupRestoreBaseFrame.h"

class RestoreThread;

class RestoreFrame: public BackupRestoreBaseFrame {
    friend class RestoreThread;
public:
    // events
    void OnBrowseButtonClick(wxCommandEvent& event);
    void OnStartButtonClick(wxCommandEvent& event);

    RestoreFrame(wxWindow* parent, Database* db);
private:
    void do_layout();
    virtual void updateControls();
protected:
    virtual void doReadConfigSettings(const std::string& prefix);
    virtual void doWriteConfigSettings(const std::string& prefix) const;
    virtual const std::string getName() const;
protected:
    wxCheckBox* checkbox_replace;
    wxCheckBox* checkbox_deactivate;
    wxCheckBox* checkbox_noshadow;
    wxCheckBox* checkbox_validity;
    wxCheckBox* checkbox_commit;
    wxCheckBox* checkbox_space;
    wxStaticText* label_pagesize;
    wxChoice* choice_pagesize;
    DECLARE_EVENT_TABLE()
};
#endif // RESTOREFRAME_H
