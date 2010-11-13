/*
  Copyright (c) 2004-2010 The FlameRobin Development Team

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

#include <wx/file.h>
#include <wx/tokenzr.h>

#include <fstream>
#include <sstream>

#include "core/FRError.h"
#include "core/StringUtils.h"
#include "frutils.h"
#include "gui/ProgressDialog.h"
#include "gui/UsernamePasswordDialog.h"
#include "metadata/column.h"
#include "metadata/relation.h"
#include "metadata/server.h"
//-----------------------------------------------------------------------------
void adjustControlsMinWidth(std::list<wxWindow*> controls)
{
    int w = 0;
    wxSize sz;
    // find widest control
    for (std::list<wxWindow*>::iterator it = controls.begin();
        it != controls.end(); ++it)
    {
        wxASSERT(*it != 0);
        sz = (*it)->GetSize();
        w = std::max(w, sz.GetWidth());
    }
    // set minimum width of all controls
    for (std::list<wxWindow*>::iterator it = controls.begin();
        it != controls.end(); ++it)
    {
        sz = (*it)->GetSize();
        (*it)->SetSize(w, sz.GetHeight());
        (*it)->SetSizeHints(w, sz.GetHeight());
    }
}
//-----------------------------------------------------------------------------
void readBlob(IBPP::Statement& st, int column, wxString& result,
    wxMBConv* conv)
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
    result = std2wx(resultBuffer, conv);
    b->Close();
}
//-----------------------------------------------------------------------------
wxString selectRelationColumns(Relation* t, wxWindow* parent)
{
    std::vector<wxString> list;
    if (!selectRelationColumnsIntoVector(t, parent, list))
        return wxEmptyString;

    std::vector<wxString>::iterator it = list.begin();
    wxString retval(*it);
    while ((++it) != list.end())
        retval += wxT(", ") + (*it);
    return retval;
}
//-----------------------------------------------------------------------------
bool selectRelationColumnsIntoVector(Relation* t, wxWindow* parent,
    std::vector<wxString>& list)
{
    t->ensureChildrenLoaded();

    wxArrayInt selected_columns;
    wxArrayString colNames;
    colNames.Alloc(t->getColumnCount());
    for (ColumnPtrs::const_iterator it = t->begin(); it != t->end(); ++it)
        colNames.Add((*it)->getName_());

    // set default selection.
    for (std::vector<wxString>::const_iterator it = list.begin();
        it != list.end(); ++it)
    {
        wxString::size_type i = colNames.Index((*it));
        if (i != wxNOT_FOUND)
            selected_columns.Add(i);
    }    
    
    bool ok = 
#if wxCHECK_VERSION(2, 9, 0)
    ::wxGetSelectedChoices(selected_columns,
#else
    ::wxGetMultipleChoices(selected_columns,
#endif
        _("Select one or more fields... (use ctrl key)"),  _("Table Fields"),
        colNames, parent) > 0;
    list.clear();
    if (!ok)
        return false;

    for (size_t i = 0; i < selected_columns.GetCount(); ++i)
    {
        Identifier temp(colNames[selected_columns[i]]);
        list.push_back(temp.getQuoted());
    }
    return true;
}
//-----------------------------------------------------------------------------
bool connectDatabase(Database* db, wxWindow* parent,
    ProgressDialog* progressdialog)
{
    wxString pass(db->getDecryptedPassword());
    if (db->getAuthenticationMode().getAlwaysAskForPassword())
    {
        // don't allow different username here
        UsernamePasswordDialog upd(wxGetActiveWindow(),
            _("Database Credentials"), db->getUsername(), true,
            _("Please enter the the database user's password:"));
        if (upd.ShowModal() != wxID_OK)
            return false;
        pass = upd.getPassword();
    }

    wxString caption(wxString::Format(_("Connecting to Database \"%s\""),
        db->getName_().c_str()));
    if (progressdialog)
    {
        progressdialog->setProgressMessage(caption);
        db->connect(pass, progressdialog);
    }
    else
    {
        ProgressDialog pd(parent, caption, 1);
        db->connect(pass, &pd);;
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
        if (sysdba)
            msg = _("Please enter the database administrator password:");
        else
        {
            if (p->isCanceled())
                msg = _("You have canceled the search for a usable username and password.");
            else
                msg = _("None of the known database credentials could be used.");
            msg = msg + wxT("\n") + _("Please enter a valid username and password:");
        }

        UsernamePasswordDialog upd(wxGetActiveWindow(),
            _("Database Credentials"), wxT("SYSDBA"), sysdba, msg);
        if (upd.ShowModal() != wxID_OK)
            return false;
        wxString username(upd.getUsername());
        wxString password(upd.getPassword());

        try
        {
            svc = IBPP::ServiceFactory(wx2std(s->getConnectionString()),
                wx2std(username), wx2std(password));
            svc->Connect();
            // exception might be thrown. If not, we store the credentials:
            if (sysdba || username.Upper() == wxT("SYSDBA"))
                s->setServiceSysdbaPassword(password);
            else
                s->setServiceCredentials(username, password);
        }
        catch(IBPP::Exception& e)
        {
            wxMessageBox(std2wx(e.ErrorMessage()), _("Error"),
                wxICON_ERROR | wxOK);
            return false;
        }
    }
    return true;
}
//-----------------------------------------------------------------------------
wxString loadEntireFile(const wxFileName& filename)
{
    if (!filename.FileExists())
    {
        wxString msg;
        msg.Printf(_("The file \"%s\" does not exist."),
            filename.GetFullPath().c_str());
        throw FRError(msg);
    }

    // read entire file into wxString buffer
    std::ifstream filex(wx2std(filename.GetFullPath()).c_str());
    if (!filex)
    {
        wxString msg;
        msg.Printf(_("The file \"%s\" cannot be opened."),
            filename.GetFullPath().c_str());
        throw FRError(msg);
    }

    std::stringstream ss;
    ss << filex.rdbuf();
    wxString s(std2wx(ss.str()));
    filex.close();
    return s;
}
//-----------------------------------------------------------------------------
