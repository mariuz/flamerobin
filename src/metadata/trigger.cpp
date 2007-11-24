/*
  Copyright (c) 2004-2007 The FlameRobin Development Team

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
#include "core/Visitor.h"
#include "engine/MetadataLoader.h"
#include "frutils.h"
#include "metadata/database.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/trigger.h"
//-----------------------------------------------------------------------------
Trigger::Trigger()
    : MetadataItem()
{
    typeM = ntTrigger;
    infoIsLoadedM = false;
}
//-----------------------------------------------------------------------------
void Trigger::getTriggerInfo(wxString& object, bool& active, int& position,
    wxString& type)
{
    if (!infoIsLoadedM)
        loadInfo();
    object = objectM;
    active = activeM;
    position = positionM;
    type = triggerTypeM;
}
//-----------------------------------------------------------------------------
wxString Trigger::getRelation()
{
    if (!infoIsLoadedM)
        loadInfo();
    return objectM;
}
//-----------------------------------------------------------------------------
void Trigger::loadInfo(bool force)
{
    infoIsLoadedM = false;
    Database *d = getDatabase();
    if (!d)
        throw FRError(_("database not set"));

    MetadataLoader* loader = d->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);

    IBPP::Statement& st1 = loader->getStatement(
        "select t.rdb$relation_name, t.rdb$trigger_sequence, "
        "t.rdb$trigger_inactive, t.rdb$trigger_type "
        "from rdb$triggers t where rdb$trigger_name = ? "
    );

    st1->Set(1, wx2std(getName_()));
    st1->Execute();
    if (st1->Fetch())
    {
        std::string objectName;
        st1->Get(1, objectName);
        objectM = std2wx(objectName);
        objectM.erase(objectM.find_last_not_of(wxT(" ")) + 1);
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
        infoIsLoadedM = true;
        if (force)
            notifyObservers();
    }
    else    // maybe trigger was dropped?
    {
        //wxMessageBox("Trigger does not exist in database");
        objectM = wxEmptyString;
    }
}
//-----------------------------------------------------------------------------
wxString Trigger::getSource() const
{
    Database* d = getDatabase();
    if (!d)
        throw FRError(_("database not set"));

    MetadataLoader* loader = d->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);

    IBPP::Statement& st1 = loader->getStatement(
        "select rdb$trigger_source from rdb$triggers where rdb$trigger_name = ?");
    st1->Set(1, wx2std(getName_()));
    st1->Execute();
    st1->Fetch();
    wxString source;
    readBlob(st1, 1, source);
    return source;
}
//-----------------------------------------------------------------------------
wxString Trigger::getTriggerType(int type)
{
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
Trigger::firingTimeType Trigger::getFiringTime()
{
    if (!infoIsLoadedM)
        loadInfo();
    if (triggerTypeM.substr(0, 6) == wxT("BEFORE"))
        return beforeTrigger;
    else
        return afterTrigger;
}
//-----------------------------------------------------------------------------
wxString Trigger::getAlterSql()
{
    wxString object, type;
    bool active;
    int position;

    getTriggerInfo(object, active, position, type);
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
wxString Trigger::getCreateSqlTemplate() const
{
    return  wxT("SET TERM ^ ;\n\n")
            wxT("CREATE TRIGGER name FOR table/view \n")
            wxT(" [IN]ACTIVE \n")
            wxT(" {BEFORE | AFTER} INSERT OR UPDATE OR DELETE \n")
            wxT(" POSITION number \n")
            wxT("AS \n")
            wxT("BEGIN \n")
            wxT("    /* enter trigger code here */ \n")
            wxT("END^\n\n")
            wxT("SET TERM ; ^\n");
}
//-----------------------------------------------------------------------------
const wxString Trigger::getTypeName() const
{
    return wxT("TRIGGER");
}
//-----------------------------------------------------------------------------
void Trigger::loadDescription()
{
    MetadataItem::loadDescription(
        wxT("select RDB$DESCRIPTION from RDB$TRIGGERS ")
        wxT("where RDB$TRIGGER_NAME = ?"));
}
//-----------------------------------------------------------------------------
void Trigger::saveDescription(wxString description)
{
    MetadataItem::saveDescription(
        wxT("update RDB$TRIGGERS set RDB$DESCRIPTION = ? ")
        wxT("where RDB$TRIGGER_NAME = ?"),
        description);
}
//-----------------------------------------------------------------------------
void Trigger::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitTrigger(*this);
}
//-----------------------------------------------------------------------------
