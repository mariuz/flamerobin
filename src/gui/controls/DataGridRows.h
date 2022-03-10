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

#ifndef DATAGRIDROWS_H
#define DATAGRIDROWS_H

#include <vector>
#include <map>
#include <list>

#include <ibpp.h>

#include "metadata/constraints.h"
#include "config/Config.h"

class Database;
class DataGridRowBuffer;
class ProgressIndicator;
class wxMBConv;

// GridCellFormats: class to cache config data for cell formatting
class GridCellFormats: public ConfigCache
{
private:
    enum ShowTimezoneInfoType
	{
        // ** Keep in sync with radiogroup control **
        // append no timezone info
        tzNone = 0,
        // append timezone raw value (id)
        tzRawId   = 1,
        // append timezone name
        tzName = 2
	};

private:
    int floatingPointPrecisionM;
    wxString dateFormatM;
    int maxBlobKBytesM;
    bool showBinaryBlobContentM;
    bool showBlobContentM;
    wxString timeFormatM;
    wxString timestampFormatM;
    ShowTimezoneInfoType showTimezoneInfoM;
    void formatAppendTz(wxString &s, IBPP::Time &t, bool hasTz,
        Database* db);
protected:
    virtual void loadFromConfig();
public:
    GridCellFormats();

    static GridCellFormats& get();

    template<typename T>
    wxString format(T value);
    wxString formatDate(int year, int month, int day);
    wxString formatTime(IBPP::Time &t, bool hasTz, Database* db);
    wxString formatTimestamp(IBPP::Timestamp &ts, bool hasTz, Database* db);

    int maxBlobBytesToFetch();
    bool parseDate(wxString::iterator& start, wxString::iterator end,
        bool consumeAll, int& year, int& month, int& day);
    bool parseTime(wxString::iterator& start, wxString::iterator end,
        int& hr, int& mn, int& sc, int& ml);
    bool parseTimestamp(wxString::iterator& start, wxString::iterator end,
        int& year, int& month, int& day, int& hr, int& mn, int& sc, int& ml);
    bool showBinaryBlobContent();
    bool showBlobContent();
};

class ResultsetColumnDef
{
private:
    wxString nameM;
protected:
    bool readOnlyM;
    bool nullableM;
public:
    ResultsetColumnDef(const wxString& name, bool readOnly = true,
        bool nullable = false);
    virtual ~ResultsetColumnDef();

    virtual wxString getAsFirebirdString(DataGridRowBuffer* buffer);
    virtual wxString getAsString(DataGridRowBuffer* buffer, Database* db) = 0;
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source) = 0;
    virtual unsigned getBufferSize() = 0;
    wxString getName();
    virtual unsigned getIndex(); // for strings and blobs
    virtual bool isNumeric();
    bool isReadOnly();
    bool isNullable();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter, Database* db) = 0;
};

struct DataGridFieldInfo
{
    bool rowInserted;
    bool rowDeleted;
    bool fieldReadOnly;
    bool fieldModified;
    bool fieldNull;
    bool fieldNA;
    bool fieldNumeric;
    bool fieldBlob;
};
struct DataGridRowsBlob
{
    IBPP::Blob blob;
    IBPP::Statement st;
    unsigned row;
    unsigned col;
};

class DataGridRows
{
private:
    Database* databaseM;
    const bool readOnlyM;
    IBPP::Statement statementM;
    std::vector<ResultsetColumnDef*> columnDefsM;
    std::vector<DataGridRowBuffer*> buffersM;
    std::map<wxString, UniqueConstraint *> statementTablesM;
    std::map<wxString, UniqueConstraint *>::iterator deleteFromM;
    std::list<UniqueConstraint> dbKeysM;
    unsigned bufferSizeM;

    void getColumnInfo(Database* db, unsigned col, bool& readOnly,
        bool& nullable);
    IBPP::Statement addWhere(UniqueConstraint* uq, wxString& stm,
        const wxString& table, DataGridRowBuffer *buffer);
public:
    DataGridRows(Database* db);
    ~DataGridRows();

    void addRow(const IBPP::Statement& statement);
    void clear();
    unsigned getRowCount();
    unsigned getRowFieldCount();
    wxString getRowFieldName(unsigned col);
    bool initialize(const IBPP::Statement& statement);

    bool isColumnNullable(unsigned col);
    bool isColumnNumeric(unsigned col);
    bool isColumnReadonly(unsigned col);
    bool isBlobColumn(unsigned col, bool* pIsTextual = 0);
    bool getFieldInfo(unsigned row, unsigned col, DataGridFieldInfo& info);
    bool isFieldReadonly(unsigned row, unsigned col);
    bool isFieldNull(unsigned row, unsigned col);
    bool isFieldNA(unsigned row, unsigned col);

    wxString getFieldValue(unsigned row, unsigned col);
    wxString setFieldValue(unsigned row, unsigned col,
        const wxString& value, bool setNull = false);
    void importBlobFile(const wxString& filename, unsigned row, unsigned col,
        ProgressIndicator *pi);
    void exportBlobFile(const wxString& filename, unsigned row, unsigned col,
        ProgressIndicator *pi);
    bool canRemoveRow(size_t row);
    bool removeRows(size_t from, size_t count, wxString& statement);

    ResultsetColumnDef* getColumnDef(unsigned col);
    void addRow(DataGridRowBuffer* buffer);

    // BLOB-Stuff
    IBPP::Blob* getBlob(unsigned row, unsigned col, bool validateBlob);
    DataGridRowsBlob setBlobPrepare(unsigned row, unsigned col);
    void setBlob(DataGridRowsBlob &b);
};

#endif
