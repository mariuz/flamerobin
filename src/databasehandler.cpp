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

#include "core/StringUtils.h"
#include "core/URIProcessor.h"
#include "gui/GUIURIHandlerHelper.h"
#include "gui/ExecuteSql.h"
#include "metadata/server.h"
#include "metadata/database.h"
#include "metadata/MetadataItemURIHandlerHelper.h"

class DatabaseInfoHandler: public URIHandler,
    private MetadataItemURIHandlerHelper, private GUIURIHandlerHelper
{
public:
    DatabaseInfoHandler() {}
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const DatabaseInfoHandler handlerInstance;
};

const DatabaseInfoHandler DatabaseInfoHandler::handlerInstance;

bool DatabaseInfoHandler::handleURI(URI& uri)
{
    bool isEditSweep, isEditForcedWrites, isEditReserve, isEditReadOnly,
        isEditPageBuffers, isEditLinger;

    isEditSweep = (uri.action == "edit_db_sweep_interval");
    isEditForcedWrites = (uri.action == "edit_db_forced_writes");
    isEditReserve = (uri.action == "edit_db_reserve_space");
    isEditReadOnly = (uri.action == "edit_db_read_only");
    isEditPageBuffers = (uri.action == "edit_db_page_buffers");
    isEditLinger = (uri.action == "edit_db_linger");


    if (!isEditSweep && !isEditForcedWrites && !isEditReserve
        && !isEditReadOnly && !isEditPageBuffers && !isEditLinger)
    {
        return false;
    }

    DatabasePtr d = extractMetadataItemPtrFromURI<Database>(uri);
    wxWindow* w = getParentWindow(uri);

    // when either the database or the window does not exist
    // return immediately. Because this function returns whether
    // the specified uri is handled, return true.
    if (!d || !w || !d->isConnected())
         return true;

    IBPP::Database& db = d->getIBPPDatabase();
    IBPP::Service svc = IBPP::ServiceFactory(
        wx2std(d->getServer()->getConnectionString()),
        db->Username(), db->UserPassword(), db->RoleName(), db->CharSet()
    );
    svc->Connect();

    if (isEditSweep || isEditPageBuffers || isEditLinger)
    {
        long oldValue = 0;
        wxString title, label;
        if (isEditSweep)
        {
            oldValue = d->getInfo().getSweep();
            title = _("Enter the new Sweep Interval");
            label = _("Sweep Interval"); 
        }
        else if (isEditPageBuffers)
        {
            oldValue = d->getInfo().getBuffers();
            title = _("Enter the new value for Page Buffers");
            label = _("Page Buffers"); 
        }
        else if (isEditLinger)
        {
            oldValue = d->getLinger();
            title = _("Enter the new value for Linger");
            label = _("Linger Value");

        }

        while (true)
        {
            wxString s;
            long value = oldValue;
            s = ::wxGetTextFromUser(title, label,
                wxString::Format("%d", value), w);

            // return from the iteration when the entered string is empty, in
            // case of cancelling the operation.
            if (s.IsEmpty())
                break;
            if (!s.ToLong(&value))
                continue;
            // return from the iteration when the interval has not changed
            if (value == oldValue)
                break;

            if (isEditSweep)
                svc->SetSweepInterval(wx2std(d->getPath()), value);
            else if (isEditPageBuffers)
                svc->SetPageBuffers(wx2std(d->getPath()), value);
            else if (isEditLinger)
                execSql(NULL, wxString(_("Alter database")), d, wxString::Format("ALTER DATABASE SET LINGER TO %d ; commit; ", value, w), true);
            // Before reloading the info, re-attach to the database
            // otherwise the sweep interval won't be changed for FB Classic
            // Server.
            db->Disconnect();
            db->Connect();
            d->loadInfo();
            break;
        }
    }

    else if (isEditForcedWrites || isEditReserve || isEditReadOnly)
    {
        bool fw = !d->getInfo().getForcedWrites();
        bool reserve = !d->getInfo().getReserve();
        bool ro = !d->getInfo().getReadOnly();

        // setting these properties requires that the database is
        // disconnected.
        db->Disconnect();

        if (isEditForcedWrites)
            svc->SetSyncWrite(wx2std(d->getPath()), fw);
        if (isEditReserve)
            svc->SetReserveSpace(wx2std(d->getPath()), reserve);
        if (isEditReadOnly)
            svc->SetReadOnly(wx2std(d->getPath()), ro);

        db->Connect();

        // load the database info because the info values are changed.
        d->loadInfo();
    }

    svc->Disconnect();
    return true;
}

