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

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "frutils.h"
#include "config/Config.h"
#include "core/Visitor.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/root.h"
#include "metadata/server.h"
//-----------------------------------------------------------------------------
using namespace std;
//-----------------------------------------------------------------------------
Server::Server()
    :MetadataItem()
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
        wx2std(db->getUsername()), wx2std(db->getDecryptedPassword()), "",
        wx2std(charset), wx2std(extra_params));
    db1->Create(dialect);
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
std::vector<User>* Server::getUsers(ProgressIndicator* progressind)
{
    usersM.clear();
    IBPP::Service svc;
    if (!::getService(this, svc, progressind))   // if cancel pressed on one of dialogs
        return 0;

    std::vector<IBPP::User> usr;
    svc->GetUsers(usr);
    for (std::vector<IBPP::User>::iterator it = usr.begin();
        it != usr.end(); ++it)
    {
        User u(*it, this);
        usersM.push_back(u);
    }
    return &usersM;
}
//-----------------------------------------------------------------------------
bool Server::getService(IBPP::Service& svc, ProgressIndicator* progressind)
{
    if (progressind)
    {
        progressind->initProgress(_("Connecting..."),
            databasesM.getChildrenCount(), 0, 1);
    }

    // first try connected databases
    for (MetadataCollection<Database>::iterator ci = databasesM.begin();
        ci != databasesM.end(); ++ci)
    {
        if (progressind && progressind->isCanceled())
            return false;
        if (!(*ci).isConnected())
            continue;
        // Use the user name and password of the connected user
        // instead of the stored ones.
        IBPP::Database& db = (*ci).getIBPPDatabase();
        if (progressind)
        {
            progressind->setProgressMessage(_("Using password of: ") +
                std2wx(db->Username()) + wxT("@") + (*ci).getName_());
            progressind->stepProgress();
        }
        try
        {
            svc = IBPP::ServiceFactory(wx2std(getConnectionString()),
                db->Username(), db->UserPassword());
            svc->Connect();
            return true;
        }
        catch(IBPP::Exception&)   // keep going if connect fails
        {
        }
    }

    // when the operation is not canceled try to user/pass of disconnected DBs
    for (MetadataCollection<Database>::const_iterator
        ci = databasesM.begin(); ci != databasesM.end(); ++ci)
    {
        if (progressind && progressind->isCanceled())
            return false;
        if ((*ci).isConnected())
            continue;
        wxString user = (*ci).getUsername();
        wxString pwd = (*ci).getDecryptedPassword();
        if (pwd.IsEmpty())
            continue;
        if (progressind)
        {
            progressind->setProgressMessage(_("Using password of: ") +
                user + wxT("@") + (*ci).getName_());
            progressind->stepProgress();
        }
        try
        {
            svc = IBPP::ServiceFactory(wx2std(getConnectionString()),
                wx2std(user), wx2std(pwd));
            svc->Connect();
            return true;
        }
        catch(IBPP::Exception&)   // keep going if connect fails
        {
        }
    }
    return false;
}
//-----------------------------------------------------------------------------
