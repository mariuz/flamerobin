/*
  Copyright (c) 2004-2011 The FlameRobin Development Team

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

//-----------------------------------------------------------------------------
#ifndef FR_DOMAIN_H
#define FR_DOMAIN_H

#include "metadata/collection.h"

class ProgressIndicator;
//-----------------------------------------------------------------------------
class Domain: public MetadataItem
{
private:
    short datatypeM, subtypeM, lengthM, precisionM, scaleM;
    bool isNotNullM, hasDefaultM;
    wxString charsetM, defaultM, collationM, checkM;
protected:
    virtual void loadProperties();
public:
    Domain(DatabasePtr database, const wxString& name);

    static wxString dataTypeToString(short datatype, short scale,
        short precision, short subtype, short length);

    void getDatatypeParts(wxString& type, wxString& size, wxString& scale);
    wxString getDatatypeAsString();
    wxString getDefault();
    wxString getCollation();
    wxString getCheckConstraint();
    wxString getCharset();
    bool isNullable();
    bool hasDefault();
    bool isString();
    bool isSystem() const;
    wxString getAlterSqlTemplate() const;
    virtual const wxString getTypeName() const;
    virtual void acceptVisitor(MetadataItemVisitor* v);
};
//-----------------------------------------------------------------------------
class Domains: public MetadataCollection<Domain>
{
protected:
    virtual void loadChildren();
public:
    Domains(DatabasePtr database);

    virtual void acceptVisitor(MetadataItemVisitor* visitor);
    void load(ProgressIndicator* progressIndicator);
    virtual const wxString getTypeName() const;
};
//-----------------------------------------------------------------------------
class SysDomains: public MetadataCollection<Domain>
{
public:
    SysDomains(DatabasePtr database);

    virtual void acceptVisitor(MetadataItemVisitor* visitor);
    virtual const wxString getTypeName() const;
};
//-----------------------------------------------------------------------------
#endif
