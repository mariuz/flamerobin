/*
  Copyright (c) 2004-2022 The FlameRobin Development Team

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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/arrstr.h>
#include <wx/ffile.h>
#include <wx/file.h>

#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/xml/xml.h>

// needed for random
#include <stdlib.h>

#include "core/ArtProvider.h"
#include "core/FRError.h"
#include "core/StringUtils.h"
#include "gui/AdvancedMessageDialog.h"
#include "gui/controls/DBHTreeControl.h"
#include "gui/DataGeneratorFrame.h"
#include "gui/ProgressDialog.h"
#include "metadata/column.h"
#include "metadata/database.h"
#include "metadata/domain.h"
#include "metadata/table.h"

// returns a value between 0 and (maxval-1)
// I wrote this as I don't know how much is rand from stdlib portable
int frRandom(double maxval)
{
    return (int)(maxval*rand()/(RAND_MAX+1.0));
}

// dd.mm.yyyy
void str2date(const wxString& str, int& date)
{
    long d,m,y;
    if (!str.Mid(0,2).ToLong(&d) || !str.Mid(3,2).ToLong(&m) ||
        !str.Mid(6,4).ToLong(&y) || !IBPP::itod(&date, y,m,d))
    {
        throw FRError(_("Invalid date: ") + str);
    }
}

// HH:MM:SS
void str2time(const wxString& str, int& mytime)
{
    long h = 0, m = 0, s = 0;
    if (!str.Mid(0, 2).ToLong(&h) || !str.Mid(3, 2).ToLong(&m)
        || !str.Mid(6, 2).ToLong(&s))
    {
        throw FRError(_("Invalid time: ") + str);
    }
    IBPP::itot(&mytime, h, m, s, 0);
}

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
            Identifier id((*fi).getReferencedTable());
            if (id.getQuoted() == t->getQuotedName())   // self reference
                continue;
            std::map<wxString, int>::iterator it =
                needs.find(id.getQuoted());
            if (it != needs.end() && (*it).second > 0)
                dependsOn.push_back(id.getQuoted());
        }
    }
    void remove(const wxString& tab_name)
    {
        std::list<wxString>::iterator it = std::find(dependsOn.begin(),
            dependsOn.end(), tab_name);
        if (it != dependsOn.end())
            dependsOn.erase(it);
    }

    Table *table;
    std::list<wxString> dependsOn;
};

// helper for saving settings
void dsAddChildNode(wxXmlNode* parentNode, const wxString nodeName,
    const wxString nodeContent)
{
    if (!nodeContent.IsEmpty())
    {
        wxXmlNode* propn = new wxXmlNode(wxXML_ELEMENT_NODE, nodeName);
        parentNode->AddChild(propn);
        propn->AddChild(new wxXmlNode(wxXML_TEXT_NODE, wxEmptyString,
            nodeContent));
    }
}

// used for loading settings from XML file
static const wxString getNodeContent(wxXmlNode* node)
{
    for (wxXmlNode* n = node->GetChildren(); (n); n = n->GetNext())
    {
        if (n->GetType() == wxXML_TEXT_NODE
            || n->GetType() == wxXML_CDATA_SECTION_NODE)
        {
            return n->GetContent();
        }
    }
    return wxEmptyString;
}

// used for loading settings from XML file
void parseTable(wxXmlNode* xmln, std::map<wxString, int>& tr)
{
    wxASSERT(xmln);
    wxString tablename;
    long records = 0;
    for (xmln = xmln->GetChildren(); (xmln); xmln = xmln->GetNext())
    {
        if (xmln->GetType() != wxXML_ELEMENT_NODE)
            continue;

        wxString value(getNodeContent(xmln));
        if (xmln->GetName() == "name")
            tablename = value;
        else if (xmln->GetName() == "records")
            value.ToLong(&records);
    }

    if (!tablename.IsEmpty())
        tr.insert(std::pair<wxString, int>(tablename, records));
}

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

    GeneratorSettings();
    GeneratorSettings(GeneratorSettings* other);
    void toXML(wxXmlNode *parent);
    wxString fromXML(wxXmlNode *parent);    // returns column name
};

GeneratorSettings::GeneratorSettings()
{
}

GeneratorSettings::GeneratorSettings(GeneratorSettings* other)
{
    valueType = other->valueType;
    range = other->range;
    sourceTable = other->sourceTable;
    sourceColumn = other->sourceColumn;
    fileName = other->fileName;
    randomValues = other->randomValues;
    nullPercent = other->nullPercent;
}

void GeneratorSettings::toXML(wxXmlNode *parent)
{
    dsAddChildNode(parent, "valueType",
        wxString::Format("%d", (int)valueType));
    dsAddChildNode(parent, "range", range);
    dsAddChildNode(parent, "sourceTable", sourceTable);
    dsAddChildNode(parent, "sourceColumn", sourceColumn);
    dsAddChildNode(parent, "fileName", fileName);
    dsAddChildNode(parent, "randomValues",
        randomValues ? "1" : "0");
    dsAddChildNode(parent, "nullPercent",
        wxString::Format("%d", nullPercent));
}

wxString GeneratorSettings::fromXML(wxXmlNode *parent)
{
    wxASSERT(parent);
    wxString colname;
    long l;
    wxXmlNode *xmln;
    for (xmln = parent->GetChildren(); (xmln); xmln = xmln->GetNext())
    {
        if (xmln->GetType() != wxXML_ELEMENT_NODE)
            continue;

        wxString value(getNodeContent(xmln));
        if (xmln->GetName() == "name")
            colname = value;
        else if (xmln->GetName() == "valueType")
        {
            if (!value.ToLong(&l))
                return wxEmptyString;
            valueType = (ValueType)l;
        }
        else if (xmln->GetName() == "range")
            range = value;
        else if (xmln->GetName() == "sourceTable")
            sourceTable = value;
        else if (xmln->GetName() == "sourceColumn")
            sourceColumn = value;
        else if (xmln->GetName() == "fileName")
            fileName = value;
        else if (xmln->GetName() == "randomValues")
            randomValues = (value == "1");
        else if (xmln->GetName() == "nullPercent")
        {
            if (!value.ToLong(&l))
                return wxEmptyString;
            nullPercent = (int)l;
        }
    }
    return colname;
}

DataGeneratorFrame::DataGeneratorFrame(wxWindow* parent, Database* db)
    :BaseFrame(parent,-1, ""), databaseM(db), loadingM(true)
{
    // until we find something better
    SetIcon(wxArtProvider::GetIcon(ART_Procedure, wxART_FRAME_ICON));
    SetTitle(_("Test Data Generator"));
    // prevent tree events from reaching the main frame
    // TODO: we need proper event handling for tree to allow multiple
    //       tree controls that are completely functional
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);

    wxBoxSizer* outerSizer;
    outerSizer = new wxBoxSizer( wxVERTICAL );

    outerPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL | wxCLIP_CHILDREN);
    wxBoxSizer* innerSizer;
    innerSizer = new wxBoxSizer( wxVERTICAL );

    mainSplitter = new wxSplitterWindow( outerPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        // wx docs say that wxSP_NOBORDER is default
        wxSP_NOBORDER);
    mainSplitter->SetMinimumPaneSize(100);
    mainSplitter->SetSashGravity( 0.5 );

    leftPanel = new wxPanel( mainSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* leftPanelSizer;
    leftPanelSizer = new wxBoxSizer( wxVERTICAL );

    leftLabel = new wxStaticText( leftPanel, wxID_ANY, "Select tables and columns", wxDefaultPosition, wxDefaultSize, 0 );
    leftPanelSizer->Add( leftLabel, 0, wxALL|wxEXPAND, 5 );

    mainTree = new DBHTreeControl(leftPanel, wxDefaultPosition, wxDefaultSize,
#if defined __WXGTK20__ || defined __WXMAC__
        // doesn't seem to work on MSW when root is hidden
        wxTR_NO_LINES | wxTR_HIDE_ROOT |
#endif
        wxTR_HAS_BUTTONS | wxBORDER_THEME);
    leftPanelSizer->Add( mainTree, 1, wxALL|wxEXPAND, 5 );

    leftPanel->SetSizer( leftPanelSizer );
    leftPanel->Layout();
    leftPanelSizer->Fit( leftPanel );
    rightPanel = new wxPanel( mainSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    rightPanelSizer = new wxBoxSizer( wxVERTICAL );

    rightLabel = new wxStaticText( rightPanel, wxID_ANY, "Configure", wxDefaultPosition, wxDefaultSize, 0 );
    rightPanelSizer->Add( rightLabel, 0, wxALL|wxEXPAND, 5 );

    tableLabel = new wxStaticText( rightPanel, wxID_ANY, "Table: table name", wxDefaultPosition, wxDefaultSize, 0 );
    tableLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, "sans"));

    rightPanelSizer->Add( tableLabel, 0, wxTOP|wxBOTTOM|wxLEFT, 10 );

    wxBoxSizer* recordsSizer = new wxBoxSizer( wxHORIZONTAL );

    recordsLabel = new wxStaticText( rightPanel, wxID_ANY, "Number of records to create:", wxDefaultPosition, wxDefaultSize, 0 );
    recordsSizer->Add( recordsLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );

    spinRecords = new wxSpinCtrl( rightPanel, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxSP_ARROW_KEYS, 0, 100000, 0);
    recordsSizer->Add( spinRecords, 0, wxRIGHT|wxLEFT, 10 );

    rightPanelSizer->Add( recordsSizer, 0, wxEXPAND, 5 );

    skipCheckbox = new wxCheckBox( rightPanel, ID_checkbox_skip, "Skip this table", wxDefaultPosition, wxDefaultSize, 0 );

    rightPanelSizer->Add( skipCheckbox, 0, wxBOTTOM|wxRIGHT|wxLEFT, 8 );

    columnLabel = new wxStaticText( rightPanel, wxID_ANY, "Column: column name", wxDefaultPosition, wxDefaultSize, 0 );
    columnLabel->SetFont( wxFont( 10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "sans" ) );

    rightPanelSizer->Add( columnLabel, 0, wxTOP|wxLEFT, 10 );

    valuetypeLabel = new wxStaticText( rightPanel, wxID_ANY, "Value type:", wxDefaultPosition, wxDefaultSize, 0 );
    rightPanelSizer->Add( valuetypeLabel, 0, wxALL, 10 );

    wxFlexGridSizer* flexSizer = new wxFlexGridSizer( 0, 2, 3, 3 );
    flexSizer->AddGrowableCol( 1 );
    flexSizer->SetFlexibleDirection( wxHORIZONTAL );

    radioSkip = new wxRadioButton( rightPanel, wxID_ANY, "Skip", wxDefaultPosition, wxDefaultSize, 0);
    flexSizer->Add( radioSkip, 0, wxALIGN_CENTER_VERTICAL, 5 );

    flexSizer->Add( 0, 0, 1, wxALL, 5 );

    radioRange = new wxRadioButton( rightPanel, wxID_ANY, "Range/mask:", wxDefaultPosition, wxDefaultSize, 0);
    flexSizer->Add( radioRange, 0, wxALIGN_CENTER_VERTICAL, 5 );

    rangeText = new wxTextCtrl( rightPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    flexSizer->Add( rangeText, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

    radioColumn = new wxRadioButton( rightPanel, wxID_ANY, "Value from column:", wxDefaultPosition, wxDefaultSize, 0);
    flexSizer->Add( radioColumn, 0, wxALIGN_CENTER_VERTICAL, 5 );

    wxArrayString tables;
    TablesPtr t(db->getTables());
    for (Tables::iterator it = t->begin(); it != t->end(); ++it)
        tables.Add((*it)->getQuotedName());
    tables.Sort();
    wxArrayString empty;

    valueSizer = new wxBoxSizer( wxHORIZONTAL );
    valueChoice = new wxChoice( rightPanel, ID_choice_value, wxDefaultPosition, wxDefaultSize, tables);
    valueSizer->Add( valueChoice, 1, wxALIGN_CENTER_VERTICAL, 5 );
    valueColumnChoice = new wxChoice( rightPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, empty);
    valueSizer->Add( valueColumnChoice, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT, 5 );
    flexSizer->Add( valueSizer, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

    radioFile = new wxRadioButton( rightPanel, wxID_ANY, "Value from file:", wxDefaultPosition, wxDefaultSize, 0);
    flexSizer->Add( radioFile, 0, wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer* filenameSizer = new wxBoxSizer( wxHORIZONTAL );

    fileText = new wxTextCtrl( rightPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    filenameSizer->Add( fileText, 1, wxALIGN_CENTER_VERTICAL, 5 );

    fileButton = new wxButton( rightPanel, ID_button_file, "...", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    filenameSizer->Add( fileButton, 0, wxLEFT, 5 );

    flexSizer->Add( filenameSizer, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

    rightPanelSizer->Add( flexSizer, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );

    randomCheckbox = new wxCheckBox( rightPanel, wxID_ANY, "Select random values rather than sequential", wxDefaultPosition, wxDefaultSize, 0 );
    randomCheckbox->SetValue(true);

    rightPanelSizer->Add( randomCheckbox, 0, wxALL, 8 );

    wxBoxSizer* nullSizer = new wxBoxSizer( wxHORIZONTAL );

    nullLabel = new wxStaticText( rightPanel, wxID_ANY, "Percentage of NULLs:", wxDefaultPosition, wxDefaultSize, 0 );
    nullSizer->Add( nullLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 10 );

    nullSpin = new wxSpinCtrl( rightPanel, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 0);
    nullSizer->Add( nullSpin, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

    nullPercentLabel = new wxStaticText( rightPanel, wxID_ANY, "%", wxDefaultPosition, wxDefaultSize, 0 );
    nullSizer->Add( nullPercentLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

    rightPanelSizer->Add( nullSizer, 0, wxEXPAND, 5 );

    copySizer = new wxBoxSizer( wxHORIZONTAL );

    copyLabel = new wxStaticText( rightPanel, wxID_ANY, "Copy settings from:", wxDefaultPosition, wxDefaultSize, 0 );
    copySizer->Add( copyLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 10 );

    copyChoice = new wxChoice( rightPanel, ID_choice_copy, wxDefaultPosition, wxDefaultSize, tables, 0 );
    copySizer->Add( copyChoice, 1, wxALIGN_CENTER_VERTICAL, 5 );
    copyColumnChoice = new wxChoice( rightPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, empty, 0 );
    copySizer->Add( copyColumnChoice, 1, wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

    copyButton = new wxButton( rightPanel, ID_button_copy, "Copy", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
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

    saveButton = new wxButton( outerPanel, ID_button_save, "Save settings", wxDefaultPosition, wxDefaultSize, 0 );
    buttonSizer->Add( saveButton, 0, wxALL, 5 );

    loadButton = new wxButton( outerPanel, ID_button_load, "Load settings", wxDefaultPosition, wxDefaultSize, 0 );
    buttonSizer->Add( loadButton, 0, wxALL, 5 );

    generateButton = new wxButton( outerPanel, ID_button_generate, "Generate data", wxDefaultPosition, wxDefaultSize, 0 );
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
    TablesPtr ts(db->getTables());
    wxTreeItemId rootNode = mainTree->addRootNode(ts.get());

    loadingM = false;

    wxTreeItemIdValue cookie;
    wxTreeItemId node = mainTree->GetFirstChild(rootNode, cookie);
    if (node.IsOk())
        mainTree->SelectItem(node);
}

DataGeneratorFrame::~DataGeneratorFrame()
{
    for (std::map<wxString, GeneratorSettings *>::iterator it =
        settingsM.begin(); it!= settingsM.end(); ++it)
    {
        delete (*it).second;
    }
}

const wxString DataGeneratorFrame::getName() const
{
    return "DataGeneratorFrame";
}

const wxRect DataGeneratorFrame::getDefaultRect() const
{
    return wxRect(-1, -1, 700, 500);
}

//! closes window if database is removed (unregistered)
void DataGeneratorFrame::subjectRemoved(Subject* subject)
{
    if (subject == databaseM)
        Close();
}

void DataGeneratorFrame::update()
{
    if (!databaseM->isConnected())
        Close();
}

bool DataGeneratorFrame::loadColumns(const wxString& tableName, wxChoice* c)
{
    Identifier id;
    id.setFromSql(tableName);
    Table *t = dynamic_cast<Table *>(databaseM->findRelation(id));
    if (!t)
        return false;
    t->ensureChildrenLoaded();
    c->Clear();
    for (ColumnPtrs::iterator it = t->begin(); it != t->end(); ++it)
        c->Append((*it)->getQuotedName());
    return true;
}

// prehaps using values from config() would be nice
wxString getDefaultRange(Domain *d)
{
    wxString dt, size, scale;
    d->getDatatypeParts(dt, size, scale);

    if (dt == "Smallint" || dt == "Float")
        return "0-100";

    if (   dt == "Numeric" || dt == "Integer" || dt == "Bigint"
        || dt == "Decimal" || dt == "Double precision")
    {
        return "0-2000000";
    }
    
    if (dt == "Boolean") // Firebird v3
    {
        return "true-false";
    }

    if (dt == "Char" || dt == "Varchar")
    {
        if (size == "1")
            return "[az,AZ]";
        else
            return size + "[az,AZ]";
    }

    if (dt == "Timestamp")
        return "01.01.1980 00:00:00-31.12.2005 23:59:59";

    if (dt == "Date")
        return "01.01.1980-31.12.2005";

    if (dt == "Time")
        return "00:00:00-23:59:59";

    return wxEmptyString;
}

// returns the setting or creates a default one
GeneratorSettings* DataGeneratorFrame::getSettings(Column *c)
{
    Table *tab = c->getTable();
    if (!tab)
        throw FRError(_("Table not set"));
    wxString s = tab->getQuotedName() + "." + c->getQuotedName();
    std::map<wxString, GeneratorSettings *>::iterator it = settingsM.find(s);
    if (it != settingsM.end())    // found
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
                Identifier table((*fi).getReferencedTable());
                Identifier column((*fi).getReferencedColumns()[cnt]);
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
    gs->nullPercent = (c->isNullable(CheckDomainNullability) ? 50 : 0);
    gs->valueType = GeneratorSettings::vtRange;
    gs->range = getDefaultRange(c->getDomain().get());
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

BEGIN_EVENT_TABLE( DataGeneratorFrame, BaseFrame )
    EVT_BUTTON( ID_button_file, DataGeneratorFrame::OnFileButtonClick )
    EVT_BUTTON( ID_button_copy, DataGeneratorFrame::OnCopyButtonClick )
    EVT_BUTTON( ID_button_save, DataGeneratorFrame::OnSaveButtonClick )
    EVT_BUTTON( ID_button_load, DataGeneratorFrame::OnLoadButtonClick )
    EVT_BUTTON( ID_button_generate, DataGeneratorFrame::OnGenerateButtonClick )
    EVT_CHECKBOX(ID_checkbox_skip, DataGeneratorFrame::OnSkipCheckboxClick)
    EVT_CHOICE(ID_choice_value, DataGeneratorFrame::OnTableValueChoiceChange)
    EVT_CHOICE(ID_choice_copy, DataGeneratorFrame::OnTableCopyChoiceChange)
    EVT_TREE_SEL_CHANGED(DBHTreeControl::ID_tree_ctrl, DataGeneratorFrame::OnTreeSelectionChanged)
END_EVENT_TABLE()

void DataGeneratorFrame::OnTableValueChoiceChange(wxCommandEvent& event)
{
    if (loadColumns(event.GetString(), valueColumnChoice))
    {
        valueColumnChoice->SetSelection(0);
        valueSizer->Layout();
        radioColumn->SetValue(true);
    }
}

void DataGeneratorFrame::OnTableCopyChoiceChange(wxCommandEvent& event)
{
    if (loadColumns(event.GetString(), copyColumnChoice))
    {
        copyColumnChoice->SetSelection(0);
        copySizer->Layout();
    }
}

void DataGeneratorFrame::OnSkipCheckboxClick(wxCommandEvent& event)
{
    if (event.IsChecked())
        spinRecords->SetValue(0);
    else
        spinRecords->SetValue(200); // TODO: load default from config
}

void DataGeneratorFrame::OnTreeSelectionChanged(wxTreeEvent& event)
{
    if (loadingM)
        return;

    saveSetting(event.GetOldItem());    // save old item
    loadSetting(event.GetItem());       // load new item
}

void DataGeneratorFrame::loadSetting(wxTreeItemId newitem)
{
    if (!newitem.IsOk())
        return;

    MetadataItem *m = mainTree->getMetadataItem(newitem);
    Table *tab = dynamic_cast<Table *>(m);
    Column *col = dynamic_cast<Column *>(m);
    showColumnSettings(col != 0);
    if (tab)
        tab->ensureChildrenLoaded();
    else if (col)
        tab = col->getTable();
    if (!tab)
        return;

    wxString tablename = tab->getQuotedName();
    tableLabel->SetLabel("Table: " + tab->getName_());
    int records = 0;    // tables are not filled by default
    std::map<wxString, int>::iterator i1 = tableRecordsM.find(tablename);
    if (i1 != tableRecordsM.end())
        records = (*i1).second;
    spinRecords->SetValue(records);
    skipCheckbox->SetValue(records <= 0);

    if (!col)
        return;
    columnLabel->SetLabel("Column: " + col->getName_());

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
}

void DataGeneratorFrame::OnLoadButtonClick(wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog fd(this, _("Select file to load"), "", "",
        _("XML files (*.xml)|*.xml|All files (*.*)|*.*"),
        wxFD_OPEN | wxFD_CHANGE_DIR);
    if (wxID_OK != fd.ShowModal())
        return;

    wxXmlDocument doc;
    wxXmlNode *root = 0;
    wxFileInputStream stream(fd.GetPath());
    if (stream.Ok() && doc.Load(stream) && doc.IsOk())
    {
        root = doc.GetRoot();
        if (root->GetName() != "dgf_root")
            root = 0;
    }

    if (root == 0)
    {
        showWarningDialog(this, _("Settings not loaded (1)"),
            _("There was an error while loading."),
            AdvancedMessageDialogButtonsOk());
        return;
    }

    // delete current settings
    for (std::map<wxString, GeneratorSettings *>::iterator it =
        settingsM.begin(); it!= settingsM.end(); ++it)
    {
        delete (*it).second;
    }
    settingsM.clear();
    tableRecordsM.clear();

    for (wxXmlNode* xmln = doc.GetRoot()->GetChildren();
        (xmln); xmln = xmln->GetNext())
    {
        if (xmln->GetType() != wxXML_ELEMENT_NODE)
            continue;
        if (xmln->GetName() == "column")
        {
            GeneratorSettings *gs = new GeneratorSettings;
            wxString name = gs->fromXML(xmln);
            if (name.IsEmpty())
            {
                showWarningDialog(this, _("Settings not loaded"),
                    _("There was an error while loading."),
                    AdvancedMessageDialogButtonsOk());
                return;
            }
            settingsM.insert(
                std::pair<wxString, GeneratorSettings *>(name, gs));
        }
        if (xmln->GetName() == "table")
            parseTable(xmln, tableRecordsM);
    }

    // update the current node
    loadSetting(mainTree->GetSelection());

    showInformationDialog(this, _("Settings loaded"),
        _("The setting where successfully loaded from file."),
        AdvancedMessageDialogButtonsOk());
}

void DataGeneratorFrame::OnSaveButtonClick(wxCommandEvent& WXUNUSED(event))
{
    saveSetting(mainTree->GetSelection());  // save current item if changed

    wxFileDialog fd(this, _("Select file to save"), "", "",
        _("XML files (*.xml)|*.xml|All files (*.*)|*.*"),
        wxFD_SAVE | wxFD_CHANGE_DIR | wxFD_OVERWRITE_PROMPT);
    if (wxID_OK != fd.ShowModal())
        return;

    wxBusyCursor wait;

    wxString dir = wxPathOnly(fd.GetPath());
    if (!wxDirExists(dir))
        wxMkdir(dir);

    wxXmlDocument doc;
    wxXmlNode* root = new wxXmlNode(wxXML_ELEMENT_NODE, "dgf_root");
    doc.SetRoot(root);

    // save tables
    for (std::map<wxString, int>::iterator it = tableRecordsM.begin();
        it != tableRecordsM.end(); ++it)
    {
        wxXmlNode* node = new wxXmlNode(wxXML_ELEMENT_NODE, "table");
        root->AddChild(node);
        dsAddChildNode(node, "name", (*it).first);
        dsAddChildNode(node, "records",
            wxString::Format("%d", (*it).second));
    }

    // save columns
    for (std::map<wxString, GeneratorSettings *>::iterator it =
        settingsM.begin(); it!= settingsM.end(); ++it)
    {
        wxXmlNode* cs = new wxXmlNode(wxXML_ELEMENT_NODE, "column");
        root->AddChild(cs);
        dsAddChildNode(cs, "name", (*it).first);
        (*it).second->toXML(cs);
    }

    if (doc.Save(fd.GetPath()))
    {
        showInformationDialog(this, _("Settings saved"),
            _("The setting where successfully saved to the file."),
            AdvancedMessageDialogButtonsOk());
    }
    else
    {
        showWarningDialog(this, _("Settings not saved"),
            _("There was an error while writing."),
            AdvancedMessageDialogButtonsOk());
    }
}

void DataGeneratorFrame::OnCopyButtonClick(wxCommandEvent& WXUNUSED(event))
{
    MetadataItem *m = mainTree->getMetadataItem(mainTree->GetSelection());
    Column *col = dynamic_cast<Column *>(m);
    if (!col)   // this should never happen as the Copy button should not
        return; // be visible if column is not selected
    Table *tab = col->getTable();
    if (!tab)
        throw FRError(_("Table not found."));

    // copy settings
    wxString name = copyChoice->GetStringSelection() + "." +
        copyColumnChoice->GetStringSelection();
    std::map<wxString, GeneratorSettings *>::iterator it =
        settingsM.find(name);
    if (it == settingsM.end())  // no settings
    {
        showWarningDialog(this, _("Nothing to copy"),
            _("That column doesn't have any settings defined."),
            AdvancedMessageDialogButtonsOk());
        return;
    }

    GeneratorSettings *n = new GeneratorSettings((*it).second);

    wxString c = tab->getQuotedName() + "." + col->getQuotedName();
    it = settingsM.find(c);
    if (it == settingsM.end())  // this should not happen either, but still
        settingsM.insert(std::pair<wxString, GeneratorSettings *>(c, n));
    else
    {
        delete (*it).second;
        (*it).second = n;
    }

    // update the current node
    loadSetting(mainTree->GetSelection());
}

void DataGeneratorFrame::OnFileButtonClick(wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog fd(this, _("Select file to load"), "", "",
        _("Text files (*.txt)|*.txt|All files (*.*)|*.*"),
        wxFD_OPEN | wxFD_CHANGE_DIR);
    if (wxID_OK != fd.ShowModal())
        return;

    fileText->SetValue(fd.GetPath());
    radioFile->SetValue(true);
}

void DataGeneratorFrame::OnGenerateButtonClick(wxCommandEvent& WXUNUSED(event))
{
    saveSetting(mainTree->GetSelection());  // save current item if changed

    std::list<Table *> order;
    if (sortTables(order))
        generateData(order);

    // perhaps add a comment like: "A total of XYZ records were inserted."
    showInformationDialog(this, _("Generator done"),
        _("Data generation completed."),
        AdvancedMessageDialogButtonsOk());
}

bool DataGeneratorFrame::sortTables(std::list<Table *>& order)
{
    // collect list of tables
    // if some table is dropped from the database, it would be
    // removed from tree, but it will remain in tableRecordsM
    // That's why we just search for existing tables that are
    // also present in tableRecordsM
    std::list<TableDep *> deps;
    TablesPtr t = databaseM->getTables();
    for (Tables::iterator it = t->begin(); it != t->end(); ++it)
    {
        std::map<wxString, int>::iterator i2 =
            tableRecordsM.find((*it)->getQuotedName());
        if (i2 != tableRecordsM.end() && (*i2).second > 0)
        {
            TableDep *td = new TableDep((*it).get(), tableRecordsM);
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
                _("A circular dependency was detected among your tables. We are unable to determine to correct order of tables for insert. Currently, the only cure is to first generate data for just one of the tables."),
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
        size_t p = range.find(",", start);
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

void setFromFile(IBPP::Statement st, int param,
    GeneratorSettings *gs, int recNo)
{
    // load strings from file to vector
    wxFileInputStream stream(gs->fileName);
    if (!stream.Ok())
        throw FRError(_("Cannot open file: ")+gs->fileName);
    wxTextInputStream text(stream);

    std::vector<wxString> values;
    while (true)
    {
        wxString s = text.ReadLine();
        if (s.IsEmpty())
            break;
        values.push_back(s);
    }
    if (values.empty())
        return;

    // select (random/sequential) string from vector
    wxString selected;
    if (gs->randomValues)
        selected = values[frRandom(values.size())];
    else
        selected = values[recNo % values.size()];

    // convert string to datatype
    int mydate, mytime;
    IBPP::SDT dt = st->ParameterType(param);
    if (st->ParameterScale(param))
        dt = IBPP::sdDouble;
    switch (dt)
    {
        case IBPP::sdBoolean: // Firebird v3
            st->Set(param, wx2std(selected)); break;
        case IBPP::sdString:
            st->Set(param, wx2std(selected));   break;
        case IBPP::sdSmallint:
        {
            long l;
            if (!selected.ToLong(&l))
                throw FRError(_("Invalid long (smallint) value: ")+selected);
            int16_t t = l;
            st->Set(param, t);
            break;
        }
        case IBPP::sdLargeint:
        {
            wxLongLong_t ll;
            if (!selected.ToLongLong(&ll))
                throw FRError(_("Invalid long long numeric value: ")+selected);
            int64_t t = ll;
            st->Set(param, t);
            break;
        }
        case IBPP::sdInteger:
        {
            long l;
            if (!selected.ToLong(&l))
                throw FRError(_("Invalid long numeric value: ")+selected);
            int32_t t = l;
            st->Set(param, t);
            break;
        }
        case IBPP::sdFloat:
        {
            double d;
            if (!selected.ToDouble(&d))
                throw FRError(_("Invalid float value: ")+selected);
            float f = d;
            st->Set(param, f);
            break;
        }
        case IBPP::sdDouble:
        {
            double d;
            if (!selected.ToDouble(&d))
                throw FRError(_("Invalid double numeric value: ")+selected);
            st->Set(param, d);
            break;
        }
        case IBPP::sdTime:
            str2time(selected, mytime);
            st->Set(param, IBPP::Time(IBPP::Time::tmNone, mytime, IBPP::Time::TZ_NONE));
            break;
        case IBPP::sdDate:
            str2date(selected, mydate);
            st->Set(param, IBPP::Date(mydate));
            break;
        case IBPP::sdTimestamp:
        {
            str2date(selected, mydate);
            str2time(selected.Mid(11), mytime);
            int y, mo, d, h, mi, s, t;
            IBPP::dtoi(mydate, &y, &mo, &d);
            IBPP::ttoi(mytime, &h, &mi, &s, &t);
            st->Set(param, IBPP::Timestamp(y, mo, d, IBPP::Time::tmNone, h, mi, s, t, IBPP::Time::TZ_NONE));
            break;
        }
        case IBPP::sdBlob:
            throw FRError(_("Blob datatype not supported"));
        case IBPP::sdArray:
            throw FRError(_("Array datatype not supported"));
    };
}

template<typename T>
void setFromOther(IBPP::Statement st, int param,
    GeneratorSettings *gs, size_t recNo)
{
    IBPP::Statement st2 =
        IBPP::StatementFactory(st->DatabasePtr(), st->TransactionPtr());

    wxString sql = "SELECT " + gs->sourceColumn + " FROM "
        + gs->sourceTable + " WHERE " + gs->sourceColumn
        + " IS NOT NULL";
    if (!gs->randomValues)
        sql += " ORDER BY 1";
    st2->Prepare(wx2std(sql));
    st2->Execute();
    std::vector<T> values;
    while (st2->Fetch())
    {
        T value;
        st2->Get(1, value);
        values.push_back(value);
        if (values.size() > recNo && !gs->randomValues)
        {
            st->Set(param, value);
            return;
        }
        if (values.size() > 99 && gs->randomValues)
            break;
    }
    if (values.size() == 0)
    {
        if (gs->nullPercent > 0)
        {
            st->SetNull(param);
            return;
        }
        else
            throw FRError(_("No records found in table: ") + gs->sourceTable);
    }

    if (gs->randomValues)
        st->Set(param, values[frRandom(values.size())]);
    else
        st->Set(param, values[recNo % values.size()]);
}

// format for values:
// number[value or range(s)]
// example: 25[az,AZ,09] means: 25 letters or numbers
// example: 10[a,x,5]       means: 10 chars, each either of 'a', 'x' or '5'
void DataGeneratorFrame::setString(IBPP::Statement st, int param,
    GeneratorSettings* gs, int recNo)
{
    wxString value;
    long chars = 1;
    size_t start = 0;
    while (start < gs->range.Length())
    {
        if (gs->range.Mid(start, 1) == "[")
        {
            size_t p = gs->range.find("]", start+1);
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
            size_t p = gs->range.find("[", start+1);
            if (p == wxString::npos)    // invalid mask
                throw FRError(_("Invalid mask, missing ["));
            wxString number = gs->range.Mid(start, p-start);
            if (!number.ToLong(&chars))
                throw FRError(_("Bad number: ")+number);
            start = p;
        }
    }
    st->Set(param, wx2std(value, databaseM->getCharsetConverter()));
}

// gs->range = x,x-y,...
template<typename T>
void setNumber(IBPP::Statement st, int param, GeneratorSettings* gs, int recNo)
{
    std::vector< std::pair<long,long> > ranges;
    long rangesize = 0;
    size_t start = 0;
    while (start < gs->range.Length())
    {
        // last
        wxString one = gs->range.Mid(start);
        size_t p = gs->range.find(",", start);
        if (p != wxString::npos)
        {
            one = gs->range.Mid(start, p-start);
            start = p + 1;
        }
        else
            start = gs->range.Length(); // exit on next loop

        p = one.find("-");
        if (p == wxString::npos)
        {
            long l;
            if (!one.ToLong(&l))
                throw FRError(_("Invalid number: ") + one);
            ranges.push_back(std::pair<long,long>(l, l));
            rangesize++;
        }
        else
        {
            long l1, l2;
            if (!one.Mid(0, p).ToLong(&l1) || !one.Mid(p+1).ToLong(&l2))
                throw FRError(_("Invalid range: ") + one);
            ranges.push_back(std::pair<long,long>(l1, l2));
            rangesize += (l2-l1+1);
        }
    }

    long toget = (gs->randomValues ?
        frRandom(rangesize) : (recNo % rangesize));

    for (std::vector< std::pair<long,long> >::iterator it =
        ranges.begin(); it != ranges.end(); ++it)
    {
        long sz = (*it).second - (*it).first + 1;
        if (sz > toget)
        {
            st->Set(param, (T)((*it).first + toget));
            return;
        }
        toget -= sz;
    }
}

void setDatetime(IBPP::Statement st, int param, GeneratorSettings* gs,
    int recNo)
{
    std::vector< std::pair<int,int> > dateRanges;
    std::vector< std::pair<int,int> > timeRanges;
    int dateRangesize = 0;
    int timeRangesize = 0;

    IBPP::SDT dt = st->ParameterType(param);
    size_t start = 0;
    while (start < gs->range.Length())
    {
        // last
        wxString one = gs->range.Mid(start);
        size_t p = gs->range.find(",", start);
        if (p != wxString::npos)
        {
            one = gs->range.Mid(start, p-start);
            start = p + 1;
        }
        else
            start = gs->range.Length(); // exit on next loop

        // convert first value
        int date, time;
        if ((dt == IBPP::sdDate || dt == IBPP::sdTimestamp))
            str2date(one.Mid(0,10), date);
        if (dt == IBPP::sdTime)
            str2time(one.Mid(0,8), time);
        if (dt == IBPP::sdTimestamp)
            str2time(one.Mid(11,8), time);

        p = one.find("-");
        if (p == wxString::npos)
        {
            if (dt == IBPP::sdDate || dt == IBPP::sdTimestamp)
            {
                dateRanges.push_back(std::pair<int,int>(date, date));
                dateRangesize++;
            }
            if (dt == IBPP::sdTime || dt == IBPP::sdTimestamp)
            {
                timeRanges.push_back(std::pair<int,int>(time, time));
                timeRangesize++;
            }
        }
        else    // range, convert second date/time
        {
            int date2, time2;
            if (dt == IBPP::sdDate)
                str2date(one.Mid(11,10), date2);
            if (dt == IBPP::sdTimestamp)
                str2date(one.Mid(20,10), date2);
            if (dt == IBPP::sdTime)
                str2time(one.Mid( 9, 8), time2);
            if (dt == IBPP::sdTimestamp)
                str2time(one.Mid(31, 8), time2);

            if (dt == IBPP::sdDate || dt == IBPP::sdTimestamp)
            {
                dateRanges.push_back(std::pair<int,int>(date, date2));
                dateRangesize += (date2-date+1);
            }
            if (dt == IBPP::sdTime || dt == IBPP::sdTimestamp)
            {
                timeRanges.push_back(std::pair<int,int>(time, time2));
                timeRangesize += ((time2-time) / 10000 + 1);
            }
        }
    }

    int dateToGet, timeToGet;
    if (gs->randomValues)
    {
        dateToGet = (dateRangesize ? frRandom(dateRangesize) : 0);
        timeToGet = (timeRangesize ? frRandom(timeRangesize) : 0);
    }
    else
    {
        dateToGet = (dateRangesize ? recNo % dateRangesize : 0);
        timeToGet = (timeRangesize ? recNo % timeRangesize : 0);
    }

    int myDate = 0;
    int myTime = 0;
    for (std::vector< std::pair<int,int> >::iterator it =
        dateRanges.begin(); it != dateRanges.end(); ++it)
    {
        int sz = (*it).second - (*it).first + 1;
        if (sz > dateToGet)
        {
            myDate = ((*it).first + dateToGet);
            break;
        }
        dateToGet -= sz;
    }
    for (std::vector< std::pair<int,int> >::iterator it =
        timeRanges.begin(); it != timeRanges.end(); ++it)
    {
        int sz = ((*it).second - (*it).first)/10000 + 1;
        if (sz > timeToGet)
        {
            myTime = ((*it).first + timeToGet*10000);
            break;
        }
        timeToGet -= sz;
    }

    if (dt == IBPP::sdDate)
        st->Set(param, IBPP::Date(myDate));
    if (dt == IBPP::sdTime)
        st->Set(param, IBPP::Time(IBPP::Time::tmNone, myTime, IBPP::Time::TZ_NONE));
    if (dt == IBPP::sdTimestamp)
    {
        int y, mo, d, h, mi, s, t;
        IBPP::dtoi(myDate, &y, &mo, &d);
        IBPP::ttoi(myTime, &h, &mi, &s, &t);
        st->Set(param, IBPP::Timestamp(y, mo, d, IBPP::Time::tmNone, h, mi, s, t, IBPP::Time::TZ_NONE));
    }
}

void DataGeneratorFrame::setParam(IBPP::Statement st, int param,
    GeneratorSettings* gs, int recNo)
{
    if (gs->nullPercent > frRandom(100))
    {
        st->SetNull(param);
        return;
    }

    if (gs->valueType == GeneratorSettings::vtColumn)   // copy from column
    {
        switch (st->ParameterType(param))
        {
            case IBPP::sdBoolean: // Firebird v3
                setFromOther<std::string>(st, param, gs, recNo);  break;
            case IBPP::sdString:
                setFromOther<std::string>(st, param, gs, recNo);  break;
            case IBPP::sdSmallint:
                setFromOther<int16_t>(st, param, gs, recNo);      break;
            case IBPP::sdInteger:
                setFromOther<int32_t>(st, param, gs, recNo);      break;
            case IBPP::sdLargeint:
                setFromOther<int64_t>(st, param, gs, recNo);      break;
            case IBPP::sdFloat:
                setFromOther<float>(st, param, gs, recNo);        break;
            case IBPP::sdDouble:
                setFromOther<double>(st, param, gs, recNo);       break;
            case IBPP::sdDate:
                setFromOther<IBPP::Date>(st, param, gs, recNo);   break;
            case IBPP::sdTime:
                setFromOther<IBPP::Time>(st, param, gs, recNo);   break;
            case IBPP::sdTimestamp:
                setFromOther<IBPP::Timestamp>(st, param, gs, recNo);  break;
            case IBPP::sdBlob:
                throw FRError(_("Blob datatype not supported"));
            case IBPP::sdArray:
                throw FRError(_("Array datatype not supported"));
        };
        return;
    }

    if (gs->valueType == GeneratorSettings::vtRange)
    {
        switch (st->ParameterType(param))
        {
            case IBPP::sdBoolean: // Firebird v3
                setString(st, param, gs, recNo);          break;
            case IBPP::sdString:
                setString(st, param, gs, recNo);          break;
            case IBPP::sdSmallint:
                setNumber<int16_t>(st, param, gs, recNo); break;
            case IBPP::sdInteger:
                setNumber<int32_t>(st, param, gs, recNo); break;
            case IBPP::sdLargeint:
                setNumber<int64_t>(st, param, gs, recNo); break;
            case IBPP::sdFloat:
                setNumber<float>  (st, param, gs, recNo); break;
            case IBPP::sdDouble:
                setNumber<double> (st, param, gs, recNo); break;
            case IBPP::sdDate:
            case IBPP::sdTime:
            case IBPP::sdTimestamp:
                setDatetime(st, param, gs, recNo);
                break;
            case IBPP::sdBlob:
                throw FRError(_("Blob datatype not supported"));
            case IBPP::sdArray:
                throw FRError(_("Array datatype not supported"));
            default:
                st->SetNull(param);
        };
    }

    if (gs->valueType == GeneratorSettings::vtFile)
        setFromFile(st, param, gs, recNo);
}

void DataGeneratorFrame::generateData(std::list<Table *>& order)
{
    ProgressDialog pd(this, _("Generating data"), 2);
    pd.doShow();
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
        wxString ins = "INSERT INTO " + (*it)->getQuotedName()
            + " (";
        wxString params(") VALUES (");
        (*it)->ensureChildrenLoaded();
        bool first = true;
        std::vector<GeneratorSettings *> colSet;
        for (ColumnPtrs::iterator col = (*it)->begin();
            col != (*it)->end(); ++col)
        {
            GeneratorSettings *gs = getSettings((*col).get());   // load or create
            if (gs->valueType == GeneratorSettings::vtSkip)
                continue;

            if (first)
                first = false;
            else
            {
                ins += ", ";
                params += ",";
            }
            ins += (*col)->getQuotedName();
            params += "?";
            colSet.push_back(gs);
        }
        if (first)  // no columns
            continue;

        IBPP::Statement st =
            IBPP::StatementFactory(databaseM->getIBPPDatabase(), tr);
        st->Prepare(wx2std(ins + params + ")"));

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

