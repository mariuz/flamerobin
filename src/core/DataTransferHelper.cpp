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

#include "core/DataTransferHelper.h"
#include "core/ProgressIndicator.h"
#include "core/StringUtils.h"
#include "metadata/column.h"
#include "metadata/database.h"
#include "metadata/relation.h"
#include "metadata/table.h"
#include "sql/Identifier.h"
#include "engine/db/IDatabase.h"
#include "engine/db/ITransaction.h"
#include "engine/db/IStatement.h"
#include "engine/db/IBlob.h"
#include "gui/controls/DataGrid.h"
#include "gui/controls/DataGridTable.h"

#include <wx/base64.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <wx/xml/xml.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace fr {

wxString DataTransferHelper::sanitizeXmlTagName(const wxString& name)
{
    wxString res;
    for (size_t i = 0; i < name.length(); ++i)
    {
        wxUniChar ch = name[i];
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_')
        {
            res += ch;
        }
        else if (ch >= '0' && ch <= '9')
        {
            if (res.empty())
                res += "_";
            res += ch;
        }
        else
        {
            res += "_";
        }
    }
    if (res.empty())
        res = "col";
    return res;
}

wxString DataTransferHelper::escapeXml(const wxString& str)
{
    wxString res = str;
    res.Replace("&", "&amp;");
    res.Replace("<", "&lt;");
    res.Replace(">", "&gt;");
    res.Replace("\"", "&quot;");
    res.Replace("'", "&apos;");
    return res;
}

wxString DataTransferHelper::escapeJson(const wxString& str)
{
    try
    {
        std::string s = json(std::string(str.ToUTF8())).dump();
        if (s.length() >= 2 && s.front() == '"' && s.back() == '"')
            return wxString::FromUTF8(s.substr(1, s.length() - 2).c_str());
        return wxString::FromUTF8(s.c_str());
    }
    catch (...)
    {
        wxString res = str;
        res.Replace("\\", "\\\\");
        res.Replace("\"", "\\\"");
        res.Replace("\n", "\\n");
        res.Replace("\r", "\\r");
        res.Replace("\t", "\\t");
        return res;
    }
}

static void writeUtf8(wxOutputStream& stream, const wxString& text)
{
    wxScopedCharBuffer buf = text.ToUTF8();
    if (buf && buf.length() > 0)
    {
        stream.Write(buf.data(), buf.length());
    }
}

bool DataTransferHelper::exportDataset(const std::vector<wxString>& columnNames,
                                      const std::vector<std::vector<DataCell>>& rows,
                                      wxOutputStream& outStream,
                                      const DataExportOptions& options,
                                      ProgressIndicator* pi)
{
    if (columnNames.empty())
        return false;

    // Determine column indices to export
    std::vector<size_t> colIndices;
    if (options.columns.empty())
    {
        for (size_t i = 0; i < columnNames.size(); ++i)
            colIndices.push_back(i);
    }
    else
    {
        for (const auto& optCol : options.columns)
        {
            for (size_t i = 0; i < columnNames.size(); ++i)
            {
                if (columnNames[i].CmpNoCase(optCol) == 0)
                {
                    colIndices.push_back(i);
                    break;
                }
            }
        }
    }

    if (colIndices.empty())
        return false;

    if (options.format == DataTransferFormat::Json)
    {
        writeUtf8(outStream, options.prettyPrint ? "[\n" : "[");
        for (size_t r = 0; r < rows.size(); ++r)
        {
            if (pi && pi->isCanceled())
                return false;

            if (r > 0)
                writeUtf8(outStream, options.prettyPrint ? ",\n" : ",");

            writeUtf8(outStream, options.prettyPrint ? "  {\n" : "{");
            bool firstField = true;

            for (size_t ci = 0; ci < colIndices.size(); ++ci)
            {
                size_t c = colIndices[ci];
                const wxString& colName = columnNames[c];
                const DataCell& cell = (c < rows[r].size()) ? rows[r][c] : DataCell::makeNull();

                if (cell.type == DataCell::Null && options.nullHandling == NullHandling::Omit)
                    continue;

                if (!firstField)
                    writeUtf8(outStream, options.prettyPrint ? ",\n" : ",");
                firstField = false;

                wxString formattedVal;
                if (cell.type == DataCell::Null)
                {
                    if (options.nullHandling == NullHandling::EmptyString)
                        formattedVal = "\"\"";
                    else
                        formattedVal = "null";
                }
                else if (cell.type == DataCell::Boolean)
                {
                    formattedVal = cell.boolVal ? "true" : "false";
                }
                else if (cell.type == DataCell::Number)
                {
                    formattedVal = cell.textVal.empty() ? wxString::FromCDouble(cell.numVal) : cell.textVal;
                }
                else if (cell.type == DataCell::Blob)
                {
                    if (options.base64Blobs)
                        formattedVal = "\"" + escapeJson(wxBase64Encode(cell.blobVal)) + "\"";
                    else
                        formattedVal = "\"" + escapeJson(cell.textVal) + "\"";
                }
                else // String
                {
                    formattedVal = "\"" + escapeJson(cell.textVal) + "\"";
                }

                if (options.prettyPrint)
                {
                    writeUtf8(outStream, wxString::Format("    \"%s\": %s",
                        escapeJson(colName).c_str(), formattedVal.c_str()));
                }
                else
                {
                    writeUtf8(outStream, wxString::Format("\"%s\":%s",
                        escapeJson(colName).c_str(), formattedVal.c_str()));
                }
            }

            writeUtf8(outStream, options.prettyPrint ? "\n  }" : "}");
            if (pi)
                pi->stepProgress();
        }
        writeUtf8(outStream, options.prettyPrint ? "\n]\n" : "]\n");
    }
    else // XML
    {
        wxString rootTag = options.xmlRootTag.empty() ? wxString("table") : sanitizeXmlTagName(options.xmlRootTag);
        wxString rowTag = options.xmlRowTag.empty() ? wxString("row") : sanitizeXmlTagName(options.xmlRowTag);

        writeUtf8(outStream, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        writeUtf8(outStream, wxString::Format("<%s xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n", rootTag.c_str()));

        for (size_t r = 0; r < rows.size(); ++r)
        {
            if (pi && pi->isCanceled())
                return false;

            writeUtf8(outStream, options.prettyPrint ? ("  <" + rowTag + ">\n") : ("<" + rowTag + ">"));

            for (size_t ci = 0; ci < colIndices.size(); ++ci)
            {
                size_t c = colIndices[ci];
                wxString tag = sanitizeXmlTagName(columnNames[c]);
                const DataCell& cell = (c < rows[r].size()) ? rows[r][c] : DataCell::makeNull();

                if (cell.type == DataCell::Null)
                {
                    if (options.nullHandling == NullHandling::Omit)
                        continue;
                    if (options.nullHandling == NullHandling::EmptyString)
                    {
                        if (options.prettyPrint)
                            writeUtf8(outStream, wxString::Format("    <%s></%s>\n", tag.c_str(), tag.c_str()));
                        else
                            writeUtf8(outStream, wxString::Format("<%s></%s>", tag.c_str(), tag.c_str()));
                    }
                    else // NullValue
                    {
                        if (options.prettyPrint)
                            writeUtf8(outStream, wxString::Format("    <%s xsi:nil=\"true\"/>\n", tag.c_str()));
                        else
                            writeUtf8(outStream, wxString::Format("<%s xsi:nil=\"true\"/>", tag.c_str()));
                    }
                }
                else if (cell.type == DataCell::Blob && options.base64Blobs)
                {
                    wxString b64 = wxBase64Encode(cell.blobVal);
                    if (options.prettyPrint)
                        writeUtf8(outStream, wxString::Format("    <%s>%s</%s>\n", tag.c_str(), b64.c_str(), tag.c_str()));
                    else
                        writeUtf8(outStream, wxString::Format("<%s>%s</%s>", tag.c_str(), b64.c_str(), tag.c_str()));
                }
                else
                {
                    wxString val = escapeXml(cell.textVal);
                    if (options.prettyPrint)
                        writeUtf8(outStream, wxString::Format("    <%s>%s</%s>\n", tag.c_str(), val.c_str(), tag.c_str()));
                    else
                        writeUtf8(outStream, wxString::Format("<%s>%s</%s>", tag.c_str(), val.c_str(), tag.c_str()));
                }
            }

            writeUtf8(outStream, options.prettyPrint ? ("  </" + rowTag + ">\n") : ("</" + rowTag + ">"));
            if (pi)
                pi->stepProgress();
        }

        writeUtf8(outStream, "</" + rootTag + ">\n");
    }

    return true;
}

bool DataTransferHelper::exportDatasetToFile(const std::vector<wxString>& columnNames,
                                            const std::vector<std::vector<DataCell>>& rows,
                                            const wxString& filePath,
                                            const DataExportOptions& options,
                                            ProgressIndicator* pi)
{
    wxFileOutputStream fos(filePath);
    if (!fos.IsOk())
        return false;
    return exportDataset(columnNames, rows, fos, options, pi);
}

bool DataTransferHelper::exportRelation(Database* db,
                                       Relation* relation,
                                       const wxString& filePath,
                                       const DataExportOptions& options,
                                       ProgressIndicator* pi,
                                       wxString* errorMessage)
{
    if (!db || !relation)
    {
        if (errorMessage)
            *errorMessage = _("Database or relation is null");
        return false;
    }

    relation->ensureChildrenLoaded();

    std::vector<wxString> allCols;
    for (auto it = relation->begin(); it != relation->end(); ++it)
    {
        allCols.push_back((*it)->getName_());
    }

    if (allCols.empty())
    {
        if (errorMessage)
            *errorMessage = _("Relation has no columns");
        return false;
    }

    std::vector<wxString> colsToExport;
    if (options.columns.empty())
    {
        colsToExport = allCols;
    }
    else
    {
        for (const auto& optCol : options.columns)
        {
            for (const auto& ac : allCols)
            {
                if (ac.CmpNoCase(optCol) == 0)
                {
                    colsToExport.push_back(ac);
                    break;
                }
            }
        }
    }

    if (colsToExport.empty())
    {
        if (errorMessage)
            *errorMessage = _("No columns selected for export");
        return false;
    }

    // Build SELECT statement
    wxString sql = "SELECT ";
    for (size_t i = 0; i < colsToExport.size(); ++i)
    {
        if (i > 0)
            sql += ", ";
        Identifier colId(colsToExport[i], db->getSqlDialect());
        sql += colId.getQuoted();
    }
    Identifier tabId(relation->getName_(), db->getSqlDialect());
    sql += " FROM " + tabId.getQuoted();

    try
    {
        fr::ITransactionPtr tr = db->getDALDatabase()->createTransaction();
        tr->start();

        fr::IStatementPtr st = db->getDALDatabase()->createStatement(tr);
        st->prepare(wx2std(sql, db->getCharsetConverter()));
        st->execute();

        wxFileOutputStream fos(filePath);
        if (!fos.IsOk())
        {
            tr->rollback();
            if (errorMessage)
                *errorMessage = wxString::Format(_("Cannot open output file '%s'"), filePath.c_str());
            return false;
        }

        std::vector<std::vector<DataCell>> fetchedRows;
        while (st->fetch())
        {
            if (pi && pi->isCanceled())
            {
                tr->rollback();
                return false;
            }

            std::vector<DataCell> rowCells;
            rowCells.reserve(colsToExport.size());

            for (int c = 0; c < (int)colsToExport.size(); ++c)
            {
                if (st->isNull(c))
                {
                    rowCells.push_back(DataCell::makeNull());
                }
                else
                {
                    ColumnType colType = st->getColumnType(c);
                    if (colType == ColumnType::Blob)
                    {
                        IBlobPtr b = st->getBlob(c);
                        if (!b)
                        {
                            rowCells.push_back(DataCell::makeNull());
                        }
                        else
                        {
                            b->open();
                            wxMemoryBuffer mb;
                            char buffer[4096];
                            while (true)
                            {
                                int r = b->read(buffer, sizeof(buffer));
                                if (r <= 0)
                                    break;
                                mb.AppendData(buffer, r);
                            }
                            b->close();
                            rowCells.push_back(DataCell::makeBlob(mb));
                        }
                    }
                    else if (colType == ColumnType::Boolean)
                    {
                        rowCells.push_back(DataCell::makeBoolean(st->getBool(c)));
                    }
                    else if (colType == ColumnType::Integer)
                    {
                        rowCells.push_back(DataCell::makeNumber(st->getInt32(c), wxString::Format("%d", st->getInt32(c))));
                    }
                    else if (colType == ColumnType::BigInt)
                    {
                        rowCells.push_back(DataCell::makeNumber((double)st->getInt64(c), wxString::Format("%lld", (long long)st->getInt64(c))));
                    }
                    else if (colType == ColumnType::Float || colType == ColumnType::Double)
                    {
                        rowCells.push_back(DataCell::makeNumber(st->getDouble(c), wxString::FromCDouble(st->getDouble(c))));
                    }
                    else
                    {
                        std::string s = st->getString(c);
                        rowCells.push_back(DataCell::makeString(std2wxIdentifier(s, db->getCharsetConverter())));
                    }
                }
            }
            fetchedRows.push_back(rowCells);
        }

        tr->commit();

        bool ok = exportDataset(colsToExport, fetchedRows, fos, options, pi);
        return ok;
    }
    catch (const std::exception& e)
    {
        if (errorMessage)
            *errorMessage = wxString::FromUTF8(e.what());
        return false;
    }
}

bool DataTransferHelper::exportDataGrid(DataGrid* grid,
                                       const wxString& filePath,
                                       const DataExportOptions& options,
                                       ProgressIndicator* pi,
                                       wxString* errorMessage)
{
    if (!grid)
    {
        if (errorMessage)
            *errorMessage = _("DataGrid is null");
        return false;
    }

    DataGridTable* table = grid->getDataGridTable();
    if (!table)
    {
        if (errorMessage)
            *errorMessage = _("Grid table is null");
        return false;
    }

    std::vector<bool> selCols(grid->getColumnsWithSelectedCells());
    std::vector<bool> selRows(grid->getRowsWithSelectedCells());

    bool hasSelectedRows = (std::find(selRows.begin(), selRows.end(), true) != selRows.end());
    if (!hasSelectedRows)
        selRows.assign(grid->GetNumberRows(), true);

    bool hasSelectedCols = (std::find(selCols.begin(), selCols.end(), true) != selCols.end());
    if (!hasSelectedCols)
        selCols.assign(grid->GetNumberCols(), true);

    std::vector<wxString> colNames;
    std::vector<size_t> colIndices;

    for (int c = 0; c < grid->GetNumberCols(); ++c)
    {
        if (!selCols[c])
            continue;
        wxString label = grid->GetColLabelValue(c);
        if (!options.columns.empty())
        {
            bool match = false;
            for (const auto& optCol : options.columns)
            {
                if (optCol.CmpNoCase(label) == 0)
                {
                    match = true;
                    break;
                }
            }
            if (!match)
                continue;
        }
        colNames.push_back(label);
        colIndices.push_back(c);
    }

    if (colNames.empty())
    {
        if (errorMessage)
            *errorMessage = _("No columns selected for export");
        return false;
    }

    std::vector<std::vector<DataCell>> rows;
    for (int r = 0; r < grid->GetNumberRows(); ++r)
    {
        if (!selRows[r])
            continue;

        std::vector<DataCell> rowCells;
        rowCells.reserve(colIndices.size());

        for (size_t cIdx : colIndices)
        {
            if (table->isNullCell(r, cIdx))
            {
                rowCells.push_back(DataCell::makeNull());
            }
            else if (table->isBlobColumn(cIdx))
            {
                fr::IBlobPtr b = table->getBlob(r, cIdx, false);
                if (!b)
                {
                    rowCells.push_back(DataCell::makeNull());
                }
                else
                {
                    try
                    {
                        b->open();
                        wxMemoryBuffer mb;
                        char buffer[4096];
                        while (true)
                        {
                            int bytesRead = b->read(buffer, sizeof(buffer));
                            if (bytesRead <= 0)
                                break;
                            mb.AppendData(buffer, bytesRead);
                        }
                        b->close();
                        rowCells.push_back(DataCell::makeBlob(mb));
                    }
                    catch (...)
                    {
                        rowCells.push_back(DataCell::makeString(table->GetValue(r, cIdx)));
                    }
                }
            }
            else
            {
                wxString val = table->GetValue(r, cIdx);
                double dVal;
                if (table->isNumericColumn(cIdx) && (val.ToCDouble(&dVal) || val.ToDouble(&dVal)))
                {
                    rowCells.push_back(DataCell::makeNumber(dVal, val));
                }
                else
                {
                    rowCells.push_back(DataCell::makeString(val));
                }
            }
        }
        rows.push_back(rowCells);
    }

    return exportDatasetToFile(colNames, rows, filePath, options, pi);
}

DataTransferFormat DataTransferHelper::detectFormat(const wxString& filePath)
{
    wxString ext = filePath.AfterLast('.').Lower();
    if (ext == "json")
        return DataTransferFormat::Json;
    if (ext == "xml")
        return DataTransferFormat::Xml;

    // Inspect content
    std::ifstream ifs(std::string(filePath.ToUTF8()));
    if (ifs.is_open())
    {
        char ch;
        while (ifs.get(ch))
        {
            if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
                continue;
            if (ch == '{' || ch == '[')
                return DataTransferFormat::Json;
            if (ch == '<')
                return DataTransferFormat::Xml;
            break;
        }
    }
    return DataTransferFormat::Json;
}

std::vector<wxString> DataTransferHelper::extractFieldsFromFile(const wxString& filePath,
                                                               DataTransferFormat format)
{
    if (format == DataTransferFormat::Auto)
        format = detectFormat(filePath);

    std::vector<wxString> fields;
    std::map<wxString, bool> seen;

    auto addField = [&](const wxString& f) {
        if (!f.empty() && seen.find(f) == seen.end())
        {
            seen[f] = true;
            fields.push_back(f);
        }
    };

    if (format == DataTransferFormat::Json)
    {
        try
        {
            std::ifstream ifs(std::string(filePath.ToUTF8()));
            if (!ifs.is_open())
                return fields;

            json j = json::parse(ifs);
            const json* arr = nullptr;

            if (j.is_array())
            {
                arr = &j;
            }
            else if (j.is_object())
            {
                static const char* candidateKeys[] = { "rows", "data", "records", "items", "table" };
                for (const char* k : candidateKeys)
                {
                    if (j.contains(k) && j[k].is_array())
                    {
                        arr = &j[k];
                        break;
                    }
                }
                if (!arr)
                {
                    for (auto it = j.begin(); it != j.end(); ++it)
                    {
                        if (it.value().is_array())
                        {
                            arr = &it.value();
                            break;
                        }
                    }
                }
            }

            if (arr)
            {
                for (const auto& item : *arr)
                {
                    if (item.is_object())
                    {
                        for (auto it = item.begin(); it != item.end(); ++it)
                        {
                            addField(wxString::FromUTF8(it.key().c_str()));
                        }
                    }
                }
            }
        }
        catch (...)
        {
        }
    }
    else // XML
    {
        wxXmlDocument doc;
        if (doc.Load(filePath))
        {
            wxXmlNode* root = doc.GetRoot();
            if (root)
            {
                wxXmlNode* row = root->GetChildren();
                while (row)
                {
                    if (row->GetType() == wxXML_ELEMENT_NODE)
                    {
                        // Check attributes of row
                        wxXmlAttribute* attr = row->GetAttributes();
                        while (attr)
                        {
                            addField(attr->GetName());
                            attr = attr->GetNext();
                        }

                        // Check sub-elements
                        wxXmlNode* col = row->GetChildren();
                        while (col)
                        {
                            if (col->GetType() == wxXML_ELEMENT_NODE)
                            {
                                addField(col->GetName());
                            }
                            col = col->GetNext();
                        }
                    }
                    row = row->GetNext();
                }
            }
        }
    }

    return fields;
}

static wxString normalizeName(const wxString& s)
{
    wxString res;
    for (size_t i = 0; i < s.length(); ++i)
    {
        wxUniChar ch = s[i];
        if (ch == '_' || ch == '-' || ch == ' ')
            continue;
        res += wxTolower(ch);
    }
    return res;
}

std::map<wxString, wxString> DataTransferHelper::autoMapFields(const std::vector<wxString>& tableColumns,
                                                              const std::vector<wxString>& fileFields)
{
    std::map<wxString, wxString> mapping;

    for (const auto& col : tableColumns)
    {
        wxString matchedField;

        // 1. Exact match
        for (const auto& field : fileFields)
        {
            if (col == field)
            {
                matchedField = field;
                break;
            }
        }

        // 2. Case-insensitive match
        if (matchedField.empty())
        {
            for (const auto& field : fileFields)
            {
                if (col.CmpNoCase(field) == 0)
                {
                    matchedField = field;
                    break;
                }
            }
        }

        // 3. Normalized match (ignoring _, -, case)
        if (matchedField.empty())
        {
            wxString normCol = normalizeName(col);
            for (const auto& field : fileFields)
            {
                if (normCol == normalizeName(field))
                {
                    matchedField = field;
                    break;
                }
            }
        }

        mapping[col] = matchedField;
    }

    return mapping;
}

bool DataTransferHelper::parseRecordsFromFile(const wxString& filePath,
                                             DataTransferFormat format,
                                             std::vector<std::map<wxString, DataCell>>& records,
                                             wxString* errorMessage)
{
    records.clear();
    if (format == DataTransferFormat::Auto)
        format = detectFormat(filePath);

    if (format == DataTransferFormat::Json)
    {
        try
        {
            std::ifstream ifs(std::string(filePath.ToUTF8()));
            if (!ifs.is_open())
            {
                if (errorMessage)
                    *errorMessage = wxString::Format(_("Cannot open JSON file '%s'"), filePath.c_str());
                return false;
            }

            json j = json::parse(ifs);
            const json* arr = nullptr;

            if (j.is_array())
            {
                arr = &j;
            }
            else if (j.is_object())
            {
                static const char* candidateKeys[] = { "rows", "data", "records", "items", "table" };
                for (const char* k : candidateKeys)
                {
                    if (j.contains(k) && j[k].is_array())
                    {
                        arr = &j[k];
                        break;
                    }
                }
                if (!arr)
                {
                    for (auto it = j.begin(); it != j.end(); ++it)
                    {
                        if (it.value().is_array())
                        {
                            arr = &it.value();
                            break;
                        }
                    }
                }
            }

            if (!arr)
            {
                if (errorMessage)
                    *errorMessage = _("JSON content does not contain a record array");
                return false;
            }

            for (const auto& item : *arr)
            {
                if (!item.is_object())
                    continue;

                std::map<wxString, DataCell> rec;
                for (auto it = item.begin(); it != item.end(); ++it)
                {
                    wxString key = wxString::FromUTF8(it.key().c_str());
                    const auto& val = it.value();

                    if (val.is_null())
                    {
                        rec[key] = DataCell::makeNull();
                    }
                    else if (val.is_boolean())
                    {
                        rec[key] = DataCell::makeBoolean(val.get<bool>());
                    }
                    else if (val.is_number_integer())
                    {
                        int64_t n = val.get<int64_t>();
                        rec[key] = DataCell::makeNumber((double)n, wxString::Format("%lld", (long long)n));
                    }
                    else if (val.is_number_float())
                    {
                        double d = val.get<double>();
                        rec[key] = DataCell::makeNumber(d, wxString::FromCDouble(d));
                    }
                    else if (val.is_string())
                    {
                        std::string s = val.get<std::string>();
                        rec[key] = DataCell::makeString(wxString::FromUTF8(s.c_str()));
                    }
                    else
                    {
                        rec[key] = DataCell::makeString(wxString::FromUTF8(val.dump().c_str()));
                    }
                }
                records.push_back(rec);
            }
            return true;
        }
        catch (const std::exception& e)
        {
            if (errorMessage)
                *errorMessage = wxString::FromUTF8(e.what());
            return false;
        }
    }
    else // XML
    {
        wxXmlDocument doc;
        if (!doc.Load(filePath))
        {
            if (errorMessage)
                *errorMessage = wxString::Format(_("Cannot parse XML file '%s'"), filePath.c_str());
            return false;
        }

        wxXmlNode* root = doc.GetRoot();
        if (!root)
        {
            if (errorMessage)
                *errorMessage = _("XML document has no root element");
            return false;
        }

        wxXmlNode* row = root->GetChildren();
        while (row)
        {
            if (row->GetType() == wxXML_ELEMENT_NODE)
            {
                std::map<wxString, DataCell> rec;

                // Attributes
                wxXmlAttribute* attr = row->GetAttributes();
                while (attr)
                {
                    rec[attr->GetName()] = DataCell::makeString(attr->GetValue());
                    attr = attr->GetNext();
                }

                // Child tags
                wxXmlNode* col = row->GetChildren();
                while (col)
                {
                    if (col->GetType() == wxXML_ELEMENT_NODE)
                    {
                        wxString colName = col->GetName();
                        wxString isNil = col->GetAttribute("xsi:nil", "");
                        if (isNil.empty())
                            isNil = col->GetAttribute("null", "");

                        if (isNil.CmpNoCase("true") == 0 || isNil == "1")
                        {
                            rec[colName] = DataCell::makeNull();
                        }
                        else
                        {
                            rec[colName] = DataCell::makeString(col->GetNodeContent());
                        }
                    }
                    col = col->GetNext();
                }
                records.push_back(rec);
            }
            row = row->GetNext();
        }
        return true;
    }
}

bool DataTransferHelper::importTable(Database* db,
                                    Table* table,
                                    const wxString& filePath,
                                    const DataImportOptions& options,
                                    ProgressIndicator* pi,
                                    DataImportResult* resultOut)
{
    DataImportResult localResult;
    DataImportResult& res = resultOut ? *resultOut : localResult;
    res.totalRecords = 0;
    res.importedRecords = 0;
    res.errorCount = 0;
    res.errorMessages.clear();

    if (!db || !table)
    {
        res.errorMessages.push_back(_("Database or Table is null"));
        return false;
    }

    std::vector<std::map<wxString, DataCell>> records;
    wxString parseErr;
    if (!parseRecordsFromFile(filePath, options.format, records, &parseErr))
    {
        res.errorMessages.push_back(parseErr.empty() ? _("Failed to parse input file") : parseErr);
        return false;
    }

    res.totalRecords = records.size();
    if (records.empty())
        return true;

    table->ensureChildrenLoaded();

    // Collect valid mappings (exclude computed columns and unmapped)
    std::vector<wxString> mappedTableCols;
    std::vector<wxString> mappedFileFields;

    for (const auto& pair : options.fieldMappings)
    {
        const wxString& tableCol = pair.first;
        const wxString& fileField = pair.second;

        if (fileField.empty() || fileField == "(None)")
            continue;

        ColumnPtr col = table->findColumn(tableCol);
        if (col && !col->getComputedSource().IsEmpty())
            continue; // Skip computed columns

        mappedTableCols.push_back(tableCol);
        mappedFileFields.push_back(fileField);
    }

    if (mappedTableCols.empty())
    {
        res.errorMessages.push_back(_("No columns are mapped for import"));
        return false;
    }

    // Build INSERT statement
    int dialect = db->getSqlDialect();
    Identifier tabId(table->getName_(), dialect);
    wxString sql = "INSERT INTO " + tabId.getQuoted() + " (";
    wxString params = ") VALUES (";

    for (size_t i = 0; i < mappedTableCols.size(); ++i)
    {
        if (i > 0)
        {
            sql += ", ";
            params += ", ";
        }
        Identifier colId(mappedTableCols[i], dialect);
        sql += colId.getQuoted();
        params += "?";
    }
    sql += params + ")";

    try
    {
        fr::ITransactionPtr tr = db->getDALDatabase()->createTransaction();
        tr->start();

        fr::IStatementPtr st = db->getDALDatabase()->createStatement(tr);
        st->prepare(wx2std(sql, db->getCharsetConverter()));

        for (size_t r = 0; r < records.size(); ++r)
        {
            if (pi && pi->isCanceled())
            {
                tr->rollback();
                res.errorMessages.push_back(_("Import canceled by user"));
                return false;
            }

            const auto& rec = records[r];
            try
            {
                for (size_t p = 0; p < mappedTableCols.size(); ++p)
                {
                    const wxString& field = mappedFileFields[p];
                    auto it = rec.find(field);
                    if (it == rec.end() || it->second.type == DataCell::Null)
                    {
                        st->setNull((int)p);
                    }
                    else
                    {
                        const DataCell& cell = it->second;
                        ColumnType colType = st->getParameterType((int)p);

                        if (colType == ColumnType::Blob)
                        {
                            if (cell.type == DataCell::Blob)
                            {
                                st->setBytes((int)p, cell.blobVal.GetData(), (int)cell.blobVal.GetDataLen());
                            }
                            else
                            {
                                wxMemoryBuffer decoded = wxBase64Decode(cell.textVal);
                                if (decoded.GetDataLen() > 0)
                                {
                                    st->setBytes((int)p, decoded.GetData(), (int)decoded.GetDataLen());
                                }
                                else
                                {
                                    st->setString((int)p, wx2std(cell.textVal, db->getCharsetConverter()));
                                }
                            }
                        }
                        else if (colType == ColumnType::Boolean)
                        {
                            bool b = cell.boolVal || cell.textVal.IsSameAs("true", false) || cell.textVal == "1";
                            st->setBool((int)p, b);
                        }
                        else if (colType == ColumnType::Integer)
                        {
                            if (cell.type == DataCell::Number)
                            {
                                st->setInt32((int)p, (int32_t)cell.numVal);
                            }
                            else
                            {
                                long lVal;
                                if (cell.textVal.ToLong(&lVal))
                                    st->setInt32((int)p, (int32_t)lVal);
                                else
                                    st->setString((int)p, wx2std(cell.textVal, db->getCharsetConverter()));
                            }
                        }
                        else if (colType == ColumnType::BigInt)
                        {
                            if (cell.type == DataCell::Number)
                            {
                                st->setInt64((int)p, (int64_t)cell.numVal);
                            }
                            else
                            {
                                wxLongLong_t llVal;
                                if (cell.textVal.ToLongLong(&llVal))
                                    st->setInt64((int)p, llVal);
                                else
                                    st->setString((int)p, wx2std(cell.textVal, db->getCharsetConverter()));
                            }
                        }
                        else if (colType == ColumnType::Float || colType == ColumnType::Double ||
                                 colType == ColumnType::Numeric || colType == ColumnType::Decimal)
                        {
                            if (cell.type == DataCell::Number)
                            {
                                st->setDouble((int)p, cell.numVal);
                            }
                            else
                            {
                                double dVal;
                                if (cell.textVal.ToCDouble(&dVal) || cell.textVal.ToDouble(&dVal))
                                    st->setDouble((int)p, dVal);
                                else
                                    st->setString((int)p, wx2std(cell.textVal, db->getCharsetConverter()));
                            }
                        }
                        else
                        {
                            st->setString((int)p, wx2std(cell.textVal, db->getCharsetConverter()));
                        }
                    }
                }

                st->execute();
                res.importedRecords++;

                if (options.commitBatchSize > 0 && (res.importedRecords % options.commitBatchSize == 0))
                {
                    tr->commit();
                    tr->start();
                    st = db->getDALDatabase()->createStatement(tr);
                    st->prepare(wx2std(sql, db->getCharsetConverter()));
                }
            }
            catch (const std::exception& ex)
            {
                res.errorCount++;
                wxString err = wxString::Format(_("Row %zu: %s"), r + 1, wxString::FromUTF8(ex.what()).c_str());
                res.errorMessages.push_back(err);

                if (options.errorHandling == ErrorHandling::AbortOnError)
                {
                    try { tr->rollback(); } catch (...) {}
                    return false;
                }
            }

            if (pi)
                pi->stepProgress();
        }

        tr->commit();
        return true;
    }
    catch (const std::exception& e)
    {
        res.errorMessages.push_back(wxString::FromUTF8(e.what()));
        return false;
    }
}

} // namespace fr
