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

#include <vector>

#include "core/URIProcessor.h"
#include "gui/CreateIndexDialog.h"
#include "gui/ExecuteSql.h"
#include "gui/GUIURIHandlerHelper.h"
#include "gui/StyleGuide.h"
#include "metadata/column.h"
#include "metadata/database.h"
#include "metadata/MetadataItemURIHandlerHelper.h"
#include "metadata/table.h"

CreateIndexDialog::CreateIndexDialog(wxWindow* parent, Table* table)
    : BaseDialog(parent, -1, wxEmptyString)
{
    // can't do anything if no table is given
    wxASSERT(table);
    tableM = table;

    SetTitle(_("Creating Index for Table ") + table->getName_());
    createControls();
    setControlsProperties();
    layoutControls();
    button_ok->SetDefault();
}

void CreateIndexDialog::createControls()
{
    label_name = new wxStaticText(getControlsPanel(), -1, _("Index name:"));
    textctrl_name = new wxTextCtrl(getControlsPanel(), ID_textcontrol_name,
        wxEmptyString);

    checkbox_unique = new wxCheckBox(getControlsPanel(), ID_check_unique,
        _("Unique index"));

    const wxString orderChoices[] = { _("Ascending"), _("Descending") };
    radiobox_order = new wxRadioBox(getControlsPanel(), ID_radio_order,
        _("Index sort order"), wxDefaultPosition, wxDefaultSize,
        sizeof(orderChoices) / sizeof(wxString), orderChoices);

    label_columns = new wxStaticText(getControlsPanel(), -1,
        _("Select one or more columns for the index:"));;
    listbox_columns = new wxListBox(getControlsPanel(), ID_list_columns,
        wxDefaultPosition, wxDefaultSize, 0, 0, wxLB_MULTIPLE);

    button_ok = new wxButton(getControlsPanel(), wxID_OK, _("Create"));
    button_cancel = new wxButton(getControlsPanel(), wxID_CANCEL, _("Cancel"));
}

void CreateIndexDialog::layoutControls()
{
    wxSizer* sizerName = new wxBoxSizer(wxHORIZONTAL);
    sizerName->Add(label_name, 0, wxALIGN_CENTER_VERTICAL);
    sizerName->AddSpacer(styleguide().getControlLabelMargin());
    sizerName->Add(textctrl_name, 1, wxEXPAND);

    wxSizer* sizerControls = new wxBoxSizer(wxVERTICAL);
    sizerControls->Add(sizerName, 0, wxEXPAND);
    sizerControls->AddSpacer(
        styleguide().getUnrelatedControlMargin(wxVERTICAL));
    sizerControls->Add(checkbox_unique, 0, wxALIGN_TOP | wxEXPAND);
    sizerControls->AddSpacer(
        styleguide().getUnrelatedControlMargin(wxVERTICAL));
    sizerControls->Add(radiobox_order, 0, wxEXPAND);
    sizerControls->AddSpacer(
        styleguide().getUnrelatedControlMargin(wxVERTICAL));
    sizerControls->Add(label_columns, 0, wxEXPAND);
    sizerControls->AddSpacer(
        styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerControls->Add(listbox_columns, 1, wxEXPAND);

    // create sizer for buttons -> styleguide class will align it correctly
    wxSizer* sizerButtons = styleguide().createButtonSizer(button_ok,
        button_cancel);
    // use method in base class to set everything up
    layoutSizers(sizerControls, sizerButtons, true);
}

void CreateIndexDialog::setControlsProperties()
{
    // suggest name for new index
    wxString indexName;
    int nr = 1;

    // Issue #214: include an underscore between the table name and the
    // sequence number so the suggested name matches the convention
    // Firebird itself uses for auto-named PK/FK/UNIQUE/CHECK constraints
    // (e.g. IDX_FB3_TEST_1 instead of IDX_FB3_TEST1).
    //
    // Gemini-flagged caveat: object identifiers are limited to 31 bytes
    // on Firebird < 4.0 (FB 4 raised it to 63). For long table names the
    // extra underscore can push the suggestion over that limit, which
    // would surface as a server-side ALTER error. Truncate the table-name
    // portion of the suggestion if needed so the assembled name still
    // fits the maximum the connected ODS supports. Existing indexes (if
    // the truncated name collides with one) cause the loop below to bump
    // the sequence number, same as before.
    int maxIdLen = 31;
    DatabasePtr db = tableM->getDatabase();
    if (db && db->getInfo().getODSVersionIsHigherOrEqualTo(13, 0))
        maxIdLen = 63;     // Firebird 4+ (ODS 13.0)

    while (indexName.IsEmpty())
    {
        // Reserve room for "IDX_", the underscore, and the sequence digits.
        wxString seq = wxString::Format("%d", nr);
        int reserved = 4 /* "IDX_" */ + 1 /* "_" */ + (int)seq.length();
        wxString tableName = tableM->getName_();
        if ((int)tableName.length() > maxIdLen - reserved)
            tableName = tableName.Left(maxIdLen - reserved);

        indexName = "IDX_" + tableName + "_" + seq;
        nr++;

        std::vector<Index>::iterator itIdx;
        for (itIdx = indices->begin(); itIdx != indices->end(); ++itIdx)
        {
            if ((*itIdx).getName_() == indexName)
            {
                indexName = wxEmptyString;
                break;
            }
        }
    }
    textctrl_name->SetValue(indexName);

    // fill listbox with table column names
    tableM->ensureChildrenLoaded();
    wxArrayString colNames;
    colNames.Alloc(tableM->getColumnCount());
    ColumnPtrs::const_iterator it;
    for (it = tableM->begin(); it != tableM->end(); ++it)
        colNames.Add((*it)->getName_());
    listbox_columns->Set(colNames);
}

const wxString CreateIndexDialog::getName() const
{
    return "CreateIndexDialog";
}

wxString CreateIndexDialog::getSelectedColumnsList()
{
    wxArrayInt selection;
    listbox_columns->GetSelections(selection);

    wxString columnsList;
    for (size_t i = 0; i < selection.size(); i++)
    {
        if (!columnsList.IsEmpty())
            columnsList += ", ";
        Identifier id(listbox_columns->GetString(selection[i]));
        columnsList += id.getQuoted();
    }
    return columnsList;
}

const wxString CreateIndexDialog::getStatementsToExecute()
{
    wxString sql("CREATE ");
    if (checkbox_unique->IsChecked())
        sql += "UNIQUE ";
    if (radiobox_order->GetSelection() == 1)
        sql += "DESCENDING ";
    sql += "INDEX " + Identifier::userString(textctrl_name->GetValue())
        + " ON " + tableM->getQuotedName() + "\n"
        + "  (" + getSelectedColumnsList() + ");\n";
    return sql;
}

void CreateIndexDialog::updateButtons()
{
    bool ok = !textctrl_name->GetValue().IsEmpty();
    if (ok)
    {
        wxArrayInt selectedColumns;
        listbox_columns->GetSelections(selectedColumns);
        ok = selectedColumns.size() > 0;
    }
    button_ok->Enable(ok);
}

//! event handling
BEGIN_EVENT_TABLE(CreateIndexDialog, BaseDialog)
    EVT_LISTBOX(CreateIndexDialog::ID_list_columns, CreateIndexDialog::OnControlChange)
    EVT_TEXT(CreateIndexDialog::ID_textcontrol_name, CreateIndexDialog::OnControlChange)
END_EVENT_TABLE()

void CreateIndexDialog::OnControlChange(wxCommandEvent& WXUNUSED(event))
{
    updateButtons();
}

class TableIndicesHandler: public URIHandler,
    private MetadataItemURIHandlerHelper, private GUIURIHandlerHelper
{
public:
    TableIndicesHandler() {};
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const TableIndicesHandler handlerInstance;
};

const TableIndicesHandler TableIndicesHandler::handlerInstance;

bool TableIndicesHandler::handleURI(URI& uri)
{
    if (uri.action != "add_index" && uri.action != "recompute_all")
        return false;

    Table* t = extractMetadataItemFromURI<Table>(uri);
    wxWindow* w = getParentWindow(uri);
    if (!t || !w)
        return true;

    wxString sql;
    wxString frameCaption;
    if (uri.action == "recompute_all")
    {
        std::vector<Index>* indices = t->getIndices();
        std::vector<Index>::iterator itIdx;
        for (itIdx = indices->begin(); itIdx != indices->end(); ++itIdx)
        {
            sql += wxString::Format("SET STATISTICS INDEX %s;\n",
                (*itIdx).getQuotedName().c_str());
        }
        frameCaption = _("Recompute All Indexes");
    }
    else // add_index
    {
        CreateIndexDialog cid(w, t);
        // NOTE: this has been moved here from OnOkButtonClick() to make frame
        //       activation work properly.  Basically activation of another
        //       frame has to happen outside wxDialog::ShowModal(), because it
        //       does at the end re-focus the last focused control, raising
        //       the parent frame over the newly created sql execution frame
        if (cid.ShowModal() == wxID_OK)
        {
            sql = cid.getStatementsToExecute();
            frameCaption = cid.GetTitle();
        }
    }
    if (!sql.IsEmpty())
        execSql(w, frameCaption, t->getDatabase(), sql, true);
    return true;
}

