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

  $Id$

  Contributor(s): Nando Dessena
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include <wx/progdlg.h>

#include "config/Config.h"
#include "core/Visitor.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/root.h"
#include "metadata/server.h"
//-----------------------------------------------------------------------------
using namespace std;
//-----------------------------------------------------------------------------
Server::Server()
{
    typeM = ntServer;

    databasesM.setParent(this);
    databasesM.setType(ntServer);
}
//-----------------------------------------------------------------------------
Server::Server(const Server& rhs)
    : MetadataItem(rhs), hostnameM(rhs.hostnameM), portM(rhs.portM)
{
    databasesM.setParent(this);
}
//-----------------------------------------------------------------------------
void Server::lockChildren()
{
    databasesM.lockSubject();
}
//-----------------------------------------------------------------------------
void Server::unlockChildren()
{
    databasesM.unlockSubject();
}
//-----------------------------------------------------------------------------
bool Server::getChildren(vector<MetadataItem*>& temp)
{
    return databasesM.getChildren(temp);
}
//-----------------------------------------------------------------------------
bool Server::orderedChildren() const
{
    bool ordered = false;
    config().getValue(wxT("OrderDatabasesInTree"), ordered);
    return ordered;
}
//-----------------------------------------------------------------------------
// returns pointer to object in vector
Database* Server::addDatabase(Database& db)
{
    Database* temp = databasesM.add(db);
    temp->setParent(this);                  // grab it from collection
    notifyObservers();
    getGlobalRoot().save();
    return temp;
}
//-----------------------------------------------------------------------------
void Server::removeDatabase(Database* db)
{
    databasesM.remove(db);
    notifyObservers();
    getGlobalRoot().save();
}
//-----------------------------------------------------------------------------
void Server::createDatabase(Database* db, int pagesize, int dialect)
{
    wxString extra_params;
    if (pagesize)
        extra_params << wxT(" PAGE_SIZE ") << pagesize;

    wxString charset(db->getConnectionCharset());
    if (!charset.empty())
        extra_params << wxT(" DEFAULT CHARACTER SET ") << charset;

    IBPP::Database db1;
    db1 = IBPP::DatabaseFactory(wx2std(getConnectionString()), wx2std(db->getPath()),
        wx2std(db->getUsername()), wx2std(db->getPassword()), "",
        wx2std(charset), wx2std(extra_params));
    db1->Create(dialect);
}
//-----------------------------------------------------------------------------
bool Server::getVersion(wxString& version, ProgressIndicator* progressIndicator)
{
    IBPP::Service svc;
    try
    {
        svc = getService(progressIndicator);
        svc->Connect();
    }
    catch (IBPP::Exception& e)
    {
        version = std2wx(e.ErrorMessage());
        return false;
    }

    std::string vrs;
    svc->GetVersion(vrs);
    svc->Disconnect();
    version = std2wx(vrs);

    return true;
}
//-----------------------------------------------------------------------------
MetadataCollection<Database>* Server::getDatabases()
{
    return &databasesM;
};
//-----------------------------------------------------------------------------
wxString Server::getHostname() const
{
    return hostnameM;
}
//-----------------------------------------------------------------------------
wxString Server::getPort() const
{
    return portM;
}
//-----------------------------------------------------------------------------
bool Server::hasConnectedDatabase() const
{
    for (MetadataCollection<Database>::const_iterator it = databasesM.begin();
        it != databasesM.end(); ++it)
    {
        if ((*it).isConnected())
            return true;
    }
    return false;
}
//-----------------------------------------------------------------------------
void Server::setHostname(wxString hostname)
{
    hostnameM = hostname;
}
//-----------------------------------------------------------------------------
void Server::setPort(wxString port)
{
    portM = port;
}
//-----------------------------------------------------------------------------
const wxString Server::getTypeName() const
{
    return wxT("SERVER");
}
//-----------------------------------------------------------------------------
void Server::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visit(*this);
}
//-----------------------------------------------------------------------------
wxString Server::getConnectionString() const
{
    wxString hostname = getHostname();
    wxString port = getPort();
    if (!hostname.empty() && !port.empty())
        return hostname + wxT("/") + port;
    else
        return hostname;
}
//-----------------------------------------------------------------------------
const wxString Server::getItemPath() const
{
    // Since database Ids are already unique, let's shorten the item paths
    // by not including the server part. Even more so if this class is bound
    // to disappear in the future.
    return wxT("");
}
//-----------------------------------------------------------------------------
IBPP::Service Server::getService(ProgressIndicator* progressIndicator)
{
    if (progressIndicator)
    {
        progressIndicator->initProgress(_("Connecting..."),
	databasesM.getChildrenCount(), 0, 1);
    }

    wxString user, pwd;
    bool canceled = false;

    // first try connected databases
    for (MetadataCollection<Database>::iterator ci = databasesM.begin();
        ci != databasesM.end(); ++ci)
    {
        if (progressIndicator && progressIndicator->isCanceled())
        {
            canceled = true;
            break;
        }

        if ((*ci).isConnected())
        {
            // Use the user name and password of the connected user
            // instead of the stored onces.
            IBPP::Database& db = (*ci).getIBPPDatabase();
            user = std2wx(db->Username());
            pwd = std2wx(db->UserPassword());

            if (progressIndicator)
            {
                progressIndicator->setProgressMessage(_("Using password of: ") +
                    user + wxT("@") + (*ci).getName_());
                progressIndicator->stepProgress();
            }
        }
    }

    // when the operation is not canceled and the user and/or password
    // is still empty, try to find them stored in the disconnected databases.
    if (!canceled && (user.IsEmpty() || pwd.IsEmpty()))
    {
        for (MetadataCollection<Database>::const_iterator ci = databasesM.begin();
            ci != databasesM.end(); ++ci)
        {
            if (!(*ci).isConnected())
            {
                user = (*ci).getUsername();
                pwd = (*ci).getPassword();

                if (!user.IsEmpty() && !pwd.IsEmpty())
                    break;
            }
        }
    }

    wxString msg;
    if (canceled)
        msg = _("You've canceled the search for a usable username and password.");
    else if (user.IsEmpty() || pwd.IsEmpty())
        msg = _("None of the credentials of the databases could be used.");

    if (!msg.IsEmpty())
    {
        wxMessageBox(msg + _("\nYou need to supply a valid username and password."),
            _("Connecting to server"), wxOK|wxICON_INFORMATION);
	user = ::wxGetTextFromUser(_("Connecting to server"),
            _("Enter username"));
        pwd = ::wxGetPasswordFromUser(_("Connecting to server"),
            _("Enter password"));
    }

    return IBPP::ServiceFactory(wx2std(getConnectionString()),
        wx2std(user), wx2std(pwd));
}
//-----------------------------------------------------------------------------
