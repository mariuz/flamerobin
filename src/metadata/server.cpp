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

  Contributor(s):
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "server.h"
//------------------------------------------------------------------------------
YServer::YServer()
{
	typeM = ntServer;

	hostnameM = "";
	portM = "3050";

	databasesM.setParent(this);
	databasesM.setType(ntServer);
}
//------------------------------------------------------------------------------
bool YServer::getChildren(std::vector<YxMetadataItem *>& temp)
{
	return databasesM.getChildren(temp);
}
//------------------------------------------------------------------------------
// returns pointer to object in vector
YDatabase* YServer::addDatabase(YDatabase& db)
{
	YDatabase *temp = databasesM.add(db);
	temp->setParent(this);					// grab it from collection
	notify();
	return temp;
}
//------------------------------------------------------------------------------
void YServer::removeDatabase(YDatabase *db)
{
	databasesM.remove(db);
	notify();
}
//------------------------------------------------------------------------------
void YServer::createDatabase(YDatabase *db, std::string extra_params, int dialect)
{
	IBPP::Database db1;
	db1 = IBPP::DatabaseFactory( hostnameM, db->getPath(), db->getUsername(), db->getPassword(),
		"", db->getCharset(), extra_params);
	db1->Create(dialect);
}
//------------------------------------------------------------------------------
const YMetadataCollection<YDatabase> *YServer::getDatabases() const
{
	return &databasesM;
};
//------------------------------------------------------------------------------
void YServer::createName()		// creates name for the node using hostname and port values
{
	nameM = hostnameM;
	if (portM != "3050")
		nameM += "/" + portM;
	databasesM.setName(nameM);
	notify();
}
//------------------------------------------------------------------------------
std::string YServer::getHostname() const
{
	return hostnameM;
}
//------------------------------------------------------------------------------
std::string YServer::getPort() const
{
	return portM;
}
//------------------------------------------------------------------------------
bool YServer::hasConnectedDatabase() const
{
	for (YMetadataCollection<YDatabase>::const_iterator it = databasesM.begin(); 
		it != databasesM.end(); ++it)
	{
		if ((*it).isConnected())
			return true;
	}
	return false;
}
//------------------------------------------------------------------------------
void YServer::setHostname(std::string hostname)
{
	hostnameM = hostname;
	createName();
}
//------------------------------------------------------------------------------
void YServer::setPort(std::string port)
{
	portM = port;
	createName();
}
//------------------------------------------------------------------------------
const std::string YServer::getTypeName() const
{
	return "SERVER";
}
//------------------------------------------------------------------------------

