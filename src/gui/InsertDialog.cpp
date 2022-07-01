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

#include <wx/ffile.h>

#include <set>

#include "core/ArtProvider.h"
#include "core/FRError.h"
#include "core/StringUtils.h"
#include "gui/controls/DataGridRowBuffer.h"
#include "gui/controls/DataGridTable.h"
#include "gui/InsertDialog.h"
#include "gui/StyleGuide.h"
#include "metadata/column.h"
#include "metadata/database.h"
#include "metadata/domain.h"
#include "metadata/generator.h"
#include "metadata/procedure.h"
#include "metadata/table.h"
#include "metadata/trigger.h"
#include "metadata/view.h"

/*
    The dialog creates a DataGridRowBuffer, and uses ResultsetColumnDefs from
    the active grid to format the values. I.e. new potential row is created
    and filled with values as the user types them.

    When all values are entered, we try to INSERT into database, and if all
    goes well, the prepared row buffer is simply added to the grid.
*/

namespace InsertOptions
{
    // make sure you keep these two in sync if you add/remove items
    const wxString insertOptionStrings[] = {
        "",
        "NULL",
        wxTRANSLATE("Skip (N/A)"),
        wxTRANSLATE("Column default"),
        wxTRANSLATE("Hexadecimal"),
        "CURRENT_DATE",
        "CURRENT_TIME",
        "CURRENT_TIMESTAMP",
        "CURRENT_USER",
        wxTRANSLATE("File..."),
        wxTRANSLATE("Generator...")
    };
    typedef enum { ioRegular = 0, ioNull, ioSkip, ioDefault, ioHex,
        ioDate, ioTime, ioTimestamp, ioUser, ioFile, ioGenerator
    } InsertOption;

    // null is also editable
    bool optionIsEditable(InsertOption i)
    {
        return (i == ioRegular || i == ioHex || i == ioNull);
    }
    bool optionAllowsCustomValue(InsertOption i)
    {
        return (optionIsEditable(i) || i == ioFile || i == ioGenerator
            || i == ioDefault);
    }
    bool optionValueLoadedFromDatabase(InsertOption i)
    {
        return (i == ioDate || i == ioTime || i == ioTimestamp
            || i == ioUser || i == ioGenerator || i == ioDefault);
    }

    InsertOption getInsertOption(wxGrid* grid, int row)
    {
        wxString opt = grid->GetCellValue(row, 2);
        for (int i=0; i<sizeof(insertOptionStrings)/sizeof(wxString); ++i)
            if (insertOptionStrings[i] == opt)
                return (InsertOption)i;
        return ioRegular;
    }

};
using namespace InsertOptions;

Generator *findAutoincGenerator(std::vector<Trigger *>& triggers, Column *c)
{
    for (std::vector<Trigger *>::iterator tt = triggers.begin();
        tt != triggers.end(); ++tt)
    {
        std::vector<Dependency> list;
        (*tt)->getDependencies(list, true);
        Generator *gen = 0;
        bool found = false;
        for (std::vector<Dependency>::iterator di = list.begin();
            di != list.end(); ++di)
        {
            MetadataItem *m = (*di).getDependentObject();
            if (!gen)
                gen = dynamic_cast<Generator *>(m);
            if (c->getTable() == m && (*di).getFields() == c->getName_())
                found = true;
        }
        if (found && gen)
            return gen;
    }
    return 0;
}

// MB: When editor is shown for NULL field, we want to reset the color from
// red to black, so that user does not see the red letters.
// I tried to do that in InsertDialog::OnCellEditorCreated. However, it
// does not work there because Show() is called *after* creating.
// Then I tried to set it in InsertDialog::OnEditorKeyDown, however, it
// turns black only after the second character. The reason is that the
// first KeyDown event is caught by the grid control and sent to
// the editor via StartingKey function without posting the key event to it
class GridCellEditorWithProperColor: public wxGridCellTextEditor
{
public:
    virtual void Show(bool show, wxGridCellAttr *attr = (wxGridCellAttr *)NULL)
    {
        wxGridCellTextEditor::Show(show, attr);
        GetControl()->SetForegroundColour(
            wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    }
};

InsertDialog::InsertDialog(wxWindow* parent, const wxString& tableName,
    DataGridTable *gridTable, IBPP::Statement& st, Database *db)
    :BaseDialog(parent, -1, wxEmptyString), tableNameM(tableName), bufferM(0),
    gridTableM(gridTable), statementM(st), databaseM(db)
{
    localSetM.setDataBaseLenguage();

    DataGridTable::FieldSet fields;
    gridTable->getFields(tableName, fields);

    // 500 should be reasonable for enough rows on the screen, but not too much
    gridM = new wxGrid(getControlsPanel(), ID_Grid, wxDefaultPosition,
        fields.size() < 12 ? wxDefaultSize : wxSize(-1, 500),
        wxWANTS_CHARS | wxBORDER_THEME);
    gridM->SetRowLabelSize(50);
    gridM->DisableDragRowSize();
    gridM->SetRowLabelAlignment(wxALIGN_RIGHT, wxALIGN_CENTRE);
    gridM->SetColLabelAlignment(wxALIGN_LEFT,  wxALIGN_CENTRE);
    gridM->SetDefaultCellAlignment(wxALIGN_LEFT,  wxALIGN_CENTRE);

    wxString labels[] = {
        wxTRANSLATE("Field name"), wxTRANSLATE("Data type"),
        wxTRANSLATE("Special"),    wxTRANSLATE("Value") };

    bufferM = new InsertedGridRowBuffer(gridTable->GetNumberCols());
    for (unsigned u = 0; (int)u < gridTable->GetNumberCols(); u++)
        bufferM->setFieldNA(u, true);

    gridM->CreateGrid(fields.size(), sizeof(labels)/sizeof(wxString));
    for (int i=0; i<sizeof(labels)/sizeof(wxString); ++i)
        gridM->SetColLabelValue(i, labels[i]);

    // preload triggers
    std::vector<Trigger *> triggers;
    if (!fields.empty()) // not empty
    {
        Table *t = (*(fields.begin())).second.second->getTable();
        t->getTriggers(triggers, Trigger::beforeIUD);
    }

    // columns 0, 1: gray, read-only
    for (int col = 0; col <= 1; ++col)
    {
        wxGridCellAttr *cro = new wxGridCellAttr();
        cro->SetBackgroundColour(gridM->GetLabelBackgroundColour());
        cro->SetReadOnly();
        gridM->SetColAttr(col, cro);
    }

    // column 2: selection with "special" colour
    wxGridCellChoiceEditor *types = new wxGridCellChoiceEditor(
        sizeof(insertOptionStrings)/sizeof(wxString),
        insertOptionStrings);
    wxGridCellAttr *ca = new wxGridCellAttr();
    ca->SetEditor(types);
    ca->SetBackgroundColour(wxColour(255, 255, 197));
    gridM->SetColAttr(2, ca);

    // column 3: editable
    GridCellEditorWithProperColor *gce = new GridCellEditorWithProperColor;
    wxGridCellAttr *gca = new wxGridCellAttr();
    gca->SetEditor(gce);
    gridM->SetColAttr(3, gca);

    int row = 0;
    for (DataGridTable::FieldSet::iterator it = fields.begin();
        it != fields.end(); ++it, ++row)
    {
        Column *c = (*it).second.second;
        ResultsetColumnDef *def = (*it).second.first;

        gridM->SetRowLabelValue(row, wxString::Format("%d", row+1));
        gridM->SetCellValue(row, 0, c->getName_());
        gridM->SetCellValue(row, 1, c->getDatatype());

        gridM->SetCellAlignment(row, 3, 
		def->isNumeric() ? wxALIGN_RIGHT : wxALIGN_LEFT, wxALIGN_CENTER);

        wxString defaultValue;
        if (c->getDefault(ReturnDomainDefault, defaultValue))
        {
            gridM->SetCellValue(row, 2, insertOptionStrings[ioDefault]);
            gridM->SetCellValue(row, 3, defaultValue);
            updateControls(row);
        }
        else
        {
            if (c->isNullable(CheckDomainNullability))
            {
                gridM->SetCellValue(row, 2, insertOptionStrings[ioNull]);
                updateControls(row);
            }
            else if (def->isNumeric())
                gridM->SetCellValue(row, 3, "0");
        }

        InsertColumnInfo ici(row, c, def, (*it).first);
        columnsM.push_back(ici);

        Generator *gen = findAutoincGenerator(triggers, c);
        if (gen)
        {
            gridM->SetCellValue(row, 2, insertOptionStrings[ioGenerator]);
            updateControls(row);
            gridM->SetCellValue(row, 3, gen->getQuotedName());
        }
    }

    checkboxInsertAnother = new wxCheckBox(getControlsPanel(), wxID_ANY,
        _("Keep dialog open after inserting"));

    button_ok = new wxButton(getControlsPanel(), wxID_OK, _("&Insert"));
    button_cancel = new wxButton(getControlsPanel(), wxID_CANCEL,
        _("&Cancel"));

    set_properties();
    do_layout();
    if (gridM->GetNumberRows() > 0)
        gridM->SetGridCursor(0, 3);
    gridM->SetFocus();

    // until we find something better
    SetIcon(wxArtProvider::GetIcon(ART_Trigger, wxART_FRAME_ICON));
}

void InsertDialog::OnClose(wxCloseEvent& WXUNUSED(event))
{
    // make sure parent window is properly activated again
    wxWindow* p = GetParent();
    if (p)
    {
        p->Enable();
        Hide();
        p->Raise();
    }
    Destroy();
}

InsertDialog::~InsertDialog()
{
    delete bufferM;
}

void InsertDialog::set_properties()
{
    SetTitle(wxString::Format(_("Insert Record(s) Into Table \"%s\""),
        tableNameM.c_str()));
    button_ok->SetDefault();
}

void InsertDialog::do_layout()
{
    gridM->AutoSize();
    wxScreenDC sdc;
    wxSize sz = sdc.GetTextExtent("CURRENT_TIMESTAMP WWW");
    gridM->SetColSize(2, sz.GetWidth());
    gridM->SetColSize(3, sz.GetWidth());    // reasonable default width?
    gridM->ForceRefresh();

    wxSizer* sizerControls = new wxBoxSizer(wxVERTICAL);
    sizerControls->Add(gridM, 1, wxEXPAND, 0);

    sizerControls->AddSpacer(
        styleguide().getUnrelatedControlMargin(wxVERTICAL));
    sizerControls->Add(checkboxInsertAnother, 0, wxEXPAND);

    wxSizer* sizerButtons =
        styleguide().createButtonSizer(button_ok, button_cancel);
    layoutSizers(sizerControls, sizerButtons, true);
}

const wxString InsertDialog::getName() const
{
    return "InsertDialog";
}

bool InsertDialog::getConfigStoresWidth() const
{
    return true;
}

bool InsertDialog::getConfigStoresHeight() const
{
    return false;
}

void InsertDialog::updateControls(int row)
{
    InsertOption ix = getInsertOption(gridM, row);
    if (!optionAllowsCustomValue(ix))
        gridM->SetCellValue(row, 3, wxEmptyString);
    gridM->SetReadOnly(row, 3, !optionIsEditable(ix));
    updateColors(this);
    if (ix == ioNull)
    {
        gridM->SetCellTextColour(row, 3, *wxRED);
        gridM->SetCellValue(row, 3, "[null]");
    }
    else
    {
        gridM->SetCellTextColour(row, 3,
            wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    }
}

void InsertDialog::storeValues()
{
    for (std::vector<InsertColumnInfo>::iterator it = columnsM.begin();
        it != columnsM.end(); ++it)
    {
        InsertOption sel = getInsertOption(gridM, (*it).row);
        wxString previous = (*it).columnDef->getAsString(bufferM, databaseM);
        wxString value = gridM->GetCellValue((*it).row, 3);
        try
        {
            if (sel == ioHex)
            {
                if ((*it).columnDef->isNumeric())   // hex -> dec
                {
                    long l;                     // should we support 64bit?
                    if (!value.ToLong(&l, 16))
                        throw FRError(_("Invalid hexadecimal number"));
                    value.Printf("%d", l);
                }
                else    // convert hex string to regular string and set
                {
                    wxString result;
                    for (size_t i = 0; i < value.Len(); i+=2)
                    {
                        unsigned long l;
                        if (!value.Mid(i, 2).ToULong(&l, 16))
                            throw FRError(_("Invalid hexadecimal character"));
                        result += wxChar(l);
                    }
                    value = result;
                }
                gridM->SetCellValue((*it).row, 2,
                    insertOptionStrings[ioRegular]);
                sel = ioRegular;
            }
            switch (sel)
            {
                case ioRegular:
                    (*it).columnDef->setFromString(bufferM, value);
                    gridM->SetCellValue((*it).row, 3,
                        (*it).columnDef->getAsString(bufferM, databaseM));
                    // there is no break; here deliberately!
                case ioFile:
                    bufferM->setFieldNull((*it).index, false);
                    bufferM->setFieldNA((*it).index, false);
                    break;
                case ioSkip:
                    bufferM->setFieldNA((*it).index, true);
                    break;
                case ioNull:
                    bufferM->setFieldNull((*it).index, true);
                    bufferM->setFieldNA((*it).index, false);
                    break;
                default:
                    break;
            }
        }
        catch(...)
        {   // we take the old value from buffer
            gridM->SetCellValue((*it).row, 3, previous);
            throw;
        }
    }
}

void InsertDialog::preloadSpecialColumns()
{
    // step 1: build list of CURRENT_* and generator values
    wxString sql("SELECT ");
    bool first = true;
    for (std::vector<InsertColumnInfo>::iterator it = columnsM.begin();
        it != columnsM.end(); ++it)
    {
        InsertOption sel = getInsertOption(gridM, (*it).row);
        if (!optionValueLoadedFromDatabase(sel))
            continue;
        if (first)
            first = false;
        else
            sql += ",";
        if (sel == ioGenerator) // generator
            sql += "GEN_ID(" + gridM->GetCellValue((*it).row, 3)
                + ", 1)";
        else if (sel == ioDefault)
        {
            if (!(*it).column->isString())
                sql += "CAST(";
            sql += gridM->GetCellValue((*it).row, 3);
            if (!(*it).column->isString())
            {   // false = no custom formatting, just the pure type
                sql += " AS " + (*it).column->getDatatype(false)
                    + ")";
            }
        }
        else // CURRENT_ USER/DATE/TIMESTAMP...
            sql += gridM->GetCellValue((*it).row, 2);
    }

    // step 2: load those from the database
    IBPP::Statement st1 = IBPP::StatementFactory(statementM->DatabasePtr(),
        statementM->TransactionPtr());
    if (!first) // we do need some data
    {
        sql += " FROM RDB$DATABASE";
        st1->Prepare(wx2std(sql));
        st1->Execute();
        st1->Fetch();
    }

    // step 3: save values into buffer and edit controls
    //         so that the next run doesn't reload generators
    unsigned col = 1;
    for (std::vector<InsertColumnInfo>::iterator it = columnsM.begin();
        it != columnsM.end(); ++it)
    {
        InsertOption sel = getInsertOption(gridM, (*it).row);
        if (!optionValueLoadedFromDatabase(sel))
            continue;
        bufferM->setFieldNA((*it).index, false);
        bufferM->setFieldNull((*it).index, st1->IsNull(col));
        if (!st1->IsNull(col))
            (*it).columnDef->setValue(bufferM, col, st1, wxConvCurrent, databaseM);
        ++col;
        if (sel != ioGenerator)  // what follows is only for generators
            continue;
        gridM->SetCellValue((*it).row, 3,
            (*it).columnDef->getAsString(bufferM, databaseM));
        gridM->SetCellValue((*it).row, 2,
            insertOptionStrings[ioRegular]);  // treat as regular value
        updateControls((*it).row);
    }
}

//! event handling
BEGIN_EVENT_TABLE(InsertDialog, BaseDialog)
    EVT_GRID_CELL_CHANGED(InsertDialog::OnGridCellChange)
    EVT_GRID_EDITOR_CREATED(InsertDialog::OnCellEditorCreated)
    EVT_BUTTON(wxID_OK, InsertDialog::OnOkButtonClick)
    EVT_BUTTON(wxID_CANCEL, InsertDialog::OnCancelButtonClick)
    EVT_CLOSE(InsertDialog::OnClose)
END_EVENT_TABLE()

void InsertDialog::OnCellEditorCreated(wxGridEditorCreatedEvent& event)
{
    wxTextCtrl *editor = dynamic_cast<wxTextCtrl *>(event.GetControl());
    if (!editor)
        return;
    editor->Connect(wxEVT_KEY_DOWN,
        wxKeyEventHandler(InsertDialog::OnEditorKeyDown),
        0, this);
}

void InsertDialog::OnEditorKeyDown(wxKeyEvent& event)
{
    wxTextCtrl *editor = dynamic_cast<wxTextCtrl *>(event.GetEventObject());
    if (event.GetKeyCode() == WXK_DELETE && editor->GetValue().IsEmpty())
    {
        // dismiss editor and set null
        gridM->HideCellEditControl();
        int row = gridM->GetGridCursorRow();
        gridM->SetCellValue(row, 2, insertOptionStrings[ioNull]);
        updateControls(row);
        return;
    }
    event.Skip();
}

void InsertDialog::OnCancelButtonClick(wxCommandEvent& WXUNUSED(event))
{
    Close();
}

void InsertDialog::OnOkButtonClick(wxCommandEvent& WXUNUSED(event))
{
    storeValues();
    preloadSpecialColumns();

    Identifier tableId(tableNameM, databaseM->getSqlDialect());

    wxString stm = "INSERT INTO " + tableId.getQuoted() + " (";
    wxString val = ") VALUES (";
    bool first = true;
    for (std::vector<InsertColumnInfo>::iterator it = columnsM.begin();
        it != columnsM.end(); ++it)
    {
        InsertOption sel = getInsertOption(gridM, (*it).row);
        if (sel == ioSkip)   // skip
            continue;

        if (first)
            first = false;
        else
        {
            stm += ",";
            val += ",";
        }
        stm += (*it).column->getQuotedName();
        if (sel == ioFile)
            val += "?";
        else if (sel == ioNull || bufferM->isFieldNull((*it).index))
            val += "NULL";
        else
        {
            val += "'" + (*it).columnDef->getAsFirebirdString(bufferM)
                + "'";
        }
    }

    IBPP::Statement st1 = IBPP::StatementFactory(statementM->DatabasePtr(),
        statementM->TransactionPtr());
    stm += val + ")";
    st1->Prepare(wx2std(stm, databaseM->getCharsetConverter()));

    // load blobs
    int index = 1;
    for (std::vector<InsertColumnInfo>::iterator it = columnsM.begin();
        it != columnsM.end(); ++it)
    {
        if (getInsertOption(gridM, (*it).row) != ioFile)
            continue;
        wxFFile fl(gridM->GetCellValue((*it).row, 3), "rb");
        if (!fl.IsOpened())
            throw FRError(_("Cannot open BLOB file."));
        IBPP::Blob b = IBPP::BlobFactory(st1->DatabasePtr(),
            st1->TransactionPtr());
        b->Create();
        uint8_t buffer[32768];
        while (!fl.Eof())
        {
            size_t len = fl.Read(buffer, 32767);    // slow when not 32k
            if (len < 1)
                break;
            b->Write(buffer, len);
        }
        fl.Close();
        b->Close();
        st1->Set(index++, b);
        bufferM->setBlob((*it).columnDef->getIndex(), b);
    }

    st1->Execute();

    // add buffer to the table and set internal buffer marker to zero
    // (to prevent deletion in destructor)
    gridTableM->addRow(bufferM, stm);

    if (checkboxInsertAnother->IsChecked())
    {
        bufferM = new InsertedGridRowBuffer(bufferM);
    }
    else
    {
        bufferM = 0;
        databaseM = 0;  // prevent other event handlers from making problems
        Close();
    }
}

// helper function for OnChoiceChange
void InsertDialog::setStringOption(InsertColumnInfo& ici, const wxString& s)
{
    if (!s.IsEmpty())
        gridM->SetCellValue(ici.row, 3, s);
    else
    {
        gridM->SetCellValue(ici.row, 2, insertOptionStrings[ioNull]);
        updateControls(ici.row);
    }
}

void InsertDialog::OnGridCellChange(wxGridEvent& event)
{
    if (!databaseM) // dialog already closed
        return;

    static wxMutex m;       // prevent reentrancy since we set the value
    wxMutexLocker ml(m);
    if (!ml.IsOk())
        return;

    int row = event.GetRow();
    InsertOption option = getInsertOption(gridM, row);

    if (event.GetCol() == 3)
    {
        if (option != ioNull && option != ioRegular)
            return;
        wxString cellValue = gridM->GetCellValue(row, 3);
        if (cellValue.IsEmpty())   // we assume null for non-string columns
        {
            // here we can change the NULL behaviour for string columns, i.e.
            // whether empty field is NULL or ''
            // if ((*it).column->isString() ...
            bufferM->setFieldNull(columnsM[row].index, true);
            gridM->SetCellValue(row, 2, insertOptionStrings[ioNull]);
            updateControls(row);
            return;
        }
        // write data to buffer and retrieve the formatted value
        try
        {
            columnsM[row].columnDef->setFromString(bufferM, cellValue);
        }
        catch(...)
        {
            event.Veto();
            throw;
        }
        gridM->SetCellValue(row, 3,
            columnsM[row].columnDef->getAsString(bufferM, databaseM));
        gridM->SetCellValue(row, 2, insertOptionStrings[ioRegular]);
        updateControls(row);
        bufferM->setFieldNull(columnsM[row].index, false);
        bufferM->setFieldNA(columnsM[row].index, false);
    }
    else if (event.GetCol() == 2)
    {
        if (optionIsEditable(option))
            gridM->SetCellValue(row, 3, wxEmptyString);

        updateControls(row);

        if (option == ioFile)
            setStringOption(columnsM[row], ::wxFileSelector(_("Select a file")));

        if (option == ioDefault)
        {
            wxString defaultValue;
            columnsM[row].column->getDefault(ReturnDomainDefault, defaultValue);
            setStringOption(columnsM[row], defaultValue);
        }

        if (option == ioGenerator)
        {
            // select generator name and store in tx
            wxArrayString as;
            GeneratorsPtr gs(databaseM->getGenerators());
            for (Generators::const_iterator it = gs->begin(); it != gs->end();
                ++it)
            {
                as.Add((*it)->getQuotedName());
            }
            wxString s(::wxGetSingleChoice(_("Select a generator"),
                _("Generator"), as, this));
            setStringOption(columnsM[row], s);
        }
    }
}
