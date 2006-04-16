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

//-----------------------------------------------------------------------------
#ifndef FR_SERVER_H
#define FR_SERVER_H

#include "ibpp/ibpp.h"

#include "metadata/collection.h"
#include "metadata/database.h"
#include "metadata/metadataitem.h"

class User: public MetadataItem
{
public:
    User(Server *parent)
        :MetadataItem()
    {
        setParent((MetadataItem *)parent);
    }

    User(const IBPP::User& src, Server *parent)
        :MetadataItem(), useridM(src.userid), groupidM(src.groupid)
    {
        setParent((MetadataItem *)parent);
        usernameM = std2wx(src.username);
        passwordM = std2wx(src.password);
        firstnameM = std2wx(src.firstname);
        middlenameM = std2wx(src.middlename);
        lastnameM = std2wx(src.lastname);
    }

    void setIBPP(IBPP::User& dest) const
    {
        dest.username = wx2std(usernameM);
        dest.password = wx2std(passwordM);
        dest.firstname = wx2std(firstnameM);
        dest.lastname = wx2std(lastnameM);
        dest.middlename = wx2std(middlenameM);
        dest.userid = useridM;
        dest.groupid = groupidM;
    }

    bool operator<(const User& rhs) const
    {
        return usernameM < rhs.usernameM;
    }

    wxString usernameM;
    wxString passwordM;
    wxString firstnameM;
    wxString middlenameM;
    wxString lastnameM;
    uint32_t useridM;
    uint32_t groupidM;
};
//-----------------------------------------------------------------------------
// this is a coupled node (in visual sense). Server equals collection of
// YDatabases in wxTree. that's why getChildren() method just copies, since
// wxTree item will have pointer to Server
class Server: public MetadataItem
{
private:
    wxString hostnameM;
    wxString portM;

    MetadataCollection<Database> databasesM;
    std::vector<User> usersM;

    wxString serviceUserM;
    wxString servicePasswordM;
    wxString serviceSysdbaPasswordM;

public:
    Server();
    Server(const Server& rhs);

    virtual void lockChildren();
    virtual void unlockChildren();

    virtual bool getChildren(std::vector<MetadataItem *>& temp);
    virtual bool orderedChildren() const;
    Database* addDatabase(Database&);
    void removeDatabase(Database*);
    MetadataCollection<Database> *getDatabases();

    void createDatabase(Database *db, int pagesize = 4096, int dialect = 3);

    // returns *connected* service
    bool getService(IBPP::Service& svc, ProgressIndicator* progressind,
        bool sysdba);
    void setServiceUser(const wxString& user);
    void setServicePassword(const wxString& pass);
    void setServiceSysdbaPassword(const wxString& pass);

    std::vector<User>* getUsers(ProgressIndicator* progressind);

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
