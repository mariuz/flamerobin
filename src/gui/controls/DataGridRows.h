/*
  Copyright (c) 2004-2007 The FlameRobin Development Team

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


  $Id$

*/

#ifndef DATAGRIDROWS_H
#define DATAGRIDROWS_H
//-----------------------------------------------------------------------------
#include <vector>
#include <map>
#include <list>

#include <ibpp.h>

#include "metadata/constraints.h"
//----------------------------------------------------------------------
class Database;
class DataGridRowBuffer;
class wxMBConv;
//----------------------------------------------------------------------
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
    virtual wxString getAsString(DataGridRowBuffer* buffer) = 0;
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source) = 0;
    virtual unsigned getBufferSize() = 0;
    wxString getName();
    virtual bool isNumeric();
    bool isReadOnly();
    bool isNullable();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter) = 0;
};
//----------------------------------------------------------------------
class DataGridRows
{
private:
    IBPP::Statement statementM;
    std::vector<ResultsetColumnDef*> columnDefsM;
    std::vector<DataGridRowBuffer*> buffersM;
    std::map<wxString, UniqueConstraint *> statementTablesM;
    std::map<wxString, UniqueConstraint *>::iterator deleteFromM;
    std::list<UniqueConstraint> dbKeysM;
    unsigned bufferSizeM;

    void getColumnInfo(Database *db, unsigned col, bool& readOnly,
        bool& nullable);
    void addWhereAndExecute(UniqueConstraint* uq, wxString& stm,
        const wxString& table, DataGridRowBuffer *buffer);
public:
    DataGridRows();
    ~DataGridRows();

    bool addRow(const IBPP::Statement& statement, wxMBConv* converter);
    void clear();
    unsigned getRowCount();
    unsigned getRowFieldCount();
    wxString getRowFieldName(unsigned col);
    bool initialize(const IBPP::Statement& statement, Database *);
    bool isRowFieldNumeric(unsigned col);
    bool isColumnReadonly(unsigned col);

    wxString getFieldValue(unsigned row, unsigned col);
    bool isFieldNull(unsigned row, unsigned col);
    bool isFieldNA(unsigned row, unsigned col);
    wxString setFieldValue(unsigned row, unsigned col,
        const wxString& value);
    bool removeRows(size_t from, size_t count, wxString& statement);

    ResultsetColumnDef *getColumnDef(unsigned col);
    void addRow(DataGridRowBuffer *);
};
//----------------------------------------------------------------------
#endif
