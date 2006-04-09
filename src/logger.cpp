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
#include <wx/filename.h>

#include "config/DatabaseConfig.h"
#include "frversion.h"
#include "logger.h"
#include "metadata/database.h"
#include "ugly.h"
//----------------------------------------------------------------------------
ExecutedStatement::ExecutedStatement(const wxString& st, const IBPP::STT& t,
        const wxString& term)
    :statement(st), type(t), terminator(term)
{
}
//----------------------------------------------------------------------------
bool Logger::log2database(const ExecutedStatement& /*st*/, Database* /*db*/)
{
    return true;
}
//----------------------------------------------------------------------------
bool Logger::log2file(Config *cfg, const ExecutedStatement& st,
    Database *db, const wxString& filename)
{
    enum { singleFile=0, multiFile };
    int logToFileType;
    cfg->getValue(wxT("LogToFileType"), logToFileType);

    wxString sql = st.statement;
    bool logSetTerm = false;
    cfg->getValue(wxT("LogSetTerm"), logSetTerm);
    // add term. to statement if missing
    if (logToFileType == singleFile || logSetTerm && st.terminator != wxT(";"))
    {
        sql.Trim();
        wxString::size_type pos = sql.rfind(st.terminator);
        if (pos == wxString::npos || pos < sql.length() - st.terminator.length())
            sql += st.terminator;
    }

    wxFile f;
    if (logToFileType == multiFile)
    {   // filename should contain stuff like: %d, %02d, %05d, etc.
        if (filename.find_last_of(wxT("%")) == wxString::npos) // % not found
            return false;
        wxString test;
        int start = 1;
        cfg->getValue(wxT("IncrementalLogFileStart"), start);
        for (int i=start; i < 100000; ++i) // dummy test for 100000
        {
            test.Printf(filename, i);
            wxFileName fn(test);

            if (!wxDirExists(fn.GetPath()))  // directory doesn't exist
                return false;

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
        if (!f.Open(filename, wxFile::write_append )) // cannot open
            return false;

    bool loggingAddHeader = true;
    cfg->getValue(wxT("LoggingAddHeader"), loggingAddHeader);
    if (loggingAddHeader)
    {
        wxString header = wxString::Format(
            _("\n/* Logged by FlameRobin %d.%d.%d at %s\n   User: %s    Database: %s */\n"),
            FR_VERSION_MAJOR, FR_VERSION_MINOR, FR_VERSION_RLS,
            wxDateTime::Now().Format().c_str(),
            db->getUsername().c_str(),
            db->getPath().c_str()
        );
        f.Write(header);
    }
    else
        f.Write(wxT("\n"));
    if (logSetTerm && st.terminator != wxT(";"))
        f.Write(wxT("SET TERM ") + st.terminator + wxT(" ;\n"));
    f.Write(sql);
    if (logSetTerm && st.terminator != wxT(";"))
        f.Write(wxT("\nSET TERM ; ") + st.terminator + wxT("\n"));
    f.Close();
    return true;
}
//----------------------------------------------------------------------------
bool Logger::logStatement(const ExecutedStatement& st, Database* db)
{
    DatabaseConfig dc(db);
    bool result = logStatementByConfig(&dc, st, db);
    if (!dc.get(wxT("ExcludeFromGlobalLogging"), false))
    {
        Config& globalConfig = config();
        return logStatementByConfig(&globalConfig, st, db);
    }
    else
        return result;
}
//---------------------------------------------------------------------------
bool Logger::logStatementByConfig(Config* cfg, const ExecutedStatement& st,
    Database *db)
{
    bool logDML = false;
    cfg->getValue(wxT("LogDML"), logDML);
    if (!logDML && st.type != IBPP::stDDL)    // logging not needed
        return true;

    bool logToFile = false;
    cfg->getValue(wxT("LogToFile"), logToFile);
    if (logToFile)
    {
        wxString logFilename;
        cfg->getValue(wxT("LogFile"), logFilename);
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
    cfg->getValue(wxT("LogToDatabase"), logToDb);
    if (logToDb)
    {
        //prepareDatabase();    <- create log table, generator, etc.
        return log2database(st, db);            // <- log it
    }
    return true;
}
//----------------------------------------------------------------------------
