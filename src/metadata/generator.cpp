/*
  Copyright (c) 2004-2013 The FlameRobin Development Team

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

//-----------------------------------------------------------------------------
#include <ibpp.h>

#include "core/FRError.h"
#include "core/StringUtils.h"
#include "engine/MetadataLoader.h"
#include "metadata/database.h"
#include "metadata/generator.h"
#include "metadata/MetadataItemVisitor.h"
#include "sql/StatementBuilder.h"
//-----------------------------------------------------------------------------
Generator::Generator(DatabasePtr database, const wxString& name)
    : MetadataItem(ntGenerator, database.get(), name)
{
}
//-----------------------------------------------------------------------------
int64_t Generator::getValue()
{
    ensurePropertiesLoaded();
    return valueM;
}
//-----------------------------------------------------------------------------
void Generator::loadProperties()
{
    setPropertiesLoaded(false);

    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);

    // IMPORTANT: for all other loading where the name of the db object is
    // Set() into a parameter getName_() is used, but for dynamically
    // building the SQL statement getQuotedName() must be used!
    std::string sqlName(wx2std(getQuotedName(), db->getCharsetConverter()));
    // do not use cached statements, because this can not be reused
    IBPP::Statement st1 = loader->createStatement(
        "select gen_id(" + sqlName + ", 0) from rdb$database");

    st1->Execute();
    st1->Fetch();
    st1->Get(1, &valueM);

    setPropertiesLoaded(true);
    notifyObservers();
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
// Generators collection
Generators::Generators(DatabasePtr database)
    : MetadataCollection<Generator>(ntGenerators, database, _("Generators"))
{
}
//-----------------------------------------------------------------------------
void Generators::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitGenerators(*this);
}
//-----------------------------------------------------------------------------
void Generators::load(ProgressIndicator* progressIndicator)
{
    wxString stmt = wxT("select rdb$generator_name from rdb$generators")
        wxT(" where (rdb$system_flag = 0 or rdb$system_flag is null)")
        wxT(" order by 1");
    setItems(getDatabase()->loadIdentifiers(stmt, progressIndicator));
}
//-----------------------------------------------------------------------------
void Generators::loadChildren()
{
    load(0);
}
//-----------------------------------------------------------------------------
const wxString Generators::getTypeName() const
{
    return wxT("GENERATOR_COLLECTION");
}
//-----------------------------------------------------------------------------
