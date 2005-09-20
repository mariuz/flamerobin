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

#include "config/Config.h"
#include "core/Visitor.h"
#include "MetadataItemVisitor.h"
#include "server.h"
//-----------------------------------------------------------------------------
using namespace std;
//-----------------------------------------------------------------------------
Server::Server()
    : MetadataItem()
{
    typeM = ntServer;

    hostnameM = wxT("");
    portM = wxT("");

    databasesM.setParent(this);
    databasesM.setType(ntServer);
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
    temp->initChildren();
    notifyObservers();
    return temp;
}
//-----------------------------------------------------------------------------
void Server::removeDatabase(Database* db)
{
    databasesM.remove(db);
    notifyObservers();
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
    db1 = IBPP::DatabaseFactory(wx2std(hostnameM), wx2std(db->getPath()),
        wx2std(db->getUsername()), wx2std(db->getPassword()), "",
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
