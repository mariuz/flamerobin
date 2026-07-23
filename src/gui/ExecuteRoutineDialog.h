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

#ifndef FR_EXECUTEROUTINEDIALOG_H
#define FR_EXECUTEROUTINEDIALOG_H

#include <wx/wx.h>
#include <wx/grid.h>
#include <vector>

#include "gui/BaseDialog.h"
#include "metadata/metadataitem.h"
#include "metadata/database.h"

struct RoutineParamInputCtrl
{
    wxString name;
    wxString datatype;
    bool isOutput;
    wxTextCtrl* textCtrl;
    wxCheckBox* nullCheckBox;
};

class ExecuteRoutineDialog : public BaseDialog
{
private:
    DatabasePtr databasePtrM;
    Database* databaseM;
    MetadataItem* routineM;

    std::vector<RoutineParamInputCtrl> paramCtrlsM;
    wxGrid* grid_results;
    wxButton* button_execute;
    wxButton* button_open_editor;

    void buildParameterInputs(wxPanel* parentPanel, wxBoxSizer* targetSizer);
    void OnExecuteClick(wxCommandEvent& event);
    void OnOpenEditorClick(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
public:
    ExecuteRoutineDialog(wxWindow* parent, MetadataItem* routineItem);
    virtual ~ExecuteRoutineDialog();
};

#endif
