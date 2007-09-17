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

#include "metadata/column.h"
#include "gui/controls/DataGridTable.h"
#include "gui/StyleGuide.h"
#include "gui/InsertDialog.h"
//-----------------------------------------------------------------------------
InsertDialog::InsertDialog(wxWindow* parent, const wxString& tableName,
    DataGridTable *gridTable):
    BaseDialog(parent, -1, wxEmptyString), tableNameM(tableName)
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

    // if you add/remove something here, make sure you update the
    // OnChoiceChange event handler and updateControls function
    wxString choices[] = {
        _(""), _("NULL"), _("Skip (N/A)"), _("Hexadecimal"), _("Octal"),
        _("CURRENT_DATE"), _("CURRENT_TIME"), _("CURRENT_TIMESTAMP"),
        _("CURRENT_USER"), _("File..."), _("Generator...") };

    std::set<Column *> fields;
    gridTable->getFields(tableName, fields);
    for (std::set<Column *>::iterator it = fields.begin(); it != fields.end();
        ++it)
    {
        wxStaticText *label1 = new wxStaticText(getControlsPanel(), wxID_ANY,
            (*it)->getName_());
        flexSizerM->Add(label1, 0, wxALIGN_CENTER_VERTICAL);

        label1 = new wxStaticText(getControlsPanel(), wxID_ANY,
            (*it)->getDatatype());
        flexSizerM->Add(label1, 0, wxALIGN_CENTER_VERTICAL);

        wxChoice *choice1 = new wxChoice(getControlsPanel(), ID_Choice,
            wxDefaultPosition, wxDefaultSize,
            sizeof(choices)/sizeof(wxString), choices, 0);
        flexSizerM->Add(choice1, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND);

        wxTextCtrl *text1 = new wxTextCtrl(getControlsPanel(), wxID_ANY,
            (*it)->getDefault());
        flexSizerM->Add(text1, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND);

        relationsM[choice1] = text1;
        if (!(*it)->hasDefault() && (*it)->isNullable())
        {
            choice1->SetStringSelection(wxT("NULL"));
            updateControls(choice1, text1); // disable editing
        }

        // TODO: if table has active BEFORE INSERT trigger that depends on:
        // this field and some generator -> activate generator option
    }

    button_ok = new wxButton(getControlsPanel(), wxID_OK,
        _("&Insert"));
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
    if (!editable)
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
void InsertDialog::OnOkButtonClick(wxCommandEvent& WXUNUSED(event))
{
    // do some stuff & if all ok:
    EndModal(wxID_OK);
}
//-----------------------------------------------------------------------------
void InsertDialog::OnChoiceChange(wxCommandEvent& event)
{
    wxChoice *c = dynamic_cast<wxChoice *>(event.GetEventObject());
    if (!c || relationsM.find(c) == relationsM.end())
        return;
    int option = event.GetSelection();
    wxTextCtrl *tx = relationsM[c];
    updateControls(c, tx);

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
}
//-----------------------------------------------------------------------------
