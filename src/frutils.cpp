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

  The Initial Developer of the Original Code is Nando Dessena.

  Portions created by the original developer
  are Copyright (C) 2004 Nando Dessena.

  All Rights Reserved.

  $Id$

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

#include "dberror.h"
#include "frutils.h"
#include "gui/ProgressDialog.h"
#include "metadata/database.h"
#include "metadata/metadataitem.h"
#include "metadata/table.h"
#include "ugly.h"
//-----------------------------------------------------------------------------
using namespace std;
//-----------------------------------------------------------------------------
void adjustControlsMinWidth(list<wxWindow*> controls)
{
    int w = 0;
    wxSize sz;
    // find widest control
    for (list<wxWindow*>::iterator it = controls.begin(); it != controls.end(); ++it)
    {
        wxASSERT(*it != 0);
        sz = (*it)->GetSize();
        w = max(w, sz.GetWidth());
    }
    // set minimum width of all controls
    for (list<wxWindow*>::iterator it = controls.begin(); it != controls.end(); ++it)
    {
        sz = (*it)->GetSize();
        (*it)->SetSize(w, sz.GetHeight());
        (*it)->SetSizeHints(w, sz.GetHeight());
    }
}
//-----------------------------------------------------------------------------
void readBlob(IBPP::Statement& st, int column, wxString& result)
{
    result = wxT("");
    if (st->IsNull(column))
        return;

    IBPP::Blob b = IBPP::BlobFactory(st->Database(), st->Transaction());
    st->Get(column, b);

    try              // if blob is empty the exception is thrown
    {                // I tried to check st1->IsNull(1) but it doesn't work
        b->Open();   // to this hack is the only way (for the time being)
    }
    catch (...)
    {
        return;
    }

    std::string resultBuffer;
    char readBuffer[8192];        // 8K block
    while (true)
    {
        int size = b->Read(readBuffer, 8192);
        if (size <= 0)
            break;
        readBuffer[size] = 0;
        resultBuffer += readBuffer;
    }
    result = std2wx(resultBuffer);
    b->Close();
}
//-----------------------------------------------------------------------------
wxString selectTableColumns(Table* t, wxWindow* parent)
{
    vector<wxString> list;
    selectTableColumnsIntoVector(t, parent, list);
    wxString retval;
    for (vector<wxString>::iterator it = list.begin(); it != list.end(); ++it)
    {
        if (it != list.begin())
            retval += wxT(", ");
        retval += (*it);
    }
    return retval;
}
//-----------------------------------------------------------------------------
bool selectTableColumnsIntoVector(Table* t, wxWindow* parent, vector<wxString>& list)
{
    t->checkAndLoadColumns();
    vector<MetadataItem*> temp;
    t->getChildren(temp);
    wxArrayString columns;
    for (vector<MetadataItem*>::const_iterator it = temp.begin(); it != temp.end(); ++it)
        columns.Add((*it)->getName_());

    wxArrayInt selected_columns;
    if (!::wxGetMultipleChoices(selected_columns, _("Select one or more fields... (use ctrl key)"),  _("Table fields"), columns, parent))
        return false;

    for (size_t i = 0; i < selected_columns.GetCount(); ++i)
    {
        Identifier temp(columns[selected_columns[i]]);
        list.push_back(temp.getQuoted());
    }

    return true;
}
//-----------------------------------------------------------------------------
void reportLastError(const wxString& actionMsg)
{
    wxMessageBox(lastError().getMessage(), actionMsg, wxOK | wxICON_ERROR);
}
//-----------------------------------------------------------------------------
bool connectDatabase(Database *db, wxWindow* parent,
    ProgressDialog* progressdialog)
{
    wxString pass;
    if (db->getPassword().empty())
    {
        wxString message(_("Enter password for user: "));
        message += db->getUsername();
        pass = ::wxGetPasswordFromUser(message, _("Connecting to database"));
        if (pass.IsEmpty())
            return false;
    }
    else
        pass = db->getPassword();

    wxString caption(wxString::Format(wxT("Connecting with Database \"%s\""),
        db->getName_().c_str()));
    bool ok;
    if (progressdialog)
    {
        progressdialog->setProgressMessage(caption);
        ok = db->connect(pass, progressdialog);
    }
    else
    {
        ProgressDialog pd(parent, caption, 1);
        ok = db->connect(pass, &pd);;
    }
    if (!ok)
    {
        reportLastError(_("Error Connecting to Database"));
        return false;
    }
    return true;
}
//-----------------------------------------------------------------------------
