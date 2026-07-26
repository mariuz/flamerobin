/*
  Copyright (c) 2004-2026 FlameRobin Development Team

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

#ifndef VECTORDATAEDITORDIALOG_H
#define VECTORDATAEDITORDIALOG_H

#include <wx/wx.h>
#include <wx/grid.h>
#include <vector>
#include "gui/BaseDialog.h"
#include "core/VectorHelper.h"

class VectorDataEditorDialog : public BaseDialog
{
private:
    wxString columnNameM;
    std::vector<float> vectorValuesM;

    wxTextCtrl* textCtrlVectorTextM;
    wxStaticText* labelDimensionsM;
    wxStaticText* labelNormM;
    wxGrid* gridValuesM;

    wxTextCtrl* textCtrlQueryVectorM;
    wxChoice* choiceMetricM;
    wxStaticText* labelDistanceM;
    wxTextCtrl* textCtrlSimilaritySqlM;

    wxButton* buttonFormatM;
    wxButton* buttonNormalizeM;
    wxButton* buttonGenerateQueryM;
    wxButton* buttonOkM;
    wxButton* buttonCancelM;

    void createControls();
    void layoutControls();
    void updateVectorStats();
    void updateQueryPreview();

    void OnTextChange(wxCommandEvent& event);
    void OnFormatClick(wxCommandEvent& event);
    void OnNormalizeClick(wxCommandEvent& event);
    void OnGenerateQueryClick(wxCommandEvent& event);

protected:
    virtual const wxString getName() const override;

public:
    VectorDataEditorDialog(wxWindow* parent, const wxString& columnName, const wxString& initialVector = "");
    virtual ~VectorDataEditorDialog();

    wxString getVectorText() const;
};

#endif // VECTORDATAEDITORDIALOG_H
