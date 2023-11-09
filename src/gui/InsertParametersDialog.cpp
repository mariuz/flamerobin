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
#include "gui/InsertParametersDialog.h"
#include "gui/StyleGuide.h"
#include "metadata/CharacterSet.h"
#include "metadata/column.h"
#include "metadata/database.h"
#include "metadata/domain.h"
#include "metadata/generator.h"
#include "metadata/procedure.h"
#include "metadata/table.h"
#include "metadata/trigger.h"
#include "metadata/view.h"
#include "ibpp.h"
#include "gui/CommandIds.h"
#include "gui/CommandManager.h"

/*
    The dialog creates a DataGridRowBuffer, and uses ResultsetColumnDefs from
    the active grid to format the values. I.e. new potential row is created
    and filled with values as the user types them.

    When all values are entered, we try to INSERT into database, and if all
    goes well, the prepared row buffer is simply added to the grid.
*/

namespace InsertParametersOptions
{
    // make sure you keep these two in sync if you add/remove items
    const wxString insertParametersOptionStrings[] = {
        "",
        "NULL",
        //wxTRANSLATE("Skip (N/A)"),
        //wxTRANSLATE("Column default"),
        wxTRANSLATE("Hexadecimal"),
        "CURRENT_DATE",
        "CURRENT_TIME",
        "CURRENT_TIMESTAMP",
        "CURRENT_USER",
        wxTRANSLATE("File...")
        //wxTRANSLATE("Generator...")
    };
    typedef enum { ioRegular = 0, ioNull, /*ioSkip, ioDefault, */ioHex,
        ioDate, ioTime, ioTimestamp, ioUser, ioFile/*, ioGenerator*/
    } InsertParametersOption;

    // null is also editable
    bool optionIsEditable(InsertParametersOption i)
    {
        return (i == ioRegular || i == ioHex || i == ioNull);
    }
    bool optionAllowsCustomValue(InsertParametersOption i)
    {
        return (optionIsEditable(i) || i == ioFile);// || i == ioGenerator
            //|| i == ioDefault);
    }
    bool optionValueLoadedFromDatabase(InsertParametersOption i)
    {
        return (i == ioDate || i == ioTime || i == ioTimestamp
            || i == ioUser);// || i == ioGenerator || i == ioDefault);
    }

    InsertParametersOption getInsertParametersOption(wxGrid* grid, int row)
    {
        wxString opt = grid->GetCellValue(row, 2);
        for (int i=0; i<sizeof(insertParametersOptionStrings)/sizeof(wxString); ++i)
            if (insertParametersOptionStrings[i] == opt)
                return (InsertParametersOption)i;
        return ioRegular;
    }

    wxString getInsertParametersOptionString(InsertParametersOption opt)
    {
        for (int i = 0; i<sizeof(insertParametersOptionStrings) / sizeof(wxString); ++i)
            if ((InsertParametersOption)i == opt)
                return insertParametersOptionStrings[i];
        return "";
    }


};

using namespace InsertParametersOptions;
/*
Generator *findAutoincGenerator2(std::vector<Trigger *>& triggers, Column *c)
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
*/
// MB: When editor is shown for NULL field, we want to reset the color from
// red to black, so that user does not see the red letters.
// I tried to do that in InsertParametersDialog::OnCellEditorCreated. However, it
// does not work there because Show() is called *after* creating.
// Then I tried to set it in InsertParametersDialog::OnEditorKeyDown, however, it
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

//TODO:
//  Improve the usability, when we have more than 1 parameter with the same name,
//  it's better to merge both, in a single one parameter, and use Set(name, value)
//  let IBPP set the values by itself
InsertParametersDialog::InsertParametersDialog(wxWindow* parent, IBPP::Statement& st, Database *db, std::map<std::string, wxString>& pParameterSaveList, std::map<std::string, wxString>& pParameterSaveListOptionNull)
    :BaseDialog(parent, -1, wxEmptyString), bufferM(0), statementM(st), databaseM(db), parameterSaveList(pParameterSaveList), parameterSaveListOptionNull(pParameterSaveListOptionNull)
{

    int count = statementM->ParametersByName().size();

    // 500 should be reasonable for enough rows on the screen, but not too much
    gridM = new wxGrid(getControlsPanel(), ID_Grid, wxDefaultPosition,
        count < 12 ? wxDefaultSize : wxSize(-1, 500),
        wxWANTS_CHARS | wxBORDER_THEME);
    gridM->SetRowLabelSize(50);
    gridM->DisableDragRowSize();
    gridM->SetRowLabelAlignment(wxALIGN_RIGHT, wxALIGN_CENTRE);
    gridM->SetColLabelAlignment(wxALIGN_LEFT,  wxALIGN_CENTRE);
    gridM->SetDefaultCellAlignment(wxALIGN_LEFT,  wxALIGN_CENTRE);

    wxString labels[] = {
        wxTRANSLATE("Field name"), wxTRANSLATE("Data type"),
        wxTRANSLATE("Special"),    wxTRANSLATE("Value") };

    bufferM = new InsertedGridRowBuffer(count);
    for (unsigned u = 0; (int)u < count; u++)
        bufferM->setFieldNA(u, true);

    gridM->CreateGrid(count, sizeof(labels)/sizeof(wxString));
    for (int i=0; i<sizeof(labels)/sizeof(wxString); ++i)
        gridM->SetColLabelValue(i, labels[i]);

    // preload triggers
    /*std::vector<Trigger *> triggers;
    if (!fields.empty()) // not empty
    {
        Table *t = (*(fields.begin())).second.second->getTable();
        t->getTriggers(triggers, Trigger::beforeIUD);
    }
    */

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
        sizeof(insertParametersOptionStrings)/sizeof(wxString),
        insertParametersOptionStrings);
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
    //for (row = 0; row < statementM->Parameters(); row++)
    for (row = 0; row < count; row++)
    //for (std::string it = statementM->ParametersByName().begin();
    ////    it != statementM->ParametersByName().end(); ++it, ++row)
    {

        std::string c = statementM->ParametersByName().at(row);
        int rowid = statementM->FindParamsByName(c).back() -1;
        gridM->SetRowLabelValue(row, wxString::Format("%d", row+1));
        gridM->SetCellValue(row, 0, c);
        try
        {
            gridM->SetCellValue(row, 1, IBPPtype2string(
                                databaseM,
                                statementM->ParameterType(rowid +1),
                                statementM->ParameterSubtype(rowid +1),
                                statementM->ParameterSize(rowid +1),
                                statementM->ParameterScale(rowid +1)).c_str());//statementM->ParameterType(row));
        }catch(...)
        {

        }

        //gridM->SetCellAlignment(
        //    def->isNumeric() ? wxALIGN_RIGHT : wxALIGN_LEFT, row, 3);
        gridM->SetCellAlignment(row, 3, wxALIGN_RIGHT, wxALIGN_CENTER); //TODO: get isNumeric() from "somewhere" (done is better than perfect, and in this case is just estetic)

        gridM->SetCellValue(row, 2, getInsertParametersOptionString(ioNull));
        updateControls(row);
        //Show prevoius data
        //TODO: load SPECIAL selected option too
        if (statementM->ParametersByName().at(row)!='?')
        {
            if (parameterSaveList.count(statementM->ParametersByName().at(row))) {
                gridM->SetCellValue(wxGridCellCoords(row, 3), parameterSaveList.at(statementM->ParametersByName().at(row)));
                gridM->SetCellValue(row, 2, parameterSaveListOptionNull.at(statementM->ParametersByName().at(row)));
                updateControls(row);
                //if (getInsertParametersOption(gridM, row)!=ioNull)
                //  gridM->SetCellValue(row, 2, insertParametersOptionStrings[ioRegular]);

                //gridM->SetCellValue(wxGridCellCoords(row, 3), parameterSaveList.at(statementM->ParametersByName().at(row)));

            }
        }

        wxString defaultValue;
        /*if (c->getDefault(ReturnDomainDefault, defaultValue))
        {
            gridM->SetCellValue(row, 2, insertParametersOptionStrings[ioDefault]);
            gridM->SetCellValue(row, 3, defaultValue);
            updateControls(row);
        }
        else
        {
            if (def->isNumeric())
                gridM->SetCellValue(row, 3, "0");
        }
        */

        //InsertParametersColumnInfo ici(row, c, def, (*it).first);
        //columnsM.push_back(ici);

        /*Generator *gen = findAutoincGenerator2(triggers, c);
        if (gen)
        {
            gridM->SetCellValue(row, 2, insertParametersOptionStrings[ioGenerator]);
            updateControls(row);
            gridM->SetCellValue(row, 3, gen->getQuotedName());
        }
        */
    }

    //checkboxInsertAnother = new wxCheckBox(getControlsPanel(), wxID_ANY,
      //  _("Keep values"));

    button_ok = new wxButton(getControlsPanel(), wxID_OK, _("&Execute/Run"));
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

void InsertParametersDialog::OnClose(wxCloseEvent& WXUNUSED(event))
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

InsertParametersDialog::~InsertParametersDialog()
{
    delete bufferM;
}

void InsertParametersDialog::set_properties()
{
    SetTitle(wxString::Format(_("Insert Parameter(s) Into SQL ")
        ));
    button_ok->SetDefault();
}

void InsertParametersDialog::do_layout()
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
    //sizerControls->Add(checkboxInsertAnother, 0, wxEXPAND);

    wxSizer* sizerButtons =
        styleguide().createButtonSizer(button_ok, button_cancel);
    layoutSizers(sizerControls, sizerButtons, true);
}

const wxString InsertParametersDialog::getName() const
{
    return "InsertParametersDialog";
}

bool InsertParametersDialog::getConfigStoresWidth() const
{
    return true;
}

bool InsertParametersDialog::getConfigStoresHeight() const
{
    return false;
}

void InsertParametersDialog::updateControls(int row)
{
    InsertParametersOption ix = getInsertParametersOption(gridM, row);
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

void InsertParametersDialog::storeValues()
{
    for (std::vector<InsertParametersColumnInfo>::iterator it = columnsM.begin();
        it != columnsM.end(); ++it)
    {
        InsertParametersOption sel = getInsertParametersOption(gridM, (*it).row);
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
                    insertParametersOptionStrings[ioRegular]);
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
                //case ioSkip:
                //    bufferM->setFieldNA((*it).index, true);
                //    break;
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

void InsertParametersDialog::preloadSpecialColumns()
{
    // step 1: build list of CURRENT_* and generator values
    wxString sql("SELECT ");
    bool first = true;
    for (std::vector<InsertParametersColumnInfo>::iterator it = columnsM.begin();
        it != columnsM.end(); ++it)
    {
        InsertParametersOption sel = getInsertParametersOption(gridM, (*it).row);
        if (!optionValueLoadedFromDatabase(sel))
            continue;
        if (first)
            first = false;
        else
            sql += ",";
        /*
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
        */
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
    for (std::vector<InsertParametersColumnInfo>::iterator it = columnsM.begin();
        it != columnsM.end(); ++it)
    {
        InsertParametersOption sel = getInsertParametersOption(gridM, (*it).row);
        if (!optionValueLoadedFromDatabase(sel))
            continue;
        bufferM->setFieldNA((*it).index, false);
        bufferM->setFieldNull((*it).index, st1->IsNull(col));
        if (!st1->IsNull(col))
            (*it).columnDef->setValue(bufferM, col, st1, wxConvCurrent, databaseM);
        ++col;
        //if (sel != ioGenerator)  // what follows is only for generators
            continue;
        gridM->SetCellValue((*it).row, 3,
            (*it).columnDef->getAsString(bufferM, databaseM));
        gridM->SetCellValue((*it).row, 2,
            insertParametersOptionStrings[ioRegular]);  // treat as regular value
        updateControls((*it).row);
    }
}

//! event handling
BEGIN_EVENT_TABLE(InsertParametersDialog, BaseDialog)
    EVT_GRID_CELL_CHANGED(InsertParametersDialog::OnGridCellChange)
    EVT_GRID_EDITOR_CREATED(InsertParametersDialog::OnCellEditorCreated)
    EVT_BUTTON(wxID_OK, InsertParametersDialog::OnOkButtonClick)
    EVT_BUTTON(wxID_CANCEL, InsertParametersDialog::OnCancelButtonClick)
    EVT_CLOSE(InsertParametersDialog::OnClose)
    EVT_MENU(Cmds::Query_Execute,             InsertParametersDialog::OnOkButtonClick)
    //EVT_UPDATE_UI(Cmds::Query_Execute,             InsertParametersDialog::OnOkButtonClick)

END_EVENT_TABLE()

void InsertParametersDialog::OnCellEditorCreated(wxGridEditorCreatedEvent& event)
{
    wxTextCtrl *editor = dynamic_cast<wxTextCtrl *>(event.GetControl());
    if (!editor)
        return;
    editor->Connect(wxEVT_KEY_DOWN,
        wxKeyEventHandler(InsertParametersDialog::OnEditorKeyDown),
        0, this);
}

void InsertParametersDialog::OnEditorKeyDown(wxKeyEvent& event)
{
    wxTextCtrl *editor = dynamic_cast<wxTextCtrl *>(event.GetEventObject());
    if (event.GetKeyCode() == WXK_DELETE && editor->GetValue().IsEmpty())
    {
        // dismiss editor and set null
        gridM->HideCellEditControl();
        int row = gridM->GetGridCursorRow();
        gridM->SetCellValue(row, 2, insertParametersOptionStrings[ioNull]);
        updateControls(row);
        return;
    }
    event.Skip();
}

void InsertParametersDialog::OnCancelButtonClick(wxCommandEvent& WXUNUSED(event))
{
    Close();
}
void InsertParametersDialog::parseDate(int row, const wxString& source)
{
    IBPP::Date idt;
    idt.Today();
    int y = idt.Year();  // defaults
    int m = idt.Month();
    int d = idt.Day();

    wxString temp(source);
    temp.Trim(true).Trim(false);

    if (temp.CmpNoCase("TOMORROW") == 0)
        idt.Add(1);
    else if (temp.CmpNoCase("YESTERDAY") == 0)
        idt.Add(-1);
    else if (temp.CmpNoCase("DATE") != 0
        && temp.CmpNoCase("NOW") != 0
        && temp.CmpNoCase("TODAY") != 0)
    {
        wxString::iterator it = temp.begin();
        if (!GridCellFormats::get().parseDate(it, temp.end(), true, y, m, d))
            throw FRError(_("Cannot parse date"));
        idt.SetDate(y, m, d);
    }
    statementM->Set(row+1, idt);
}
void InsertParametersDialog::parseTime(int row, const wxString& source)
{
    IBPP::Time itm;
    itm.Now();

    wxString temp(source);
    temp.Trim(true).Trim(false);

    if (temp.CmpNoCase("TIME") != 0 && temp.CmpNoCase("NOW") != 0)
    {
        wxString::iterator it = temp.begin();
        int hr = 0, mn = 0, sc = 0, ms = 0;
        if (!GridCellFormats::get().parseTime(it, temp.end(), hr, mn, sc, ms))
            throw FRError(_("Cannot parse time"));
        itm.SetTime(IBPP::Time::tmNone, hr, mn, sc, 10 * ms, IBPP::Time::TZ_NONE);
    }
    statementM->Set(row+1, itm);
}

void InsertParametersDialog::parseTimeStamp(int row, const wxString& source)
{

    IBPP::Timestamp its;
    its.Today(); // defaults to no time

    wxString temp(source);
    temp.Trim(true).Trim(false);

    if (temp.CmpNoCase("TOMORROW") == 0)
        its.Add(1);
    else if (temp.CmpNoCase("YESTERDAY") == 0)
        its.Add(-1);
    else if (temp.CmpNoCase("NOW") == 0)
        its.Now(); // with time
    else if (temp.CmpNoCase("DATE") != 0
        && temp.CmpNoCase("TODAY") != 0)
    {
        // get date
        int y = its.Year();  // defaults
        int m = its.Month();
        int d = its.Day();
        int hr = 0, mn = 0, sc = 0, ms = 0;
        wxString::iterator it = temp.begin();
        if (!GridCellFormats::get().parseTimestamp(it, temp.end(),
            y, m, d, hr, mn, sc, ms))
        {
            throw FRError(_("Cannot parse timestamp"));
        }
        its.SetDate(y, m, d);
        its.SetTime(IBPP::Time::tmNone, hr, mn, sc, 10 * ms, IBPP::Time::TZ_NONE);
    }

    // all done, set the value
    statementM->Set(row+1, its);
}

void InsertParametersDialog::OnOkButtonClick(wxCommandEvent& WXUNUSED(event))
{
    //storeValues();
    //preloadSpecialColumns();

    swWaitForParameterInputTime.Pause();
    size_t resumedRow = 0;
    parameterSaveList.clear();
    parameterSaveListOptionNull.clear();
    for (resumedRow = 0; resumedRow < statementM->ParametersByName().size(); ++resumedRow)
    {
        wxString value = gridM->GetCellValue(resumedRow, 3);
        wxString param = gridM->GetCellValue(resumedRow, 0);
        //std::string value2 = wx2std(gridM->GetCellValue(row, 3), databaseM->getCharsetConverter());
        //std::string param2 = wx2std(gridM->GetCellValue(row, 0), databaseM->getCharsetConverter());

        InsertParametersOption sel = getInsertParametersOption(gridM, resumedRow);

        parameterSaveList[wx2std(param, databaseM->getCharsetConverter())] = value;
        parameterSaveListOptionNull[wx2std(param, databaseM->getCharsetConverter())] = gridM->GetCellValue(resumedRow, 2); // sel == ioNull;

        int row = 0;
        std::vector<int> parameterslist = statementM->FindParamsByName(wx2std(param, databaseM->getCharsetConverter()));
        for (std::vector<int>::const_iterator it = parameterslist.begin(); it != parameterslist.end(); ++it)
        {
            row = (*it)-1;
            IBPP::SDT parameterType = statementM->ParameterType(parameterslist.back()); //statementM->ParameterType(row + 1);
            int subtype = statementM->ParameterSubtype(parameterslist.back());

            if (sel == ioNull)
            {
                statementM->SetNull(row + 1);
                continue;
            }
            if (value.empty())
            {
                if (parameterType != IBPP::SDT::sdString)
                {
                    statementM->SetNull(row + 1);
                    continue;
                }
            }
            if (sel == ioFile)
            {
                wxFFile fl(gridM->GetCellValue(resumedRow, 3), "rb");
                if (!fl.IsOpened())
                    throw FRError(_("Cannot open BLOB file."));
                IBPP::Blob b = IBPP::BlobFactory(statementM->DatabasePtr(),
                    statementM->TransactionPtr());
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
                //st1->Set(index++, b);
                //bufferM->setBlob((*it).columnDef->getIndex(), b);
                //
                statementM->Set(row + 1, b);
                continue;
            }

            switch (parameterType)
            {
            case IBPP::SDT::sdArray:
                throw FRError(_("Unsupported array format"));
                break;
            case IBPP::SDT::sdBlob: break;  //Done before
            case IBPP::SDT::sdDate:
                parseDate(row, value);
                break;
            case IBPP::SDT::sdTime:
                parseTime(row, value);
                break;
            case IBPP::SDT::sdTimestamp:
                parseTimeStamp(row, value);
                break;
            case IBPP::SDT::sdString:
                if (subtype == 1) {

                    if (value.length() % 2 == 1)
                        throw FRError(_("Invalid HEX value value"));
                    std::vector<char> octet = std::vector<char>();
                    wxString::iterator ci = value.begin();
                    wxString::iterator end = value.end();

                    wxString num;
                    while (ci != end)
                    {
                        wxChar c = (wxChar)*ci;
                        num = c;
                        ++ci;
                        c = (wxChar)*ci;
                        num += c;
                        ++ci;
                        octet.push_back(std::stoi(wx2std(num, databaseM->getCharsetConverter()), nullptr, 16));
                    }
                    while (octet.size() < statementM->ParameterSize(parameterslist.back()))
                        octet.push_back(0x0);
                    statementM->Set(row + 1, octet.data(), octet.size());
                }
                else
                    statementM->Set(row + 1, wx2std(value, databaseM->getCharsetConverter()));
                break;
            case IBPP::SDT::sdSmallint:
                long d;
                if (!value.ToLong(&d))
                    throw FRError(_("Invalid integer value"));
                statementM->Set(row + 1, (int)d);
                break;
            case IBPP::SDT::sdInteger:
                long d1;
                if (!value.ToLong(&d1))
                    throw FRError(_("Invalid integer value"));
                statementM->Set(row + 1, (int)d1);
                break;
            case IBPP::SDT::sdLargeint:
                wxLongLong_t d2;
                if (!value.ToLongLong(&d2))
                    throw FRError(_("Invalid large integer value"));
                statementM->Set(row + 1, (int64_t)d2);
                break;
            case IBPP::SDT::sdFloat:
                double d3;
                if (!value.ToDouble(&d3))
                    throw FRError(_("Invalid float numeric value"));
                statementM->Set(row + 1, (float)d3);
                break;
            case IBPP::SDT::sdDouble:
                double d4;
                if (!value.ToDouble(&d4))
                    throw FRError(_("Invalid double numeric value"));
                statementM->Set(row + 1, d4);
                break;
            case IBPP::SDT::sdBoolean:
                if ((value.Upper() == "TRUE") || (value.Upper() == "FALSE") || (value == "0") || (value == "1")) {
                    bool b1 = (value.Upper() == "TRUE" || value == "1");
                    statementM->Set(row + 1, b1);
                }else
                    throw FRError(_("Invalid boolean value"));
                    break;

            }
        }

        //throw FRError(_("Cannot open BLOB file."));
    }
/*
    for (std::vector<InsertParametersColumnInfo>::iterator it = columnsM.begin();
        it != columnsM.end(); ++it)
    {
        InsertParametersOption sel = getInsertParametersOption(gridM, (*it).row);
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
    */
    // load blobs
    /*
    int index = 1;
    for (std::vector<InsertParametersColumnInfo>::iterator it = columnsM.begin();
        it != columnsM.end(); ++it)
    {
        if (getInsertParametersOption(gridM, (*it).row) != ioFile)
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
*/
    /*if (checkboxInsertAnother->IsChecked())
    {
        //bufferM = new InsertedGridRowBuffer(bufferM);
    }
    else*/
    {
        bufferM = 0;
        databaseM = 0;  // prevent other event handlers from making problems
        Close();
    }
}

// helper function for OnChoiceChange
void InsertParametersDialog::setStringOption(int row, const wxString& s)
{
    if (!s.IsEmpty())
        gridM->SetCellValue(row, 3, s);
    else
    {
        gridM->SetCellValue(row, 2, insertParametersOptionStrings[ioNull]);
        //updateControls(row);
    }
}

void InsertParametersDialog::OnGridCellChange(wxGridEvent& event)
{
    if (!databaseM) // dialog already closed
        return;

    static wxMutex m;       // prevent reentrancy since we set the value
    wxMutexLocker ml(m);
    if (!ml.IsOk())
        return;

    int row = event.GetRow();
    InsertParametersOption option = getInsertParametersOption(gridM, row);

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
            //bufferM->setFieldNull(columnsM[row].index, true);
            gridM->SetCellValue(row, 2, insertParametersOptionStrings[ioNull]);
            updateControls(row);
            return;
        }
        // write data to buffer and retrieve the formatted value
        try
        {
            //Erro
            //columnsM[row].columnDef->setFromString(bufferM, cellValue);
        }
        catch(...)
        {
            event.Veto();
            throw;
        }

        /*gridM->SetCellValue(row, 3,
            columnsM[row].columnDef->getAsString(bufferM));
            */
        gridM->SetCellValue(row, 2, insertParametersOptionStrings[ioRegular]);
        updateControls(row);
        /*bufferM->setFieldNull(columnsM[row].index, false);
        bufferM->setFieldNA(columnsM[row].index, false);
        */
    }
    else if (event.GetCol() == 2)
    {
        if (optionIsEditable(option))
            gridM->SetCellValue(row, 3, wxEmptyString);

        updateControls(row);

        if (option == ioFile)
            setStringOption(row, ::wxFileSelector(_("Select a file")));
/*
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
        */
    }

}
