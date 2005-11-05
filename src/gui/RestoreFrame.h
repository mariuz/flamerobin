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

//-----------------------------------------------------------------------------
#ifndef RESTOREFRAME_H
#define RESTOREFRAME_H

#include <wx/wx.h>

#include "BackupRestoreBaseFrame.h"

class RestoreThread;
//-----------------------------------------------------------------------------
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

    static wxString getFrameId(Database* db);
protected:
    virtual void doReadConfigSettings(const wxString& prefix);
    virtual void doWriteConfigSettings(const wxString& prefix) const;
    virtual const wxString getName() const;
public:
    RestoreFrame(wxWindow* parent, Database* db);

    static RestoreFrame* findFrameFor(Database* db);
private:
    // event handling
    void OnBrowseButtonClick(wxCommandEvent& event);
    void OnStartButtonClick(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif // RESTOREFRAME_H
