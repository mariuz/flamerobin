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

#ifndef FR_ROOT_H
#define FR_ROOT_H
//-----------------------------------------------------------------------------
#include <vector>

#include <boost/shared_ptr.hpp>

#include "metadata/metadataitem.h"
#include "metadata/server.h"

class Database;
class Root;
class wxXmlNode;

typedef boost::shared_ptr<Root> SharedRootPtr;
typedef boost::shared_ptr<Server> SharedServerPtr;
typedef std::vector<SharedServerPtr> SharedServers;
//-----------------------------------------------------------------------------
class Root: public MetadataItem
{
private:
    SharedServers serversM;
    Server* unregLocalDatabasesM;

    wxString fileNameM;
    wxString getFileName();

    bool parseDatabase(SharedServerPtr server, wxXmlNode* xmln);
    bool parseServer(wxXmlNode* xmln);
protected:
    virtual void lockChildren();
    virtual void unlockChildren();
public:
    Root();
    ~Root();

    SharedServerPtr addServer();
    void addServer(SharedServerPtr server);
    void removeServer(Server* server);
    void addUnregisteredDatabase(SharedDatabasePtr database);

    SharedServers::iterator begin();
    SharedServers::iterator end();
    SharedServers::const_iterator begin() const;
    SharedServers::const_iterator end() const;

    virtual bool getChildren(std::vector<MetadataItem*>& temp);

    bool load();
    bool save();
    virtual const wxString getItemPath() const;

    void disconnectAllDatabases();

    virtual void acceptVisitor(MetadataItemVisitor* visitor);
};
//-----------------------------------------------------------------------------
#endif
