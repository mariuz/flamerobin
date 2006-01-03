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

  Contributor(s): ________________.
*/

#ifndef FR_LOGGER_H
#define FR_LOGGER_H
//-----------------------------------------------------------------------------
// Functions used to log successfully executed statements
// in database or textual files
#include <ibpp.h>
//-----------------------------------------------------------------------------
class ExecutedStatement
{
public:
    wxString statement;
    IBPP::STT type;
    wxString terminator;
    ExecutedStatement(const wxString& st, const IBPP::STT& t,
        const wxString& term);
};
//-----------------------------------------------------------------------------
class Database;
class Config;

class Logger            // maybe we'll extend this later
{
private:
    static bool log2database(const ExecutedStatement& st, Database *db);
    static bool log2file(Config *, const ExecutedStatement& st, Database *db, const wxString& filename);
    static bool logStatementByConfig(Config *cfg, const ExecutedStatement& st, Database *db);
public:
    static bool logStatement(const ExecutedStatement& st, Database *db);
};
//-----------------------------------------------------------------------------
#endif
