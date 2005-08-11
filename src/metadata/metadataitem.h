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
#ifndef FR_METADATA_H
#define FR_METADATA_H

#include <string>
#include <vector>

#include "item.h"

// forward declarations
class Database;
class Dependency;

//------------------------------------------------------------------------------
typedef enum { ntUnknown, ntRoot, ntServer, ntDatabase,
	ntTable, ntView, ntProcedure, ntTrigger, ntGenerator, ntFunction,	ntSysTable,			// each item
	ntTables, ntViews, ntProcedures, ntTriggers, ntGenerators,	ntFunctions, ntSysTables,	// ^^^ parent of those
	ntColumn, ntDomains, ntRole, ntRoles, ntDomain, ntParameter, ntIndex,
    ntException, ntExceptions,
	ntPrimaryKey, ntComputed,		// these are used only for images
	ntLastType
} NodeType;
//------------------------------------------------------------------------------
NodeType getTypeByName(std::string name);
//------------------------------------------------------------------------------
class MetadataItem: public Item
{
protected:
	std::string nameM;
	MetadataItem *parentM;
	std::string descriptionM;
	bool descriptionLoadedM;
	NodeType typeM;
public:
    virtual void accept(Visitor *v);

	bool getDependencies(std::vector<Dependency>& list, bool ofObject);	// load from db

	MetadataItem();
	virtual ~MetadataItem() {};

	Database *getDatabase() const;

	virtual bool getChildren(std::vector<MetadataItem *>& temp);
	virtual size_t getChildrenCount() const { return 0; };
	void drop();	// removes its children (by calling drop() for each) and notifies it's parent
    virtual bool orderedChildren() const { return false; };

	// returns CREATE SQL statement template
	virtual std::string getCreateSqlTemplate() const { return ""; };

    // returns complete DROP SQL statement
    virtual std::string getDropSqlStatement() const;

	// getters/setters
	virtual MetadataItem *getParent() const;
	void setParent(MetadataItem *parent);
	virtual const std::string& getName() const;
	virtual std::string getPrintableName();
	void setName(std::string name);
	virtual NodeType getType() const;
	void setType(NodeType type);

	// returns the name of the data type (f. ex. TABLE)
	virtual const std::string getTypeName() const;

	// returns the item path, currently only used to store settings in config().
	// It could also be used to locate items in the DBH tree.
	virtual const std::string getItemPath() const;

	// returns the name of the metadata item that contributes to the path. The
	// predefined implementation just returns getName().
	virtual const std::string getPathName() const;

	// items description (in database)
	std::string getDescription();
	bool setDescription(std::string description);
	virtual std::string getDescriptionSql() const;
	virtual std::string getChangeDescriptionSql() const;

	// returns true if the metadata item is a system (as opposed to user-defined) item.
	virtual bool isSystem() const;
};
//------------------------------------------------------------------------------
//! masks the object it points to so others see it transparently
class Dependency: public MetadataItem
{
private:
	MetadataItem *objectM;
	std::string fieldsM;
public:
	virtual MetadataItem *getParent() const { return objectM->getParent(); };
	virtual const std::string& getName() const { return objectM->getName(); };
	virtual NodeType getType() const { return objectM->getType(); };
	virtual const std::string getTypeName() const { return objectM->getTypeName(); };

	Dependency(MetadataItem *object) { objectM = object; };
	std::string getFields() const { return fieldsM; };
	void addField(const std::string& name) { if (!fieldsM.empty()) fieldsM += ","; fieldsM += name; };
	void setFields(const std::string& fields) { fieldsM = fields; };
};
//------------------------------------------------------------------------------
#endif
