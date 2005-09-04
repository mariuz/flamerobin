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
// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/datetime.h>
#include <wx/file.h>

#include "config/DatabaseConfig.h"
#include "frversion.h"
#include "logger.h"
#include "metadata/database.h"
#include "ugly.h"

//----------------------------------------------------------------------------
bool Logger::log2database(const executedStatement& /*st*/, Database* /*db*/)
{
    return true;
}
//----------------------------------------------------------------------------
bool Logger::log2file(Config *cfg, const executedStatement& st,
    Database *db, const std::string& filename)
{
    enum { singleFile=0, multiFile };
    int logToFileType;
    cfg->getValue("LogToFileType", logToFileType);

    std::string sql = st.statement;
    if (logToFileType == singleFile)         // add ; to statement if missing
    {
        sql.erase(sql.find_last_not_of(" \n\t\r")+1);           // trim
        std::string::size_type pos = sql.find_last_of(";");
        if (pos == std::string::npos || pos < sql.length() - 1)
            sql += ";";
    }

    wxFile f;
    if (logToFileType == multiFile)
    {
        if (filename.find_last_of("%d") == std::string::npos) // %d not found
            return false;
        wxString test;
        int start = 1;
        cfg->getValue("IncrementalLogFileStart", start);
        for (int i=start; i < 100000; ++i) // dummy test for 100000
        {
            test.Printf(std2wx(filename), i);
            if (!wxFileExists(test))
            {
                if (f.Open(test, wxFile::write))
                    break;
            }
        }
        if (!f.IsOpened())
            return false;
    }
    else
        if (!f.Open(std2wx(filename), wxFile::write_append )) // cannot open
            return false;

    bool loggingAddHeader = true;
    cfg->getValue("LoggingAddHeader", loggingAddHeader);
    if (loggingAddHeader)
    {
        wxString header = wxString::Format(
            _("\n/* Logged by FlameRobin %d.%d.%d at %s\n   User: %s    Database: %s */\n"),
            FR_VERSION_MAJOR, FR_VERSION_MINOR, FR_VERSION_RELEASE,
            wxDateTime::Now().Format().c_str(),
            std2wx(db->getUsername()).c_str(),
            std2wx(db->getPath()).c_str()
        );
        f.Write(header);
    }
    else
        f.Write(wxT("\n"));
    f.Write(std2wx(sql));
    f.Close();
    return true;
}
//----------------------------------------------------------------------------
bool Logger::logStatement(const executedStatement& st, Database *db)
{
    DatabaseConfig dc(db);
    logStatementByConfig(&dc, st, db);
    if (!dc.get("ExcludeFromGlobalLogging", false))
    {
        Config& globalConfig = config();
        logStatementByConfig(&globalConfig, st, db);
    }
}
//---------------------------------------------------------------------------
bool Logger::logStatementByConfig(Config *cfg, const executedStatement& st,
    Database *db)
{
    bool logDML = false;
    cfg->getValue("LogDML", logDML);
    if (!logDML && st.type != IBPP::stDDL)    // logging not needed
        return true;

    bool logToFile = false;
    cfg->getValue("LogToFile", logToFile);
    if (logToFile)
    {
        std::string logFilename;
        cfg->getValue("LogFile", logFilename);
        if (logFilename.empty())
        {
            ::wxMessageBox(
                _("Logging to file enabled, but log filename not set"),
                _("Warning, no filename"), wxICON_WARNING|wxOK
            );
            return false;
        }
        return log2file(cfg, st, db, logFilename);
    }
    bool logToDb = false;
    cfg->getValue("LogToDatabase", logToDb);
    if (logToDb)
    {
        //prepareDatabase();    <- create log table, generator, etc.
        return log2database(st, db);            // <- log it
    }
    return true;
}
//----------------------------------------------------------------------------
