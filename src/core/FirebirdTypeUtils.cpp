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

#include "core/StringUtils.h"
#include "metadata/database.h"
#include "metadata/CharacterSet.h"

wxString IBPPtype2string(Database* db, IBPP::SDT t, int subtype, int size,
    int scale)
{
    if (scale != 0)
    {
        int precision = (size == 2) ? 4 : (size == 4 ? 9 : 18);
        return wxString::Format("NUMERIC(%d,%d)", precision, scale < 0 ? -scale : scale);
    }
    if (t == IBPP::sdString)
    {
        int bpc = db->getCharsetById(subtype)->getBytesPerChar();
        if (subtype == 1) // charset OCTETS
            return wxString::Format("OCTETS(%d)", bpc ? size / bpc : size);
        return wxString::Format("STRING(%d)", bpc ? size / bpc : size);
    }
    switch (t)
    {
    case IBPP::sdArray:     return "ARRAY";
    case IBPP::sdBlob:      return wxString::Format("BLOB SUB_TYPE %d", subtype);
    case IBPP::sdDate:      return "DATE";
    case IBPP::sdTime:      return "TIME";
    case IBPP::sdTimestamp: return "TIMESTAMP";
    case IBPP::sdSmallint:  return "SMALLINT";
    case IBPP::sdInteger:   return "INTEGER";
    case IBPP::sdLargeint:  return "BIGINT";
    case IBPP::sdFloat:     return "FLOAT";
    case IBPP::sdDouble:    return "DOUBLE PRECISION";
    case IBPP::sdBoolean:   return "BOOLEAN";
    case IBPP::sdTimeTz:    return "TIME WITH TIMEZONE";
    case IBPP::sdTimestampTz: return "TIMESTAMP WITH TIMEZONE";
    case IBPP::sdInt128:    return "INT128";
    case IBPP::sdDec16:     return "DECFLOAT(16)";
    case IBPP::sdDec34:     return "DECFLOAT(34)";
    default:                return "UNKNOWN";
    }
}

wxString DALtype2string(Database* db, fr::ColumnType t, int subtype, int size,
    int scale)
{
    if (scale != 0)
    {
        int precision = (size == 2) ? 4 : (size == 4 ? 9 : 18);
        return wxString::Format("NUMERIC(%d,%d)", precision, scale < 0 ? -scale : scale);
    }

    switch (t)
    {
    case fr::ColumnType::Varchar:
    case fr::ColumnType::Char:
    {
        int bpc = db->getCharsetById(subtype)->getBytesPerChar();
        if (subtype == 1) // charset OCTETS
            return wxString::Format("OCTETS(%d)", bpc ? size / bpc : size);
        return wxString::Format("STRING(%d)", bpc ? size / bpc : size);
    }
    case fr::ColumnType::Blob:      return wxString::Format("BLOB SUB_TYPE %d", subtype);
    case fr::ColumnType::Date:      return "DATE";
    case fr::ColumnType::Time:      return "TIME";
    case fr::ColumnType::Timestamp: return "TIMESTAMP";
    case fr::ColumnType::Integer:   return "INTEGER";
    case fr::ColumnType::BigInt:    return "BIGINT";
    case fr::ColumnType::Float:     return "FLOAT";
    case fr::ColumnType::Double:    return "DOUBLE PRECISION";
    case fr::ColumnType::Boolean:   return "BOOLEAN";
    case fr::ColumnType::TimeTz:    return "TIME WITH TIMEZONE";
    case fr::ColumnType::TimestampTz: return "TIMESTAMP WITH TIMEZONE";
    case fr::ColumnType::Int128:    return "INT128";
    case fr::ColumnType::Decfloat16:     return "DECFLOAT(16)";
    case fr::ColumnType::Decfloat34:     return "DECFLOAT(34)";
    default:                return "UNKNOWN";
    }
}

wxString isolationLevelToString(fr::TransactionIsolationLevel level)
{
    switch (level)
    {
    case fr::TransactionIsolationLevel::Consistency:     return _("Consistency");
    case fr::TransactionIsolationLevel::Concurrency:     return _("Concurrency");
    case fr::TransactionIsolationLevel::ReadCommitted:   return _("Read Committed");
    case fr::TransactionIsolationLevel::ReadDirty:       return _("Read Dirty");
    case fr::TransactionIsolationLevel::ReadConsistency: return _("Read Consistency");
    default:                                             return _("Unknown");
    }
}
