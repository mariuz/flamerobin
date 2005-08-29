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

//
//
//
//
//-----------------------------------------------------------------------------
#ifndef FR_DOMAIN_H
#define FR_DOMAIN_H

#include <string>
#include "metadataitem.h"

class Domain: public MetadataItem
{
private:
	short datatypeM, subtypeM, lengthM, precisionM, scaleM;
	std::string charsetM;
	bool infoLoadedM;

public:
    virtual void accept(Visitor *v);

	Domain();

	static std::string datatype2string(short datatype, short scale, short precision, short subtype, short length);
	bool loadInfo();
	std::string getDatatypeAsString();
	void getDatatypeParts(std::string& type, std::string& size, std::string& scale);
	std::string getCharset();
	virtual const std::string getTypeName() const;
	virtual std::string getCreateSqlTemplate() const;
	virtual std::string getPrintableName();
};
//-----------------------------------------------------------------------------
#endif
