/*
  The contents of this file are subject to the Initial Developer's Public
  License Version 1.0 (the "License"); you may not use this file except in
  compliance with the License. You may obtain a copy of the License here:
  http://www.flamerobin.org/license.html.

  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.

  The Original Code is FlameRobin (TM).

  The Initial Developer of the Original Code is Milan Babuskov.

  Portions created by the original developer
  are Copyright (C) 2004 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s): Michael Hieke
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

//-----------------------------------------------------------------------------
#include <sstream>

#include <ibpp.h>

#include "core/Visitor.h"
#include "database.h"
#include "dberror.h"
#include "generator.h"
#include "metadataitem.h"
//-----------------------------------------------------------------------------
Generator::Generator():
    MetadataItem()
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
bool Generator::loadValue(bool force)
{
    if (!force && valueLoadedM)
        return true;

    Database *d = getDatabase();
    if (!d)
    {
        lastError().setMessage("Database not set.");
        return false;
    }

    IBPP::Database& db = d->getIBPPDatabase();

    try
    {
        IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
        tr1->Start();
        IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
        st1->Prepare("select gen_id(" + getName() + ", 0) from rdb$database");
        st1->Execute();
        st1->Fetch();
        int64_t value;
        st1->Get(1, &value);
        tr1->Commit();
        setValue(value);
        return true;
    }
    catch (IBPP::Exception &e)
    {
        lastError().setMessage(e.ErrorMessage());
    }
    catch (...)
    {
        lastError().setMessage("System error.");
    }
    return false;
}
//-----------------------------------------------------------------------------
std::string Generator::getPrintableName()
{
    if (!valueLoadedM)
        return getName();

    std::ostringstream s;
    s << getName() << " = " << valueM;
    return s.str();
}
//-----------------------------------------------------------------------------
std::string Generator::getCreateSqlTemplate() const
{
    return  "CREATE GENERATOR name;\n"
            "SET GENERATOR name TO value;\n";
}
//-----------------------------------------------------------------------------
const std::string Generator::getTypeName() const
{
    return "GENERATOR";
}
//-----------------------------------------------------------------------------
void Generator::accept(Visitor *v)
{
    v->visit(*this);
}
//-----------------------------------------------------------------------------
