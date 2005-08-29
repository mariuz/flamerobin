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

  Contributor(s): Marius Popa, Nando Dessena
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include <fstream>
#include <sstream>
#include <string>

#include <wx/filefn.h>

#include "config/Config.h"
#include "database.h"
#include "root.h"
#include "server.h"
#include "ugly.h"
#include "visitor.h"
//------------------------------------------------------------------------------
using namespace std;
//------------------------------------------------------------------------------
//! access to the singleton root of the DBH.
Root& getGlobalRoot()
{
	static Root globalRoot;
	return globalRoot;
}
//------------------------------------------------------------------------------
Root::Root()
    : MetadataItem(), fileNameM(""), dirtyM(false), nextIdM(1)
{
	setName("Firebird Servers");
	typeM = ntRoot;
}
//------------------------------------------------------------------------------
Root::~Root()
{
    if (dirtyM)
        save();
}
//------------------------------------------------------------------------------
//! loads servers.xml file and:

//! creates server nodes, fills their properties
//! creates database nodes for server nodes, fills their properties
//! returns: false if file cannot be loaded, true otherwise
//
bool Root::load()
{
	ifstream file(getFileName().c_str());
	if (!file)
		return false;

	// These have to be pointers since when they get pushed to vector, they relocate
	// so in order to set the right Parent for database objects, we need to know its exact
	// address (that's returned by function add()).
	// Also, we want ctor to be called for every object in vector
	Server *server = NULL;			// current server
	Database *database = NULL;		// current db

	// I had to do it this way, since standard line << file, doesn't work good if data has spaces in it.
	stringstream ss;			// read entire file into string buffer
	ss << file.rdbuf();
	string s(ss.str());
	// skip xml encoding
	string::size_type t = s.find('\n');
	string line = s.substr(0, t);
	s.erase(0, t);

    while (true)
	{
		string::size_type t = s.find('\n');
		if (t == string::npos)
			break;

		string line = s.substr(0, t);
		s.erase(0, t+1);

		string::size_type start = line.find('<');
		if (start == string::npos)
			continue;

		string::size_type end = line.find('>');
		if (end == string::npos)
			continue;

		string::size_type start2 = line.find("</");
		string option = line.substr(start+1, end-start-1);
		string value;
		if (start2 != string::npos && start2-end-1 > 0)
			value = line.substr(end+1, start2-end-1);

        // root tags
        if (option == "nextId")
        {
            stringstream ss;
	        ss << value;
	        ss >> nextIdM;
        }
        
        // server start and end tags
        if (option == "server")
		{
			Server temp;
			server = addServer(temp);
			server->lockSubject();
		}
		if (option == "/server" && server)
        {
			// backward compatibility with FR < 0.3.0
            if (server->getName().empty())
				server->setName(server->getConnectionString());
            server->unlockSubject();
            server = 0;
        }
        
        // database start and end tag
		if (option == "database" && server)
		{
			Database temp;
			database = server->addDatabase(temp);
		}
		if (option == "/database" && database)
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
		if (option == "name" && server)
		{
            if (database)
                database->setName(value);
            else
			    server->setName(value);
		}

        // database-specific subtags
        if (option == "id" && database)
        {
            int id;
            stringstream ss;
            ss << value;
            ss >> id;
            database->setId(id);
        }

		// server-specific subtags
        if (option == "host" && server)
			server->setHostname(value);
		if (option == "port" && server)
			server->setPort(value);
        // database-specific subtags
		if (option == "path" && database)
			database->setPath(value);
		if (option == "charset" && database)
			database->setCharset(value);
		if (option == "username" && database)
			database->setUsername(value);
		if (option == "password" && database)
			database->setPassword(value);
		if (option == "role" && database)
			database->setRole(value);
	}

	file.close();
	return true;
}
//------------------------------------------------------------------------------
Server *Root::addServer(Server& server)
{
	Server *temp = serversM.add(server);
	temp->setParent(this);					// grab it from collection
	dirtyM = true;
	notify();
	return temp;
}
//------------------------------------------------------------------------------
void Root::removeServer(Server* server)
{
	serversM.remove(server);
	dirtyM = true;
	notify();
}
//------------------------------------------------------------------------------
// browses the server nodes, and their database nodes
// saves everything to servers.xml file
// returns: false if file cannot be opened for writing, true otherwise
//
bool Root::save()
{
    // create directory if it doesn't exist yet.
    wxString dir = wxPathOnly(std2wx(getFileName()));
    if (!wxDirExists(dir))
        wxMkdir(dir);
    ofstream file(getFileName().c_str());
	if (!file)
		return false;
	file << "<?xml version='1.0' encoding='ISO-8859-1'?>\n";
    file << "<root>\n";
    file << "\t<nextId>" << nextIdM << "</nextId>\n";
	for (std::list<Server>::iterator it = serversM.begin(); it != serversM.end(); ++it)
	{
		file << "\t<server>\n";
		file << "\t\t<name>" << it->getName() << "</name>\n";
		file << "\t\t<host>" << it->getHostname() << "</host>\n";
		file << "\t\t<port>" << it->getPort() << "</port>\n";

		for (std::list<Database>::iterator it2 = it->getDatabases()->begin(); it2 != it->getDatabases()->end(); ++it2)
		{
			it2->resetCredentials();	// clean up eventual extra credentials
			file << "\t\t<database>\n";
			file << "\t\t\t<id>" << it2->getId() << "</id>\n";
			file << "\t\t\t<name>" << it2->getName() << "</name>\n";
			file << "\t\t\t<path>" << it2->getPath() << "</path>\n";
			file << "\t\t\t<charset>" << it2->getCharset() << "</charset>\n";
			file << "\t\t\t<username>" << it2->getUsername() << "</username>\n";
			file << "\t\t\t<password>" << it2->getPassword() << "</password>\n";
			file << "\t\t\t<role>" << it2->getRole() << "</role>\n";
			file << "\t\t</database>\n";
		}
		file << "\t</server>\n";
	}
	file << "</root>\n";

	file.close();
	return true;
}
//------------------------------------------------------------------------------
void Root::notifyAllServers()
{
	for (MetadataCollection<Server>::iterator it = serversM.begin(); it != serversM.end(); ++it)
		(*it).notify();
}
//------------------------------------------------------------------------------
bool Root::getChildren(vector<MetadataItem *>& temp)
{
	return serversM.getChildren(temp);
}
//------------------------------------------------------------------------------
bool Root::orderedChildren() const
{
    bool ordered = false;
    config().getValue("OrderServersInTree", ordered);
    return ordered;
}
//------------------------------------------------------------------------------
const string Root::getItemPath() const
{
	// Root is root, don't make the path strings any longer than needed.
	return "";
}
//------------------------------------------------------------------------------
string Root::getFileName()
{
	if (fileNameM.empty())
		fileNameM = config().getDBHFileName();
	return fileNameM;
}
//------------------------------------------------------------------------------
void Root::accept(Visitor *v)
{
	v->visit(*this);
}
//------------------------------------------------------------------------------
const unsigned int Root::getNextId()
{
    return nextIdM++;
}
//------------------------------------------------------------------------------
