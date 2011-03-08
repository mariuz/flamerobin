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

#include <ibpp.h>

#include "core/FRError.h"
#include "core/StringUtils.h"
#include "engine/MetadataLoader.h"
#include "metadata/database.h"
#include "metadata/exception.h"
#include "metadata/MetadataItemVisitor.h"
#include "sql/StatementBuilder.h"
//-----------------------------------------------------------------------------
Exception::Exception(DatabasePtr database, const wxString& name)
    : MetadataItem(ntException, database.get(), name), numberM(0)
{
}
//-----------------------------------------------------------------------------
wxString Exception::getMessage()
{
    ensurePropertiesLoaded();
    return messageM;
}
//-----------------------------------------------------------------------------
int Exception::getNumber()
{
    ensurePropertiesLoaded();
    return numberM;
}
//-----------------------------------------------------------------------------
void Exception::loadProperties()
{
    setPropertiesLoaded(false);

    Database* d = getDatabase(wxT("Exception::loadProperties"));
    MetadataLoader* loader = d->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);

    IBPP::Statement& st1 = loader->getStatement(
        "select RDB$MESSAGE, RDB$EXCEPTION_NUMBER from RDB$EXCEPTIONS"
        " where RDB$EXCEPTION_NAME = ?");
    st1->Set(1, wx2std(getName_(), d->getCharsetConverter()));
    st1->Execute();
    st1->Fetch();
    std::string message;
    st1->Get(1, message);
    messageM = std2wx(message);
    st1->Get(2, numberM);

    setPropertiesLoaded(true);
}
//-----------------------------------------------------------------------------
wxString Exception::getAlterSql()
{
    wxString message = getMessage();
    message.Replace(wxT("'"), wxT("''"));

    StatementBuilder sb;
    sb << kwALTER << ' ' << kwEXCEPTION << ' ' << getQuotedName() << wxT(" '")
        << message << wxT("';");
    return sb;
}
//-----------------------------------------------------------------------------
const wxString Exception::getTypeName() const
{
    return wxT("EXCEPTION");
}
//-----------------------------------------------------------------------------
void Exception::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitException(*this);
}
//-----------------------------------------------------------------------------
// Exceptions collection
Exceptions::Exceptions(DatabasePtr database)
    : MetadataCollection<Exception>(ntExceptions, database, _("Exceptions"))
{
}
//-----------------------------------------------------------------------------
void Exceptions::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitExceptions(*this);
}
//-----------------------------------------------------------------------------
void Exceptions::load(ProgressIndicator* progressIndicator)
{
    wxString stmt = wxT("select rdb$exception_name from rdb$exceptions")
        wxT(" order by 1");
    setItems(getDatabase()->loadIdentifiers(stmt, progressIndicator));
}
//-----------------------------------------------------------------------------
void Exceptions::loadChildren()
{
    load(0);
}
//-----------------------------------------------------------------------------
