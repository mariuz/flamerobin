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
#include "config.h"
#include "root.h"
#include "server.h"
#include "database.h"
//------------------------------------------------------------------------------
// This code should be wx-clean. Only std library
using namespace std;

//------------------------------------------------------------------------------
//! access to the singleton root of the DBH.
YRoot& getGlobalRoot()
{
	static YRoot globalRoot;
	return globalRoot;
}
//------------------------------------------------------------------------------
//! loads servers.xml file and:

//! creates server nodes, fills their properties
//! creates database nodes for server nodes, fills their properties
//! returns: false if file cannot be loaded, true otherwise
//
bool YRoot::load()
{
	std::ifstream file(getFileName().c_str());
	if (!file)
		return false;

	// These have to be pointers since when they get pushed to vector, they relocate
	// so in order to set the right Parent for database objects, we need to know its exact
	// address (that's returned by function add()).
	// Also, we want ctor to be called for every object in vector
	YServer *server = 0;			// current server
	YDatabase *database = 0;		// current db

	// I had to do it this way, since standard line << file, doesn't work good if data has spaces in it.
	std::stringstream ss;			// read entire file into string buffer
	ss << file.rdbuf();
	std::string s(ss.str());
	// skip xml encoding
	std::string::size_type t = s.find('\n');
	std::string line = s.substr(0, t);
	s.erase(0, t);

	while (true)
	{
		std::string::size_type t = s.find('\n');
		if (t == std::string::npos)
			break;

		std::string line = s.substr(0, t);
		s.erase(0, t+1);

		string::size_type start = line.find('<');
		if (start == string::npos)
			continue;

		string::size_type end = line.find('>');
		if (end == string::npos)
			continue;

		string::size_type start2 = line.find("</");
		std::string option = line.substr(start+1, end-start-1);
		std::string value;
		if (start2 != string::npos && start2-end-1 > 0)
			value = line.substr(end+1, start2-end-1);
		else
			value.empty();

		if (option == "server")				// new server
		{
			YServer temp;
			server = addServer(temp);
			server->lockSubject();
		}

		if (option == "/server" && server)
			server->unlockSubject();

		if (option == "host" && server)
			server->setHostname(value);
		if (option == "port" && server)
			server->setPort(value);

		if (option == "database" && server)	// database definition complete
		{
			YDatabase temp;
			database = server->addDatabase(temp);		// add it to the list
		}

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
	//notify();
	return true;
}
//------------------------------------------------------------------------------
YServer *YRoot::addServer(YServer& server)
{
	YServer *temp = serversM.add(server);
	temp->setParent(this);					// grab it from collection
	notify();
	return temp;
}
//------------------------------------------------------------------------------
void YRoot::removeServer(YServer* server)
{
	serversM.remove(server);
	notify();
}
//------------------------------------------------------------------------------
// browses the server nodes, and their database nodes
// saves everything to servers.xml file
// returns: false if file cannot be opened for writing, true otherwise
//
bool YRoot::save()
{
	std::ofstream file(getFileName().c_str());
	if (!file)
		return false;
	file << "<?xml version='1.0' encoding='ISO-8859-2'?>\n";
	for (std::list<YServer>::const_iterator it = serversM.begin(); it != serversM.end(); ++it)
	{
		file << "<server>\n";
		file << "\t<host>" << it->getHostname() << "</host>\n";
		file << "\t<port>" << it->getPort() << "</port>\n";

		for (std::list<YDatabase>::const_iterator it2 = it->getDatabases()->begin(); it2 != it->getDatabases()->end(); ++it2)
		{
			file << "\t<database>\n";
			file << "\t\t<path>" << it2->getPath() << "</path>\n";
			file << "\t\t<charset>" << it2->getCharset() << "</charset>\n";
			file << "\t\t<username>" << it2->getUsername() << "</username>\n";
			file << "\t\t<password>" << it2->getPassword() << "</password>\n";
			file << "\t\t<role>" << it2->getRole() << "</role>\n";
			file << "\t</database>\n";
		}

		file << "</server>\n";
	}

	file.close();
	return true;
}
//------------------------------------------------------------------------------
YRoot::YRoot()
	: fileNameM("")
{
	parentM = 0;
	nameM = "Firebird Servers";
	typeM = ntRoot;
}
//------------------------------------------------------------------------------
YRoot::~YRoot()
{
	save();
}
//------------------------------------------------------------------------------
bool YRoot::getChildren(std::vector<YxMetadataItem *>& temp)
{
	return serversM.getChildren(temp);
}
//------------------------------------------------------------------------------
bool YRoot::orderedChildren() const
{
    bool ordered = false;
    config().getValue("OrderServersInTree", ordered);
    return ordered;
}
//------------------------------------------------------------------------------
const std::string YRoot::getItemPath() const
{
	// Root is root, don't make the path strings any longer than needed.
	return "";
}
//------------------------------------------------------------------------------
std::string YRoot::getFileName()
{
	if (fileNameM.empty())
		fileNameM = config().getDBHFileName();
	return fileNameM;
}

