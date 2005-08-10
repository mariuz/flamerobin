/*
  The contents of this file are subject to the Initial Developer's Public
  License Version 1.0 (the "License"); you may not use this file except in
  compliance with the License. You may obtain a copy of the License here:
  http://www.flamerobin.org/license.html.

  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.

  The Original Code is FlameRobin (TM).

  The Initial Developer of the Original Code is Milan Babuskov.

  Portions created by the original developer
  are Copyright (C) 2004 Milan Babuskov.

  All Rights Reserved.

  Contributor(s): Michael Hieke
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

#include <string>
#include "styleguide.h"
#include "ugly.h"
#include "urihandler.h"
#include "ExecuteSqlFrame.h"
#include "ReorderFieldsDialog.h"

//! included xpm files, so that icons are compiled into executable
namespace reorder_icons {
    #include "up.xpm"
    #include "down.xpm"
};
//-----------------------------------------------------------------------------
ReorderFieldsDialog::ReorderFieldsDialog(wxWindow* parent, Table *table):
    BaseDialog(parent, -1, wxEmptyString)
{
	tableM = table;
	tableM->checkAndLoadColumns();
	tableM->attach(this);

    const wxString fields_choices[] = {
        _("List of fields")
    };
    list_box_fields = new wxListBox(getControlsPanel(), ID_list_box_fields, wxDefaultPosition,
        wxDefaultSize, 1, fields_choices, wxLB_SINGLE);
    button_first = new wxBitmapButton(getControlsPanel(), ID_button_first, wxBitmap(reorder_icons::up_xpm));
    button_up = new wxBitmapButton(getControlsPanel(), ID_button_up, wxBitmap(reorder_icons::up_xpm));
    button_down = new wxBitmapButton(getControlsPanel(), ID_button_down, wxBitmap(reorder_icons::down_xpm));
    button_last = new wxBitmapButton(getControlsPanel(), ID_button_last, wxBitmap(reorder_icons::down_xpm));
    button_ok = new wxButton(getControlsPanel(), ID_button_ok, _("Reorder"));
    button_cancel = new wxButton(getControlsPanel(), ID_button_cancel, _("Cancel"));

    set_properties();
    do_layout();

    SetTitle(_("Reordering fields of table ") + std2wx(table->getName()));
	update();
}
//-----------------------------------------------------------------------------
//! implementation details
void ReorderFieldsDialog::do_layout()
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
//-----------------------------------------------------------------------------
const std::string ReorderFieldsDialog::getName() const
{
    return "ReorderFieldsDialog";
}
//-----------------------------------------------------------------------------
void ReorderFieldsDialog::moveSelected(int moveby)
{
	int sel = list_box_fields->GetSelection();
    if (sel == -1)
        return;
    int newpos = sel + moveby;
    if (newpos < 0)
        newpos = 0;
    if (newpos >= list_box_fields->GetCount())
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
//-----------------------------------------------------------------------------
//! closes window if table is removed (dropped/disconnected,etc.)
void ReorderFieldsDialog::removeObservedObject(Subject *object)
{
	Observer::removeObservedObject(object);
	if (object == tableM)
		Close();
}
//-----------------------------------------------------------------------------
void ReorderFieldsDialog::set_properties()
{
    button_ok->SetDefault();
}
//-----------------------------------------------------------------------------
void ReorderFieldsDialog::update()
{
	std::vector<MetadataItem *> temp;
	tableM->getChildren(temp);

	list_box_fields->Clear();
    for (std::vector<MetadataItem *>::iterator it = temp.begin(); it != temp.end(); ++it)
		list_box_fields->Append(std2wx((*it)->getName()));
    updateButtons();
}
//-----------------------------------------------------------------------------
void ReorderFieldsDialog::updateButtons()
{
	int sel = list_box_fields->GetSelection();
    button_first->Enable(sel > 0);
    button_up->Enable(sel > 0);
    int itemcnt = list_box_fields->GetCount();
    button_down->Enable(sel >= 0 && sel < itemcnt - 1);
    button_last->Enable(sel >= 0 && sel < itemcnt - 1);
}
//-----------------------------------------------------------------------------
//! event handling
BEGIN_EVENT_TABLE(ReorderFieldsDialog, BaseDialog)
    EVT_LISTBOX(ReorderFieldsDialog::ID_list_box_fields, ReorderFieldsDialog::OnListBoxSelChange)
	EVT_BUTTON(ReorderFieldsDialog::ID_button_down, ReorderFieldsDialog::OnDownButtonClick)
	EVT_BUTTON(ReorderFieldsDialog::ID_button_first, ReorderFieldsDialog::OnFirstButtonClick)
	EVT_BUTTON(ReorderFieldsDialog::ID_button_last, ReorderFieldsDialog::OnLastButtonClick)
	EVT_BUTTON(ReorderFieldsDialog::ID_button_up, ReorderFieldsDialog::OnUpButtonClick)
	EVT_BUTTON(ReorderFieldsDialog::ID_button_ok, ReorderFieldsDialog::OnOkButtonClick)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void ReorderFieldsDialog::OnListBoxSelChange(wxCommandEvent& WXUNUSED(event))
{
    updateButtons();
}
//-----------------------------------------------------------------------------
void ReorderFieldsDialog::OnOkButtonClick(wxCommandEvent& WXUNUSED(event))
{
	wxString sql;
	for (int i=0; i<list_box_fields->GetCount(); ++i)
    {
		sql += wxT("ALTER TABLE ") + std2wx(tableM->getName()) + wxT(" ALTER ")
			+ list_box_fields->GetString(i) + wxT(" POSITION ") + wxString::Format(wxT("%d"), i+1) + wxT(";\n");
    }

	// create ExecuteSqlFrame with option to close at once
    ExecuteSqlFrame *eff = new ExecuteSqlFrame(GetParent(), -1, _("Executing change script"));
	eff->setDatabase(tableM->getDatabase());
	eff->setSql(sql);
	Close();
	eff->executeAllStatements(true);		// true = user must commit/rollback + frame is closed at once
	eff->Show();
}
//-----------------------------------------------------------------------------
void ReorderFieldsDialog::OnDownButtonClick(wxCommandEvent& WXUNUSED(event))
{
    moveSelected(1);
}
//-----------------------------------------------------------------------------
void ReorderFieldsDialog::OnFirstButtonClick(wxCommandEvent& WXUNUSED(event))
{
    moveSelected(-list_box_fields->GetCount());
}
//-----------------------------------------------------------------------------
void ReorderFieldsDialog::OnLastButtonClick(wxCommandEvent& WXUNUSED(event))
{
    moveSelected(list_box_fields->GetCount());
}
//-----------------------------------------------------------------------------
void ReorderFieldsDialog::OnUpButtonClick(wxCommandEvent& WXUNUSED(event))
{
    moveSelected(-1);
}
//-----------------------------------------------------------------------------
class ReorderFieldsHandler: public URIHandler
{
public:
	bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const ReorderFieldsHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const ReorderFieldsHandler ReorderFieldsHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool ReorderFieldsHandler::handleURI(URI& uri)
{
	if (uri.action != "reorder_fields")
		return false;

	Table *t = (Table *)getObject(uri);
	wxWindow *w = getWindow(uri);
	if (!t || !w)
		return true;

	ReorderFieldsDialog rfd(w, t);
	rfd.ShowModal();
	return true;
}
//-----------------------------------------------------------------------------
