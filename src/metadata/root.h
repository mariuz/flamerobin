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
#include "metadata/metadataitem.h"
#include "metadata/server.h"

class Database;
class wxXmlNode;
//-----------------------------------------------------------------------------
class Root: public MetadataItem
{
private:
    MetadataCollection<Server> serversM;

    Server* unregLocalDatabasesM;

    wxString fileNameM;
    wxString getFileName();
    bool dirtyM;
    bool loadingM;
    unsigned int nextIdM;

    bool parseDatabase(Server* server, wxXmlNode* xmln);
    bool parseServer(wxXmlNode* xmln);
public:
    Root();
    ~Root();

    virtual void lockChildren();
    virtual void unlockChildren();

    Server* addServer(Server& server);
    void removeServer(Server* server);

    Database* addUnregisteredDatabase(Database& database);

    virtual bool getChildren(std::vector<MetadataItem*>& temp);

    bool load();
    bool save();
    virtual const wxString getItemPath() const;

    // updates all servers (observer pattern)
    void notifyAllServers();
    void disconnectAllDatabases();

    // increments the Id generator and returns the value.
    const unsigned int getNextId();

    virtual void acceptVisitor(MetadataItemVisitor* visitor);
};
//-----------------------------------------------------------------------------
Root& getGlobalRoot();
//-----------------------------------------------------------------------------
#endif
