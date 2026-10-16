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

#ifndef FR_DATATRANSFERHELPER_H
#define FR_DATATRANSFERHELPER_H

#include <wx/wx.h>
#include <wx/mstream.h>
#include <wx/wfstream.h>
#include <vector>
#include <map>
#include <string>

class Database;
class Table;
class Relation;
class DataGrid;
class ProgressIndicator;

namespace fr {

enum class DataTransferFormat {
    Auto,
    Json,
    Xml
};

enum class NullHandling {
    NullValue,     // JSON: null, XML: <col xsi:nil="true"/>
    Omit,          // JSON: omit key, XML: omit tag
    EmptyString    // JSON: "", XML: <col></col>
};

enum class ErrorHandling {
    AbortOnError,
    IgnoreErrors
};

struct DataCell {
    enum Type { Null, String, Number, Boolean, Blob };
    Type type = Null;
    wxString textVal;
    double numVal = 0.0;
    bool boolVal = false;
    wxMemoryBuffer blobVal;

    static DataCell makeNull() {
        DataCell c;
        c.type = Null;
        return c;
    }
    static DataCell makeString(const wxString& s) {
        DataCell c;
        c.type = String;
        c.textVal = s;
        return c;
    }
    static DataCell makeNumber(double n, const wxString& text = wxEmptyString) {
        DataCell c;
        c.type = Number;
        c.numVal = n;
        c.textVal = text.empty() ? wxString::FromCDouble(n) : text;
        return c;
    }
    static DataCell makeBoolean(bool b) {
        DataCell c;
        c.type = Boolean;
        c.boolVal = b;
        c.textVal = b ? "true" : "false";
        return c;
    }
    static DataCell makeBlob(const wxMemoryBuffer& buf) {
        DataCell c;
        c.type = Blob;
        c.blobVal = buf;
        return c;
    }
};

struct DataExportOptions {
    DataTransferFormat format = DataTransferFormat::Json;
    std::vector<wxString> columns;       // columns to export; empty means all
    bool prettyPrint = true;
    NullHandling nullHandling = NullHandling::NullValue;
    bool base64Blobs = true;
    wxString xmlRootTag = "table";
    wxString xmlRowTag = "row";
    wxString encoding = "UTF-8";
};

struct DataImportOptions {
    DataTransferFormat format = DataTransferFormat::Auto;
    // Map of target Table column name -> source file field name
    std::map<wxString, wxString> fieldMappings;
    int commitBatchSize = 500;
    ErrorHandling errorHandling = ErrorHandling::AbortOnError;
};

struct DataImportResult {
    size_t totalRecords = 0;
    size_t importedRecords = 0;
    size_t errorCount = 0;
    std::vector<wxString> errorMessages;
};

class DataTransferHelper {
public:
    // String & format utility functions
    static wxString sanitizeXmlTagName(const wxString& name);
    static wxString escapeXml(const wxString& str);
    static wxString escapeJson(const wxString& str);

    // Export in-memory dataset
    static bool exportDataset(const std::vector<wxString>& columnNames,
                              const std::vector<std::vector<DataCell>>& rows,
                              wxOutputStream& outStream,
                              const DataExportOptions& options,
                              ProgressIndicator* pi = nullptr);

    static bool exportDatasetToFile(const std::vector<wxString>& columnNames,
                                    const std::vector<std::vector<DataCell>>& rows,
                                    const wxString& filePath,
                                    const DataExportOptions& options,
                                    ProgressIndicator* pi = nullptr);

    // Export from Database Table or View
    static bool exportRelation(Database* db,
                               Relation* relation,
                               const wxString& filePath,
                               const DataExportOptions& options,
                               ProgressIndicator* pi = nullptr,
                               wxString* errorMessage = nullptr);

    // Export from DataGrid
    static bool exportDataGrid(DataGrid* grid,
                               const wxString& filePath,
                               const DataExportOptions& options,
                               ProgressIndicator* pi = nullptr,
                               wxString* errorMessage = nullptr);

    // Import utilities
    static DataTransferFormat detectFormat(const wxString& filePath);
    static std::vector<wxString> extractFieldsFromFile(const wxString& filePath,
                                                       DataTransferFormat format = DataTransferFormat::Auto);
    static std::map<wxString, wxString> autoMapFields(const std::vector<wxString>& tableColumns,
                                                      const std::vector<wxString>& fileFields);

    static bool parseRecordsFromFile(const wxString& filePath,
                                     DataTransferFormat format,
                                     std::vector<std::map<wxString, DataCell>>& records,
                                     wxString* errorMessage = nullptr);

    // Import into Database Table
    static bool importTable(Database* db,
                            Table* table,
                            const wxString& filePath,
                            const DataImportOptions& options,
                            ProgressIndicator* pi = nullptr,
                            DataImportResult* resultOut = nullptr);
};

} // namespace fr

#endif // FR_DATATRANSFERHELPER_H
