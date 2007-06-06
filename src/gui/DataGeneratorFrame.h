/*
Copyright (c) 2007 The FlameRobin Development Team

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


  $Id$

*/
#ifndef FR_DATAGENERATORFRAME_H
#define FR_DATAGENERATORFRAME_H
//-----------------------------------------------------------------------------
#include <wx/wx.h>
#include <wx/treectrl.h>
#include <wx/panel.h>
#include <wx/spinctrl.h>
#include <wx/radiobut.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/splitter.h>

#include "core/Observer.h"
#include "gui/BaseFrame.h"

class Database;
//-----------------------------------------------------------------------------
class DataGeneratorFrame: public BaseFrame, public Observer
{
    DECLARE_EVENT_TABLE()
protected:
    Database* databaseM;
    virtual const wxString getName() const;
    virtual const wxRect getDefaultRect() const;

    enum
    {
        ID_button_file = 1000,
        ID_button_save,
        ID_button_load,
        ID_button_generate,
        ID_button_copy
    };

    wxPanel* outerPanel;
    wxSplitterWindow* mainSplitter;
    wxPanel* leftPanel;
    wxStaticText* leftLabel;
    wxTreeCtrl* mainTree;
    wxPanel* rightPanel;
    wxStaticText* rightLabel;
    wxStaticText* tableLabel;
    wxStaticText* recordsLabel;
    wxSpinCtrl* spinRecords;
    wxCheckBox* skipCheckbox;
    wxStaticText* columnLabel;
    wxStaticText* valuetypeLabel;
    wxRadioButton* radioSkip;
    wxRadioButton* radioRange;
    wxTextCtrl* rangeText;
    wxRadioButton* radioColumn;
    wxChoice* valueChoice;
    wxRadioButton* radioFile;
    wxTextCtrl* fileText;
    wxButton* fileButton;
    wxCheckBox* randomCheckbox;
    wxStaticText* nullLabel;
    wxSpinCtrl* nullSpin;
    wxStaticText* nullPercentLabel;
    wxStaticText* copyLabel;
    wxChoice* copyChoice;
    wxButton* copyButton;
    wxButton* saveButton;
    wxButton* loadButton;
    wxButton* generateButton;

    void OnFileButtonClick(wxCommandEvent& event);
    void OnCopyButtonClick(wxCommandEvent& event);
    void OnSaveButtonClick(wxCommandEvent& event);
    void OnLoadButtonClick(wxCommandEvent& event);
    void OnGenerateButtonClick(wxCommandEvent& event);
public:
    DataGeneratorFrame(wxWindow* parent, Database* db);
    void removeSubject(Subject* subject);
    void update();
};
//-----------------------------------------------------------------------------
#endif
