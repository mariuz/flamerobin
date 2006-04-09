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
    bool getService(IBPP::Service& svc, ProgressIndicator* progressind = 0);

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
