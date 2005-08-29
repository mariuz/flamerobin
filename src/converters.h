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

#ifndef FR_CONVERTERS_H
#define FR_CONVERTERS_H
//--------------------------------------------------------------------------------------
#include <string>

#include <ibpp.h>
//--------------------------------------------------------------------------------------
//! Various functions that convert IBPP datatypes into human readable form
std::string GetHumanDate(int year, int month, int day, std::string DateFormat);
std::string GetHumanTime(int hour, int minute, int second, std::string TimeFormat);
std::string GetHumanTimestamp(IBPP::Timestamp ts, std::string DateFormat, 
    std::string TimeFormat);
bool CreateString(IBPP::Statement& st, int col, std::string& s);
//--------------------------------------------------------------------------------------
#endif // FR_CONVERTERS_H
