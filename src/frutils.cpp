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

#include "dberror.h"
#include "frutils.h"
#include "gui/ProgressDialog.h"
#include "metadata/database.h"
#include "metadata/metadataitem.h"
#include "metadata/table.h"
#include "metadata/server.h"
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

    IBPP::Blob b = IBPP::BlobFactory(st->DatabasePtr(), st->TransactionPtr());
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
    if (db->getDecryptedPassword().empty())
    {
        wxString message(_("Enter password for user: "));
        message += db->getUsername();
        pass = ::wxGetPasswordFromUser(message, _("Connecting to database"));
        if (pass.IsEmpty())
            return false;
    }
    else
        pass = db->getDecryptedPassword();

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
bool getService(Server* s, IBPP::Service& svc, ProgressIndicator* p,
    bool sysdba)
{
    if (!s->getService(svc, p, sysdba))
    {
        wxString msg;
        if (p->isCanceled())
            msg = _("You've canceled the search for a usable username and password.");
        else
            msg = _("None of the credentials of the databases could be used.");
        if (sysdba)
            msg << _("\nYou need to supply a valid password for SYSDBA.");
        else
            msg << _("\nYou need to supply a valid username and password.");
        wxMessageBox(msg, _("Connecting to server"), wxOK|wxICON_INFORMATION);
        wxString user(wxT("SYSDBA"));
        if (!sysdba)
        {
            user = ::wxGetTextFromUser(_("Connecting to server"), _("Enter username"));
            if (user.IsEmpty())
                return false;
        }
        wxString pass = ::wxGetPasswordFromUser(_("Connecting to server"),
            sysdba ? _("Enter SYSDBA password") : _("Enter password"));
        if (pass.IsEmpty())
            return false;
        try
        {
            svc = IBPP::ServiceFactory(wx2std(s->getConnectionString()),
                wx2std(user), wx2std(pass));
            svc->Connect();
            // exception might be thrown. If not, we store the credentials:
            if (sysdba || user.Upper() == wxT("SYSDBA"))
                s->setServiceSysdbaPassword(pass);
            else
            {
                s->setServiceUser(user);
                s->setServicePassword(pass);
            }
        }
        catch(IBPP::Exception& e)
        {
            wxMessageBox(std2wx(e.ErrorMessage()), _("Error"),
                wxICON_ERROR|wxOK);
            return false;
        }
    }
    return true;
}
//-----------------------------------------------------------------------------
