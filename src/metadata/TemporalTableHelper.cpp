/*
  Copyright (c) 2004-2026 The FlameRobin Development Team

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

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "metadata/TemporalTableHelper.h"
#include "sql/Identifier.h"

wxString TemporalTableHelper::generateTemporalTableDDL(const wxString& tableName,
    const wxString& sysStartCol,
    const wxString& sysEndCol,
    const wxString& periodName)
{
    Identifier idTable(tableName);
    Identifier idStart(sysStartCol.IsEmpty() ? "SYS_START" : sysStartCol);
    Identifier idEnd(sysEndCol.IsEmpty() ? "SYS_END" : sysEndCol);
    Identifier idPeriod(periodName.IsEmpty() ? "SYSTEM_TIME" : periodName);

    wxString ddl = "ALTER TABLE " + idTable.getQuoted() + "\n"
        "  ADD " + idStart.getQuoted() + " TIMESTAMP WITH TIME ZONE GENERATED ALWAYS AS ROW START,\n"
        "  ADD " + idEnd.getQuoted() + " TIMESTAMP WITH TIME ZONE GENERATED ALWAYS AS ROW END,\n"
        "  ADD PERIOD FOR " + idPeriod.getQuoted() + " (" + idStart.getQuoted() + ", " + idEnd.getQuoted() + ");\n";

    return ddl;
}

wxString TemporalTableHelper::generateHistoricalAsOfQuery(const wxString& tableName, const wxString& asOfTimestamp)
{
    Identifier idTable(tableName);
    wxString ts = asOfTimestamp.IsEmpty() ? "2026-01-01 00:00:00" : asOfTimestamp;
    return "SELECT * FROM " + idTable.getQuoted() + " FOR SYSTEM_TIME AS OF '" + ts + "';";
}

wxString TemporalTableHelper::generateHistoricalBetweenQuery(const wxString& tableName, const wxString& startTime, const wxString& endTime)
{
    Identifier idTable(tableName);
    wxString t1 = startTime.IsEmpty() ? "2026-01-01 00:00:00" : startTime;
    wxString t2 = endTime.IsEmpty() ? "2026-12-31 23:59:59" : endTime;
    return "SELECT * FROM " + idTable.getQuoted() + " FOR SYSTEM_TIME BETWEEN '" + t1 + "' AND '" + t2 + "';";
}
