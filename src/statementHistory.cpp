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

#include <wx/ffile.h>
#include <wx/filefn.h>
#include <map>

#include "config/Config.h"
#include "metadata/database.h"
#include "statementHistory.h"

wxString StatementHistory::getFilename(StatementHistory::Position item)
{
    wxString fn = config().getUserHomePath() + "history/";
    if (!wxDirExists(fn))
        wxMkdir(fn);

    for (Position i=0; i<storageNameM.Length(); ++i)
        fn += wxString::Format("%04x", storageNameM[i]);
    fn << "_ITEM_" << (item);
    return fn;
}

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

StatementHistory::StatementHistory(const StatementHistory& source)
{
    storageNameM = source.storageNameM;
    sizeM = source.sizeM;
}

//! reads granularity from config() and gives pointer to appropriate history object
StatementHistory& StatementHistory::get(Database* db)
{
    enum historyGranularity { hgCommonToAll = 0, hgPerDatabaseName, hgPerDatabase };
    historyGranularity hg = (historyGranularity)(config().get("statementHistoryGranularity", (int)hgPerDatabase));
    if (hg == hgCommonToAll)
    {
        static StatementHistory st(wxEmptyString);
        return st;
    }

    else if (hg == hgPerDatabaseName)
    {
        static std::map<wxString, StatementHistory> stm;
        if (stm.find(db->getName_()) == stm.end())
        {
            StatementHistory st("DATABASENAME" + db->getName_());
            stm.insert(std::pair<wxString, StatementHistory>(db->getName_(), st));
        }
        return (*(stm.find(db->getName_()))).second;
    }

    else // (hg == hgPerDatabase)
    {
        static std::map<Database*, StatementHistory> stm;
        if (stm.find(db) == stm.end())
        {
            StatementHistory st("DATABASE" + db->getId());
            stm.insert(std::pair<Database*, StatementHistory>(db, st));
        }
        return (*(stm.find(db))).second;
    }
}

wxDateTime StatementHistory::getDateTime(StatementHistory::Position pos)
{
    if (pos < sizeM)
    {
        return wxDateTime(::wxFileModificationTime(getFilename(pos)));
    }
    return wxInvalidDateTime;
}

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

void StatementHistory::add(const wxString& str)
{
    if (str.Strip().IsEmpty() ||    // empty or too big string
        (config().get("limitHistoryItemSize", false) &&
        int(str.Length()) > 1024 * config().get("statementHistoryItemSize", 500)))
    {
        return;
    }

    if (sizeM == 0 || get(sizeM-1) != str)
    {
        wxFFile f(getFilename(sizeM), "wb+");
        if (f.IsOpened())
        {
            f.Write(str);
            f.Close();
            sizeM++;
        }
    }
}

StatementHistory::Position StatementHistory::size()
{
    return sizeM;
}

void StatementHistory::deleteItems(
    const std::vector<StatementHistory::Position>& items)
{
    // remove the files (remembering the "lowest" deleted)
    Position start = size();
    for (std::vector<Position>::const_iterator ci = items.begin();
        ci != items.end(); ++ci)
    {
        wxRemoveFile(getFilename(*ci));
        if ((*ci) < start)
            start = (*ci);
    }

    // move existing files
    for (Position currentpos = start; currentpos < size(); currentpos++)
    {
        if (!wxFileExists(getFilename(currentpos)))
            continue;
        if (currentpos != start)
            wxRenameFile(getFilename(currentpos), getFilename(start));
        start++;
    }

    // set new size
    sizeM = start;
}

