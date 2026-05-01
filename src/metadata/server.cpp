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

#include <algorithm>

#include "config/Config.h"
#include "engine/db/DatabaseFactory.h"
#include "core/ProgressIndicator.h"
#include "core/StringUtils.h"
#include "frutils.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/root.h"
#include "metadata/server.h"

Server::Server()
    : MetadataItem(ntServer)
{
    setChildrenLoaded(true);
}

DatabasePtrs Server::getDatabases() const
{
    return databasesM;
}

void Server::lockChildren()
{
    std::for_each(databasesM.begin(), databasesM.end(),
        std::mem_fn(&Database::lockSubject));
}

void Server::unlockChildren()
{
    std::for_each(databasesM.begin(), databasesM.end(),
        std::mem_fn(&Database::unlockSubject));
}

bool Server::getChildren(std::vector<MetadataItem*>& temp)
{
    if (databasesM.empty())
        return false;
    std::transform(databasesM.begin(), databasesM.end(),
        std::back_inserter(temp), std::mem_fn(&DatabasePtr::get));
    return !databasesM.empty();
}

DatabasePtr Server::addDatabase()
{
    DatabasePtr database(new Database());
    addDatabase(database);
    return database;
}

void Server::addDatabase(DatabasePtr database)
{
    if (database)
    {
        databasesM.push_back(database);
        database->setServer(shared_from_this());
        notifyObservers();
    }
}

void Server::removeDatabase(DatabasePtr database)
{
    DatabasePtrs::iterator it = std::remove(databasesM.begin(),
        databasesM.end(), database);
    if (it != databasesM.end())
    {
        databasesM.erase(it, databasesM.end());
        notifyObservers();
    }
}

wxString Server::getHostname() const
{
    return hostnameM;
}

wxString Server::getPort() const
{
    return portM;
}

bool Server::hasConnectedDatabase() const
{
    DatabasePtrs::const_iterator it = std::find_if(
        databasesM.begin(), databasesM.end(),
        std::mem_fn(&Database::isConnected));
    return it != databasesM.end();
}

void Server::setHostname(wxString hostname)
{
    hostnameM = hostname;
}

void Server::setPort(wxString port)
{
    portM = port;
}

const wxString Server::getTypeName() const
{
    return "SERVER";
}

void Server::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitServer(*this);
}

const wxString Server::getItemPath() const
{
    // Since database Ids are already unique, let's shorten the item paths
    // by not including the server part. Even more so if this class is bound
    // to disappear in the future.
    return "";
}

struct SortUsers
{
    bool operator() (UserPtr user1, UserPtr user2)
    {
        return user1->getUsername() < user2->getUsername();
    }
};

UserPtrs Server::getUsers(ProgressIndicator* progressind)
{
    usersM.clear();
    fr::IServicePtr svc = getDALService(progressind, true);   // true = SYSDBA
    if (!svc)
        return usersM;

    std::vector<fr::UserData> usr;
    svc->getUsers(usr);
    for (const auto& u : usr)
    {
        UserPtr uptr(new User(shared_from_this(), u));
        usersM.push_back(uptr);
    }

    std::sort(usersM.begin(), usersM.end(), SortUsers());
    return usersM;
}

void Server::setServiceCredentials(const wxString& user, const wxString& pass)
{
    serviceUserM = user;
    servicePasswordM = pass;
}

void Server::setServiceSysdbaPassword(const wxString& pass)
{
    serviceSysdbaPasswordM = pass;
}

fr::IServicePtr Server::getDALService(ProgressIndicator* progressind, bool sysdba)
{
    if (progressind)
    {
        progressind->initProgress(_("Connecting..."),
            databasesM.size() + 2, 0, 1);
    }

    auto createAndConnect = [this, progressind](const std::string& user, const std::string& pass, const std::string& role = "", const std::string& charset = "") -> fr::IServicePtr {
        try
        {
            fr::IServicePtr svc = fr::DatabaseFactory::createService();
            svc->setConnectionString(wx2std(getConnectionString()));
            svc->setCredentials(user, pass);
            // TODO: support role and charset in IService if needed
            svc->connect();
            return svc;
        }
        catch(...)
        {
            return nullptr;
        }
    };

    // check if we already had some successful connections
    if (!serviceSysdbaPasswordM.empty())  // we have sysdba pass
    {
        if (progressind)
        {
            progressind->setProgressMessage(_("Using current SYSDBA password"));
            progressind->stepProgress();
        }
        fr::IServicePtr svc = createAndConnect("SYSDBA", wx2std(serviceSysdbaPasswordM));
        if (svc) return svc;
        serviceSysdbaPasswordM.clear();
    }

    if (progressind && progressind->isCanceled())
        return nullptr;

    // check if we have non-sysdba connection
    if (!sysdba && !serviceUserM.IsEmpty())
    {
        if (progressind)
        {
            progressind->setProgressMessage(wxString::Format(
                _("Using current %s password"), serviceUserM.c_str()));
            progressind->stepProgress();
        }
        fr::IServicePtr svc = createAndConnect(wx2std(serviceUserM), wx2std(servicePasswordM));
        if (svc) return svc;
        serviceUserM.Clear();
        servicePasswordM.Clear();
    }

    // first try connected databases
    for (DatabasePtrs::iterator ci = databasesM.begin();
        ci != databasesM.end(); ++ci)
    {
        if (progressind && progressind->isCanceled())
            return nullptr;
        if (!(*ci)->isConnected())
            continue;
        // Use the user name and password of the connected user
        // instead of the stored ones.
        fr::IDatabasePtr db = (*ci)->getDALDatabase();
        if (sysdba && wxString(db->getUsername()).Upper() != "SYSDBA")
            continue;
        if (progressind)
        {
            progressind->setProgressMessage(_("Using password of: ") +
                wxString(db->getUsername()) + "@" + (*ci)->getName_());
            progressind->stepProgress();
        }
        
        fr::IServicePtr svc = createAndConnect(db->getUsername(), db->getUserPassword());
        if (svc)
        {
            if (sysdba)
                serviceSysdbaPasswordM = wxString(db->getUserPassword());
            else
            {
                serviceUserM = wxString(db->getUsername());
                servicePasswordM = wxString(db->getUserPassword());
            }
            return svc;
        }
    }

    // when the operation is not canceled try to user/pass of disconnected DBs
    for (DatabasePtrs::const_iterator ci = databasesM.begin();
        ci != databasesM.end(); ++ci)
    {
        if (progressind && progressind->isCanceled())
            return nullptr;
        if ((*ci)->isConnected())
            continue;
        wxString user = (*ci)->getUsername();
        wxString pwd = (*ci)->getDecryptedPassword();
        if (pwd.IsEmpty() || (sysdba && user.Upper() != "SYSDBA"))
            continue;
        if (progressind)
        {
            progressind->setProgressMessage(_("Using password of: ") +
                user + "@" + (*ci)->getName_());
            progressind->stepProgress();
        }
        
        fr::IServicePtr svc = createAndConnect(wx2std(user), wx2std(pwd));
        if (svc)
        {
            if (sysdba)
                serviceSysdbaPasswordM = pwd;
            else
            {
                serviceUserM = user;
                servicePasswordM = pwd;
            }
            return svc;
        }
    }
    return nullptr;
}

bool Server::getService(IBPP::Service& svc, ProgressIndicator* progressind,
    bool sysdba)
{
    fr::IServicePtr dalSvc = getDALService(progressind, sysdba);
    if (!dalSvc)
        return false;

    if (auto ibppSvc = std::dynamic_pointer_cast<fr::IbppService>(dalSvc))
    {
        svc = ibppSvc->getIBPPService();
        return true;
    }
    return false;
}

