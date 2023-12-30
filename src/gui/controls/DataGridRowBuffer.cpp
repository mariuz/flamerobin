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

#include "gui/controls/DataGridRowBuffer.h"

// Time + timestamp internal struct
union TimeZoneBufferValue
{
    int rawValue;
    struct _s
    {
        uint16_t timeZone16;
        uint8_t isGmtFallback:1;
    } s;
};

DataGridRowBuffer::DataGridRowBuffer(unsigned fieldCount)
{
    isModifiedM = 0;
    isDeletedM = 0;
    isDeletableIsSetM = 0;
    isDeletableM = 0;
    // initialize with field count, all fields initially NULL
    // there's no need to preallocate the uint8 buffer or string array
    DataGridRowBufferFieldAttr initValue;
    initValue.isStringLoaded = false;
    initValue.isNull = true;
    fieldAttrM.resize(fieldCount, initValue);
}

DataGridRowBuffer::DataGridRowBuffer(const DataGridRowBuffer* other)
{
    fieldAttrM = other->fieldAttrM;
    dataM = other->dataM;
    stringsM = other->stringsM;
    blobsM = other->blobsM;

    isModifiedM = other->isModifiedM;
    isDeletedM = other->isDeletedM;
    isDeletableIsSetM = other->isDeletableIsSetM;
    isDeletableM = other->isDeletableM;
}

wxString DataGridRowBuffer::getString(unsigned index)
{
    if (index >= stringsM.size())
        return wxEmptyString;
    return stringsM[index];
}

IBPP::Blob* DataGridRowBuffer::getBlob(unsigned index)
{
    if (index >= blobsM.size())
        return 0;
    return &(blobsM[index]);
}

bool DataGridRowBuffer::getValue(unsigned offset, double& value)
{
    if (offset + sizeof(double) > dataM.size())
        return false;
    value = *((double*)&dataM[offset]);
    return true;
}

bool DataGridRowBuffer::getValue(unsigned offset, float& value)
{
    if (offset + sizeof(float) > dataM.size())
        return false;
    value = *((float*)&dataM[offset]);
    return true;
}

bool DataGridRowBuffer::getValue(unsigned offset, dec16_t& value)
{
    if (offset + sizeof(dec16_t) > dataM.size())
        return false;
    value = *((dec16_t*)&dataM[offset]);
    return true;
}

bool DataGridRowBuffer::getValue(unsigned offset, dec34_t& value)
{
    if (offset + sizeof(dec34_t) > dataM.size())
        return false;
    value = *((dec34_t*)&dataM[offset]);
    return true;
}

bool DataGridRowBuffer::getValue(unsigned offset, int& value)
{
    if (offset + sizeof(int) > dataM.size())
        return false;
    value = *((int*)&dataM[offset]);
    return true;
}

bool DataGridRowBuffer::getValue(unsigned offset, int64_t& value)
{
    if (offset + sizeof(int64_t) > dataM.size())
        return false;
    value = *((int64_t*)&dataM[offset]);
    return true;
}

bool DataGridRowBuffer::getValue(unsigned offset, int128_t& value)
{
    if (offset + sizeof(int128_t) > dataM.size())
        return false;
    value = *((int128_t*)&dataM[offset]);
    return true;
}

bool DataGridRowBuffer::getValue(unsigned offset, IBPP::DBKey& value,
    unsigned size)
{
    if (offset + size > dataM.size())
        return false;
    value.SetKey(&dataM[offset], size);
    return true;
}

bool DataGridRowBuffer::getValue32(unsigned offset, int& timeZone, bool& isGmtFallback)
{
    TimeZoneBufferValue value;
    if (!getValue(offset, value.rawValue))
        return false;
    timeZone = value.s.timeZone16;
    isGmtFallback = (bool)value.s.isGmtFallback;
    return true;
}

bool DataGridRowBuffer::isFieldNA(unsigned /*num*/)
{
    return false;
}

void DataGridRowBuffer::setFieldNA(unsigned /* num */, bool /* isNA */)
{
    // should never happen
    invalidateIsDeletable();
}

bool DataGridRowBuffer::isFieldNull(unsigned num)
{
    return (num < fieldAttrM.size() && fieldAttrM[num].isNull);
}

void DataGridRowBuffer::setFieldNull(unsigned num, bool isNull)
{
    if (num < fieldAttrM.size())
    {
        fieldAttrM[num].isNull = isNull;
        invalidateIsDeletable();
    }
}

bool DataGridRowBuffer::isStringLoaded(unsigned num)
{
    return (num < fieldAttrM.size() && fieldAttrM[num].isStringLoaded);
}

void DataGridRowBuffer::setStringLoaded(unsigned num, bool isLoaded)
{
    if (num < fieldAttrM.size())
    {
        fieldAttrM[num].isStringLoaded = isLoaded;
        invalidateIsDeletable();
    }
}

void DataGridRowBuffer::setString(unsigned num, const wxString& value)
{
    if (num >= stringsM.size())
        stringsM.resize(num + 1, wxEmptyString);
    stringsM[num] = value;
    fieldAttrM[num].isStringLoaded = true;
    invalidateIsDeletable();
}

void DataGridRowBuffer::setBlob(unsigned num, IBPP::Blob value)
{
    if (num >= blobsM.size())
        blobsM.resize(num + 1);
    blobsM[num] = value;
    invalidateIsDeletable();
}

void DataGridRowBuffer::setValue(unsigned offset, double value)
{
    if (offset + sizeof(double) > dataM.size())
        dataM.resize(offset + sizeof(double), 0);
    *((double*)&dataM[offset]) = value;
    invalidateIsDeletable();
}

void DataGridRowBuffer::setValue(unsigned offset, float value)
{
    if (offset + sizeof(float) > dataM.size())
        dataM.resize(offset + sizeof(float), 0);
    *((float*)&dataM[offset]) = value;
    invalidateIsDeletable();
}

void DataGridRowBuffer::setValue(unsigned offset, dec16_t value)
{
    if (offset + sizeof(dec16_t) > dataM.size())
        dataM.resize(offset + sizeof(dec16_t), 0);
    *((dec16_t*)&dataM[offset]) = value;
    invalidateIsDeletable();
}

void DataGridRowBuffer::setValue(unsigned offset, dec34_t value)
{
    if (offset + sizeof(dec34_t) > dataM.size())
        dataM.resize(offset + sizeof(dec34_t), 0);
    *((dec34_t*)&dataM[offset]) = value;
    invalidateIsDeletable();
}

void DataGridRowBuffer::setValue(unsigned offset, int value)
{
    if (offset + sizeof(int) > dataM.size())
        dataM.resize(offset + sizeof(int), 0);
    *((int*)&dataM[offset]) = value;
    invalidateIsDeletable();
}

void DataGridRowBuffer::setValue(unsigned offset, int64_t value)
{
    if (offset + sizeof(int64_t) > dataM.size())
        dataM.resize(offset + sizeof(int64_t), 0);
    *((int64_t*)&dataM[offset]) = value;
    invalidateIsDeletable();
}

void  DataGridRowBuffer::setValue(unsigned offset, int128_t value)
{
    if (offset + sizeof(int128_t) > dataM.size())
        dataM.resize(offset + sizeof(int128_t), 0);
    *((int128_t*)&dataM[offset]) = value;
    invalidateIsDeletable();
}

void DataGridRowBuffer::setValue(unsigned offset, IBPP::DBKey value)
{
    if (offset + value.Size() > dataM.size())
        dataM.resize(offset + value.Size(), 0);
    value.GetKey(&dataM[offset], value.Size());
    invalidateIsDeletable();
}

void DataGridRowBuffer::setValue32(unsigned offset, int timeZone, bool isGmtFallback)
{
    TimeZoneBufferValue value;
    value.s.timeZone16 = (uint16_t)timeZone;
    value.s.isGmtFallback = isGmtFallback;
    setValue(offset, value.rawValue);
}

bool DataGridRowBuffer::isInserted()
{
    return false;
}

bool DataGridRowBuffer::isFieldModified(unsigned /*num*/)
{
    // TODO: maintain on a per-field basis
    return isModifiedM != 0;
}

void DataGridRowBuffer::setIsModified(bool value)
{
    isModifiedM = (value) ? 1 : 0;
}

void DataGridRowBuffer::invalidateIsDeletable()
{
    isDeletableIsSetM = 0;
    isDeletableM = 0;
}

bool DataGridRowBuffer::isDeletable()
{
    wxASSERT(isDeletableIsSetM);
    return isDeletableM != 0;
}

bool DataGridRowBuffer::isDeletableIsSet()
{
    return isDeletableIsSetM != 0;
}

void DataGridRowBuffer::setIsDeletable(bool value)
{
    isDeletableIsSetM = 1;
    isDeletableM = value;
}

bool DataGridRowBuffer::isDeleted()
{
    return isDeletedM != 0;
}

void DataGridRowBuffer::setIsDeleted(bool value)
{
    isDeletedM = (value) ? 1 : 0;
}

InsertedGridRowBuffer::InsertedGridRowBuffer(unsigned fieldCount)
    :DataGridRowBuffer(fieldCount)
{
}

InsertedGridRowBuffer::InsertedGridRowBuffer(const InsertedGridRowBuffer* b2)
    :DataGridRowBuffer(b2)
{
    naFieldsM = b2->naFieldsM;
}

bool InsertedGridRowBuffer::isInserted()
{
    return true;
}

bool InsertedGridRowBuffer::isFieldNA(unsigned num)
{
    return (num < naFieldsM.size() && naFieldsM[num]);
}

void InsertedGridRowBuffer::setFieldNA(unsigned num, bool isNA)
{
    if (num < naFieldsM.size())
        naFieldsM[num] = isNA;
    else if (isNA)  // we need to resize and set
    {
        naFieldsM.resize(num + 1, false);
        naFieldsM[num] = true;
    }
    invalidateIsDeletable();
}

