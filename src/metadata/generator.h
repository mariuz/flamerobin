/*
  Copyright (c) 2004-2009 The FlameRobin Development Team

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

#ifndef FR_GENERATOR_H
#define FR_GENERATOR_H

#include "frtypes.h"
#include "metadata/metadataitem.h"
//-----------------------------------------------------------------------------
class Generator: public MetadataItem
{
private:
    int64_t valueM;
    bool valueLoadedM;
    void setValue(int64_t value);
protected:
    virtual void loadDescription();
    virtual void saveDescription(wxString description);
public:
    Generator();
    // overrides MetadataItem::getCreateSqlTemplate()
    wxString getCreateSqlTemplate() const;

    void loadValue();
    int64_t getValue();

    virtual wxString getPrintableName();
    virtual const wxString getTypeName() const;
    virtual void acceptVisitor(MetadataItemVisitor* visitor);
};
//-----------------------------------------------------------------------------
#endif
