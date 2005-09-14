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
  The function getTriggerType is adapted from Firebird projects isql code

  Portions created by the original developer
  are Copyright (C) 2004 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s): Nando Dessena
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

#include "core/Visitor.h"
#include "database.h"
#include "dberror.h"
#include "frutils.h"
#include "metadataitem.h"
#include "MetadataItemVisitor.h"
#include "trigger.h"
#include "ugly.h"
//-----------------------------------------------------------------------------
Trigger::Trigger()
    : MetadataItem()
{
    typeM = ntTrigger;
    infoIsLoadedM = false;
}
//-----------------------------------------------------------------------------
bool Trigger::getTriggerInfo(wxString& object, bool& active, int& position, wxString& type)
{
    if (!infoIsLoadedM && !loadInfo())
        return false;
    object = objectM;
    active = activeM;
    position = positionM;
    type = triggerTypeM;
    return true;
}
//-----------------------------------------------------------------------------
bool Trigger::getRelation(wxString& relation)
{
    if (!infoIsLoadedM)
        if (!loadInfo())
            return false;
    relation = objectM;
    return true;
}
//-----------------------------------------------------------------------------
bool Trigger::loadInfo(bool force)
{
    infoIsLoadedM = false;
    Database *d = getDatabase();
    if (!d)
    {
        lastError().setMessage(wxT("database not set"));
        return false;
    }

    IBPP::Database& db = d->getIBPPDatabase();

    try
    {
        IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
        tr1->Start();
        IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
        st1->Prepare(
            "select t.rdb$relation_name, t.rdb$trigger_sequence, t.rdb$trigger_inactive, t.rdb$trigger_type "
            "from rdb$triggers t where rdb$trigger_name = ? "
        );

        st1->Set(1, wx2std(getName()));
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
            tr1->Commit();
            infoIsLoadedM = true;
            if (force)
                notifyObservers();
        }
        else    // maybe trigger was dropped?
        {
            //wxMessageBox("Trigger does not exist in database");
            return false;
        }
        return true;
    }
    catch (IBPP::Exception &e)
    {
        lastError().setMessage(std2wx(e.ErrorMessage()));
    }
    catch (...)
    {
        lastError().setMessage(_("System error."));
    }

    return false;
}
//-----------------------------------------------------------------------------
bool Trigger::getSource(wxString& source) const
{
    Database* d = getDatabase();
    if (!d)
    {
        lastError().setMessage(wxT("database not set"));
        return false;
    }

    IBPP::Database& db = d->getIBPPDatabase();

    try
    {
        IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
        tr1->Start();
        IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
        st1->Prepare("select rdb$trigger_source from rdb$triggers where rdb$trigger_name = ?");
        st1->Set(1, wx2std(getName()));
        st1->Execute();
        st1->Fetch();
        readBlob(st1, 1, source);
        tr1->Commit();
        return true;
    }
    catch (IBPP::Exception &e)
    {
        lastError().setMessage(std2wx(e.ErrorMessage()));
    }
    catch (...)
    {
        lastError().setMessage(_("System error."));
    }

    return false;
}
//-----------------------------------------------------------------------------
wxString Trigger::getTriggerType(int type)
{
    wxString res;

    std::vector<wxString> prefix_types, suffix_types;
    prefix_types.push_back(wxT("BEFORE"));
    prefix_types.push_back(wxT("AFTER"));

    suffix_types.push_back(wxT(""));
    suffix_types.push_back(wxT("INSERT"));
    suffix_types.push_back(wxT("UPDATE"));
    suffix_types.push_back(wxT("DELETE"));

    #define TRIGGER_ACTION_PREFIX(value) ((value + 1) & 1)
    #define TRIGGER_ACTION_SUFFIX(value, slot) (((value + 1) >> (slot * 2 - 1)) & 3)

    wxString result;
    int prefix = TRIGGER_ACTION_PREFIX(type);
    result = prefix_types[prefix];

    int suffix = TRIGGER_ACTION_SUFFIX(type, 1);
    result += wxT(" ") + suffix_types[suffix];
    suffix = TRIGGER_ACTION_SUFFIX(type, 2);
    if (suffix)
        result += wxT(" OR ") + suffix_types[suffix];
    suffix = TRIGGER_ACTION_SUFFIX(type, 3);
    if (suffix)
        result += wxT(" OR ") + suffix_types[suffix];
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
    wxString object, source, type;
    bool active;
    int position;

    if (!getTriggerInfo(object, active, position, type) || !getSource(source))
        return lastError().getMessage();

    wxString sql;
    sql << wxT("SET TERM ^ ;\nALTER TRIGGER ") << getName();
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
void Trigger::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visit(*this);
}
//-----------------------------------------------------------------------------
