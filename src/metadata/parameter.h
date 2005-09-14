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
#ifndef FR_PARAMETER_H
#define FR_PARAMETER_H

#include "column.h"
#include "metadataitem.h"

typedef enum { ptInput, ptOutput } ParameterType;

class Parameter: public Column
{
private:
	ParameterType parameterTypeM;
public:
	Parameter();
	Parameter(wxString source, int parameterType);
	wxString getPrintableName();
	ParameterType getParameterType() const;
    virtual void acceptVisitor(MetadataItemVisitor* visitor);
};
//-----------------------------------------------------------------------------
#endif
