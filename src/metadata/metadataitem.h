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
class Root;
class Database;
class Dependency;

//------------------------------------------------------------------------------
using namespace std;
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
NodeType getTypeByName(string name);
//------------------------------------------------------------------------------
class MetadataItem: public Item
{
private:
	string nameM;
	MetadataItem *parentM;
	string descriptionM;
	bool descriptionLoadedM;
protected:
	NodeType typeM;
public:
    virtual void accept(Visitor *v);

	bool getDependencies(vector<Dependency>& list, bool ofObject);	// load from db

	MetadataItem();
	virtual ~MetadataItem() {};

	Database *getDatabase() const;
    Root* getRoot() const;

	virtual bool getChildren(vector<MetadataItem *>& temp);
	virtual size_t getChildrenCount() const { return 0; };
	void drop();	// removes its children (by calling drop() for each) and notifies it's parent
    virtual bool orderedChildren() const { return false; };

	// returns CREATE SQL statement template
	virtual string getCreateSqlTemplate() const { return ""; };

    // returns complete DROP SQL statement
    virtual string getDropSqlStatement() const;

	// getters/setters
	virtual MetadataItem *getParent() const;
	void setParent(MetadataItem *parent);
	virtual const string& getName() const;
	virtual string getPrintableName();
	void setName(string name);
	virtual NodeType getType() const;
	void setType(NodeType type);

	// returns the name of the data type (f. ex. TABLE)
	virtual const string getTypeName() const;

	// returns the item path, currently only used to store settings in config().
	// It could also be used to locate items in the DBH tree.
	virtual const string getItemPath() const;

	// returns the id string of the metadata item that contributes to the path. The
	// predefined implementation just returns getId().
	virtual const string getPathId() const;
    // returns the id of the item (to be saved in config files, etc.).
    // The predefined implementation just returns getName().
    virtual const string getId() const;

	// items description (in database)
	string getDescription();
	bool setDescription(string description);
	virtual string getDescriptionSql() const;
	virtual string getChangeDescriptionSql() const;

	// returns true if the metadata item is a system (as opposed to user-defined) item.
	virtual bool isSystem() const;

    static const string pathSeparator;
};
//------------------------------------------------------------------------------
//! masks the object it points to so others see it transparently
class Dependency: public MetadataItem
{
private:
	MetadataItem *objectM;
	string fieldsM;
public:
	virtual MetadataItem *getParent() const { return objectM->getParent(); };
	virtual const string& getName() const { return objectM->getName(); };
	virtual NodeType getType() const { return objectM->getType(); };
	virtual const string getTypeName() const { return objectM->getTypeName(); };

	Dependency(MetadataItem *object) { objectM = object; };
	string getFields() const { return fieldsM; };
	void addField(const string& name) { if (!fieldsM.empty()) fieldsM += ","; fieldsM += name; };
	void setFields(const string& fields) { fieldsM = fields; };
};
//------------------------------------------------------------------------------
#endif
