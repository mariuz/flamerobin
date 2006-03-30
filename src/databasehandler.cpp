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

  The Initial Developer of the Original Code is Bart Bakker.

  Portions created by the original developer
  are Copyright (C) 2006 Bart Bakker.

  All Rights Reserved.

  $Id: databaseshandlers.cpp $

  Contributor(s): Milan Babuskov.
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
//-----------------------------------------------------------------------------

#include "core/FRError.h"
#include "dberror.h"
#include "metadata/server.h"
#include "metadata/database.h"
#include "ugly.h"
#include "urihandler.h"

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
        long sweep = d->getInfo()->getSweep();

        while (true)
        {
            wxString s;
            s = ::wxGetTextFromUser(_("Enter the sweep interval"), _("Sweep interval"), _(""));

            // return from the iteration when the entered string is empty, in
            // case of cancelling the operation.
            if (s.IsEmpty())
                    break;
            if (!s.ToLong(&sweep))
                    continue;

            svc->SetSweepInterval(wx2std(d->getPath()), sweep);

            // load the database info because the sweep interval has changed
            d->getInfo()->loadInfo((&d->getIBPPDatabase()));
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

        // load the database info because the value of forced writes changed
        d->getInfo()->loadInfo(&db);
        d->notifyObservers();
    }

    svc->Disconnect();
    return true;
}
//-----------------------------------------------------------------------------

