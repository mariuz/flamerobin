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

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "gui/ExecuteRoutineDialog.h"
#include "gui/ExecuteSqlFrame.h"
#include "gui/StyleGuide.h"
#include "metadata/procedure.h"
#include "metadata/function.h"
#include "metadata/parameter.h"
#include "metadata/RoutineHelper.h"
#include "engine/db/IDatabase.h"
#include "engine/db/ITransaction.h"
#include "engine/db/IStatement.h"
#include "core/StringUtils.h"

enum {
    ID_button_execute_routine = 5001,
    ID_button_open_in_editor
};

BEGIN_EVENT_TABLE(ExecuteRoutineDialog, BaseDialog)
    EVT_BUTTON(ID_button_execute_routine, ExecuteRoutineDialog::OnExecuteClick)
    EVT_BUTTON(ID_button_open_in_editor, ExecuteRoutineDialog::OnOpenEditorClick)
END_EVENT_TABLE()

ExecuteRoutineDialog::ExecuteRoutineDialog(wxWindow* parent, MetadataItem* routineItem)
    : BaseDialog(parent, -1, wxEmptyString), routineM(routineItem)
{
    databasePtrM = routineM ? routineM->getDatabase() : nullptr;
    databaseM = databasePtrM.get();

    if (routineM)
        SetTitle(wxString::Format(_("Interactive Routine Executor - %s"), routineM->getName_().c_str()));

    wxBoxSizer* sizerMain = new wxBoxSizer(wxVERTICAL);

    // Input parameters panel
    wxStaticBoxSizer* sizerParamsBox = new wxStaticBoxSizer(wxVERTICAL, this, _("Input Parameters"));
    wxPanel* panelParams = new wxPanel(this, -1);
    wxBoxSizer* sizerParams = new wxBoxSizer(wxVERTICAL);

    buildParameterInputs(panelParams, sizerParams);
    panelParams->SetSizer(sizerParams);
    sizerParamsBox->Add(panelParams, 1, wxEXPAND | wxALL, 4);

    sizerMain->Add(sizerParamsBox, 0, wxEXPAND | wxALL, 6);

    // Action buttons
    wxBoxSizer* sizerBtns = new wxBoxSizer(wxHORIZONTAL);
    button_execute = new wxButton(this, ID_button_execute_routine, _("Execute Routine"));
    button_open_editor = new wxButton(this, ID_button_open_in_editor, _("Open in SQL Editor"));

    sizerBtns->Add(button_execute, 0, wxRIGHT, 6);
    sizerBtns->Add(button_open_editor, 0);
    sizerMain->Add(sizerBtns, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxALIGN_RIGHT, 6);

    // Output Result Grid
    sizerMain->Add(new wxStaticText(this, -1, _("Execution Result Set:")), 0, wxLEFT | wxTOP, 6);
    grid_results = new wxGrid(this, -1, wxDefaultPosition, wxSize(-1, 200));
    grid_results->CreateGrid(0, 0);
    grid_results->EnableEditing(false);
    sizerMain->Add(grid_results, 1, wxEXPAND | wxALL, 6);

    wxSizer* sizerButtons = CreateButtonSizer(wxCLOSE);
    sizerMain->Add(sizerButtons, 0, wxEXPAND | wxALL, 6);

    SetSizerAndFit(sizerMain);
    SetSize(wxSize(680, 580));
}

ExecuteRoutineDialog::~ExecuteRoutineDialog()
{
}

void ExecuteRoutineDialog::buildParameterInputs(wxPanel* parentPanel, wxBoxSizer* targetSizer)
{
    paramCtrlsM.clear();
    if (!routineM) return;

    Procedure* p = dynamic_cast<Procedure*>(routineM);
    if (p)
    {
        p->ensureChildrenLoaded();
        for (auto it = p->begin(); it != p->end(); ++it)
        {
            Parameter* param = (*it).get();
            if (!param || param->isOutputParameter()) continue;

            wxBoxSizer* rowSizer = new wxBoxSizer(wxHORIZONTAL);
            wxStaticText* labelName = new wxStaticText(parentPanel, -1, param->getName_(), wxDefaultPosition, wxSize(140, -1));
            wxStaticText* labelType = new wxStaticText(parentPanel, -1, wxString::Format("(%s)", param->getDatatype().c_str()), wxDefaultPosition, wxSize(120, -1));
            wxTextCtrl* textCtrl = new wxTextCtrl(parentPanel, -1, wxEmptyString);
            wxCheckBox* nullCheck = new wxCheckBox(parentPanel, -1, _("NULL"));

            wxString defVal;
            if (param->getDefault(IgnoreDomainDefault, defVal) && !defVal.IsEmpty())
                textCtrl->SetValue(defVal);

            rowSizer->Add(labelName, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
            rowSizer->Add(labelType, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
            rowSizer->Add(textCtrl, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
            rowSizer->Add(nullCheck, 0, wxALIGN_CENTER_VERTICAL);

            targetSizer->Add(rowSizer, 0, wxEXPAND | wxTOP | wxBOTTOM, 4);

            RoutineParamInputCtrl ctrlInfo;
            ctrlInfo.name = param->getName_();
            ctrlInfo.datatype = param->getDatatype();
            ctrlInfo.isOutput = false;
            ctrlInfo.textCtrl = textCtrl;
            ctrlInfo.nullCheckBox = nullCheck;
            paramCtrlsM.push_back(ctrlInfo);
        }
    }

    Function* f = dynamic_cast<Function*>(routineM);
    if (f)
    {
        f->ensureChildrenLoaded();
        for (auto it = f->begin(); it != f->end(); ++it)
        {
            Parameter* param = (*it).get();
            if (!param || param->isOutputParameter()) continue;

            wxBoxSizer* rowSizer = new wxBoxSizer(wxHORIZONTAL);
            wxStaticText* labelName = new wxStaticText(parentPanel, -1, param->getName_(), wxDefaultPosition, wxSize(140, -1));
            wxStaticText* labelType = new wxStaticText(parentPanel, -1, wxString::Format("(%s)", param->getDatatype().c_str()), wxDefaultPosition, wxSize(120, -1));
            wxTextCtrl* textCtrl = new wxTextCtrl(parentPanel, -1, wxEmptyString);
            wxCheckBox* nullCheck = new wxCheckBox(parentPanel, -1, _("NULL"));

            wxString defVal;
            if (param->getDefault(IgnoreDomainDefault, defVal) && !defVal.IsEmpty())
                textCtrl->SetValue(defVal);

            rowSizer->Add(labelName, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
            rowSizer->Add(labelType, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
            rowSizer->Add(textCtrl, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
            rowSizer->Add(nullCheck, 0, wxALIGN_CENTER_VERTICAL);

            targetSizer->Add(rowSizer, 0, wxEXPAND | wxTOP | wxBOTTOM, 4);

            RoutineParamInputCtrl ctrlInfo;
            ctrlInfo.name = param->getName_();
            ctrlInfo.datatype = param->getDatatype();
            ctrlInfo.isOutput = false;
            ctrlInfo.textCtrl = textCtrl;
            ctrlInfo.nullCheckBox = nullCheck;
            paramCtrlsM.push_back(ctrlInfo);
        }
    }

    if (paramCtrlsM.empty())
    {
        targetSizer->Add(new wxStaticText(parentPanel, -1, _("Routine takes no input parameters.")), 0, wxALL, 6);
    }
}

void ExecuteRoutineDialog::OnExecuteClick(wxCommandEvent& WXUNUSED(event))
{
    if (!routineM || !databaseM) return;

    if (!databaseM->isConnected())
    {
        try { databaseM->connect(databaseM->getRawPassword()); }
        catch (const std::exception& e)
        {
            wxMessageBox(wxString::FromUTF8(e.what()), _("Connection Error"), wxOK | wxICON_ERROR, this);
            return;
        }
    }

    wxString sql = RoutineHelper::getRoutineExecutionTemplate(routineM);
    if (sql.IsEmpty()) return;

    try
    {
        auto dalDb = databaseM->getDALDatabase();
        auto tr = dalDb->createTransaction();
        tr->start();
        auto st = dalDb->createStatement(tr);

        st->prepare(wx2std(sql));

        // Bind input parameters
        for (size_t i = 0; i < paramCtrlsM.size(); ++i)
        {
            const auto& ctrlInfo = paramCtrlsM[i];
            int paramIndex = (int)i;

            if (ctrlInfo.nullCheckBox->IsChecked())
            {
                st->setNull(paramIndex);
            }
            else
            {
                wxString val = ctrlInfo.textCtrl->GetValue();
                st->setString(paramIndex, wx2std(val));
            }
        }

        st->execute();

        // Populate result grid
        if (grid_results->GetNumberRows() > 0)
            grid_results->DeleteRows(0, grid_results->GetNumberRows());
        if (grid_results->GetNumberCols() > 0)
            grid_results->DeleteCols(0, grid_results->GetNumberCols());

        int colCount = st->getColumnCount();
        grid_results->AppendCols(colCount);
        for (int c = 0; c < colCount; ++c)
        {
            grid_results->SetColLabelValue(c, wxString::FromUTF8(st->getColumnName(c).c_str()));
        }

        int rowIdx = 0;
        while (st->fetch())
        {
            grid_results->AppendRows(1);
            for (int c = 0; c < colCount; ++c)
            {
                if (st->isNull(c))
                    grid_results->SetCellValue(rowIdx, c, "<NULL>");
                else
                    grid_results->SetCellValue(rowIdx, c, wxString::FromUTF8(st->getString(c).c_str()));
            }
            rowIdx++;
        }

        tr->commit();
        grid_results->AutoSizeColumns();
    }
    catch (const std::exception& e)
    {
        wxMessageBox(wxString::FromUTF8(e.what()), _("Execution Error"), wxOK | wxICON_ERROR, this);
    }
}

void ExecuteRoutineDialog::OnOpenEditorClick(wxCommandEvent& WXUNUSED(event))
{
    if (!routineM || !databaseM) return;

    wxString sql = RoutineHelper::getRoutineExecutionTemplate(routineM);
    if (!sql.IsEmpty())
    {
        ExecuteSqlFrame* eff = new ExecuteSqlFrame(GetParent(), -1, _("Routine Execution"), databasePtrM);
        eff->setSql(sql);
        eff->Show(true);
        Close();
    }
}
