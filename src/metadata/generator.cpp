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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

//-----------------------------------------------------------------------------
#include <sstream>

#include <ibpp.h>

#include "core/FRError.h"
#include "core/StringUtils.h"
#include "engine/MetadataLoader.h"
#include "metadata/database.h"
#include "metadata/generator.h"
#include "metadata/MetadataItemVisitor.h"
//-----------------------------------------------------------------------------
Generator::Generator()
    : MetadataItem()
{
    typeM = ntGenerator;
    valueLoadedM = false;
}
//-----------------------------------------------------------------------------
int64_t Generator::getValue()
{
    loadValue();
    return valueM;
}
//-----------------------------------------------------------------------------
void Generator::setValue(int64_t value)
{
    if (!valueLoadedM || valueM != value)
    {
        valueM = value;
        valueLoadedM = true;
        notifyObservers();
    }
}
//-----------------------------------------------------------------------------
void Generator::loadValue()
{
    Database* d = getDatabase(wxT("Generator::loadValue"));
    MetadataLoader* loader = d->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);

    // do not use cached statements, because this can not be reused
    IBPP::Statement st1 = loader->createStatement(
        "select gen_id(" + wx2std(getQuotedName()) + ", 0) from rdb$database");
    st1->Execute();
    st1->Fetch();
    int64_t value;
    st1->Get(1, &value);
    setValue(value);
}
//-----------------------------------------------------------------------------
wxString Generator::getPrintableName()
{
    if (!valueLoadedM)
        return getName_();

    std::ostringstream ss;
    ss << wx2std(getName_()) << " = " << valueM;
    return std2wx(ss.str());
}
//-----------------------------------------------------------------------------
wxString Generator::getCreateSqlTemplate() const
{
    return  wxT("CREATE GENERATOR name;\n")
            wxT("SET GENERATOR name TO value;\n");
}
//-----------------------------------------------------------------------------
const wxString Generator::getTypeName() const
{
    return wxT("GENERATOR");
}
//-----------------------------------------------------------------------------
void Generator::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitGenerator(*this);
}
//-----------------------------------------------------------------------------
