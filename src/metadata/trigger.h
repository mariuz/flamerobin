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
#ifndef FR_TRIGGER_H
#define FR_TRIGGER_H

#include "metadataitem.h"

class YTrigger: public YxMetadataItem
{
private:
	bool infoIsLoadedM;
	std::string objectM;
	bool activeM;
	int positionM;
	std::string triggerTypeM;

public:
	enum firingTimeType { afterTrigger, beforeTrigger };
	std::string getCreateSqlTemplate() const;	// overrides YxMetadataItem::getCreateSqlTemplate()

	bool loadInfo(bool force = false);
	bool getTriggerInfo(std::string& object, bool& active, int& position, std::string& type);
	bool getSource(std::string& source) const;
	static std::string getTriggerType(int flags);
	firingTimeType getFiringTime();
	std::string getAlterSql();

	YTrigger();
	virtual const std::string getTypeName() const;
};
//------------------------------------------------------------------------------
#endif
