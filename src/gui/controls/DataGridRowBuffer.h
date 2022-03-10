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

#ifndef FR_DATAGRIDROWBUFFER_H
#define FR_DATAGRIDROWBUFFER_H

#include <ibpp.h>
#include <core/FRInt128.h>
#include <core/FRDecimal.h>


struct DataGridRowBufferFieldAttr
// use bits instead of bool here to save memory
{
    // Field is null or not
    bool isNull:1; // accesed by indexM
    // The buffer (stringsM) is loaded or not 
    // ATT: isStringLoaded is used with stringsM (see below)
    //      the size of stringsM can be less than fieldCount.
    //      It is accesed by stringIndexM.
    bool isStringLoaded:1;  // accessed by stringIndexM !!
};

// DataGridRowBuffer class
class DataGridRowBuffer
{
private:
    // use bits instead of bool here to use less memory
    bool isModifiedM:1;
    bool isDeletedM:1;
    bool isDeletableIsSetM:1;
    bool isDeletableM:1;
protected:
    std::vector<DataGridRowBufferFieldAttr> fieldAttrM;
    std::vector<uint8_t> dataM;
    std::vector<wxString> stringsM;
    std::vector<IBPP::Blob> blobsM;
    void invalidateIsDeletable();
    void setIsModified(bool value);
public:
    DataGridRowBuffer(unsigned fieldCount);
    DataGridRowBuffer(const DataGridRowBuffer* other);
    virtual ~DataGridRowBuffer() {}

    wxString getString(unsigned index);
    IBPP::Blob *getBlob(unsigned index);
    bool getValue(unsigned offset, double& value);
    bool getValue(unsigned offset, float& value);
    bool getValue(unsigned offset, dec16_t& value);
    bool getValue(unsigned offset, dec34_t& value);
    bool getValue(unsigned offset, int& value);
    bool getValue(unsigned offset, int64_t& value);
    bool getValue(unsigned offset, int128_t& value);
    bool getValue(unsigned offset, IBPP::DBKey& value, unsigned size);
    bool isFieldNull(unsigned num);
    void setFieldNull(unsigned num, bool isNull);
    virtual bool isFieldNA(unsigned num);
    virtual void setFieldNA(unsigned num, bool isNA);
    bool isStringLoaded(unsigned num);
    void setStringLoaded(unsigned num, bool isLoaded);
    void setString(unsigned num, const wxString& value);
    void setBlob(unsigned num, IBPP::Blob b);
    void setValue(unsigned offset, double value);
    void setValue(unsigned offset, float value);
    void setValue(unsigned offset, dec16_t value);
    void setValue(unsigned offset, dec34_t value);
    void setValue(unsigned offset, int value);
    void setValue(unsigned offset, int64_t value);
    void setValue(unsigned offset, int128_t value);
    void setValue(unsigned offset, IBPP::DBKey value);

    virtual bool isInserted();
    bool isFieldModified(unsigned num);
    bool isDeletable();
    bool isDeletableIsSet();
    void setIsDeletable(bool value);
    bool isDeleted();
    void setIsDeleted(bool value);
};

// class for rows inserted by user - to minimize memory usage of regular rows
// and also speed up code in DataGridRows::isFieldReadonly
class InsertedGridRowBuffer: public DataGridRowBuffer
{
protected:
    std::vector<bool> naFieldsM;
public:
    InsertedGridRowBuffer(unsigned fieldCount);
    InsertedGridRowBuffer(const InsertedGridRowBuffer* other);

    virtual bool isInserted();
    virtual bool isFieldNA(unsigned num);
    virtual void setFieldNA(unsigned num, bool isNA);
};

#endif
