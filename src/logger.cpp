/*
  Copyright (c) 2004-2022 The FlameRobin Development Team

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
*/
// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/datetime.h>
#include <wx/file.h>
#include <wx/filename.h>

#include "config/DatabaseConfig.h"
#include "core/StringUtils.h"
#include "frversion.h"
#include "gui/AdvancedMessageDialog.h"
#include "logger.h"
#include "sql/SqlStatement.h"
#include "metadata/database.h"

bool Logger::log2database(Config *cfg, const SqlStatement& stm, Database* db)
{
    wxMBConv* conv = db->getCharsetConverter();

    IBPP::Transaction tr = IBPP::TransactionFactory(db->getIBPPDatabase());
    try
    {
        tr->Start();
        IBPP::Statement st = IBPP::StatementFactory(db->getIBPPDatabase(), tr);

        // find next id
        wxString sql = "SELECT gen_id(FLAMEROBIN$LOG_GEN, 1) FROM rdb$database";
        if (cfg->get("LoggingUsesCustomSelect", false))
        {
            sql = cfg->get("LoggingCustomSelect",
                wxString("SELECT 1+MAX(ID) FROM FLAMEROBIN$LOG"));
        }
        st->Prepare(wx2std(sql, conv));
        st->Execute();
        int cnt = 1;
        if (st->Fetch() && !st->IsNull(1))
            st->Get(1, cnt);

        st->Prepare("INSERT INTO FLAMEROBIN$LOG (id, object_type, \
            object_name, sql_statement) values (?,?,?,?)");
        st->Set(1, cnt);
        if (stm.isDDL())
        {
            st->Set(2, wx2std(getNameOfType(stm.getObjectType()), conv));
            st->Set(3, wx2std(stm.getName(), conv));
        }
        else
        {
            st->SetNull(2);
            st->SetNull(3);
        }
        IBPP::Blob bl = IBPP::BlobFactory(st->DatabasePtr(), tr);
        bl->Save(wx2std(stm.getStatement(), conv));
        st->Set(4, bl);
        st->Execute();
        tr->Commit();
        return true;
    }
    catch (IBPP::Exception &e)
    {
        showWarningDialog(0, _("Logging to database failed"),
            wxString(e.what(), *conv), AdvancedMessageDialogButtonsOk());
    }
    catch (...)
    {
        showWarningDialog(0, _("Logging to database failed"),
            _("Unexpected C++ exception"), AdvancedMessageDialogButtonsOk());
    }
    return false;
}

bool Logger::log2file(Config *cfg, const SqlStatement& st,
    Database *db, const wxString& filename)
{
    enum { singleFile=0, multiFile };
    int logToFileType;
    cfg->getValue("LogToFileType", logToFileType);

    wxString sql = st.getStatement();
    bool logSetTerm = false;
    cfg->getValue("LogSetTerm", logSetTerm);
    // add term. to statement if missing
    if (logToFileType == singleFile || (logSetTerm && st.getTerminator() != ";"))
    {
        sql.Trim();
        wxString::size_type pos = sql.rfind(st.getTerminator());
        if (pos == wxString::npos || pos < sql.length() - st.getTerminator().length())
            sql += st.getTerminator();
    }

    wxFile f;
    if (logToFileType == multiFile)
    {   // filename should contain stuff like: %d, %02d, %05d, etc.
        if (filename.find_last_of("%") == wxString::npos) // % not found
        {
            showWarningDialog(0, _("Logging to file failed"),
                _("Multiple file option selected, but path string does not contain the % character"),
                AdvancedMessageDialogButtonsOk());
            return false;
        }
        wxString test;
        int start = 1;
        cfg->getValue("IncrementalLogFileStart", start);
        for (int i=start; i < 100000; ++i) // dummy test for 100000
        {
            test.Printf(filename, i);
            wxFileName fn(test);

            if (!wxDirExists(fn.GetPath()))  // directory doesn't exist
            {
                showWarningDialog(0, _("Logging to file failed"),
                    wxString::Format(_("Directory %s does not exist"), fn.GetPath().c_str()),
                    AdvancedMessageDialogButtonsOk());
                return false;
            }

            if (!wxFileExists(test))
            {
                if (f.Open(test, wxFile::write))
                    break;
            }
        }
        if (!f.IsOpened())
        {
            showWarningDialog(0, _("Logging to file failed"),
                _("Cannot open log file."), AdvancedMessageDialogButtonsOk());
            return false;
        }
    }
    else if (!f.Open(filename, wxFile::write_append )) // cannot open
    {
        showWarningDialog(0, _("Logging to file failed"),
            _("Cannot open log file for writing."),
            AdvancedMessageDialogButtonsOk());
        return false;
    }

    bool loggingAddHeader = true;
    cfg->getValue("LoggingAddHeader", loggingAddHeader);
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
        f.Write("\n");
    if (logSetTerm && st.getTerminator() != ";")
        f.Write("SET TERM " + st.getTerminator() + " ;\n");
    f.Write(sql);
    if (logSetTerm && st.getTerminator() != ";")
        f.Write("\nSET TERM ; " + st.getTerminator() + "\n");
    f.Close();
    return true;
}

bool Logger::logStatement(const SqlStatement& st, Database* db)
{
    DatabaseConfig dc(db, config());
    bool result = logStatementByConfig(&dc, st, db);
    if (!dc.get("ExcludeFromGlobalLogging", false))
    {
        Config& globalConfig = config();
        result = result && logStatementByConfig(&globalConfig, st, db);
    }
    return result;
}

bool Logger::prepareDatabase(Database *db)
{
    IBPP::Transaction tr = IBPP::TransactionFactory(db->getIBPPDatabase());
    try
    {
        // create table
        if (db->findByNameAndType(ntTable, "FLAMEROBIN$LOG") == 0)
        {
            tr->Start();
            IBPP::Statement st = IBPP::StatementFactory(db->getIBPPDatabase(), tr);
            st->Prepare("create table FLAMEROBIN$LOG ( \
                id integer not null, \
                object_type varchar(10), \
                object_name char(31), \
                sql_statement blob sub_type 1, \
                executed_at timestamp default current_timestamp, \
                user_name char(31) default current_user )");
            st->Execute();
            tr->Commit();
            db->addObject(ntTable, "FLAMEROBIN$LOG");
        }

        // create generator
        if (db->findByNameAndType(ntGenerator, "FLAMEROBIN$LOG_GEN") == 0)
        {
            tr->Start();
            IBPP::Statement st = IBPP::StatementFactory(db->getIBPPDatabase(), tr);
            st->Prepare("create generator FLAMEROBIN$LOG_GEN");
            st->Execute();
            tr->Commit();
            db->addObject(ntGenerator, "FLAMEROBIN$LOG_GEN");
        }
        return true;
    }
    catch (IBPP::Exception &e)
    {
        showWarningDialog(0, _("Creation of logging objects failed"),
            e.what(), AdvancedMessageDialogButtonsOk());
    }
    catch (...)
    {
        showWarningDialog(0, _("Creation of logging objects failed"),
            _("Unexpected C++ exception"), AdvancedMessageDialogButtonsOk());
    }
    return false;
}

bool Logger::logStatementByConfig(Config* cfg, const SqlStatement& st,
    Database *db)
{
    bool logDML = false;
    cfg->getValue("LogDML", logDML);
    if (!logDML && !st.isDDL())    // logging not needed
        return true;

    bool logToFile = false;
    cfg->getValue("LogToFile", logToFile);
    if (logToFile)
    {
        wxString logFilename;
        cfg->getValue("LogFile", logFilename);
        if (logFilename.empty())
        {
            showWarningDialog(0,
                _("Logging to file enabled, but log filename not set"),
                _("No filename"), AdvancedMessageDialogButtonsOk()
            );
            return false;
        }
        return log2file(cfg, st, db, logFilename);
    }
    bool logToDb = false;
    cfg->getValue("LogToDatabase", logToDb);
    if (logToDb)
    {
        if (!prepareDatabase(db))
            return false;
        return log2database(cfg, st, db);            // <- log it
    }
    return true;
}

