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

  Contributor(s):
*/

//
//
//
//
//-----------------------------------------------------------------------------
#ifndef FR_COLUMN_H
#define FR_COLUMN_H

#include "metadataitem.h"
#include "domain.h"

class Column: public MetadataItem
{
private:
	bool notnullM, computedM;
	std::string sourceM, computedSourceM, collationM;
public:
    virtual void accept(Visitor *v);

	Column();
	void Init(bool notnull, std::string source, bool computed, std::string computedSource, std::string collation);
	virtual std::string getPrintableName();
	std::string getDatatype();
    virtual std::string getDropSqlStatement() const;

	bool isNullable() const;
	bool isPrimaryKey() const;
	bool isComputed() const;
	std::string getSource() const;
	std::string getCollation() const;
	Domain *getDomain() const;
};
//-----------------------------------------------------------------------------
#endif
