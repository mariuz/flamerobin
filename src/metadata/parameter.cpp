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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include <sstream>
#include <string>

#include "column.h"
#include "domain.h"
#include "parameter.h"
//------------------------------------------------------------------------------
YParameter::YParameter(std::string source, int parameterType)
	: YColumn()
{
	YColumn::Init(true, source, false, "");
	typeM = ntParameter;
	parameterTypeM = (parameterType == 0 ? ptInput : ptOutput);
}
//------------------------------------------------------------------------------
YParameter::YParameter()
	: YColumn()
{
	typeM = ntParameter;
	parameterTypeM = ptInput;
}
//------------------------------------------------------------------------------
std::string YParameter::getPrintableName()
{
	std::string ret;
	ret = (parameterTypeM == ptInput ? "in " : "out ") + nameM + " " + getDomain()->getDatatypeAsString();
	return ret;
}
//------------------------------------------------------------------------------
ParameterType YParameter::getParameterType() const
{
	return parameterTypeM;
}
//------------------------------------------------------------------------------
