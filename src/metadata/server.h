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

#include <vector>

#include <boost/enable_shared_from_this.hpp>

#include <ibpp.h>

#include "metadata/collection.h"
#include "metadata/database.h"
#include "metadata/MetadataClasses.h"
#include "metadata/metadataitem.h"
#include "metadata/User.h"

typedef std::vector<User> UserList;
//-----------------------------------------------------------------------------
class Server : public MetadataItem,
    public boost::enable_shared_from_this<Server>
{
private:
    wxString hostnameM;
    wxString portM;

    DatabasePtrs databasesM;
    UserList usersM;

    wxString serviceUserM;
    wxString servicePasswordM;
    wxString serviceSysdbaPasswordM;
protected:
    virtual void lockChildren();
    virtual void unlockChildren();
public:
    Server();

    virtual bool getChildren(std::vector<MetadataItem *>& temp);

    DatabasePtr addDatabase();
    void addDatabase(DatabasePtr database);
    void removeDatabase(DatabasePtr database);

    DatabasePtrs::iterator begin();
    DatabasePtrs::iterator end();
    DatabasePtrs::const_iterator begin() const;
    DatabasePtrs::const_iterator end() const;

    void createDatabase(DatabasePtr db, int pagesize = 4096, int dialect = 3);

    // returns *connected* service
    bool getService(IBPP::Service& svc, ProgressIndicator* progressind,
        bool sysdba);
    void setServiceCredentials(const wxString& user, const wxString& pass);
    void setServiceSysdbaPassword(const wxString& pass);

    UserList* getUsers(ProgressIndicator* progressind);

    // setters/getters
    wxString getHostname() const;
    wxString getPort() const;
    // returns the server-related portion of the connection wxString,
    // that is server name and port number if specified.
    wxString getConnectionString() const;
    static wxString makeConnectionString(const wxString& hostname,
        const wxString& port);

    void setHostname(wxString hostname);
    void setPort(wxString port);
    virtual const wxString getTypeName() const;

    bool hasConnectedDatabase() const;
    virtual const wxString getItemPath() const;
    virtual void acceptVisitor(MetadataItemVisitor* visitor);
};
//-----------------------------------------------------------------------------
#endif
