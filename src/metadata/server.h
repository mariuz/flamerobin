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

//-----------------------------------------------------------------------------
#ifndef FR_SERVER_H
#define FR_SERVER_H

#include "collection.h"
#include "database.h"
#include "metadataitem.h"
//-----------------------------------------------------------------------------
// this is a coupled node (in visual sense). Server equals collection of YDatabases in wxTree
// that's why getChildren() method just copies, since wxTree item will have pointer to Server.
class Server: public MetadataItem
{
private:
	std::string hostnameM;
	std::string portM;

	MetadataCollection<Database> databasesM;

public:
	Server();
	virtual bool getChildren(std::vector<MetadataItem *>& temp);
    virtual bool orderedChildren() const;
	Database* addDatabase(Database&);
	void removeDatabase(Database*);
	MetadataCollection<Database> *getDatabases();

	void createDatabase(Database *db, int pagesize = 4096, int dialect = 3);

	// setters/getters
	std::string getHostname() const;
	std::string getPort() const;
	// returns the server-related portion of the connection string,
    // that is server name and port number if specified.
    std::string getConnectionString() const;

	void setHostname(std::string hostname);
	void setPort(std::string port);
	virtual const std::string getTypeName() const;

	bool hasConnectedDatabase() const;
	virtual const string getItemPath() const;
    virtual void acceptVisitor(MetadataItemVisitor* visitor);
};
//-----------------------------------------------------------------------------
#endif
