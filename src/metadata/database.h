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

  Contributor(s): Nando Dessena
*/

//
//
//
//
//------------------------------------------------------------------------------
#ifndef FR_DATABASE_H
#define FR_DATABASE_H

#include "metadataitem.h"
#include "domain.h"
#include "function.h"
#include "collection.h"
#include "domain.h"
#include "exception.h"
#include "generator.h"
#include "procedure.h"
#include "role.h"
#include "table.h"
#include "trigger.h"
#include "view.h"
#include <map>
#include <ibpp.h>

// for threading stuff
#include <wx/wx.h>
#include <wx/thread.h>

//------------------------------------------------------------------------------
class YDatabase: public YxMetadataItem
{
private:
	IBPP::Database databaseM;
	bool connectedM;

	std::string pathM;
	std::string charsetM;
	std::string roleM;
	std::string usernameM;
	std::string passwordM;
	std::string validPasswordM;		// set when connection successful

	YMetadataCollection<YDomain> domainsM;
    YMetadataCollection<YException> exceptionsM;
    YMetadataCollection<YFunction> functionsM;
    YMetadataCollection<YGenerator> generatorsM;
    YMetadataCollection<YProcedure> proceduresM;
    YMetadataCollection<YRole> rolesM;
    YMetadataCollection<YTable> tablesM;
    YMetadataCollection<YTrigger> triggersM;
    YMetadataCollection<YView> viewsM;

	std::multimap<std::string, std::string> collationsM;
	void loadCollations();

public:
	YDatabase();
	virtual bool getChildren(std::vector<YxMetadataItem *>& temp);

	YMetadataCollection<YGenerator>::const_iterator generatorsBegin();
	YMetadataCollection<YGenerator>::const_iterator generatorsEnd();
	YMetadataCollection<YDomain>::const_iterator domainsBegin();
	YMetadataCollection<YDomain>::const_iterator domainsEnd();
	YMetadataCollection<YTable>::const_iterator tablesBegin();
	YMetadataCollection<YTable>::const_iterator tablesEnd();

	void clear();				// sets all values to empty string
	bool isConnected() const;
	bool connect(std::string password);
	bool disconnect();
	bool reconnect() const;

	std::string YDatabase::loadDomainNameForColumn(std::string table, std::string field);
	bool loadObjects(NodeType type);
	//std::string getLoadingSql(NodeType type);

	YxMetadataItem *findByNameAndType(NodeType nt, std::string name);
	void refreshByType(NodeType type);
	void dropObject(YxMetadataItem *object);
	bool addObject(NodeType type, std::string name);
	bool parseCommitedSql(std::string sql);		// reads a DDL statement and does accordingly

	std::vector<std::string> getCollations(std::string charset);

	//! fill vector with names of all tables, views, etc.
	void getIdentifiers(std::vector<std::string>& temp);

	std::string getPath() const;
	std::string getCharset() const;
	std::string getUsername() const;
	std::string getPassword() const;
	std::string getRole() const;
	IBPP::Database& getDatabase();
	void setPath(std::string value);
	void setCharset(std::string value);
	void setUsername(std::string value);
	void setPassword(std::string value);
	void setRole(std::string value);
	virtual const std::string getTypeName() const;
};
//----------------------------------------------------------------------------
#endif
