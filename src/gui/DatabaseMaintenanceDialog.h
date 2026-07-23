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

#ifndef FR_DATABASEMAINTENANCEDIALOG_H
#define FR_DATABASEMAINTENANCEDIALOG_H

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include <wx/gauge.h>
#include <vector>

#include "gui/BaseDialog.h"
#include "metadata/database.h"

struct IndexHealthInfo
{
    wxString name;
    wxString relationName;
    bool isUnique;
    double selectivity;
    bool isActive;
};

class DatabaseMaintenanceDialog : public BaseDialog
{
private:
    Database* databaseM;

    wxListCtrl* list_indices;
    wxButton* button_recalc_all_indices;
    wxButton* button_rebuild_all_indices;
    wxButton* button_recalc_selected_index;

    wxStaticText* text_ods;
    wxStaticText* text_page_size;
    wxStaticText* text_oldest_tx;
    wxStaticText* text_next_tx;
    wxButton* button_run_sweep;

    wxTextCtrl* text_log;
    wxGauge* progress_gauge;

    std::vector<IndexHealthInfo> indicesM;

    void loadIndicesInfo();
    void loadHealthInfo();
    void logMessage(const wxString& msg);

    void OnRecalcAllIndices(wxCommandEvent& event);
    void OnRebuildAllIndices(wxCommandEvent& event);
    void OnRecalcSelectedIndex(wxCommandEvent& event);
    void OnRunSweep(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
public:
    DatabaseMaintenanceDialog(wxWindow* parent, Database* db);
    virtual ~DatabaseMaintenanceDialog();
};

#endif
