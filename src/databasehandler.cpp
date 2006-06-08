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

#include "core/FRError.h"
#include "core/StringUtils.h"
#include "dberror.h"
#include "metadata/server.h"
#include "metadata/database.h"
#include "urihandler.h"
//-----------------------------------------------------------------------------
class DatabaseInfoHandler: public URIHandler
{
public:
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const DatabaseInfoHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const DatabaseInfoHandler DatabaseInfoHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool DatabaseInfoHandler::handleURI(URI& uri)
{
    bool isEditSweep, isEditForcedWrites;

    isEditSweep = (uri.action == wxT("edit_sweep_interval"));
    isEditForcedWrites = (uri.action == wxT("edit_forced_writes"));

    if (!isEditSweep && !isEditForcedWrites)
        return false;

    Database* d = (Database*)getObject(uri);
    wxWindow* w = getWindow(uri);

    // when either the database or the window does not exist
    // return immediately. Because this function returns whether
    // the specified uri is handled, return true.
    if (!d || !w || !d->isConnected())
         return true;

    IBPP::Database& db = d->getIBPPDatabase();
    IBPP::Service svc = IBPP::ServiceFactory(
        wx2std(d->getServer()->getConnectionString()),
        db->Username(), db->UserPassword());
    svc->Connect();

    if (isEditSweep)
    {
        long oldSweep = d->getInfo()->getSweep();

        while (true)
        {
            wxString s;
            long sweep = oldSweep;
            s = ::wxGetTextFromUser(_("Enter the sweep interval"),
                _("Sweep Interval"), wxString::Format(wxT("%d"), sweep), w);

            // return from the iteration when the entered string is empty, in
            // case of cancelling the operation.
            if (s.IsEmpty())
                break;
            if (!s.ToLong(&sweep))
                continue;
            // return from the iteration when the interval has not changed
            if (sweep == oldSweep)
                break;

            svc->SetSweepInterval(wx2std(d->getPath()), sweep);

            // load the database info because the sweep interval has been
            // changed. Before loading the info, re-attach to the database
            // otherwise the sweep interval won't be changed for FB Classic
            // Server.
            db->Disconnect();
            db->Connect();
            d->getInfo()->loadInfo(&db);
            d->notifyObservers();
            break;
        }
    }

    if (isEditForcedWrites)
    {
        bool forced = !d->getInfo()->getForcedWrites();

        // disconnect the database before changing SyncWrites. When the
        // database remains connected, you can set SyncWrites for about
        // three times before the database will be locked and unavailable
        // until the database server is restarted.
        db->Disconnect();

        svc->SetSyncWrite(wx2std(d->getPath()), forced);

        // connect to the database again
        db->Connect();

        // load the database info because the value of forced writes been
        // changed.
        d->getInfo()->loadInfo(&db);
        d->notifyObservers();
    }

    svc->Disconnect();
    return true;
}
//-----------------------------------------------------------------------------

