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

#ifndef FR_ROOT_H
#define FR_ROOT_H
//-----------------------------------------------------------------------------
#include "metadataitem.h"
#include "server.h"
//-----------------------------------------------------------------------------
class Root: public MetadataItem
{
private:
    MetadataCollection<Server> serversM;
    std::string fileNameM;
    std::string getFileName();
    bool dirtyM;
    unsigned int nextIdM;
public:
    Root();
    ~Root();

    Server *addServer(Server& server);
    void removeServer(Server* server);

    virtual bool getChildren(std::vector<MetadataItem*>& temp);
    virtual bool orderedChildren() const;

    bool load();
    bool save();
    virtual const std::string getItemPath() const;

    // updates all servers (observer pattern)
    void notifyAllServers();

    // increments the Id generator and returns the value.
    const unsigned int getNextId();

    virtual void acceptVisitor(MetadataItemVisitor* visitor);
};
//-----------------------------------------------------------------------------
Root& getGlobalRoot();
//-----------------------------------------------------------------------------
#endif
