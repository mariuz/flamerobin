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

#include "ugly.h"
#include "config.h"
#include "main.h"
#include "metadata/database.h"
#include "logger.h"
//-----------------------------------------------------------------------------
bool Logger::log2database(const executedStatement& /*st*/, YDatabase* /*db*/)
{
	return true;
}
//-----------------------------------------------------------------------------
bool Logger::log2file(const executedStatement& st, YDatabase *db, const std::string& filename)
{
	enum { singleFile=0, multiFile };
	int logToFileType;
	config().getValue("LogToFileType", logToFileType);

	std::string sql = st.statement;
	if (logToFileType == singleFile)								// add ; to statement if missing
	{
		sql.erase(sql.find_last_not_of(" \n\t\r")+1);				// trim
		std::string::size_type pos = sql.find_last_of(";");
		if (pos == std::string::npos || pos < sql.length() - 1)		// add ; at end
			sql += ";";
	}

	wxFile f;
	if (logToFileType == multiFile)
	{
		if (filename.find_last_of("%d") == std::string::npos)		// %d not found, bail out
			return false;
		wxString test;
		for (int i=1; i < 100000; ++i)								// dummy test for 100000
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
		if (!f.Open(std2wx(filename), wxFile::write_append ))		// cannot open file
			return false;

	bool loggingAddHeader = true;
	config().getValue("LoggingAddHeader", loggingAddHeader);
	if (loggingAddHeader)
	{
		wxString header = wxString::Format(
			_("\n/* Logged by FlameRobin %s at %s\n   User: %s    Database: %s */\n"),
			wxString(wxT(FR_VERSION)).c_str(),
			wxDateTime::Now().Format().c_str(),
			std2wx(db->getUsername()).c_str(),
			std2wx(db->getPath()).c_str()
		);
		f.Write(header);
	}
	f.Write(std2wx(sql));
	f.Close();
	return true;
}
//-----------------------------------------------------------------------------
bool Logger::logStatement(const executedStatement& st, YDatabase *db)
{
	bool logDML = false;
	config().getValue("LogDML", logDML);
	if (!logDML && st.type != IBPP::stDDL)	// logging not needed
		return true;

	bool logToFile = false;
	config().getValue("LogToFile", logToFile);
	if (logToFile)
	{
		std::string logFilename;
		config().getValue("LogFile", logFilename);
		if (logFilename.empty())
		{
			::wxMessageBox(_("Logging to file enabled, but log filename not set"), _("Warning, no filename"), wxICON_WARNING);
			return false;
		}
		return log2file(st, db, logFilename);
	}
	bool logToDb = false;
	config().getValue("LogToDatabase", logToDb);
	if (logToDb)
	{
		//prepareDatabase();	<- create log table, generator, etc. if needed
		return log2database(st, db);			// <- log it
	}
	return true;
}
//-----------------------------------------------------------------------------
