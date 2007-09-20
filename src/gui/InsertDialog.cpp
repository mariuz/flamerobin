/*
Copyright (c) 2004, 2005, 2006 The FlameRobin Development Team

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

#include <set>

#include "core/FRError.h"
#include "core/StringUtils.h"
#include "metadata/column.h"
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
InsertDialog::InsertDialog(wxWindow* parent, const wxString& tableName,
    DataGridTable *gridTable, IBPP::Statement& st)
    :BaseDialog(parent, -1, wxEmptyString), tableNameM(tableName), bufferM(0),
    gridTableM(gridTable), statementM(st)
{
	flexSizerM = new wxFlexGridSizer( 2, 4, 8, 8 );
	flexSizerM->AddGrowableCol( 3 );
	flexSizerM->SetFlexibleDirection( wxBOTH );

    wxString labels[] = { _("Field name"), _("Data type"), _("Special"),
        _("Value") };
    for (int i=0; i<sizeof(labels)/sizeof(wxString); ++i)
    {
        wxStaticText *st = new wxStaticText(getControlsPanel(), wxID_ANY,
            labels[i]);
        flexSizerM->Add(st);
        wxFont f = st->GetFont();
        f.SetWeight(wxFONTWEIGHT_BOLD);
        st->SetFont(f);
    }

    // TODO: we need a better way to handle this
    // if you add/remove something here, make sure you update the
    // in all the places in this file
    wxString choices[] = {
        _(""), _("NULL"), _("Skip (N/A)"), _("Hexadecimal"), _("Octal"),
        _("CURRENT_DATE"), _("CURRENT_TIME"), _("CURRENT_TIMESTAMP"),
        _("CURRENT_USER"), _("File..."), _("Generator...") };

    // initialize the buffer with NULLs
    bufferM = new DataGridRowBuffer(gridTable->GetNumberCols());

    DataGridTable::FieldSet fields;
    gridTable->getFields(tableName, fields);
    for (DataGridTable::FieldSet::iterator it = fields.begin();
        it != fields.end(); ++it)
    {
        wxStaticText *label1 = new wxStaticText(getControlsPanel(), wxID_ANY,
            (*it).first->getName_());
        flexSizerM->Add(label1, 0, wxALIGN_CENTER_VERTICAL);

        label1 = new wxStaticText(getControlsPanel(), wxID_ANY,
            (*it).first->getDatatype());
        flexSizerM->Add(label1, 0, wxALIGN_CENTER_VERTICAL);

        wxChoice *choice1 = new wxChoice(getControlsPanel(), ID_Choice,
            wxDefaultPosition, wxDefaultSize,
            sizeof(choices)/sizeof(wxString), choices, 0);
        flexSizerM->Add(choice1, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND);

        wxTextCtrl *text1 = new wxTextCtrl(getControlsPanel(), wxID_ANY,
            (*it).first->getDefault(), wxDefaultPosition, wxDefaultSize,
            (*it).second.first->isNumeric() ? wxTE_RIGHT : 0);
        flexSizerM->Add(text1, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND);

        text1->Connect(wxEVT_KILL_FOCUS,
            wxFocusEventHandler(InsertDialog::OnEditFocusLost), 0, this);

        if (!(*it).first->hasDefault())
        {
            if ((*it).first->isNullable())
            {
                choice1->SetStringSelection(wxT("NULL"));
                updateControls(choice1, text1); // disable editing
            }
            else if ((*it).second.first->isNumeric())
                text1->SetValue(wxT("0"));
        }

        InsertColumnInfo ici(choice1, text1, (*it).first, (*it).second.first,
            (*it).second.second);
        columnsM.push_back(ici);

        // TODO: if table has active BEFORE INSERT trigger that depends on:
        // this field and some generator -> activate generator option
    }

    button_ok = new wxButton(getControlsPanel(), wxID_OK, _("&Insert"));
    button_cancel = new wxButton(getControlsPanel(), wxID_CANCEL,
        _("&Cancel"));

    set_properties();
    do_layout();

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
void InsertDialog::updateControls(wxChoice *c, wxTextCtrl *t)
{
    int option = c->GetSelection();
    // _(""), _("NULL"), _("Skip (N/A)"), _("Hexadecimal"), _("Octal"),
    // _("CURRENT_DATE"), _("CURRENT_TIME"), _("CURRENT_TIMESTAMP"),
    // _("CURRENT_USER"), _("File..."), _("Generator...") };
    bool editable = (option == 0 || option == 3 || option == 4);
    if (!editable && option < 9)
        t->SetValue(wxEmptyString);
    t->SetEditable(editable);
    updateColors(this);
}
//-----------------------------------------------------------------------------
//! event handling
BEGIN_EVENT_TABLE(InsertDialog, BaseDialog)
    EVT_BUTTON(wxID_OK, InsertDialog::OnOkButtonClick)
    EVT_CHOICE(InsertDialog::ID_Choice, InsertDialog::OnChoiceChange)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void InsertDialog::OnEditFocusLost(wxFocusEvent& event)
{
    FR_TRY

    // txt control lost the focus, we reformat the value
    wxTextCtrl *tx = dynamic_cast<wxTextCtrl *>(event.GetEventObject());
    if (!tx)
        return;

    std::vector<InsertColumnInfo>::iterator it = columnsM.begin();
    for (; it != columnsM.end(); ++it)
        if ((*it).textCtrl == tx)
            break;
    if (it == columnsM.end())
        return;
    if ((*it).choice->GetSelection() != 0)  // we only care for strings
        return;

    if (tx->GetValue().IsEmpty())   // we assume null for non-string columns
    {
        bufferM->setFieldNull((*it).index, true);
        (*it).choice->SetSelection(1);
        return;
    }

    // write data to buffer and retrieve the formatted value
    bufferM->setFieldNull((*it).index, false);

    wxString previous =  (*it).columnDef->getAsString(bufferM);
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
void InsertDialog::preloadSpecialColumns()
{
    // step 1: build list of CURRENT_* and generator values
    wxString sql(wxT("SELECT "));
    bool first = true;
    for (std::vector<InsertColumnInfo>::iterator it = columnsM.begin();
        it != columnsM.end(); ++it)
    {
        int sel = (*it).choice->GetSelection();
        if (sel < 5 || sel == 9)
            continue;
        if (first)
            first = false;
        else
            sql += wxT(",");
        if (sel == 10) // generator
            sql += wxT("GEN_ID(") + (*it).textCtrl->GetValue() + wxT(", 1)");
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
        int sel = (*it).choice->GetSelection();
        if (sel < 5 || sel == 9)
            continue;
        (*it).columnDef->setValue(bufferM, col++, st1, wxConvCurrent);
        if (sel != 10)  // what follows is only for generators
            continue;
        (*it).textCtrl->SetValue((*it).columnDef->getAsString(bufferM));
        (*it).choice->SetSelection(0);  // treat as regular value
    }
}
//-----------------------------------------------------------------------------
void InsertDialog::OnOkButtonClick(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    preloadSpecialColumns();

    wxString stm = wxT("INSERT INTO ") + tableNameM + wxT(" (");
    wxString val = wxT(") VALUES (");
    bool first = true;
    for (std::vector<InsertColumnInfo>::iterator it = columnsM.begin();
        it != columnsM.end(); ++it)
    {
        int sel = (*it).choice->GetSelection();
        if (sel == 2)   // skip
            continue;

        if (first)
            first = false;
        else
        {
            stm += wxT(",");
            val += wxT(",");
        }
        stm += (*it).column->getQuotedName();
        if (sel == 9)
        {
                // file (we need ? parameters and load content from file)
        }
        else if (sel == 1)  // NULL
            val += wxT("NULL");
        else
            val += wxT("'")+(*it).columnDef->getAsFirebirdString(bufferM)+wxT("'");
    }

    IBPP::Statement st1 = IBPP::StatementFactory(statementM->DatabasePtr(),
        statementM->TransactionPtr());
    stm += val + wxT(")");
    st1->Prepare(wx2std(stm));
    // load blobs here and Set parameters
    st1->Execute();

    // add buffer to the table and set internal buffer marker to zero
    // (to prevent deletion in destructor)
    gridTableM->addRow(bufferM, stm);
    bufferM = 0;

    EndModal(wxID_OK);

    FR_CATCH
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
    int option = event.GetSelection();
    updateControls(c, (*it).textCtrl);

    // _(""), _("NULL"), _("Skip (N/A)"), _("Hexadecimal"), _("Octal"),
    // _("CURRENT_DATE"), _("CURRENT_TIME"), _("CURRENT_TIMESTAMP"),
    // _("CURRENT_USER"), _("File..."), _("Generator...") };
    if (option == 9)
    {
        // select file and store in tx
    }
    if (option == 10)
    {
        // select generator name and store in tx
    }

    FR_CATCH
}
//-----------------------------------------------------------------------------
