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

#include <string>
#include <map>
#include "config/Config.h"
#include "metadata/database.h"
#include "statementHistory.h"

class Server;
//-----------------------------------------------------------------------------
StatementHistory::StatementHistory()
{
    // TODO: load from file
    positionM = 0;
}
//-----------------------------------------------------------------------------
StatementHistory::StatementHistory(const StatementHistory& source)
{
    positionM = source.positionM;
    statementsM.assign(source.statementsM.begin(), source.statementsM.end());
}
//-----------------------------------------------------------------------------
StatementHistory::~StatementHistory()
{
    // TODO: save to file
}
//-----------------------------------------------------------------------------
//! reads granularity from config() and gives pointer to appropriate history object
StatementHistory& StatementHistory::get(Database *db)
{
    enum historyGranularity { hgCommonToAll = 0, hgPerServer, hgPerDatabaseName, hgPerDatabase };
    historyGranularity hg = (historyGranularity)(config().get("statementHistoryGranularity", (int)hgPerDatabase));
    if (hg == hgCommonToAll)
    {
        static StatementHistory st;
        return st;
    }

    else if (hg == hgPerServer)
    {
        static std::map<Server *, StatementHistory> stm;
        if (stm.find(db->getServer()) == stm.end())
        {
            StatementHistory st;
            stm.insert(std::pair<Server *, StatementHistory>(db->getServer(), st));
        }
        return (*(stm.find(db->getServer()))).second;
    }

    else if (hg == hgPerDatabaseName)
    {
        static std::map<std::string, StatementHistory> stm;
        if (stm.find(db->getName()) == stm.end())
        {
            StatementHistory st;
            stm.insert(std::pair<std::string, StatementHistory>(db->getName(), st));
        }
        return (*(stm.find(db->getName()))).second;
    }

    else // (hg == hgPerDatabase)
    {
        static std::map<Database *, StatementHistory> stm;
        if (stm.find(db) == stm.end())
        {
            StatementHistory st;
            stm.insert(std::pair<Database *, StatementHistory>(db, st));
        }
        return (*(stm.find(db))).second;
    }
}
//-----------------------------------------------------------------------------
bool StatementHistory::isAtStart()
{
    return positionM == 0;
};
//-----------------------------------------------------------------------------
bool StatementHistory::isAtEnd()
{
    return (positionM + 1) >= statementsM.size();
}
//-----------------------------------------------------------------------------
wxString StatementHistory::previous()
{
    if (!isAtStart())
        positionM--;
    return getCurrent();
}
//-----------------------------------------------------------------------------
wxString StatementHistory::next()
{
    if (!isAtEnd())
        positionM++;
    return getCurrent();
}
//-----------------------------------------------------------------------------
wxString StatementHistory::getCurrent()
{
    if (positionM >= statementsM.size())    // safety check
        positionM = statementsM.size() - 1;

    if (statementsM.empty())
        return wxEmptyString;
    else
        return statementsM[positionM];
}
//-----------------------------------------------------------------------------
void StatementHistory::add(const wxString& str)
{
    if (statementsM.empty() || str != statementsM.back())  // avoid duplicates
    {
        statementsM.push_back(str);
        checkSize();
    }
    positionM = statementsM.size() - 1; // current = last
}
//-----------------------------------------------------------------------------
void StatementHistory::setCurrent(const wxString& str)
{
    if (statementsM.empty())
    {
        statementsM.push_back(str);
        positionM = 0;
    }
    else
        statementsM[positionM] = str;
    checkSize();
}
//-----------------------------------------------------------------------------
void StatementHistory::checkSize()
{
    int historySize = config().get("statementHistorySize", -1);     // -1 = unlimited
    if (historySize == -1)
        return;
    while (statementsM.size() > (std::deque<wxString>::size_type)historySize)
    {
        statementsM.pop_front();
        positionM--;
    }
}
//-----------------------------------------------------------------------------
