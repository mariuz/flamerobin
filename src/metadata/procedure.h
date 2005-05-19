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
#ifndef FR_PROCEDURE_H
#define FR_PROCEDURE_H

#include "metadataitem.h"
#include "parameter.h"

class YProcedure: public YxMetadataItem
{
private:
	bool parametersLoadedM;
	bool loadParameters();
	YMetadataCollection <YParameter> parametersM;

public:
	std::string getCreateSqlTemplate() const;	// overrides YxMetadataItem::getCreateSqlTemplate()

	bool getChildren(std::vector<YxMetadataItem *>& temp);
	YParameter *addParameter(YParameter &c);

	std::string getSelectStatement(bool withColumns);
	bool isSelectable();

	bool checkAndLoadParameters(bool force = false);
	bool getSource(std::string& source);
	std::string getAlterSql();
	std::string getDefinition();

	YProcedure();
	virtual const std::string getTypeName() const;
};
//------------------------------------------------------------------------------
#endif
