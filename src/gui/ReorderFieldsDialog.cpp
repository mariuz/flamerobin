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

#include <string>

#include "core/ArtProvider.h"
#include "core/URIProcessor.h"
#include "gui/ExecuteSql.h"
#include "gui/GUIURIHandlerHelper.h"
#include "gui/ReorderFieldsDialog.h"
#include "gui/StyleGuide.h"
#include "metadata/column.h"
#include "metadata/database.h"
#include "metadata/MetadataItemURIHandlerHelper.h"
#include "metadata/table.h"

//! included xpm files, so that icons are compiled into executable
namespace reorder_icons {
    #include "up.xpm"
    #include "down.xpm"
};

ReorderFieldsDialog::ReorderFieldsDialog(wxWindow* parent, Table* table)
    : BaseDialog(parent, -1, wxEmptyString)
{
    tableM = table;
    tableM->ensureChildrenLoaded();

    SetTitle(_("Reordering Fields of Table ") + table->getName_());
    createControls();
    layoutControls();
    tableM->attachObserver(this, true);
    button_ok->SetDefault();
}

void ReorderFieldsDialog::createControls()
{
    const wxString fields_choices[] = {
        _("List of fields")
    };
    list_box_fields = new wxListBox(getControlsPanel(), ID_list_box_fields,
        wxDefaultPosition, wxDefaultSize, 1, fields_choices, wxLB_SINGLE);
    wxSize bmpSize(16, 16);
    // TODO: ART_GO_UP_FIRST missing
    button_first = new wxBitmapButton(getControlsPanel(), ID_button_first, wxBitmap(reorder_icons::up_xpm));
    button_up = new wxBitmapButton(getControlsPanel(), ID_button_up,
        wxArtProvider::GetBitmap(wxART_GO_UP, wxART_TOOLBAR, bmpSize));
    button_down = new wxBitmapButton(getControlsPanel(), ID_button_down,
        wxArtProvider::GetBitmap(wxART_GO_DOWN, wxART_TOOLBAR, bmpSize));
    // TODO: ART_GO_DOWN_LAST missing
    button_last = new wxBitmapButton(getControlsPanel(), ID_button_last, wxBitmap(reorder_icons::down_xpm));
    button_ok = new wxButton(getControlsPanel(), wxID_OK, _("Reorder"));
    button_cancel = new wxButton(getControlsPanel(), wxID_CANCEL, _("Cancel"));
}

void ReorderFieldsDialog::layoutControls()
{
    wxBoxSizer* sizerBtns1 = new wxBoxSizer(wxVERTICAL);
    sizerBtns1->Add(0, 0, 1, wxEXPAND);
    sizerBtns1->Add(button_first, 0, wxALIGN_CENTER_HORIZONTAL);
    sizerBtns1->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerBtns1->Add(button_up, 0, wxALIGN_CENTER_HORIZONTAL);
    sizerBtns1->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerBtns1->Add(button_down, 0, wxALIGN_CENTER_HORIZONTAL);
    sizerBtns1->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerBtns1->Add(button_last, 0, wxALIGN_CENTER_HORIZONTAL);
    sizerBtns1->Add(0, 0, 1, wxEXPAND);

    wxBoxSizer* sizerControls = new wxBoxSizer(wxHORIZONTAL);
    sizerControls->Add(list_box_fields, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND);
    sizerControls->Add(styleguide().getUnrelatedControlMargin(wxHORIZONTAL), 0);
    sizerControls->Add(sizerBtns1, 0, wxEXPAND);

    // create sizer for buttons -> styleguide class will align it correctly
    wxSizer* sizerButtons = styleguide().createButtonSizer(button_ok, button_cancel);

    // use method in base class to set everything up
    layoutSizers(sizerControls, sizerButtons, true);
}

const wxString ReorderFieldsDialog::getName() const
{
    return "ReorderFieldsDialog";
}

void ReorderFieldsDialog::moveSelected(int moveby)
{
    int sel = list_box_fields->GetSelection();
    if (sel == -1)
        return;
    int newpos = sel + moveby;
    if (newpos < 0)
        newpos = 0;
    if (newpos >= (int)list_box_fields->GetCount())
        newpos = list_box_fields->GetCount() - 1;
    if (newpos != sel)
    {
        wxString tmp = list_box_fields->GetString(sel);
        list_box_fields->Delete(sel);
        list_box_fields->Insert(tmp, newpos);
        list_box_fields->SetSelection(newpos);
        updateButtons();
    }
}

//! closes window if table is removed (dropped/disconnected,etc.)
void ReorderFieldsDialog::subjectRemoved(Subject* subject)
{
    if (subject == tableM)
        Close();
}

void ReorderFieldsDialog::update()
{
    wxArrayString colNames;
    colNames.Alloc(tableM->getColumnCount());
    ColumnPtrs::const_iterator it;
    for (it = tableM->begin(); it != tableM->end(); ++it)
        colNames.Add((*it)->getName_());
    list_box_fields->Set(colNames);
    updateButtons();
}

void ReorderFieldsDialog::updateButtons()
{
    int sel = list_box_fields->GetSelection();
    button_first->Enable(sel > 0);
    button_up->Enable(sel > 0);
    int itemcnt = list_box_fields->GetCount();
    button_down->Enable(sel >= 0 && sel < itemcnt - 1);
    button_last->Enable(sel >= 0 && sel < itemcnt - 1);
}

const wxString ReorderFieldsDialog::getStatementsToExecute()
{
    wxString sql;
    for (int i = 0; i < (int)list_box_fields->GetCount(); ++i)
    {
        Identifier temp(list_box_fields->GetString(i));
        sql += wxString::Format("ALTER TABLE %s ALTER %s POSITION %d;\n",
            tableM->getQuotedName().c_str(),
            temp.getQuoted().c_str(), i + 1);
    }
    return sql;
}

//! event handling
BEGIN_EVENT_TABLE(ReorderFieldsDialog, BaseDialog)
    EVT_LISTBOX(ReorderFieldsDialog::ID_list_box_fields, ReorderFieldsDialog::OnListBoxSelChange)
    EVT_BUTTON(ReorderFieldsDialog::ID_button_down, ReorderFieldsDialog::OnDownButtonClick)
    EVT_BUTTON(ReorderFieldsDialog::ID_button_first, ReorderFieldsDialog::OnFirstButtonClick)
    EVT_BUTTON(ReorderFieldsDialog::ID_button_last, ReorderFieldsDialog::OnLastButtonClick)
    EVT_BUTTON(ReorderFieldsDialog::ID_button_up, ReorderFieldsDialog::OnUpButtonClick)
END_EVENT_TABLE()

void ReorderFieldsDialog::OnListBoxSelChange(wxCommandEvent& WXUNUSED(event))
{
    updateButtons();
}

void ReorderFieldsDialog::OnDownButtonClick(wxCommandEvent& WXUNUSED(event))
{
    moveSelected(1);
}

void ReorderFieldsDialog::OnFirstButtonClick(wxCommandEvent& WXUNUSED(event))
{
    moveSelected(-(int)list_box_fields->GetCount());
}

void ReorderFieldsDialog::OnLastButtonClick(wxCommandEvent& WXUNUSED(event))
{
    moveSelected(list_box_fields->GetCount());
}

void ReorderFieldsDialog::OnUpButtonClick(wxCommandEvent& WXUNUSED(event))
{
    moveSelected(-1);
}

class ReorderFieldsHandler: public URIHandler,
    private MetadataItemURIHandlerHelper, private GUIURIHandlerHelper
{
public:
    ReorderFieldsHandler() {};
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const ReorderFieldsHandler handlerInstance;
};

const ReorderFieldsHandler ReorderFieldsHandler::handlerInstance;

bool ReorderFieldsHandler::handleURI(URI& uri)
{
    if (uri.action != "reorder_fields")
        return false;

    Table* t = extractMetadataItemFromURI<Table>(uri);
    wxWindow* w = getParentWindow(uri);
    if (!t || !w)
        return true;

    ReorderFieldsDialog rfd(w, t);
    // NOTE: this has been moved here from OnOkButtonClick() to make frame
    //       activation work properly.  Basically activation of another
    //       frame has to happen outside wxDialog::ShowModal(), because it
    //       does at the end re-focus the last focused control, raising
    //       the parent frame over the newly created sql execution frame
    if (rfd.ShowModal() == wxID_OK)
    {
        execSql(w, rfd.GetTitle(), t->getDatabase(),
            rfd.getStatementsToExecute(), true);
    }
    return true;
}

