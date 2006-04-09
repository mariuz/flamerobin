/*
Copyright (c) 2004, 2005, 2006 The FlameRobin Development Team

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


  $Id$

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
