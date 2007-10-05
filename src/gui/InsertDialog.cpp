/*
Copyright (c) 2004-2007 The FlameRobin Development Team

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
#include <set>

#include "core/FRError.h"
#include "core/StringUtils.h"
#include "metadata/column.h"
#include "metadata/database.h"
#include "gui/controls/DataGridRowBuffer.h"
#include "gui/controls/DataGridTable.h"
#include "gui/StyleGuide.h"
#include "gui/InsertDialog.h"
/*
    The dialog creates a DataGridRowBuffer, and uses ResultsetColumnDefs from
    the active grid to format the values. I.e. new potential row is created
    and filled with values as the user types them.

    When all values are entered, we try to INSERT into database, and if all
    goes well, the prepared row buffer is simply added to the grid.
*/
//-----------------------------------------------------------------------------
namespace InsertOptions
{
    // make sure you keep these two in sync if you add/remove items
    const wxString insertOptionStrings[] = {
        wxT(""),
        wxT("NULL"),
        wxTRANSLATE("Skip (N/A)"),
        wxTRANSLATE("Column default"),
        wxTRANSLATE("Hexadecimal"),
        wxT("CURRENT_DATE"),
        wxT("CURRENT_TIME"),
        wxT("CURRENT_TIMESTAMP"),
        wxT("CURRENT_USER"),
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
};
using namespace InsertOptions;
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
InsertDialog::InsertDialog(wxWindow* parent, const wxString& tableName,
    DataGridTable *gridTable, IBPP::Statement& st, Database *db)
    :BaseDialog(parent, -1, wxEmptyString), tableNameM(tableName), bufferM(0),
    gridTableM(gridTable), statementM(st), databaseM(db)
{
    flexSizerM = new wxFlexGridSizer( 2, 4, 8, 8 );
    flexSizerM->AddGrowableCol( 3 );
    flexSizerM->SetFlexibleDirection( wxBOTH );

    wxString labels[] = {
        wxTRANSLATE("Field name"), wxTRANSLATE("Data type"),
        wxTRANSLATE("Special"),    wxTRANSLATE("Value") };
    for (int i=0; i<sizeof(labels)/sizeof(wxString); ++i)
    {
        wxStaticText *st = new wxStaticText(getControlsPanel(), wxID_ANY,
            labels[i]);
        flexSizerM->Add(st);
        wxFont f = st->GetFont();
        f.SetWeight(wxFONTWEIGHT_BOLD);
        st->SetFont(f);
    }

    bufferM = new InsertedGridRowBuffer(gridTable->GetNumberCols());
    for (unsigned u = 0; (int)u < gridTable->GetNumberCols(); u++)
        bufferM->setFieldNA(u, true);

    DataGridTable::FieldSet fields;
    gridTable->getFields(tableName, fields);

    // preload triggers
    std::vector<Trigger *> triggers;
    if (!fields.empty()) // not empty
    {
        Table *t = (*(fields.begin())).second.second->getTable();
        t->getTriggers(triggers, Trigger::beforeTrigger);
    }

    for (DataGridTable::FieldSet::iterator it = fields.begin();
        it != fields.end(); ++it)
    {
        Column *c = (*it).second.second;
        ResultsetColumnDef *def = (*it).second.first;
        wxStaticText *label1 = new wxStaticText(getControlsPanel(), wxID_ANY,
            c->getName_());
        flexSizerM->Add(label1, 0, wxALIGN_CENTER_VERTICAL);

        label1 = new wxStaticText(getControlsPanel(), wxID_ANY,
            c->getDatatype());
        flexSizerM->Add(label1, 0, wxALIGN_CENTER_VERTICAL);

        wxChoice *choice1 = new wxChoice(getControlsPanel(), ID_Choice,
            wxDefaultPosition, wxDefaultSize, sizeof(insertOptionStrings)
                / sizeof(wxString), insertOptionStrings, 0);
        flexSizerM->Add(choice1, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND);
        // wxWidgets seem to default to -1 for some reason
        choice1->SetSelection(ioRegular);

        wxTextCtrl *text1 = new wxTextCtrl(getControlsPanel(), ID_TextCtrl,
            c->getDefault(), wxDefaultPosition, wxDefaultSize,
            def->isNumeric() ? wxTE_RIGHT : 0);
        flexSizerM->Add(text1, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND);

        text1->Connect(wxEVT_KILL_FOCUS,
            wxFocusEventHandler(InsertDialog::OnEditFocusLost), 0, this);
        text1->Connect(wxEVT_SET_FOCUS,
            wxFocusEventHandler(InsertDialog::OnEditFocusSet), 0, this);

        if (c->hasDefault())
        {
            choice1->SetSelection(ioDefault);
            updateControls(choice1, text1);
        }
        else
        {
            if (c->isNullable())
            {
                choice1->SetSelection(ioNull);
                updateControls(choice1, text1);
            }
            else if (def->isNumeric())
                text1->SetValue(wxT("0"));
        }

        InsertColumnInfo ici(choice1, text1, c, def, (*it).first);
        columnsM.push_back(ici);

        Generator *gen = findAutoincGenerator(triggers, c);
        if (gen)
        {
            choice1->SetSelection(ioGenerator);
            updateControls(choice1, text1);
            text1->SetValue(gen->getQuotedName());
        }
    }

    button_ok = new wxButton(getControlsPanel(), wxID_OK, _("&Insert"));
    button_cancel = new wxButton(getControlsPanel(), wxID_CANCEL,
        _("&Cancel"));

    set_properties();
    do_layout();
    if (columnsM.size())
        columnsM[0].choice->SetFocus();

    // TODO: if dialog height is greater than screen height: reduce to 80%
    //       (it should show scrollbars?)

    // until we find something better
    #include "trigger32.xpm"
    wxBitmap bmp = wxBitmap(trigger_xpm);
    wxIcon icon;
    icon.CopyFromBitmap(bmp);
    SetIcon(icon);
}
//-----------------------------------------------------------------------------
InsertDialog::~InsertDialog()
{
    delete bufferM;
}
//-----------------------------------------------------------------------------
void InsertDialog::set_properties()
{
    SetTitle(_("Insert into ") + tableNameM);
    button_ok->SetDefault();
}
//-----------------------------------------------------------------------------
void InsertDialog::do_layout()
{
    wxBoxSizer* sizerControls = new wxBoxSizer(wxVERTICAL);
    sizerControls->Add(flexSizerM, 0, wxEXPAND, 0);

    wxSizer* sizerButtons =
        styleguide().createButtonSizer(button_ok, button_cancel);
    layoutSizers(sizerControls, sizerButtons);
}
//-----------------------------------------------------------------------------
const wxString InsertDialog::getName() const
{
    return wxT("InsertDialog");
}
//-----------------------------------------------------------------------------
bool InsertDialog::getConfigStoresWidth() const
{
    return true;
}
//-----------------------------------------------------------------------------
bool InsertDialog::getConfigStoresHeight() const
{
    return false;
}
//-----------------------------------------------------------------------------
void InsertDialog::updateControls(wxChoice *c, wxTextCtrl *tx)
{
    InsertOption ix = (InsertOption)(c->GetSelection());
    if (!optionAllowsCustomValue(ix))
        tx->SetValue(wxEmptyString);
    tx->SetEditable(optionIsEditable(ix));
    updateColors(this);
    if (ix == ioNull)
    {
        tx->SetForegroundColour(*wxRED);
        tx->SetValue(wxT("[null]"));
    }
    else
    {
        tx->SetForegroundColour(
            wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    }
}
//-----------------------------------------------------------------------------
void InsertDialog::storeValues()
{
    for (std::vector<InsertColumnInfo>::iterator it = columnsM.begin();
        it != columnsM.end(); ++it)
    {
        InsertOption sel = (InsertOption)((*it).choice->GetSelection());
        wxString previous = (*it).columnDef->getAsString(bufferM);
        wxString value = (*it).textCtrl->GetValue();
        try
        {
            if (sel == ioHex)
            {
                if ((*it).columnDef->isNumeric())   // hex -> dec
                {
                    long l;                     // should we support 64bit?
                    if (!value.ToLong(&l, 16))
                        throw FRError(_("Invalid hexadecimal number"));
                    value.Printf(wxT("%d"), l);
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
                (*it).choice->SetSelection(ioRegular);
                sel = ioRegular;
            }
            switch (sel)
            {
                case ioRegular:
                    (*it).columnDef->setFromString(bufferM, value);
                    (*it).textCtrl->SetValue((*it)
                        .columnDef->getAsString(bufferM));
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
            (*it).textCtrl->SetValue(previous);
            throw;
        }
    }
}
//-----------------------------------------------------------------------------
void InsertDialog::preloadSpecialColumns()
{
    // step 1: build list of CURRENT_* and generator values
    wxString sql(wxT("SELECT "));
    bool first = true;
    for (std::vector<InsertColumnInfo>::iterator it = columnsM.begin();
        it != columnsM.end(); ++it)
    {
        InsertOption sel = (InsertOption)((*it).choice->GetSelection());
        if (!optionValueLoadedFromDatabase(sel))
            continue;
        if (first)
            first = false;
        else
            sql += wxT(",");
        if (sel == ioGenerator) // generator
            sql += wxT("GEN_ID(") + (*it).textCtrl->GetValue() + wxT(", 1)");
        else if (sel == ioDefault)
        {
            if (!(*it).column->isString())
                sql += wxT("CAST(");
            sql += (*it).textCtrl->GetValue();
            if (!(*it).column->isString())
            {   // false = no custom formatting, just the pure type
                sql += wxT(" AS ") + (*it).column->getDatatype(false)
                    + wxT(")");
            }
        }
        else
            sql += (*it).choice->GetStringSelection();
    }

    // step 2: load those from the database
    IBPP::Statement st1 = IBPP::StatementFactory(statementM->DatabasePtr(),
        statementM->TransactionPtr());
    if (!first) // we do need some data
    {
        sql += wxT(" FROM RDB$DATABASE");
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
        InsertOption sel = (InsertOption)((*it).choice->GetSelection());
        if (!optionValueLoadedFromDatabase(sel))
            continue;
        bufferM->setFieldNA((*it).index, false);
        bufferM->setFieldNull((*it).index, st1->IsNull(col));
        if (!st1->IsNull(col))
            (*it).columnDef->setValue(bufferM, col++, st1, wxConvCurrent);
        if (sel != ioGenerator)  // what follows is only for generators
            continue;
        (*it).textCtrl->SetValue((*it).columnDef->getAsString(bufferM));
        (*it).choice->SetSelection(ioRegular);  // treat as regular value
        updateControls((*it).choice, (*it).textCtrl);
    }
}
//-----------------------------------------------------------------------------
//! event handling
BEGIN_EVENT_TABLE(InsertDialog, BaseDialog)
    EVT_BUTTON(wxID_OK, InsertDialog::OnOkButtonClick)
    EVT_CHOICE(InsertDialog::ID_Choice, InsertDialog::OnChoiceChange)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
// clear the text from NULL field when focused
void InsertDialog::OnEditFocusSet(wxFocusEvent& event)
{
    FR_TRY

    wxTextCtrl *tx = dynamic_cast<wxTextCtrl *>(event.GetEventObject());
    if (!tx)
        return;

    std::vector<InsertColumnInfo>::iterator it = columnsM.begin();
    for (; it != columnsM.end(); ++it)
        if ((*it).textCtrl == tx)
            break;
    if (it == columnsM.end())   // this should never happen
        return;
    if ((*it).choice->GetSelection() == ioNull)
    {
        tx->SetForegroundColour(
            wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
        tx->SetValue(wxEmptyString);
    }

    FR_CATCH
}
//-----------------------------------------------------------------------------
void InsertDialog::OnEditFocusLost(wxFocusEvent& event)
{
    FR_TRY

    wxTextCtrl *tx = dynamic_cast<wxTextCtrl *>(event.GetEventObject());
    if (!tx)
        return;

    std::vector<InsertColumnInfo>::iterator it = columnsM.begin();
    for (; it != columnsM.end(); ++it)
        if ((*it).textCtrl == tx)
            break;
    if (it == columnsM.end())   // this should never happen
        return;
    if ((*it).choice->GetSelection() != ioNull &&
        (*it).choice->GetSelection() != ioRegular)
    {
        return;
    }
    if (tx->GetValue().IsEmpty())   // we assume null for non-string columns
    {
        // here we can change the NULL behaviour for string columns, i.e.
        // whether empty field is NULL or ''
        // if ((*it).column->isString() ...
        bufferM->setFieldNull((*it).index, true);
        (*it).choice->SetSelection(ioNull);
        updateControls((*it).choice, (*it).textCtrl);
        return;
    }

    // write data to buffer and retrieve the formatted value
    (*it).choice->SetSelection(ioRegular);
    updateControls((*it).choice, (*it).textCtrl);
    bufferM->setFieldNull((*it).index, false);

    wxString previous = (*it).columnDef->getAsString(bufferM);
    try
    {
        (*it).columnDef->setFromString(bufferM, tx->GetValue());
        tx->SetValue((*it).columnDef->getAsString(bufferM));
    }
    catch(...)
    {
        tx->SetValue(previous);     // we take the old value from buffer

        // This is commented out as it doesn't work nice:
        // 1. there is reentrancy problem (easily fixed with some bool flag)
        // 2. user might want to simply close the dialog, changing the
        //    focus here prevents that (at least with wxGTK 2.8.4)
        // If you know a way around that, uncomment these 2 lines:
        //tx->SetFocus();
        //tx->SetSelection(-1, -1);   // select all

        throw;
    }

    FR_CATCH
}
//-----------------------------------------------------------------------------
void InsertDialog::OnOkButtonClick(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    storeValues();
    preloadSpecialColumns();

    wxString stm = wxT("INSERT INTO ") + tableNameM + wxT(" (");
    wxString val = wxT(") VALUES (");
    bool first = true;
    for (std::vector<InsertColumnInfo>::iterator it = columnsM.begin();
        it != columnsM.end(); ++it)
    {
        InsertOption sel = (InsertOption)((*it).choice->GetSelection());
        if (sel == ioSkip)   // skip
            continue;

        if (first)
            first = false;
        else
        {
            stm += wxT(",");
            val += wxT(",");
        }
        stm += (*it).column->getQuotedName();
        if (sel == ioFile)
            val += wxT("?");
        else if (sel == ioNull)  // NULL
            val += wxT("NULL");
        else
        {
            val += wxT("'") + (*it).columnDef->getAsFirebirdString(bufferM)
                + wxT("'");
        }
    }

    IBPP::Statement st1 = IBPP::StatementFactory(statementM->DatabasePtr(),
        statementM->TransactionPtr());
    stm += val + wxT(")");
    st1->Prepare(wx2std(stm));

    // load blobs
    int index = 1;
    for (std::vector<InsertColumnInfo>::iterator it = columnsM.begin();
        it != columnsM.end(); ++it)
    {
        if ((InsertOption)((*it).choice->GetSelection()) != ioFile)
            continue;
        wxFFile fl((*it).textCtrl->GetValue(), wxT("rb"));
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
    bufferM = 0;

    EndModal(wxID_OK);

    FR_CATCH
}
//-----------------------------------------------------------------------------
// helper function for OnChoiceChange
void InsertDialog::setStringOption(InsertColumnInfo& ici, const wxString& s)
{
    if (!s.IsEmpty())
        ici.textCtrl->SetValue(s);
    else
    {
        ici.choice->SetSelection(ioNull);
        updateControls(ici.choice, ici.textCtrl);
    }
}
//-----------------------------------------------------------------------------
void InsertDialog::OnChoiceChange(wxCommandEvent& event)
{
    FR_TRY

    wxChoice *c = dynamic_cast<wxChoice *>(event.GetEventObject());
    if (!c)
        return;
    // find related text control
    std::vector<InsertColumnInfo>::iterator it = columnsM.begin();
    for (; it != columnsM.end(); ++it)
        if ((*it).choice == c)
            break;

    InsertOption option = (InsertOption)(event.GetSelection());
    if (optionIsEditable(option))
        (*it).textCtrl->Clear();

    updateControls(c, (*it).textCtrl);

    if (option == ioFile)
        setStringOption(*it, ::wxFileSelector(_("Select a file")));

    if (option == ioDefault)
        setStringOption(*it, (*it).column->getDefault());

    if (option == ioGenerator)
    {
        // select generator name and store in tx
        wxArrayString as;
        for (MetadataCollection<Generator>::const_iterator ci =
            databaseM->generatorsBegin(); ci != databaseM->generatorsEnd();
            ++ci)
        {
            as.Add((*ci).getQuotedName());
        }
        wxString s(::wxGetSingleChoice(_("Select a generator"),
            _("Generator"), as, this));
        setStringOption(*it, s);
    }

    FR_CATCH
}
//-----------------------------------------------------------------------------
