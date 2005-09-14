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
#ifndef FR_DOMAIN_H
#define FR_DOMAIN_H

#include "metadataitem.h"
//-----------------------------------------------------------------------------
class Domain: public MetadataItem
{
private:
	short datatypeM, subtypeM, lengthM, precisionM, scaleM;
	wxString charsetM;
	bool infoLoadedM;

public:
	Domain();

	static wxString datatype2string(short datatype, short scale, short precision, short subtype, short length);
	bool loadInfo();
	wxString getDatatypeAsString();
	void getDatatypeParts(wxString& type, wxString& size, wxString& scale);
	wxString getCharset();
	virtual const wxString getTypeName() const;
	virtual wxString getCreateSqlTemplate() const;
	virtual wxString getPrintableName();
    virtual void acceptVisitor(MetadataItemVisitor* v);
};
//-----------------------------------------------------------------------------
#endif
