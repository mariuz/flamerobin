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

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include <algorithm>

#include "config/Config.h"
#include "core/ProgressIndicator.h"
#include "core/StringUtils.h"
#include "core/Visitor.h"
#include "frutils.h"
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
DatabaseCollection::iterator Server::begin()
{
    return databasesM.begin();
}
//-----------------------------------------------------------------------------
DatabaseCollection::iterator Server::end()
{
    return databasesM.end();
}
//-----------------------------------------------------------------------------
DatabaseCollection::const_iterator Server::begin() const
{
    return databasesM.begin();
}
//-----------------------------------------------------------------------------
DatabaseCollection::const_iterator Server::end() const
{
    return databasesM.end();
}
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
    DatabaseCollection::const_iterator it = std::find_if(
        databasesM.begin(), databasesM.end(),
        std::mem_fun_ref<bool, Database>(&Database::isConnected));
    return it != databasesM.end();
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
    visitor->visitServer(*this);
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
UserList* Server::getUsers(ProgressIndicator* progressind)
{
    usersM.clear();
    IBPP::Service svc;
    if (!::getService(this, svc, progressind, true))   // true = SYSDBA
        return 0;

    std::vector<IBPP::User> usr;
    svc->GetUsers(usr);
    for (std::vector<IBPP::User>::iterator it = usr.begin();
        it != usr.end(); ++it)
    {
        User u(*it, this);
        usersM.push_back(u);
    }

    std::sort(usersM.begin(), usersM.end());
    return &usersM;
}
//-----------------------------------------------------------------------------
void Server::setServiceUser(const wxString& user)
{
    serviceUserM = user;
}
//-----------------------------------------------------------------------------
void Server::setServicePassword(const wxString& pass)
{
    servicePasswordM = pass;
}
//-----------------------------------------------------------------------------
void Server::setServiceSysdbaPassword(const wxString& pass)
{
    serviceSysdbaPasswordM = pass;
}
//-----------------------------------------------------------------------------
bool Server::getService(IBPP::Service& svc, ProgressIndicator* progressind,
    bool sysdba)
{
    if (progressind)
    {
        progressind->initProgress(_("Connecting..."),
            databasesM.getChildrenCount() + 2, 0, 1);
    }

    // check if we already had some successful connections
    if (!serviceSysdbaPasswordM.IsEmpty())  // we have sysdba pass
    {
        if (progressind)
        {
            progressind->setProgressMessage(_("Using current SYSDBA password"));
            progressind->stepProgress();
        }
        try
        {
            svc = IBPP::ServiceFactory(wx2std(getConnectionString()),
                "SYSDBA", wx2std(serviceSysdbaPasswordM));
            svc->Connect();
            return true;
        }
        catch(IBPP::Exception&)   // keep going if connect fails
        {
            serviceSysdbaPasswordM.Clear();
        }
    }
    if (progressind && progressind->isCanceled())
        return false;
    // check if we have non-sysdba connection
    if (!sysdba && !serviceUserM.IsEmpty())
    {
        if (progressind)
        {
            progressind->setProgressMessage(wxString::Format(
                _("Using current %s password"), serviceUserM.c_str()));
            progressind->stepProgress();
        }
        try
        {
            svc = IBPP::ServiceFactory(wx2std(getConnectionString()),
                wx2std(serviceUserM), wx2std(servicePasswordM));
            svc->Connect();
            return true;
        }
        catch(IBPP::Exception&)   // keep going if connect fails
        {
            serviceUserM.Clear();
            servicePasswordM.Clear();
        }
    }

    // first try connected databases
    for (DatabaseCollection::iterator ci = databasesM.begin();
        ci != databasesM.end(); ++ci)
    {
        if (progressind && progressind->isCanceled())
            return false;
        if (!(*ci).isConnected())
            continue;
        // Use the user name and password of the connected user
        // instead of the stored ones.
        IBPP::Database& db = (*ci).getIBPPDatabase();
        if (sysdba && std2wx(db->Username()).Upper() != wxT("SYSDBA"))
            continue;
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
            if (sysdba)
                serviceSysdbaPasswordM = std2wx(db->UserPassword());
            else
            {
                serviceUserM = std2wx(db->Username());
                servicePasswordM = std2wx(db->UserPassword());
            }
            return true;
        }
        catch(IBPP::Exception&)   // keep going if connect fails
        {
        }
    }

    // when the operation is not canceled try to user/pass of disconnected DBs
    for (DatabaseCollection::const_iterator ci = databasesM.begin();
        ci != databasesM.end(); ++ci)
    {
        if (progressind && progressind->isCanceled())
            return false;
        if ((*ci).isConnected())
            continue;
        wxString user = (*ci).getUsername();
        wxString pwd = (*ci).getDecryptedPassword();
        if (pwd.IsEmpty() || sysdba && user.Upper() != wxT("SYSDBA"))
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
            if (sysdba)
                serviceSysdbaPasswordM = pwd;
            else
            {
                serviceUserM = user;
                servicePasswordM = pwd;
            }
            return true;
        }
        catch(IBPP::Exception&)   // keep going if connect fails
        {
        }
    }
    return false;
}
//-----------------------------------------------------------------------------
