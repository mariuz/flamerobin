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

//
//
//
//
//------------------------------------------------------------------------------
#ifndef FR_TABLE_H
#define FR_TABLE_H

#include "collection.h"
#include "column.h"
#include "constraints.h"
#include "trigger.h"
#include "metadataitemwithcolumns.h"
//------------------------------------------------------------------------------
class YTable: public YxMetadataItemWithColumns
{
private:
	ColumnConstraint primaryKeyM;			// table can have only one pk
	bool primaryKeyLoadedM;
	bool loadPrimaryKey();

	std::vector<ForeignKey> foreignKeysM;
	bool foreignKeysLoadedM;
	bool loadForeignKeys();

	std::vector<CheckConstraint> checkConstraintsM;
	bool checkConstraintsLoadedM;
	bool loadCheckConstraints();

	std::vector<ColumnConstraint> uniqueConstraintsM;
	bool uniqueConstraintsLoadedM;
	bool loadUniqueConstraints();

public:
	virtual std::string getCreateSqlTemplate() const;

	virtual bool loadColumns();			// update the keys info too
	bool getTriggers(std::vector<YTrigger *>& list, YTrigger::firingTimeType beforeOrAfter);

	ColumnConstraint *getPrimaryKey();
	std::vector<ForeignKey> *getForeignKeys();
	std::vector<CheckConstraint> *getCheckConstraints();
	std::vector<ColumnConstraint> *getUniqueConstraints();

	std::string getInsertStatement();
	//std::string getUpdateStatement();		// use primary key info
	//std::string getDeleteStatement();
	YTable();
	virtual const std::string getTypeName() const;
};
//------------------------------------------------------------------------------
#endif
