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

//-----------------------------------------------------------------------------
#ifndef FR_SERVER_H
#define FR_SERVER_H

#include <ibpp.h>

#include "metadata/collection.h"
#include "metadata/database.h"
#include "metadata/metadataitem.h"
#include "metadata/User.h"

typedef std::vector<User> UserList;
typedef MetadataCollection<Database> DatabaseCollection;
//-----------------------------------------------------------------------------
// this is a coupled node (in visual sense). Server equals collection of
// YDatabases in wxTree. that's why getChildren() method just copies, since
// wxTree item will have pointer to Server
class Server: public MetadataItem
{
private:
    wxString hostnameM;
    wxString portM;

    DatabaseCollection databasesM;
    UserList usersM;

    wxString serviceUserM;
    wxString servicePasswordM;
    wxString serviceSysdbaPasswordM;
protected:
    virtual void doSetChildrenLoaded(bool loaded);
    virtual void lockChildren();
    virtual void unlockChildren();
public:
    Server();
    Server(const Server& rhs);

    virtual bool getChildren(std::vector<MetadataItem *>& temp);
    Database* addDatabase(Database& db);
    void removeDatabase(Database* db);

    DatabaseCollection::iterator begin();
    DatabaseCollection::iterator end();
    DatabaseCollection::const_iterator begin() const;
    DatabaseCollection::const_iterator end() const;

    void createDatabase(Database *db, int pagesize = 4096, int dialect = 3);

    // returns *connected* service
    bool getService(IBPP::Service& svc, ProgressIndicator* progressind,
        bool sysdba);
    void setServiceUser(const wxString& user);
    void setServicePassword(const wxString& pass);
    void setServiceSysdbaPassword(const wxString& pass);

    UserList* getUsers(ProgressIndicator* progressind);

    // setters/getters
    wxString getHostname() const;
    wxString getPort() const;
    // returns the server-related portion of the connection wxString,
    // that is server name and port number if specified.
    wxString getConnectionString() const;

    void setHostname(wxString hostname);
    void setPort(wxString port);
    virtual const wxString getTypeName() const;

    bool hasConnectedDatabase() const;
    virtual const wxString getItemPath() const;
    virtual void acceptVisitor(MetadataItemVisitor* visitor);
};
//-----------------------------------------------------------------------------
#endif
