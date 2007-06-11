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

// needed for random
#include <stdlib.h>

#include "core/FRError.h"
#include "core/StringUtils.h"
#include "myTreeCtrl.h"
#include "treeitem.h"
#include "gui/AdvancedMessageDialog.h"
#include "gui/ProgressDialog.h"
#include "metadata/database.h"
#include "DataGeneratorFrame.h"
//-----------------------------------------------------------------------------
// returns a value between 0 and (maxval-1)
// I wrote this as I don't know how much is rand from stdlib portable
int frRandom(double maxval)
{
    return (int)(maxval*rand()/(RAND_MAX+1.0));
}
//-----------------------------------------------------------------------------
// only used in function OnGenerateButtonClick
class TableDep
{
public:
    TableDep(Table *t, std::map<wxString, int>& needs)
    {
        table = t;
        std::vector<ForeignKey> *fk = t->getForeignKeys();
        for (std::vector<ForeignKey>::iterator fi = fk->begin();
            fi != fk->end(); ++fi)
        {
            Identifier id((*fi).referencedTableM);
            if (id.getQuoted() == t->getQuotedName())   // self reference
                continue;
            std::map<wxString, int>::iterator it =
                needs.find(id.getQuoted());
            if (it != needs.end() && (*it).second > 0)
                dependsOn.push_back(id.getQuoted());
        }
    }
    void remove(const wxString& table)
    {
        std::list<wxString>::iterator it = std::find(dependsOn.begin(),
            dependsOn.end(), table);
        if (it != dependsOn.end())
            dependsOn.erase(it);
    }

    Table *table;
    std::list<wxString> dependsOn;
};
//-----------------------------------------------------------------------------
class GeneratorSettings
{
public:
    typedef enum { vtSkip, vtRange, vtColumn, vtFile } ValueType;
    ValueType valueType;
    wxString range;
    wxString sourceTable;
    wxString sourceColumn;
    wxString fileName;
    bool randomValues;
    int nullPercent;

    wxString toString();    // returns the settings as a single-line string
    void fromString(const wxString& s); // loads settings from a string
};
//-----------------------------------------------------------------------------
wxString GeneratorSettings::toString()
{
    wxString s;
    s += wxString::Format(wxT("%d%d%d|"),
        (int)valueType,
        (randomValues ? 1 : 0),
        nullPercent);
    s += range + wxT("|");
    s += sourceTable + wxT("|");
    s += sourceColumn + wxT("|");
    s += fileName;
    return s;
}
//-----------------------------------------------------------------------------
void GeneratorSettings::fromString(const wxString& s)
{
    long l;
    if (!s.Mid(0, 1).ToLong(&l))
        throw FRError(_("Bad input for field: valueType"));
    valueType = (ValueType)l;

    if (!s.Mid(1, 1).ToLong(&l))
        throw FRError(_("Bad input from field: randomValues"));
    randomValues = (l == 1);

    size_t p = s.find(wxT("|"));
    if (p == wxString::npos || !s.Mid(2, p-2).ToLong(&l))
        throw FRError(_("Bad input for field: nullPercent"));
    nullPercent = l;

    size_t q = s.find(wxT("|"), p+1);
    if (q == wxString::npos)
        throw FRError(_("Bad input for field: range"));
    range = s.Mid(p+1, q-p-1);

    p = s.find(wxT("|"), q+1);
    if (p ==  wxString::npos)
        throw FRError(_("Bad input for field: sourceTable"));
    sourceTable = s.Mid(q+1, p-q-1);

    q=p;
    p = s.find(wxT("|"), q+1);
    if (p ==  wxString::npos)
        throw FRError(_("Bad input for field: sourceColumn"));
    sourceColumn = s.Mid(q+1, p-q-1);

    q = s.find(wxT("|"), q+1);
    if (q ==  wxString::npos)
        throw FRError(_("Bad input for field: fileName"));
    fileName = s.Mid(p+1, q-p-1);
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

    dbCharsetConversionM.setConnectionCharset(db->getConnectionCharset());

    SetTitle(_("Test Data Generator"));
    // prevent tree events from reaching the main frame
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
        wxTR_HAS_BUTTONS | wxSUNKEN_BORDER | wxTR_HIDE_ROOT );
    leftPanelSizer->Add( mainTree, 1, wxALL|wxEXPAND, 5 );

    leftPanel->SetSizer( leftPanelSizer );
    leftPanel->Layout();
    leftPanelSizer->Fit( leftPanel );
    rightPanel = new wxPanel( mainSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
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

    skipCheckbox = new wxCheckBox( rightPanel, ID_checkbox_skip, wxT("Skip this table"), wxDefaultPosition, wxDefaultSize, 0 );

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

    radioRange = new wxRadioButton( rightPanel, wxID_ANY, wxT("Range/mask:"), wxDefaultPosition, wxDefaultSize, 0);
    flexSizer->Add( radioRange, 0, wxALIGN_CENTER_VERTICAL, 5 );

    rangeText = new wxTextCtrl( rightPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    flexSizer->Add( rangeText, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

    radioColumn = new wxRadioButton( rightPanel, wxID_ANY, wxT("Value from column:"), wxDefaultPosition, wxDefaultSize, 0);
    flexSizer->Add( radioColumn, 0, wxALIGN_CENTER_VERTICAL, 5 );

    wxArrayString tables;
    MetadataCollection<Table>* t = db->getCollection<Table>();
    for (MetadataCollection<Table>::iterator it = t->begin();
        it != t->end(); ++it)
    {
        tables.Add((*it).getQuotedName());
    }
    tables.Sort();
    // prepend
    wxArrayString empty;

    valueSizer = new wxBoxSizer( wxHORIZONTAL );
    valueChoice = new wxChoice( rightPanel, ID_choice_value, wxDefaultPosition, wxDefaultSize, tables);
    valueSizer->Add( valueChoice, 1, wxALIGN_CENTER_VERTICAL, 5 );
    valueColumnChoice = new wxChoice( rightPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, empty);
    valueSizer->Add( valueColumnChoice, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT, 5 );
    flexSizer->Add( valueSizer, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

    radioFile = new wxRadioButton( rightPanel, wxID_ANY, wxT("Value from file:"), wxDefaultPosition, wxDefaultSize, 0);
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

    copySizer = new wxBoxSizer( wxHORIZONTAL );

    copyLabel = new wxStaticText( rightPanel, wxID_ANY, wxT("Copy settings from:"), wxDefaultPosition, wxDefaultSize, 0 );
    copySizer->Add( copyLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 10 );

    copyChoice = new wxChoice( rightPanel, ID_choice_copy, wxDefaultPosition, wxDefaultSize, tables, 0 );
    copySizer->Add( copyChoice, 1, wxALIGN_CENTER_VERTICAL, 5 );
    copyColumnChoice = new wxChoice( rightPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, empty, 0 );
    copySizer->Add( copyColumnChoice, 1, wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

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

    loadingM = false;

    // we need to manually update each table (that's bad)
    wxTreeItemIdValue cookie;
    wxTreeItemId node = mainTree->GetFirstChild(root, cookie);
    if (node.IsOk())
        mainTree->SelectItem(node);
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
bool DataGeneratorFrame::loadColumns(const wxString& tableName, wxChoice* c)
{
    Identifier id;
    id.setFromSql(tableName);
    Table *t = dynamic_cast<Table *>(databaseM->findRelation(id));
    if (!t)
        return false;
    t->checkAndLoadColumns();
    c->Clear();
    for (MetadataCollection<Column>::iterator it = t->begin();
        it != t->end(); ++it)
    {
        c->Append((*it).getQuotedName());
    }
    return true;
}
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
        if (size == wxT("1"))
            return wxT("[az,AZ]");
        else
            return size + wxT("[az,AZ]");
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
    wxString fkt, fkc;
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
                fkt = table.getQuoted();
                fkc = column.getQuoted();
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
    gs->nullPercent = (c->isNullable() ? 50 : 0);
    gs->valueType = GeneratorSettings::vtRange;
    gs->range = getDefaultRange(c->getDomain());
    if (!c->getComputedSource().IsEmpty())
        gs->valueType = GeneratorSettings::vtSkip;
    else if (!fkc.IsEmpty())
    {
        gs->valueType = GeneratorSettings::vtColumn;
        gs->sourceTable = fkt;
        gs->sourceColumn = fkc;
    }
    return gs;
}
//-----------------------------------------------------------------------------
void DataGeneratorFrame::showColumnSettings(bool show)
{
    if (loadingM)
        return;

    wxWindow *ww[] = {
        columnLabel, valuetypeLabel,    radioSkip,     radioRange,
        rangeText,   radioColumn,       valueChoice,   radioFile,
        fileText,    fileButton,        nullLabel,     randomCheckbox,
        nullSpin,    copyLabel,         copyChoice,    nullPercentLabel,
        copyButton,  valueColumnChoice, copyColumnChoice };
    for (int i = 0; i < sizeof(ww)/sizeof(wxWindow *); ++i)
        if (ww[i])
            ww[i]->Show(show);
    rightPanelSizer->Layout();
}
//-----------------------------------------------------------------------------
void DataGeneratorFrame::saveSetting(wxTreeItemId item)
{
    if (!item.IsOk())
        return;

    // save previous settings
    MetadataItem *m = mainTree->getMetadataItem(item);
    Table *tab = dynamic_cast<Table *>(m);
    Column *col = dynamic_cast<Column *>(m);
    if (!tab && col)
        tab = col->getTable();
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
    if (col)
    {
        GeneratorSettings *gs = getSettings(col);
        gs->range = rangeText->GetValue();
        gs->fileName = fileText->GetValue();
        gs->sourceTable = valueChoice->GetStringSelection();
        gs->sourceColumn = valueColumnChoice->GetStringSelection();
        wxRadioButton *btns[4] = { radioSkip, radioRange, radioColumn,
            radioFile };
        for (int i=0; i<4; ++i)
            if (btns[i]->GetValue())
                gs->valueType = (GeneratorSettings::ValueType)i;
        gs->randomValues = randomCheckbox->IsChecked();
        gs->nullPercent = nullSpin->GetValue();
    }
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE( DataGeneratorFrame, BaseFrame )
    EVT_BUTTON( ID_button_file, DataGeneratorFrame::OnFileButtonClick )
    EVT_BUTTON( ID_button_copy, DataGeneratorFrame::OnCopyButtonClick )
    EVT_BUTTON( ID_button_save, DataGeneratorFrame::OnSaveButtonClick )
    EVT_BUTTON( ID_button_load, DataGeneratorFrame::OnLoadButtonClick )
    EVT_BUTTON( ID_button_generate, DataGeneratorFrame::OnGenerateButtonClick )
    EVT_CHECKBOX(ID_checkbox_skip, DataGeneratorFrame::OnSkipCheckboxClick)
    EVT_CHOICE(ID_choice_value, DataGeneratorFrame::OnTableValueChoiceChange)
    EVT_CHOICE(ID_choice_copy, DataGeneratorFrame::OnTableCopyChoiceChange)
    EVT_TREE_SEL_CHANGED(myTreeCtrl::ID_tree_ctrl, DataGeneratorFrame::OnTreeSelectionChanged)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void DataGeneratorFrame::OnTableValueChoiceChange(wxCommandEvent& event)
{
    FR_TRY

    if (loadColumns(event.GetString(), valueColumnChoice))
    {
        valueColumnChoice->SetSelection(0);
        valueSizer->Layout();
        radioColumn->SetValue(true);
    }

    FR_CATCH
}
//-----------------------------------------------------------------------------
void DataGeneratorFrame::OnTableCopyChoiceChange(wxCommandEvent& event)
{
    FR_TRY

    if (loadColumns(event.GetString(), copyColumnChoice))
    {
        copyColumnChoice->SetSelection(0);
        copySizer->Layout();
    }

    FR_CATCH
}
//-----------------------------------------------------------------------------
void DataGeneratorFrame::OnSkipCheckboxClick(wxCommandEvent& event)
{
    FR_TRY

    if (event.IsChecked())
        spinRecords->SetValue(0);
    else
        spinRecords->SetValue(10); // TODO: load default from config

    FR_CATCH
}
//-----------------------------------------------------------------------------
void DataGeneratorFrame::OnTreeSelectionChanged(wxTreeEvent& event)
{
    if (loadingM)
        return;

    FR_TRY

    saveSetting(event.GetOldItem());

    wxTreeItemId newitem = event.GetItem();
    if (!newitem.IsOk())
        return;

    MetadataItem *m = mainTree->getMetadataItem(newitem);
    Table *tab = dynamic_cast<Table *>(m);
    Column *col = dynamic_cast<Column *>(m);
    showColumnSettings(col != 0);
    if (tab)
    {
        tab->checkAndLoadColumns();
        TreeItem *d = (TreeItem *)(mainTree->GetItemData(newitem));
        d->update();
    }
    if (!tab && col)
        tab = col->getTable();
    if (!tab)
        return;

    wxString tablename = tab->getQuotedName();
    tableLabel->SetLabel(wxT("Table: ") + tab->getName_());
    int records = 0;    // tables are not filled by default
    std::map<wxString, int>::iterator i1 = tableRecordsM.find(tablename);
    if (i1 != tableRecordsM.end())
        records = (*i1).second;
    spinRecords->SetValue(records);
    skipCheckbox->SetValue(records <= 0);

    if (!col)
        return;
    columnLabel->SetLabel(wxT("Column: ") + col->getName_());

    // copy settings from gs to controls
    GeneratorSettings *gs = getSettings(col);
    rangeText->SetValue(gs->range);
    fileText->SetValue(gs->fileName);
    if (!gs->sourceTable.IsEmpty())
    {
        valueChoice->SetStringSelection(gs->sourceTable);
        if (loadColumns(gs->sourceTable, valueColumnChoice))
            valueColumnChoice->SetStringSelection(gs->sourceColumn);
    }
    else
    {
        valueChoice->SetSelection(wxNOT_FOUND);
        valueColumnChoice->SetSelection(wxNOT_FOUND);
    }
    valueSizer->Layout();

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
    wxString s;
    if (!f.IsOpened() || !f.ReadAll(&s))
    {
        wxMessageBox(_("Cannot open file."), _("Error"), wxOK|wxICON_ERROR);
        return;
    }
    wxBusyCursor wait;

    // parse line by line
    size_t start = 0;
    size_t p;
    wxString name, definition;
    for (bool first = true; true; start = p+1)
    {
        p = s.find(wxT("\n"), start);
        if (p == wxString::npos)
            break;

        if (first)
        {
            name = s.Mid(start, p-start);
            if (name.Trim().IsEmpty())
                break;
            first = false;
            continue;
        }

        definition = s.Mid(start, p-start);
        GeneratorSettings *gs = new GeneratorSettings;
        gs->fromString(definition);
        settingsM.insert(std::pair<wxString, GeneratorSettings *>(name, gs));
        first = true;
    }

    f.Close();

    showInformationDialog(this, _("Settings loaded"),
        _("The setting where successfully loaded from file."),
        AdvancedMessageDialogButtonsOk());

    FR_CATCH
}
//-----------------------------------------------------------------------------
void DataGeneratorFrame::OnSaveButtonClick(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    saveSetting(mainTree->GetSelection());  // save current item if changed

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

    // save internal settings structures
    wxFile f;
    if (!f.Open(fd.GetPath(), wxFile::write))
    {
        wxMessageBox(_("Cannot write to file."), _("Error"), wxOK|wxICON_ERROR);
        return;
    }

    for (std::map<wxString, GeneratorSettings *>::iterator it =
        settingsM.begin(); it!= settingsM.end(); ++it)
    {
        f.Write((*it).first + wxT("\n") + (*it).second->toString() + wxT("\n"));
    }

    if (f.IsOpened())
        f.Close();

    showInformationDialog(this, _("Settings saved"),
        _("The setting where successfully saved to the file."),
        AdvancedMessageDialogButtonsOk());

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

    wxFileDialog fd(this, _("Select file to load"), wxT(""), wxT(""),
        _("Text files (*.txt)|*.txt|All files (*.*)|*.*"),
#if wxCHECK_VERSION(2, 8, 0)
        wxFD_OPEN | wxFD_CHANGE_DIR);
#else
        wxOPEN | wxCHANGE_DIR);
#endif
    if (wxID_OK != fd.ShowModal())
        return;

    fileText->SetValue(fd.GetPath());
    radioFile->SetValue(true);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void DataGeneratorFrame::OnGenerateButtonClick(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    saveSetting(mainTree->GetSelection());  // save current item if changed

    std::list<TableDep *> deps;
    std::list<Table *> order;

    if (sortTables(deps, order))
        generateData(order);

    // perhaps add a comment like: "A total of XYZ records were inserted."
    showInformationDialog(this, _("Generator done"),
        _("Data generation completed."),
        AdvancedMessageDialogButtonsOk());

    FR_CATCH
}
//-----------------------------------------------------------------------------
bool DataGeneratorFrame::sortTables(std::list<TableDep *>& deps,
    std::list<Table *>& order)
{
    // collect list of tables
    // if some table is dropped from the database, it would be
    // removed from tree, but it will remain in tableRecordsM
    // That's why we just search for existing tables that are
    // also present in tableRecordsM
    MetadataCollection<Table>* t = databaseM->getCollection<Table>();
    for (MetadataCollection<Table>::iterator it = t->begin();
        it != t->end(); ++it)
    {
        std::map<wxString, int>::iterator i2 =
            tableRecordsM.find((*it).getQuotedName());
        if (i2 != tableRecordsM.end() && (*i2).second > 0)
        {
            TableDep *td = new TableDep(&(*it), tableRecordsM);
            deps.push_back(td);
        }
    }

    // Topological sorting:
    // take out independent tables one by one and remove them from
    // dependency lists of those depending on them
    while (!deps.empty())
    {
        bool removed = false;
        for (std::list<TableDep *>::iterator it = deps.begin();
            it != deps.end(); ++it)
        {
            if ((*it)->dependsOn.size() != 0)   // has dependencies
                continue;
            order.push_back((*it)->table);
            wxString tablename = (*it)->table->getQuotedName();
            for (std::list<TableDep *>::iterator i2 = deps.begin();
                i2 != deps.end(); ++i2)
            {
                (*i2)->remove(tablename);
            }

            delete (*it);
            deps.erase(it);
            removed = true;
            break;
        }

        if (!removed)
        {
            showWarningDialog(this, _("Circular dependency"),
                _("A circular dependency was detected among your tables. We are unable to determine to correct order of tables for insert."),
                AdvancedMessageDialogButtonsOk());

            // release memory
            for (std::list<TableDep *>::iterator it = deps.begin();
                it != deps.end(); ++it)
            {
                delete (*it);
            }
            return false;
        }
    }
    return true;
}
//-----------------------------------------------------------------------------
// range = comma separated list of values or ranges
wxString getCharFromRange(const wxString& range, bool rnd, int recNo,
    int charNo, int chars)
{
    wxString valueset;
    size_t start = 0;
    while (start < range.Length())
    {
        // last
        wxString one = range.Mid(start);
        size_t p = range.find(wxT(","), start);
        if (p != wxString::npos)
        {
            one = range.Mid(start, p-start);
            start = p + 1;
        }
        else
            start = range.Length(); // exit on next loop
        if (one.Length() == 1)
            valueset += one;
        else if (one.Length() == 2)     // range
        {
            for (wxChar c = one[0]; c <= one[1]; c++)
                valueset += c;
        }
        else
            throw FRError(_("Bad range: section length not 1 or 2: ") + one);
    }

    if (rnd)
        return valueset.Mid(frRandom(valueset.Length()), 1);

    // sequential: we support stuff like 001,002,003 or AAA,AAB,AAC
    //             by converting the record counter to number with n-th base
    //             where n is a number of characters in valueset
    int base = valueset.Length();
    int record = recNo;
    for (int i=0; i<chars-charNo-1; i++)    // get the charNo-th digit
        record /= base;
    return valueset.Mid(record % base, 1);
}
//-----------------------------------------------------------------------------
// format for values:
// number[value or range(s)]
// example: 25[az,AZ,09] means: 25 letters or numbers
// example: 10[a,x,5]       means: 10 chars, each either of 'a', 'x' or '5'
void DataGeneratorFrame::setString(IBPP::Statement st, int param,
    GeneratorSettings* gs, int recNo)
{
    if (gs->nullPercent > frRandom(100))
    {
        st->SetNull(param);
        return;
    }

    // switch on value type and do accordingly
    if (gs->valueType == GeneratorSettings::vtRange)
    {
        wxString value;
        long chars = 1;
        int start = 0;
        while (start < gs->range.Length())
        {
            if (gs->range.Mid(start, 1) == wxT("["))
            {
                int p = gs->range.find(wxT("]"), start+1);
                if (p == wxString::npos)    // invalid mask
                    throw FRError(_("Invalid mask: missing ]"));
                for (int i = 0; i < chars; i++)
                {
                    value += getCharFromRange(gs->range.Mid(start+1,
                        p-start-1), gs->randomValues, recNo, i, chars);
                }
                start = p+1;
                chars = 1;
            }
            else
            {
                int p = gs->range.find(wxT("["), start+1);
                if (p == wxString::npos)    // invalid mask
                    throw FRError(_("Invalid mask, missing ["));
                wxString number = gs->range.Mid(start, p-start);
                if (!number.ToLong(&chars))
                    throw FRError(_("Bad number: ")+number);
                start = p;
            }
        }

        st->Set(param, wx2std(value, dbCharsetConversionM.getConverter()));
    }
}
//-----------------------------------------------------------------------------
void setNumber(IBPP::Statement st, int param, GeneratorSettings* gs, int recNo)
{
    //if (gs->nullPercent > random(100))
    {
        st->SetNull(param);
        return;
    }
    // switch on value type and do accordingly
}
//-----------------------------------------------------------------------------
void setDatetime(IBPP::Statement st, int param, GeneratorSettings* gs,
    int recNo)
{
    //if (gs->nullPercent > random(100))
    {
        st->SetNull(param);
        return;
    }
    // switch on value type and do accordingly
}
//-----------------------------------------------------------------------------
void DataGeneratorFrame::setParam(IBPP::Statement st, int param,
    GeneratorSettings* gs, int recNo)
{
    switch (st->ParameterType(param))
    {
        case IBPP::sdString:
            setString(st, param, gs, recNo);
            break;
        case IBPP::sdSmallint:
        case IBPP::sdInteger:
        case IBPP::sdLargeint:
        case IBPP::sdFloat:
        case IBPP::sdDouble:
            setNumber(st, param, gs, recNo);
            break;
        case IBPP::sdDate:
        case IBPP::sdTime:
        case IBPP::sdTimestamp:
            setDatetime(st, param, gs, recNo);
            break;
        //case sdBlob:
        //case sdArray:
        default:
            st->SetNull(param);
    };
}
//-----------------------------------------------------------------------------
void DataGeneratorFrame::generateData(std::list<Table *>& order)
{
    ProgressDialog pd(this, _("Generating data"), 2);
    pd.Show();
    pd.initProgress(_("Inserting into tables"), order.size());

    // one big transaction (perhaps this should be configurable)
    IBPP::Transaction tr =
        IBPP::TransactionFactory(databaseM->getIBPPDatabase());
    tr->Start();

    for (std::list<Table *>::iterator it = order.begin();
        it != order.end(); ++it)
    {
        pd.setProgressMessage((*it)->getName_(), 1);
        pd.stepProgress();

        std::map<wxString, int>::iterator i2 =
            tableRecordsM.find((*it)->getQuotedName());
        int records = (*i2).second;

        pd.initProgress(wxString::Format(_("Inserting %d records."), records),
            records, 0, 2);

        // collect columns + create insert statement
        wxString ins = wxT("INSERT INTO ") + (*it)->getQuotedName()
            + wxT(" (");
        wxString params(wxT(") VALUES ("));
        (*it)->checkAndLoadColumns();
        bool first = true;
        std::vector<GeneratorSettings *> colSet;
        for (MetadataCollection<Column>::iterator col = (*it)->begin();
            col != (*it)->end(); ++col)
        {
            std::map<wxString, GeneratorSettings *>::iterator si =
                settingsM.find((*it)->getQuotedName() + wxT(".")
                + (*col).getQuotedName());
            if (si != settingsM.end() && (*si).second->valueType !=
                GeneratorSettings::vtSkip)
            {
                if (first)
                    first = false;
                else
                {
                    ins += wxT(", ");
                    params += wxT(",");
                }
                ins += (*col).getQuotedName();
                params += wxT("?");
                colSet.push_back((*si).second);
            }
        }
        if (first)  // no columns
            continue;

        IBPP::Statement st =
            IBPP::StatementFactory(databaseM->getIBPPDatabase(), tr);
        st->Prepare(wx2std(ins + params + wxT(")")));

        for (int i = 0; i < records; i++)
        {
            if (pd.isCanceled())
                return;
            pd.stepProgress(1, 2);
            for (int p = 0; p < st->Parameters(); ++p)
                setParam(st, p+1, colSet[p], i);
            st->Execute();
        }
    }

    tr->Commit();
}
//-----------------------------------------------------------------------------
