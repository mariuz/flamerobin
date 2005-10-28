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

  Contributor(s): Marius Popa, Nando Dessena
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

#include <wx/filefn.h>
#include <wx/filename.h>

#include <fstream>
#include <sstream>

#include "config/Config.h"
#include "database.h"
#include "MetadataItemVisitor.h"
#include "root.h"
#include "server.h"
#include "ugly.h"
//-----------------------------------------------------------------------------
using namespace std;
//-----------------------------------------------------------------------------
//! access to the singleton root of the DBH.
Root& getGlobalRoot()
{
    static Root globalRoot;
    return globalRoot;
}
//-----------------------------------------------------------------------------
Root::Root()
    : MetadataItem(), fileNameM(wxT("")), dirtyM(false), nextIdM(1)
{
    setName(wxT("Home"));
    typeM = ntRoot;
}
//-----------------------------------------------------------------------------
Root::~Root()
{
    if (dirtyM)
        save();
}
//-----------------------------------------------------------------------------
//! loads servers.xml file and:
//! creates server nodes, fills their properties
//! creates database nodes for server nodes, fills their properties
//! returns: false if file cannot be loaded, true otherwise
//
bool Root::load()
{
    wxFileName fileName = getFileName();
    if (!fileName.FileExists())
        return false;

    ifstream file(wx2std(fileName.GetFullPath()).c_str());
    if (!file)
        return false;

    // These have to be pointers since when they get pushed to vector, they relocate
    // so in order to set the right Parent for database objects, we need to know its exact
    // address (that's returned by function add()).
    // Also, we want ctor to be called for every object in vector
    Server *server = NULL;            // current server
    Database *database = NULL;        // current db

    // I had to do it this way, since standard line << file, doesn't work good if data has spaces in it.
    stringstream ss;            // read entire file into wxString buffer
    ss << file.rdbuf();
    wxString s(std2wx(ss.str()));
    // skip xml encoding
    wxString::size_type t = s.find('\n');
    wxString line = s.substr(0, t);
    s.erase(0, t);

    while (true)
    {
        wxString::size_type t = s.find('\n');
        if (t == wxString::npos)
            break;

        wxString line = s.substr(0, t);
        s.erase(0, t + 1);

        wxString::size_type start = line.find('<');
        if (start == wxString::npos)
            continue;

        wxString::size_type end = line.find('>');
        if (end == wxString::npos)
            continue;

        wxString::size_type start2 = line.find(wxT("</"));
        wxString option = line.substr(start + 1, end - start - 1);
        wxString value;
        if (start2 != wxString::npos && start2 - end - 1 > 0)
            value = line.substr(end + 1, start2 - end - 1);

        // root tags
        if (option == wxT("nextId"))
        {
            unsigned long longNextId;
            value.ToULong(&longNextId);
            nextIdM = longNextId;
        }

        // server start and end tags
        if (option == wxT("server"))
        {
            Server temp;
            server = addServer(temp);
            server->lockSubject();
        }
        if (option == wxT("/server") && server)
        {
            // backward compatibility with FR < 0.3.0
            if (server->getName().empty())
                server->setName(server->getConnectionString());
            server->unlockSubject();
            server = 0;
        }

        // database start and end tag
        if (option == wxT("database") && server)
        {
            Database temp;
            database = server->addDatabase(temp);
        }
        if (option == wxT("/database") && database)
        {
            // make sure the database has an Id before Root::save() is called,
            // otherwise a new Id will be generated then, but the generator value
            // will not be stored because it's at the beginning of the file.
            database->getId();
            // backward compatibility with FR < 0.3.0
            if (database->getName().empty())
                database->setName(database->extractNameFromConnectionString());
            database = 0;
        }

        // common subtags
        if (option == wxT("name") && server)
        {
            if (database)
                database->setName(value);
            else
                server->setName(value);
        }

        // database-specific subtags
        if (option == wxT("id") && database)
        {
            long id;
            value.ToLong(&id);
            database->setId(id);
        }

        // server-specific subtags
        if (option == wxT("host") && server)
            server->setHostname(value);
        if (option == wxT("port") && server)
            server->setPort(value);
        // database-specific subtags
        if (option == wxT("path") && database)
            database->setPath(value);
        if (option == wxT("charset") && database)
            database->setConnectionCharset(value);
        if (option == wxT("username") && database)
            database->setUsername(value);
        if (option == wxT("password") && database)
            database->setPassword(value);
        if (option == wxT("role") && database)
            database->setRole(value);
    }

    file.close();
    return true;
}
//-----------------------------------------------------------------------------
Server* Root::addServer(Server& server)
{
    Server* temp = serversM.add(server);
    temp->setParent(this);                    // grab it from collection
    dirtyM = true;
    notifyObservers();
    getGlobalRoot().save();
    return temp;
}
//-----------------------------------------------------------------------------
void Root::removeServer(Server* server)
{
    serversM.remove(server);
    dirtyM = true;
    notifyObservers();
    getGlobalRoot().save();
}
//-----------------------------------------------------------------------------
// browses the server nodes, and their database nodes
// saves everything to servers.xml file
// returns: false if file cannot be opened for writing, true otherwise
//
bool Root::save()
{
    // create directory if it doesn't exist yet.
    wxString dir = wxPathOnly(getFileName());
    if (!wxDirExists(dir))
        wxMkdir(dir);
    ofstream file(wx2std(getFileName()).c_str());
    if (!file)
        return false;
    file << "<?xml version='1.0' encoding='ISO-8859-1'?>\n";
    file << "<root>\n";
    file << "\t<nextId>" << nextIdM << "</nextId>\n";
    for (std::list<Server>::iterator it = serversM.begin(); it != serversM.end(); ++it)
    {
        file << "\t<server>\n";
        file << "\t\t<name>" << wx2std(it->getName()) << "</name>\n";
        file << "\t\t<host>" << wx2std(it->getHostname()) << "</host>\n";
        file << "\t\t<port>" << wx2std(it->getPort()) << "</port>\n";

        for (std::list<Database>::iterator it2 = it->getDatabases()->begin(); it2 != it->getDatabases()->end(); ++it2)
        {
            it2->resetCredentials();    // clean up eventual extra credentials
            file << "\t\t<database>\n";
            file << "\t\t\t<id>" << wx2std(it2->getId()) << "</id>\n";
            file << "\t\t\t<name>" << wx2std(it2->getName()) << "</name>\n";
            file << "\t\t\t<path>" << wx2std(it2->getPath()) << "</path>\n";
            file << "\t\t\t<charset>" << wx2std(it2->getConnectionCharset()) << "</charset>\n";
            file << "\t\t\t<username>" << wx2std(it2->getUsername()) << "</username>\n";
            file << "\t\t\t<password>" << wx2std(it2->getPassword()) << "</password>\n";
            file << "\t\t\t<role>" << wx2std(it2->getRole()) << "</role>\n";
            file << "\t\t</database>\n";
        }
        file << "\t</server>\n";
    }
    file << "</root>\n";

    file.close();
    return true;
}
//-----------------------------------------------------------------------------
void Root::notifyAllServers()
{
    for (MetadataCollection<Server>::iterator it = serversM.begin(); it != serversM.end(); ++it)
        (*it).notifyObservers();
}
//-----------------------------------------------------------------------------
bool Root::getChildren(vector<MetadataItem *>& temp)
{
    return serversM.getChildren(temp);
}
//-----------------------------------------------------------------------------
bool Root::orderedChildren() const
{
    bool ordered = false;
    config().getValue(wxT("OrderServersInTree"), ordered);
    return ordered;
}
//-----------------------------------------------------------------------------
const wxString Root::getItemPath() const
{
    // Root is root, don't make the path strings any longer than needed.
    return wxT("");
}
//-----------------------------------------------------------------------------
wxString Root::getFileName()
{
    if (fileNameM.empty())
        fileNameM = config().getDBHFileName();
    return fileNameM;
}
//-----------------------------------------------------------------------------
const unsigned int Root::getNextId()
{
    return nextIdM++;
}
//-----------------------------------------------------------------------------
void Root::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visit(*this);
}
//-----------------------------------------------------------------------------
