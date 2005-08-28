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

  Contributor(s): ________________.
*/

// Functions used to log successfully executed statements
// in database or textual files
//
//
#ifndef FR_LOGGER_H
#define FR_LOGGER_H
#include "ibpp.h"
//-----------------------------------------------------------------------------
class executedStatement
{
public:
    std::string statement;
    IBPP::STT type;
    executedStatement(const std::string& st, const IBPP::STT& t): statement(st), type(t) {};
};
//------------------------------------------------------------------------------
class Database;

class Logger            // maybe we'll extend this later
{
private:
    static bool log2database(const executedStatement& st, Database *db);
    static bool log2file(const executedStatement& st, Database *db, const std::string& filename);
public:
    static bool logStatement(const executedStatement& st, Database *db);
};
//------------------------------------------------------------------------------
#endif
