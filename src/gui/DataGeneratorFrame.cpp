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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/ffile.h>
#include <wx/file.h>

#include "metadata/database.h"
#include "DataGeneratorFrame.h"
//-----------------------------------------------------------------------------
DataGeneratorFrame::DataGeneratorFrame(wxWindow* parent, Database* db)
    :BaseFrame(parent,-1, wxT("")), databaseM(db)
{
    SetTitle(_("Test Data Generator"));
    //SetSizeHints( wxDefaultSize, wxDefaultSize );

    wxBoxSizer* outerSizer;
    outerSizer = new wxBoxSizer( wxVERTICAL );

    outerPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL | wxCLIP_CHILDREN | wxNO_FULL_REPAINT_ON_RESIZE);
        //wxTAB_TRAVERSAL );
    wxBoxSizer* innerSizer;
    innerSizer = new wxBoxSizer( wxVERTICAL );

    mainSplitter = new wxSplitterWindow( outerPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D);
    mainSplitter->SetMinimumPaneSize(100);
    mainSplitter->SetSashGravity( 0.5 );

    leftPanel = new wxPanel( mainSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* leftPanelSizer;
    leftPanelSizer = new wxBoxSizer( wxVERTICAL );

    leftLabel = new wxStaticText( leftPanel, wxID_ANY, wxT("Select tables and columns"), wxDefaultPosition, wxDefaultSize, 0 );
    leftPanelSizer->Add( leftLabel, 0, wxALL|wxEXPAND, 5 );

    mainTree = new wxTreeCtrl( leftPanel, wxID_ANY, wxDefaultPosition, wxSize(200,100), wxTR_DEFAULT_STYLE|wxTR_HIDE_ROOT);
    leftPanelSizer->Add( mainTree, 1, wxALL|wxEXPAND, 5 );

    leftPanel->SetSizer( leftPanelSizer );
    leftPanel->Layout();
    leftPanelSizer->Fit( leftPanel );
    rightPanel = new wxPanel( mainSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* rightPanelSizer;
    rightPanelSizer = new wxBoxSizer( wxVERTICAL );

    rightLabel = new wxStaticText( rightPanel, wxID_ANY, wxT("Configure"), wxDefaultPosition, wxDefaultSize, 0 );
    rightPanelSizer->Add( rightLabel, 0, wxALL|wxEXPAND, 5 );

    tableLabel = new wxStaticText( rightPanel, wxID_ANY, wxT("Table: table name"), wxDefaultPosition, wxDefaultSize, 0 );
    tableLabel->SetFont( wxFont( 10, 74, 90, 92, false, wxT("sans") ) );

    rightPanelSizer->Add( tableLabel, 0, wxTOP|wxBOTTOM|wxLEFT, 10 );

    wxBoxSizer* recordsSizer = new wxBoxSizer( wxHORIZONTAL );

    recordsLabel = new wxStaticText( rightPanel, wxID_ANY, wxT("Number of records to create:"), wxDefaultPosition, wxDefaultSize, 0 );
    recordsSizer->Add( recordsLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );

    spinRecords = new wxSpinCtrl( rightPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10, 0);
    recordsSizer->Add( spinRecords, 0, wxRIGHT|wxLEFT, 10 );

    rightPanelSizer->Add( recordsSizer, 0, wxEXPAND, 5 );

    skipCheckbox = new wxCheckBox( rightPanel, wxID_ANY, wxT("Skip this table"), wxDefaultPosition, wxDefaultSize, 0 );

    rightPanelSizer->Add( skipCheckbox, 0, wxBOTTOM|wxRIGHT|wxLEFT, 8 );

    columnLabel = new wxStaticText( rightPanel, wxID_ANY, wxT("Column: column name"), wxDefaultPosition, wxDefaultSize, 0 );
    columnLabel->SetFont( wxFont( 10, 74, 90, 92, false, wxT("sans") ) );

    rightPanelSizer->Add( columnLabel, 0, wxTOP|wxLEFT, 10 );

    valuetypeLabel = new wxStaticText( rightPanel, wxID_ANY, wxT("Value type:"), wxDefaultPosition, wxDefaultSize, 0 );
    rightPanelSizer->Add( valuetypeLabel, 0, wxALL, 10 );

    wxFlexGridSizer* flexSizer = new wxFlexGridSizer( 2, 2, 3, 3 );
    flexSizer->AddGrowableCol( 1 );
    flexSizer->SetFlexibleDirection( wxHORIZONTAL );

    radioSkip = new wxRadioButton( rightPanel, wxID_ANY, wxT("Skip"), wxDefaultPosition, wxDefaultSize, 0);
    flexSizer->Add( radioSkip, 0, wxALIGN_CENTER_VERTICAL, 5 );

    flexSizer->Add( 0, 0, 1, wxALL, 5 );

    radioRange = new wxRadioButton( rightPanel, wxID_ANY, wxT("Range"), wxDefaultPosition, wxDefaultSize, 0);
    flexSizer->Add( radioRange, 0, wxALIGN_CENTER_VERTICAL, 5 );

    rangeText = new wxTextCtrl( rightPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    flexSizer->Add( rangeText, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

    radioColumn = new wxRadioButton( rightPanel, wxID_ANY, wxT("Value from column:"), wxDefaultPosition, wxDefaultSize, 0);
    flexSizer->Add( radioColumn, 0, wxALIGN_CENTER_VERTICAL, 5 );

    wxString valueChoiceChoices[] = {  };
    int valueChoiceNChoices = sizeof( valueChoiceChoices ) / sizeof( wxString );
    valueChoice = new wxChoice( rightPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, valueChoiceNChoices, valueChoiceChoices, 0 );
    flexSizer->Add( valueChoice, 0, wxEXPAND, 5 );

    radioFile = new wxRadioButton( rightPanel, wxID_ANY, wxT("Value from file"), wxDefaultPosition, wxDefaultSize, 0);
    flexSizer->Add( radioFile, 0, wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer* filenameSizer = new wxBoxSizer( wxHORIZONTAL );

    fileText = new wxTextCtrl( rightPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    filenameSizer->Add( fileText, 1, wxALIGN_CENTER_VERTICAL, 5 );

    fileButton = new wxButton( rightPanel, ID_button_file, wxT("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    filenameSizer->Add( fileButton, 0, wxLEFT, 5 );

    flexSizer->Add( filenameSizer, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

    rightPanelSizer->Add( flexSizer, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );

    randomCheckbox = new wxCheckBox( rightPanel, wxID_ANY, wxT("Select random values rather than sequential"), wxDefaultPosition, wxDefaultSize, 0 );
    randomCheckbox->SetValue(true);

    rightPanelSizer->Add( randomCheckbox, 0, wxALL, 8 );

    wxBoxSizer* nullSizer = new wxBoxSizer( wxHORIZONTAL );

    nullLabel = new wxStaticText( rightPanel, wxID_ANY, wxT("Percentage of NULLs:"), wxDefaultPosition, wxDefaultSize, 0 );
    nullSizer->Add( nullLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 10 );

    nullSpin = new wxSpinCtrl( rightPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10, 0);
    nullSizer->Add( nullSpin, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

    nullPercentLabel = new wxStaticText( rightPanel, wxID_ANY, wxT("%"), wxDefaultPosition, wxDefaultSize, 0 );
    nullSizer->Add( nullPercentLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

    rightPanelSizer->Add( nullSizer, 0, wxEXPAND, 5 );

    wxBoxSizer* copySizer;
    copySizer = new wxBoxSizer( wxHORIZONTAL );

    copyLabel = new wxStaticText( rightPanel, wxID_ANY, wxT("Copy settings from"), wxDefaultPosition, wxDefaultSize, 0 );
    copySizer->Add( copyLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 10 );

    wxString copyChoiceChoices[] = {  };
    int copyChoiceNChoices = sizeof( copyChoiceChoices ) / sizeof( wxString );
    copyChoice = new wxChoice( rightPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, copyChoiceNChoices, copyChoiceChoices, 0 );
    copySizer->Add( copyChoice, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

    copyButton = new wxButton( rightPanel, ID_button_copy, wxT("Copy"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    copySizer->Add( copyButton, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

    rightPanelSizer->Add( copySizer, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );

    rightPanel->SetSizer( rightPanelSizer );
    rightPanel->Layout();
    rightPanelSizer->Fit( rightPanel );
    mainSplitter->SplitVertically( leftPanel, rightPanel, 200 );
    innerSizer->Add( mainSplitter, 1, wxALL|wxEXPAND, 10 );

    wxBoxSizer* buttonSizer;
    buttonSizer = new wxBoxSizer( wxHORIZONTAL );

    buttonSizer->Add( 0, 0, 1, wxALL, 5 );

    saveButton = new wxButton( outerPanel, ID_button_save, wxT("Save settings"), wxDefaultPosition, wxDefaultSize, 0 );
    buttonSizer->Add( saveButton, 0, wxALL, 5 );

    loadButton = new wxButton( outerPanel, ID_button_load, wxT("Load settings"), wxDefaultPosition, wxDefaultSize, 0 );
    buttonSizer->Add( loadButton, 0, wxALL, 5 );

    generateButton = new wxButton( outerPanel, ID_button_generate, wxT("Generate data"), wxDefaultPosition, wxDefaultSize, 0 );
    buttonSizer->Add( generateButton, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

    buttonSizer->Add( 10, 0, 0, wxRIGHT, 0 );

    innerSizer->Add( buttonSizer, 0, wxEXPAND|wxALL, 10 );

    outerPanel->SetSizer( innerSizer );
    outerPanel->Layout();
    innerSizer->Fit( outerPanel );
    outerSizer->Add( outerPanel, 1, wxEXPAND, 5 );

    SetSizer( outerSizer );
    Layout();
    outerSizer->Fit(this);
    outerSizer->SetSizeHints(this);
    mainSplitter->UpdateSize();
}
//-----------------------------------------------------------------------------
const wxString DataGeneratorFrame::getName() const
{
    return wxT("DataGeneratorFrame");
}
//-----------------------------------------------------------------------------
const wxRect DataGeneratorFrame::getDefaultRect() const
{
    return wxRect(-1, -1, 700, 500);
}
//-----------------------------------------------------------------------------
//! closes window if database is removed (unregistered)
void DataGeneratorFrame::removeSubject(Subject* subject)
{
    Observer::removeSubject(subject);
    if (subject == databaseM)
        Close();
}
//-----------------------------------------------------------------------------
void DataGeneratorFrame::update()
{
    if (!databaseM->isConnected())
        Close();
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE( DataGeneratorFrame, BaseFrame )
    EVT_BUTTON( ID_button_file, DataGeneratorFrame::OnFileButtonClick )
    EVT_BUTTON( ID_button_copy, DataGeneratorFrame::OnCopyButtonClick )
    EVT_BUTTON( ID_button_save, DataGeneratorFrame::OnSaveButtonClick )
    EVT_BUTTON( ID_button_load, DataGeneratorFrame::OnLoadButtonClick )
    EVT_BUTTON( ID_button_generate, DataGeneratorFrame::OnGenerateButtonClick )
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void DataGeneratorFrame::OnLoadButtonClick(wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog fd(this, _("Select file to load"), wxT(""), wxT(""),
        _("Text files (*.txt)|*.txt|All files (*.*)|*.*"),
#if wxCHECK_VERSION(2, 8, 0)
        wxFD_OPEN | wxFD_CHANGE_DIR);
#else
        wxOPEN | wxCHANGE_DIR);
#endif
    if (wxID_OK != fd.ShowModal())
        return;

    wxFFile f(fd.GetPath());
    if (!f.IsOpened())
    {
        wxMessageBox(_("Cannot open file."), _("Error"), wxOK|wxICON_ERROR);
        return;
    }
    wxBusyCursor wait;
    wxString s;
    f.ReadAll(&s);
    f.Close();

    // TODO: update internal settings structures
}
//-----------------------------------------------------------------------------
void DataGeneratorFrame::OnSaveButtonClick(wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog fd(this, _("Select file to save"), wxT(""), wxT(""),
        _("Text files (*.txt)|*.txt|All files (*.*)|*.*"),
#if wxCHECK_VERSION(2, 8, 0)
        wxFD_SAVE | wxFD_CHANGE_DIR | wxFD_OVERWRITE_PROMPT);
#else
        wxSAVE |wxCHANGE_DIR | wxOVERWRITE_PROMPT);
#endif

    if (wxID_OK != fd.ShowModal())
        return;

    wxBusyCursor wait;

    // TODO: save internal settings structures
    /*

    wxFile f;
    if (!f.Open(fd.GetPath(), wxFile::write) || !f.Write(s))
    {
        wxMessageBox(_("Cannot write to file."), _("Error"), wxOK|wxICON_ERROR);
        return;
    }

    if (f.IsOpened())
        f.Close();
    */
}
//-----------------------------------------------------------------------------
void DataGeneratorFrame::OnCopyButtonClick(wxCommandEvent& WXUNUSED(event))
{
}
//-----------------------------------------------------------------------------
void DataGeneratorFrame::OnFileButtonClick(wxCommandEvent& WXUNUSED(event))
{
}
//-----------------------------------------------------------------------------
void DataGeneratorFrame::OnGenerateButtonClick(wxCommandEvent& WXUNUSED(event))
{
}
//-----------------------------------------------------------------------------
