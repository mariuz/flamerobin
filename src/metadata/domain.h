/*
  Copyright (c) 2004-2008 The FlameRobin Development Team

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

#include "metadata/metadataitem.h"
//-----------------------------------------------------------------------------
class Domain: public MetadataItem
{
private:
    short datatypeM, subtypeM, lengthM, precisionM, scaleM;
    bool isNotNullM, hasDefaultM;
    wxString charsetM, defaultM, collationM, checkM;
    bool infoLoadedM;
protected:
    virtual void loadDescription();
    virtual void saveDescription(wxString description);
public:
    Domain();
    void loadInfo();

    static wxString datatype2string(short datatype, short scale,
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
    virtual const wxString getTypeName() const;
    virtual wxString getCreateSqlTemplate() const;
    wxString getAlterSqlTemplate() const;
    virtual wxString getPrintableName();
    virtual void acceptVisitor(MetadataItemVisitor* v);
};
//-----------------------------------------------------------------------------
#endif
