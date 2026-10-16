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

#include <iostream>
#include <wx/string.h>
#include <wx/mstream.h>
#include <wx/wfstream.h>
#include <wx/filename.h>
#include "core/DataTransferHelper.h"
#include <nlohmann/json.hpp>

using namespace fr;

int main()
{
    bool ok = true;

    auto check = [](bool condition, const char* name) {
        if (!condition) {
            std::cerr << "FAIL: " << name << "\n";
            return false;
        }
        std::cout << "PASS: " << name << "\n";
        return true;
    };

    // Test 1: Sanitize XML tag name
    ok = check(DataTransferHelper::sanitizeXmlTagName("Customer Name") == "Customer_Name", "sanitizeXmlTagName with space") && ok;
    ok = check(DataTransferHelper::sanitizeXmlTagName("1st_Order") == "_1st_Order", "sanitizeXmlTagName starting with number") && ok;
    ok = check(DataTransferHelper::sanitizeXmlTagName("ITEM#ID") == "ITEM_ID", "sanitizeXmlTagName with special char") && ok;
    ok = check(DataTransferHelper::sanitizeXmlTagName("") == "col", "sanitizeXmlTagName empty") && ok;

    // Test 2: Escape XML and JSON
    ok = check(DataTransferHelper::escapeXml("<tag & 'quote' \"test\">") == "&lt;tag &amp; &apos;quote&apos; &quot;test&quot;&gt;", "escapeXml characters") && ok;
    ok = check(DataTransferHelper::escapeJson("Line 1\nLine 2\t\"quoted\"") == "Line 1\\nLine 2\\t\\\"quoted\\\"", "escapeJson characters") && ok;

    // Test 3: Auto-map fields
    std::vector<wxString> tableCols = { "ID", "CUSTOMER_NAME", "BIRTH_DATE", "IS_ACTIVE", "EXTRA_NOTES" };
    std::vector<wxString> fileFields = { "id", "customer_name", "birthDate", "is_active" };
    auto mapping = DataTransferHelper::autoMapFields(tableCols, fileFields);
    ok = check(mapping["ID"] == "id", "autoMapFields ID exact/case-insensitive") && ok;
    ok = check(mapping["CUSTOMER_NAME"] == "customer_name", "autoMapFields CUSTOMER_NAME") && ok;
    ok = check(mapping["BIRTH_DATE"] == "birthDate", "autoMapFields BIRTH_DATE normalized") && ok;
    ok = check(mapping["IS_ACTIVE"] == "is_active", "autoMapFields IS_ACTIVE") && ok;
    ok = check(mapping["EXTRA_NOTES"].empty(), "autoMapFields EXTRA_NOTES unmapped") && ok;

    // Test 4: Export to JSON and verify with nlohmann::json
    std::vector<wxString> cols = { "ID", "NAME", "SCORE", "ACTIVE", "AVATAR", "NOTE" };
    wxMemoryBuffer blobData;
    const char* sampleBytes = "BINARY_BLOB_CONTENT";
    blobData.AppendData(sampleBytes, strlen(sampleBytes));

    std::vector<std::vector<DataCell>> rows = {
        {
            DataCell::makeNumber(1, "1"),
            DataCell::makeString("Alice"),
            DataCell::makeNumber(98.5, "98.5"),
            DataCell::makeBoolean(true),
            DataCell::makeBlob(blobData),
            DataCell::makeNull()
        },
        {
            DataCell::makeNumber(2, "2"),
            DataCell::makeString("Bob <Developer> & Designer"),
            DataCell::makeNumber(85.0, "85"),
            DataCell::makeBoolean(false),
            DataCell::makeNull(),
            DataCell::makeString("Some note")
        }
    };

    DataExportOptions jsonOpts;
    jsonOpts.format = DataTransferFormat::Json;
    jsonOpts.prettyPrint = true;
    jsonOpts.nullHandling = NullHandling::NullValue;
    jsonOpts.base64Blobs = true;

    wxMemoryOutputStream memJson;
    bool expJsonOk = DataTransferHelper::exportDataset(cols, rows, memJson, jsonOpts);
    ok = check(expJsonOk, "exportDataset JSON success") && ok;

    wxString jsonStr = wxString::FromUTF8(static_cast<const char*>(memJson.GetOutputStreamBuffer()->GetBufferStart()),
                                         memJson.GetOutputStreamBuffer()->GetBufferSize());
    ok = check(!jsonStr.empty(), "JSON output not empty") && ok;

    try {
        auto parsedJson = nlohmann::json::parse(std::string(jsonStr.ToUTF8()));
        ok = check(parsedJson.is_array() && parsedJson.size() == 2, "JSON parsed as array with 2 items") && ok;
        ok = check(parsedJson[0]["NAME"] == "Alice", "JSON row 0 NAME") && ok;
        ok = check(parsedJson[0]["ACTIVE"] == true, "JSON row 0 ACTIVE boolean") && ok;
        ok = check(parsedJson[0]["NOTE"].is_null(), "JSON row 0 NOTE is null") && ok;
        ok = check(parsedJson[0]["AVATAR"] == "QklOQVJZX0JMT0JfQ09OVEVOVA==", "JSON row 0 AVATAR base64") && ok;
        ok = check(parsedJson[1]["NAME"] == "Bob <Developer> & Designer", "JSON row 1 NAME") && ok;
        ok = check(parsedJson[1]["ACTIVE"] == false, "JSON row 1 ACTIVE boolean") && ok;
    } catch (const std::exception& e) {
        std::cerr << "JSON Parse error: " << e.what() << "\n";
        ok = check(false, "JSON parsing verification") && ok;
    }

    // Test 5: Export JSON with NullHandling::Omit
    DataExportOptions jsonOmitOpts = jsonOpts;
    jsonOmitOpts.nullHandling = NullHandling::Omit;
    wxMemoryOutputStream memJsonOmit;
    DataTransferHelper::exportDataset(cols, rows, memJsonOmit, jsonOmitOpts);
    wxString jsonOmitStr = wxString::FromUTF8(static_cast<const char*>(memJsonOmit.GetOutputStreamBuffer()->GetBufferStart()),
                                             memJsonOmit.GetOutputStreamBuffer()->GetBufferSize());
    try {
        auto parsedOmit = nlohmann::json::parse(std::string(jsonOmitStr.ToUTF8()));
        ok = check(!parsedOmit[0].contains("NOTE"), "JSON row 0 NOTE omitted") && ok;
        ok = check(!parsedOmit[1].contains("AVATAR"), "JSON row 1 AVATAR omitted") && ok;
    } catch (...) {
        ok = check(false, "JSON omit parse verification") && ok;
    }

    // Test 6: Export to XML and parse back
    DataExportOptions xmlOpts;
    xmlOpts.format = DataTransferFormat::Xml;
    xmlOpts.prettyPrint = true;
    xmlOpts.nullHandling = NullHandling::NullValue;
    xmlOpts.base64Blobs = true;
    xmlOpts.xmlRootTag = "CUSTOMERS";
    xmlOpts.xmlRowTag = "CUSTOMER";

    wxMemoryOutputStream memXml;
    bool expXmlOk = DataTransferHelper::exportDataset(cols, rows, memXml, xmlOpts);
    ok = check(expXmlOk, "exportDataset XML success") && ok;

    wxString xmlStr = wxString::FromUTF8(static_cast<const char*>(memXml.GetOutputStreamBuffer()->GetBufferStart()),
                                         memXml.GetOutputStreamBuffer()->GetBufferSize());
    ok = check(xmlStr.Find("<CUSTOMERS") != wxNOT_FOUND, "XML contains root tag CUSTOMERS") && ok;
    ok = check(xmlStr.Find("<CUSTOMER>") != wxNOT_FOUND, "XML contains row tag CUSTOMER") && ok;
    ok = check(xmlStr.Find("<NOTE xsi:nil=\"true\"/>") != wxNOT_FOUND, "XML contains xsi:nil for NULL") && ok;
    ok = check(xmlStr.Find("&lt;Developer&gt; &amp; Designer") != wxNOT_FOUND, "XML contains escaped special characters") && ok;
    ok = check(xmlStr.Find("QklOQVJZX0JMT0JfQ09OVEVOVA==") != wxNOT_FOUND, "XML contains base64 blob") && ok;

    // Test 7: Export to temporary file and parse back
    wxString tmpJsonFile = wxFileName::CreateTempFileName("fr_dt_test_json");
    wxString tmpXmlFile = wxFileName::CreateTempFileName("fr_dt_test_xml");
    // Ensure file names end with proper extensions for detectFormat
    wxString testJsonPath = tmpJsonFile + ".json";
    wxString testXmlPath = tmpXmlFile + ".xml";

    DataTransferHelper::exportDatasetToFile(cols, rows, testJsonPath, jsonOpts);
    DataTransferHelper::exportDatasetToFile(cols, rows, testXmlPath, xmlOpts);

    ok = check(DataTransferHelper::detectFormat(testJsonPath) == DataTransferFormat::Json, "detectFormat JSON") && ok;
    ok = check(DataTransferHelper::detectFormat(testXmlPath) == DataTransferFormat::Xml, "detectFormat XML") && ok;

    // Test 8: Extract fields from file
    std::vector<wxString> jsonFields = DataTransferHelper::extractFieldsFromFile(testJsonPath, DataTransferFormat::Json);
    ok = check(jsonFields.size() == 6, "extractFieldsFromFile JSON size 6") && ok;
    ok = check(std::find(jsonFields.begin(), jsonFields.end(), "NAME") != jsonFields.end(), "extractFieldsFromFile JSON contains NAME") && ok;

    std::vector<wxString> xmlFields = DataTransferHelper::extractFieldsFromFile(testXmlPath, DataTransferFormat::Xml);
    ok = check(xmlFields.size() >= 5, "extractFieldsFromFile XML fields") && ok;
    ok = check(std::find(xmlFields.begin(), xmlFields.end(), "NAME") != xmlFields.end(), "extractFieldsFromFile XML contains NAME") && ok;

    // Test 9: Parse records from file
    std::vector<std::map<wxString, DataCell>> parsedJsonRecords;
    wxString parseErr;
    bool parseJsonOk = DataTransferHelper::parseRecordsFromFile(testJsonPath, DataTransferFormat::Json, parsedJsonRecords, &parseErr);
    ok = check(parseJsonOk && parsedJsonRecords.size() == 2, "parseRecordsFromFile JSON success") && ok;
    ok = check(parsedJsonRecords[0]["NAME"].textVal == "Alice", "parseRecordsFromFile JSON row 0 NAME") && ok;
    ok = check(parsedJsonRecords[0]["NOTE"].type == DataCell::Null, "parseRecordsFromFile JSON row 0 NOTE null") && ok;
    ok = check(parsedJsonRecords[1]["ACTIVE"].boolVal == false, "parseRecordsFromFile JSON row 1 ACTIVE boolean") && ok;

    std::vector<std::map<wxString, DataCell>> parsedXmlRecords;
    bool parseXmlOk = DataTransferHelper::parseRecordsFromFile(testXmlPath, DataTransferFormat::Xml, parsedXmlRecords, &parseErr);
    ok = check(parseXmlOk && parsedXmlRecords.size() == 2, "parseRecordsFromFile XML success") && ok;
    ok = check(parsedXmlRecords[0]["NAME"].textVal == "Alice", "parseRecordsFromFile XML row 0 NAME") && ok;
    ok = check(parsedXmlRecords[0]["NOTE"].type == DataCell::Null, "parseRecordsFromFile XML row 0 NOTE null") && ok;
    ok = check(parsedXmlRecords[1]["NAME"].textVal == "Bob <Developer> & Designer", "parseRecordsFromFile XML row 1 unescaped NAME") && ok;

    // Cleanup temp files
    wxRemoveFile(tmpJsonFile);
    wxRemoveFile(tmpXmlFile);
    wxRemoveFile(testJsonPath);
    wxRemoveFile(testXmlPath);

    return ok ? 0 : 1;
}
