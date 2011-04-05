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

//-----------------------------------------------------------------------------
#include <ibpp.h>

#include "core/FRError.h"
#include "core/StringUtils.h"
#include "engine/MetadataLoader.h"
#include "frutils.h"
#include "metadata/database.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/trigger.h"
//-----------------------------------------------------------------------------
Trigger::Trigger(DatabasePtr database, const wxString& name)
    : MetadataItem(ntTrigger, database.get(), name)
{
}
//-----------------------------------------------------------------------------
void Trigger::getTriggerInfo(wxString& object, bool& active, int& position,
    wxString& type, bool& isDatabaseTrigger)
{
    ensurePropertiesLoaded();
    isDatabaseTrigger = isDatabaseTriggerM;
    object = objectM;
    active = activeM;
    position = positionM;
    type = triggerTypeM;
}
//-----------------------------------------------------------------------------
wxString Trigger::getTriggerRelation()
{
    ensurePropertiesLoaded();
    // gcc 4.4.3 on Ubuntu didn't accept wxEmptyString
    return (isDatabaseTriggerM ? wxT("") : objectM);
}
//-----------------------------------------------------------------------------
void Trigger::loadProperties()
{
    setPropertiesLoaded(false);

    Database* d = getDatabase(wxT("Trigger::loadInfo"));
    MetadataLoader* loader = d->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);

    IBPP::Statement& st1 = loader->getStatement(
        "select t.rdb$relation_name, t.rdb$trigger_sequence, "
        "t.rdb$trigger_inactive, t.rdb$trigger_type "
        "from rdb$triggers t where rdb$trigger_name = ? "
    );

    st1->Set(1, wx2std(getName_(), d->getCharsetConverter()));
    st1->Execute();
    if (st1->Fetch())
    {
        isDatabaseTriggerM = st1->IsNull(1);
        if (!isDatabaseTriggerM)
        {
            std::string objname;
            st1->Get(1, objname);
            objectM = std2wxIdentifier(objname, d->getCharsetConverter());
        }
        st1->Get(2, &positionM);

        short temp;
        if (st1->IsNull(3))
            temp = 0;
        else
            st1->Get(3, &temp);
        activeM = (temp == 0);

        int ttype;
        st1->Get(4, &ttype);
        triggerTypeM = getTriggerType(ttype);
    }
    else    // maybe trigger was dropped?
    {
        //wxMessageBox("Trigger does not exist in database");
        objectM = wxEmptyString;
    }

    setPropertiesLoaded(true);
}
//-----------------------------------------------------------------------------
wxString Trigger::getSource() const
{
    Database* d = getDatabase(wxT("Trigger::getSource"));
    MetadataLoader* loader = d->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);

    IBPP::Statement& st1 = loader->getStatement(
        "select rdb$trigger_source from rdb$triggers where rdb$trigger_name = ?");
    st1->Set(1, wx2std(getName_(), d->getCharsetConverter()));
    st1->Execute();
    st1->Fetch();
    wxString source;
    readBlob(st1, 1, source, d->getCharsetConverter());
    return source;
}
//-----------------------------------------------------------------------------
wxString Trigger::getTriggerType(int type)
{
    if (type >= 8192 && type <= 8196)   // database triggers
    {
        wxString ttype[] = {
            wxT("CONNECT"), wxT("DISCONNECT"), wxT("TRANSACTION START"),
            wxT("TRANSACTION COMMIT"), wxT("TRANSACTION ROLLBACK") };
        return wxString(wxT("ON ")) + ttype[type - 8192];
    }

    // For explanation: read README.universal_triggers file in Firebird's
    //                  doc/sql.extensions directory
    wxString result(type % 2 ? wxT("BEFORE ") : wxT("AFTER "));
    wxString types[] = { wxT("INSERT"), wxT("UPDATE"), wxT("DELETE") };
    type++;         // compensate for decrement
    type >>= 1;     // remove bit 0
    for (int i = 0; i < 3; ++i, type >>= 2)
    {
        if (type % 4)
        {
            if (i)
                result += wxT(" OR ");
            result += types[ (type%4) - 1 ];
        }
    }
    return result;
}
//-----------------------------------------------------------------------------
Trigger::fireTimeType Trigger::getFiringTime()
{
    ensurePropertiesLoaded();
    if (isDatabaseTriggerM)
        return databaseTrigger;
    if (triggerTypeM.substr(0, 6) == wxT("BEFORE"))
        return beforeTrigger;
    else
        return afterTrigger;
}
//-----------------------------------------------------------------------------
wxString Trigger::getAlterSql()
{
    wxString object, type;
    bool active, db;
    int position;

    getTriggerInfo(object, active, position, type, db);
    wxString source = getSource();
    wxString sql;
    sql << wxT("SET TERM ^ ;\nALTER TRIGGER ") << getQuotedName();
    if (active)
        sql << wxT(" ACTIVE\n");
    else
        sql << wxT(" INACTIVE\n");
    sql << type;
    sql << wxT(" POSITION ");
    sql << position << wxT("\n");
    sql << source;
    sql << wxT("^\nSET TERM ; ^");
    return sql;
}
//-----------------------------------------------------------------------------
const wxString Trigger::getTypeName() const
{
    return wxT("TRIGGER");
}
//-----------------------------------------------------------------------------
void Trigger::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitTrigger(*this);
}
//-----------------------------------------------------------------------------
// Triggers collection
Triggers::Triggers(DatabasePtr database)
    : MetadataCollection<Trigger>(ntTriggers, database, _("Triggers"))
{
}
//-----------------------------------------------------------------------------
void Triggers::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitTriggers(*this);
}
//-----------------------------------------------------------------------------
void Triggers::load(ProgressIndicator* progressIndicator)
{
    wxString stmt = wxT("select rdb$trigger_name from rdb$triggers")
        wxT(" where (rdb$system_flag = 0 or rdb$system_flag is null)")
        wxT(" order by 1");
    setItems(getDatabase()->loadIdentifiers(stmt, progressIndicator));
}
//-----------------------------------------------------------------------------
void Triggers::loadChildren()
{
    load(0);
}
//-----------------------------------------------------------------------------
