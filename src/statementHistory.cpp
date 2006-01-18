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

  Contributor(s):
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

#include <wx/ffile.h>
#include <wx/filefn.h>
#include <map>

#include "config/Config.h"
#include "metadata/database.h"
#include "metadata/server.h"
#include "statementHistory.h"
#include "ugly.h"

class Server;
//-----------------------------------------------------------------------------
using namespace std;
//-----------------------------------------------------------------------------
wxString StatementHistory::getFilename(size_t item)
{
	wxString fn = config().getUserHomePath() + wxT("history/");
	if (!wxDirExists(fn))
		wxMkdir(fn);

	for (size_t i=0; i<storageNameM.Length(); ++i)
		fn += wxString::Format(wxT("%04x"), storageNameM[i]);
	fn << wxT("_ITEM_") << (item);
	return fn;
}
//-----------------------------------------------------------------------------
StatementHistory::StatementHistory(const wxString& storageName)
{
    storageNameM = storageName;
    sizeM = 0;
    while (true)    // load history
    {
		if (!wxFileExists(getFilename(sizeM)))
			break;
		sizeM++;
    }
}
//-----------------------------------------------------------------------------
StatementHistory::StatementHistory(const StatementHistory& source)
{
    storageNameM = source.storageNameM;
	sizeM = source.sizeM;
}
//-----------------------------------------------------------------------------
StatementHistory::~StatementHistory()
{
}
//-----------------------------------------------------------------------------
//! reads granularity from config() and gives pointer to appropriate history object
StatementHistory& StatementHistory::get(Database* db)
{
    enum historyGranularity { hgCommonToAll = 0, hgPerDatabaseName, hgPerDatabase };
    historyGranularity hg = (historyGranularity)(config().get(wxT("statementHistoryGranularity"), (int)hgPerDatabase));
    if (hg == hgCommonToAll)
    {
        static StatementHistory st(wxEmptyString);
        return st;
    }

    else if (hg == hgPerDatabaseName)
    {
        static map<wxString, StatementHistory> stm;
        if (stm.find(db->getName_()) == stm.end())
        {
            StatementHistory st(wxT("DATABASENAME") + db->getName_());
            stm.insert(pair<wxString, StatementHistory>(db->getName_(), st));
        }
        return (*(stm.find(db->getName_()))).second;
    }

    else // (hg == hgPerDatabase)
    {
        static map<Database*, StatementHistory> stm;
        if (stm.find(db) == stm.end())
        {
            StatementHistory st(wxT("DATABASE") + db->getId());
            stm.insert(pair<Database*, StatementHistory>(db, st));
        }
        return (*(stm.find(db))).second;
    }
}
//-----------------------------------------------------------------------------
wxString StatementHistory::get(StatementHistory::Position pos)
{
    if (pos < sizeM)
	{
		wxFFile f(getFilename(pos), "rb");
		if (!f.IsOpened())
			return wxEmptyString;
		wxString retval;
		if (f.ReadAll(&retval))
		{
			f.Close();
			return retval;
		}
	}
	return wxEmptyString;
}
//-----------------------------------------------------------------------------
StatementHistory::Position StatementHistory::add(const wxString& str)
{
	if (sizeM == 0 || get(sizeM-1) != str)
	{
		wxFFile f(getFilename(sizeM), wxT("wb+"));
		if (f.IsOpened())
		{
			f.Write(str);
			f.Close();
			sizeM++;
			checkSize();
		}
    }
    return sizeM - 1;
}
//-----------------------------------------------------------------------------
StatementHistory::Position StatementHistory::size()
{
    return sizeM;
}
//-----------------------------------------------------------------------------
void StatementHistory::checkSize()
{
    if (!config().get(wxT("limitHistorySize"), false))
        return;

    int historySize = config().get(wxT("statementHistorySize"), 50);     // -1 = unlimited
    while (sizeM > (deque<wxString>::size_type)historySize)
	{
		// reorder stuff or truncate files or whatever
	}
}
//-----------------------------------------------------------------------------
