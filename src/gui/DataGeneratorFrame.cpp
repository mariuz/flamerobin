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

#include <wx/arrstr.h>
#include <wx/ffile.h>
#include <wx/file.h>

#include "core/FRError.h"
#include "myTreeCtrl.h"
#include "treeitem.h"
#include "gui/ProgressDialog.h"
#include "metadata/database.h"
#include "DataGeneratorFrame.h"
//-----------------------------------------------------------------------------
class GeneratorSettings
{
public:
    typedef enum { vtSkip, vtRange, vtColumn, vtFile } ValueType;
    ValueType valueType;
    wxString range;
    wxString sourceColumn;
    wxString fileName;
    bool randomValues;
    int nullPercent;
};
//-----------------------------------------------------------------------------
void loadColumns(Database *db, wxArrayString& as, ProgressDialog& pd)
{
    MetadataCollection<Table>* t = db->getCollection<Table>();
    pd.initProgress(_("Loading tables"), t->getChildrenCount());

    // load list of tables and columns from database
    for (MetadataCollection<Table>::iterator it = t->begin();
        it != t->end(); ++it)
    {
        pd.setProgressMessage((*it).getName_());
        pd.stepProgress();
        if (pd.isCanceled())
            return;

        (*it).checkAndLoadColumns();
        for (MetadataCollection<Column>::iterator i2 = (*it).begin();
            i2 != (*it).end(); ++i2)
        {
            as.Add((*it).getQuotedName() + wxT(".") + (*i2).getQuotedName());
        }
    }
    as.Sort();
    // prepend
    as.Insert(wxT("[none]"), 0);
}
//-----------------------------------------------------------------------------
DataGeneratorFrame::DataGeneratorFrame(wxWindow* parent, Database* db)
    :BaseFrame(parent,-1, wxT("")), databaseM(db), loadingM(true)
{
    #include "procedure32.xpm"
    wxBitmap bmp(procedure_xpm);
    wxIcon icon;
    icon.CopyFromBitmap(bmp);
    SetIcon(icon);

    ProgressDialog pd(this, _("Loading table columns..."), 1);
    pd.Show();
    wxArrayString columns;
    loadColumns(db, columns, pd);


    SetTitle(_("Test Data Generator"));
    // just in case some tree event escapes us
    // TODO: we need proper event handling for tree to allow multiple
    //       tree controls that are completely functional
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);

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

    mainTree = new myTreeCtrl(leftPanel, wxDefaultPosition, wxDefaultSize,
#if defined __WXGTK20__ || defined __WXMAC__
        wxTR_NO_LINES |
#endif
        wxTR_HAS_BUTTONS | wxSUNKEN_BORDER);
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

    spinRecords = new wxSpinCtrl( rightPanel, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxSP_ARROW_KEYS, 0, 100000, 0);
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

    valueChoice = new wxChoice( rightPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        columns );
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

    nullSpin = new wxSpinCtrl( rightPanel, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 0);
    nullSizer->Add( nullSpin, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

    nullPercentLabel = new wxStaticText( rightPanel, wxID_ANY, wxT("%"), wxDefaultPosition, wxDefaultSize, 0 );
    nullSizer->Add( nullPercentLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

    rightPanelSizer->Add( nullSizer, 0, wxEXPAND, 5 );

    wxBoxSizer* copySizer;
    copySizer = new wxBoxSizer( wxHORIZONTAL );

    copyLabel = new wxStaticText( rightPanel, wxID_ANY, wxT("Copy settings from"), wxDefaultPosition, wxDefaultSize, 0 );
    copySizer->Add( copyLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 10 );

    copyChoice = new wxChoice( rightPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        columns, 0 );
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

    mainTree->allowContextMenu(false);
    TreeItem* rootdata = new TreeItem(mainTree);
    wxTreeItemId root = mainTree->AddRoot(_("Tables"),
        mainTree->getItemImage(ntTables), -1, rootdata);
    db->getCollection<Table>()->attachObserver(rootdata);
    rootdata->update();
    mainTree->Expand(root);

    // we need to manually update each table (that's bad)
    wxTreeItemIdValue cookie;
    wxTreeItemId node = mainTree->GetFirstChild(root, cookie);
    if (node.IsOk())
        mainTree->SelectItem(node);
    for (; node.IsOk(); node = mainTree->GetNextChild(root, cookie))
    {
        TreeItem *d = (TreeItem *)(mainTree->GetItemData(node));
        if (d)
            d->update();
    }

    loadingM = false;
}
//-----------------------------------------------------------------------------
DataGeneratorFrame::~DataGeneratorFrame()
{
    for (std::map<wxString, GeneratorSettings *>::iterator it =
        settingsM.begin(); it!= settingsM.end(); ++it)
    {
        delete (*it).second;
    }
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
    EVT_TREE_SEL_CHANGED(myTreeCtrl::ID_tree_ctrl, DataGeneratorFrame::OnTreeSelectionChanged)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
// prehaps using values from config() would be nice
wxString getDefaultRange(Domain *d)
{
    wxString dt, size, scale;
    d->getDatatypeParts(dt, size, scale);

    if (dt == wxT("Smallint") || dt == wxT("Float"))
        return wxT("0-100");

    if (   dt == wxT("Numeric") || dt == wxT("Integer")
        || dt == wxT("Decimal") || dt == wxT("Double precision"))
    {
        return wxT("0-2000000");
    }

    if (dt == wxT("Char") || dt == wxT("Varchar"))
    {
        long l;
        size.ToLong(&l);
        return wxString(wxChar('N'), l);
    }

    if (dt == wxT("Timestamp"))
        return wxT("01.01.1980 00:00:00-31.12.2005 23:59:59");

    if (dt == wxT("Date"))
        return wxT("01.01.1980-31.12.2005");

    if (dt == wxT("Time"))
        return wxT("00:00:00-23:59:59");

    return wxEmptyString;
}
//-----------------------------------------------------------------------------
GeneratorSettings* DataGeneratorFrame::getSettings(Column *c)
{
    Table *tab = c->getTable();
    if (!tab)
        throw FRError(_("Table not set"));
    wxString s = tab->getQuotedName() + wxT(".") + c->getQuotedName();
    std::map<wxString, GeneratorSettings *>::iterator it = settingsM.find(s);
    if (it != settingsM.end())    // not found
        return (*it).second;

    // check FK info
    wxString fkc;
    std::vector<ForeignKey> *fk = tab->getForeignKeys();
    for (std::vector<ForeignKey>::iterator fi = fk->begin(); fi != fk->end();
        ++fi)
    {
        int cnt = 0;
        for (std::vector<wxString>::const_iterator ci = (*fi).begin();
            ci != (*fi).end(); ++ci, ++cnt)
        {
            Identifier id(*ci);
            if (id.getQuoted() == c->getQuotedName())
            {
                Identifier table((*fi).referencedTableM);
                Identifier column((*fi).referencedColumnsM[cnt]);
                fkc = table.getQuoted() + wxT(".") + column.getQuoted();
                break;
            }
        }
        if (!fkc.IsEmpty())
            break;
    }

    // check primary/unique
    bool isUnique = false;
    PrimaryKeyConstraint *pk = tab->getPrimaryKey();
    if (pk)
    {
        for (std::vector<wxString>::const_iterator ci = pk->begin();
            ci != pk->end(); ++ci)
        {
            Identifier id(*ci);
            if (id.getQuoted() == c->getQuotedName())
            {
                isUnique = true;
                break;
            }
        }
    }
    if (!isUnique)
    {
        std::vector<UniqueConstraint> *uq = tab->getUniqueConstraints();
        for (std::vector<UniqueConstraint>::iterator ui = uq->begin();
            !isUnique && ui != uq->end(); ++ui)
        {
            for (std::vector<wxString>::const_iterator ci = (*ui).begin();
                ci != (*ui).end(); ++ci)
            {
                Identifier id(*ci);
                if (id.getQuoted() == c->getQuotedName())
                {
                    isUnique = true;
                    break;
                }
            }
        }
    }

    // It would be cool if we could detect simple check constraints:
    // value in (1, 2, 3, 4)
    // value between 1 and 4
    // value < 5
    // value = 1 or value = 2 or value = 3 or value = 4

    GeneratorSettings *gs = new GeneratorSettings;
    settingsM.insert(std::pair<wxString, GeneratorSettings*>(s, gs));
    gs->randomValues = !isUnique;
    gs->sourceColumn = wxT("[none]");
    gs->nullPercent = (c->isNullable() ? 50 : 0);
    gs->valueType = GeneratorSettings::vtRange;
    gs->range = getDefaultRange(c->getDomain());
    if (!c->getComputedSource().IsEmpty())
        gs->valueType = GeneratorSettings::vtSkip;
    else if (!fkc.IsEmpty())
    {
        gs->valueType = GeneratorSettings::vtColumn;
        gs->sourceColumn = fkc;
    }
    return gs;
}
//-----------------------------------------------------------------------------
void DataGeneratorFrame::showColumnSettings(bool show)
{
    if (loadingM)
        return;
    //rangeText->Enable(show);
    //fileText->Enable(show);
    // ... etc.

    wxWindow *ww[] = {
        columnLabel, valuetypeLabel, radioSkip,     radioRange,
        rangeText,   radioColumn,    valueChoice,   radioFile,
        fileText,    fileButton,     nullLabel,     randomCheckbox,
        nullSpin,    copyLabel,      copyChoice,    nullPercentLabel,
        copyButton };
    for (int i = 0; i < sizeof(ww)/sizeof(wxWindow *); ++i)
        if (ww[i])
            ww[i]->Show(show);
}
//-----------------------------------------------------------------------------
void DataGeneratorFrame::OnTreeSelectionChanged(wxTreeEvent& event)
{
    if (loadingM)
        return;

    FR_TRY

    wxTreeItemId olditem = event.GetOldItem();
    if (olditem.IsOk())
    {
        // save previous settings
        MetadataItem *m = mainTree->getMetadataItem(olditem);
        Table *tab = dynamic_cast<Table *>(m);
        if (tab)
        {
            int rec = spinRecords->GetValue();
            wxString tname = tab->getQuotedName();
            std::map<wxString, int>::iterator i1 =
                tableRecordsM.find(tname);
            if (i1 == tableRecordsM.end())
                tableRecordsM.insert(std::pair<wxString,int>(tname, rec));
            else
                (*i1).second = rec;
        }
        Column *col = dynamic_cast<Column *>(m);
        if (col)
        {
            GeneratorSettings *gs = getSettings(col);
            gs->range = rangeText->GetValue();
            gs->fileName = fileText->GetValue();
            gs->sourceColumn = valueChoice->GetStringSelection();
            wxRadioButton *btns[4] = { radioSkip, radioRange, radioColumn,
                radioFile };
            for (int i=0; i<4; ++i)
                if (btns[i]->GetValue())
                    gs->valueType = (GeneratorSettings::ValueType)i;
            gs->randomValues = randomCheckbox->IsChecked();
            gs->nullPercent = nullSpin->GetValue();
        }
    }

    wxTreeItemId newitem = event.GetItem();
    if (!newitem.IsOk())
        return;

    MetadataItem *m = mainTree->getMetadataItem(newitem);
    Table *tab = dynamic_cast<Table *>(m);
    Column *col = dynamic_cast<Column *>(m);
    showColumnSettings(col != 0);
    if (!tab && col)
        tab = col->getTable();
    if (!tab)
        return;

    wxString tablename = tab->getQuotedName();
    tableLabel->SetLabel(wxT("Table: ") + tab->getName_());
    int records = 200; // TODO: load default from config
    std::map<wxString, int>::iterator i1 = tableRecordsM.find(tablename);
    if (i1 != tableRecordsM.end())
        records = (*i1).second;
    spinRecords->SetValue(records);
    skipCheckbox->SetValue(records > 0);

    if (!col)
        return;
    columnLabel->SetLabel(wxT("Column: ") + col->getName_());

    // copy settings from gs to controls
    GeneratorSettings *gs = getSettings(col);
    rangeText->SetValue(gs->range);
    fileText->SetValue(gs->fileName);
    valueChoice->SetStringSelection(gs->sourceColumn);
    switch (gs->valueType)
    {
        case GeneratorSettings::vtSkip:   radioSkip->SetValue(true);   break;
        case GeneratorSettings::vtRange:  radioRange->SetValue(true);  break;
        case GeneratorSettings::vtColumn: radioColumn->SetValue(true); break;
        case GeneratorSettings::vtFile:   radioFile->SetValue(true);   break;
    }
    randomCheckbox->SetValue(gs->randomValues);
    nullSpin->SetValue(gs->nullPercent);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void DataGeneratorFrame::OnLoadButtonClick(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

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

    FR_CATCH
}
//-----------------------------------------------------------------------------
void DataGeneratorFrame::OnSaveButtonClick(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

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

    FR_CATCH
}
//-----------------------------------------------------------------------------
void DataGeneratorFrame::OnCopyButtonClick(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY


    FR_CATCH
}
//-----------------------------------------------------------------------------
void DataGeneratorFrame::OnFileButtonClick(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY


    FR_CATCH
}
//-----------------------------------------------------------------------------
void DataGeneratorFrame::OnGenerateButtonClick(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY


    FR_CATCH
}
//-----------------------------------------------------------------------------
