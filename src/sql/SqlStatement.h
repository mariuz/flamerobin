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
  are Copyright (C) 2005 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s):
*/
//-----------------------------------------------------------------------------
#ifndef FR_SQL_STATEMENT_H
#define FR_SQL_STATEMENT_H

#include <vector>
#include <map>
#include "metadata/metadataitem.h"
#include "SqlTokenizer.h"
class Relation;
class Database;
//-----------------------------------------------------------------------------
typedef enum
{
    actNONE, actALTER, actCREATE, actCREATE_OR_ALTER, actDECLARE, actDROP,
    actRECREATE, actSET, actUPDATE
} SqlAction;
//-----------------------------------------------------------------------------
class TokenList
{
private:
    std::vector<SqlTokenType> tokensM;
public:
	size_t size() const;
	void add(const SqlTokenType& item);
	SqlTokenType& operator[](const size_t& index);
};
//-----------------------------------------------------------------------------
class SqlStatement
{
public:
	SqlStatement(const wxString& sql, Database *db);

	bool isDDL() const;
	SqlAction getAction() const;
	bool actionIs(const SqlAction& act, NodeType nt = ntUnknown) const;
	NodeType getObjectType() const;
	MetadataItem* getObject();
	Identifier getIdentifier();
	wxString getName();
	wxString getFieldName();
	bool isAlterColumn() const;
	bool isDatatype() const;
	Relation* getCreateTriggerRelation();

protected:
    TokenList tokensM;
	size_t identifierTokenIndexM;
    std::map<int, wxString> tokenStringsM;

	MetadataItem *objectM;
	Database *databaseM;
	NodeType objectTypeM;
	SqlAction actionM;
    Identifier nameM;
    Identifier fieldNameM;	// table columns
	bool isAlterColumnM;
	bool isDatatypeM;
};
//-----------------------------------------------------------------------------
#endif
