/*
  Copyright (c) 2004-2015 The FlameRobin Development Team

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

#include <ibpp.h>

#include "core/FRError.h"
#include "core/StringUtils.h"
#include "engine/MetadataLoader.h"
#include "frutils.h"
#include "metadata/database.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/trigger.h"
#include "sql/StatementBuilder.h"

/* static */
Trigger::FiringTime Trigger::getFiringTime(int type)
{
    if (type == 8192)
        return databaseConnect;
    if (type == 8193)
        return databaseDisconnect;
    if (type == 8194)
        return transactionStart;
    if (type == 8195)
        return transactionCommit;
    if (type == 8196)
        return transactionRollback;
    if (type % 2)
        return beforeIUD;
    if (type)
        return afterIUD;
    return invalid;
}

Trigger::Trigger(DatabasePtr database, const wxString& name)
    : MetadataItem(ntTrigger, database.get(), name)
{
}

bool Trigger::getActive()
{
    ensurePropertiesLoaded();
    return activeM;
}

wxString Trigger::getFiringEvent()
{
    ensurePropertiesLoaded();

    StatementBuilder sb;
    wxString result;
    FiringTime time = getFiringTime(typeM);
    switch (time)
    {
        case databaseConnect:
            sb << kwON << ' ' << kwCONNECT;
            break;
        case databaseDisconnect:
            sb << kwON << ' ' << kwDISCONNECT;
            break;
        case transactionStart:
            sb << kwON << ' ' << kwTRANSACTION << ' ' << kwSTART;
            break;
        case transactionCommit:
            sb << kwON << ' ' << kwTRANSACTION << ' ' << kwCOMMIT;
            break;
        case transactionRollback:
            sb << kwON << ' ' << kwTRANSACTION << ' ' << kwROLLBACK;
            break;
        default:
            break;
    }

    if (time == beforeIUD || time == afterIUD)
    {
        if (time == beforeIUD)
            sb << kwBEFORE;
        else
            sb << kwAFTER;
        sb << ' ';
        // For explanation: read README.universal_triggers file in Firebird's
        //                  doc/sql.extensions directory
        wxString types[] = { SqlTokenizer::getKeyword(kwINSERT),
            SqlTokenizer::getKeyword(kwUPDATE),
            SqlTokenizer::getKeyword(kwDELETE) };
        int type = typeM + 1;    // compensate for decrement
        type >>= 1;              // remove bit 0
        for (int i = 0; i < 3; ++i, type >>= 2)
        {
            if (type % 4)
            {
                if (i)
                    sb << ' ' << kwOR << ' ';
                sb << types[ (type%4) - 1 ];
            }
        }
    }
    return sb;
}

Trigger::FiringTime Trigger::getFiringTime()
{
    ensurePropertiesLoaded();
    return getFiringTime(typeM);
}

int Trigger::getPosition()
{
    ensurePropertiesLoaded();
    return positionM;
}

wxString Trigger::getRelationName()
{
    ensurePropertiesLoaded();
    if (!isDatabaseTrigger())
        return relationNameM;
    return wxEmptyString;
}

wxString Trigger::getSource()
{
    ensurePropertiesLoaded();
    return sourceM;
}

void Trigger::loadProperties()
{
    setPropertiesLoaded(false);

    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);

    IBPP::Statement& st1 = loader->getStatement(
        "select t.rdb$relation_name, t.rdb$trigger_sequence, "
        "t.rdb$trigger_inactive, t.rdb$trigger_type, rdb$trigger_source "
        "from rdb$triggers t where rdb$trigger_name = ? "
    );

    st1->Set(1, wx2std(getName_(), db->getCharsetConverter()));
    st1->Execute();
    if (st1->Fetch())
    {
        if (st1->IsNull(1))
            relationNameM.clear();
        else
        {
            std::string objname;
            st1->Get(1, objname);
            relationNameM = std2wxIdentifier(objname, db->getCharsetConverter());
        }
        st1->Get(2, &positionM);

        short temp;
        if (st1->IsNull(3))
            temp = 0;
        else
            st1->Get(3, &temp);
        activeM = (temp == 0);

        st1->Get(4, &typeM);

        readBlob(st1, 5, sourceM, db->getCharsetConverter());
    }
    else // maybe trigger was dropped?
    {
        relationNameM.clear();
        activeM = false;
        positionM = -1;
        sourceM.clear();
        typeM = 0;
    }

    setPropertiesLoaded(true);
}

wxString Trigger::getAlterSql()
{
    ensurePropertiesLoaded();

    StatementBuilder sb;
    sb << StatementBuilder::DisableLineWrapping;

    sb << kwSET << ' ' << kwTERMINATOR << " ^ ;"
        << StatementBuilder::NewLine;

    sb << kwALTER << ' ' << kwTRIGGER << ' ' << getQuotedName() << ' ';
    if (activeM)
        sb << kwACTIVE;
    else
        sb << kwINACTIVE;
    sb << StatementBuilder::NewLine << getFiringEvent()
        << ' ' << kwPOSITION << ' ' << wxString::Format("%d", positionM)
        << StatementBuilder::NewLine;
    sb << sourceM + "^" << StatementBuilder::NewLine;

    sb << kwSET << ' ' << kwTERMINATOR << " ; ^"
        << StatementBuilder::NewLine;
    return sb;
}

bool Trigger::isDatabaseTrigger()
{
    ensurePropertiesLoaded();
    switch (getFiringTime(typeM))
    {
        case databaseConnect:
        case databaseDisconnect:
            return true;
        default:
            return false;
    }
}

const wxString Trigger::getTypeName() const
{
    return "TRIGGER";
}

void Trigger::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitTrigger(*this);
}

// Triggers collection
Triggers::Triggers(DatabasePtr database)
    : MetadataCollection<Trigger>(ntTriggers, database, _("Triggers"))
{
}

void Triggers::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitTriggers(*this);
}

void Triggers::load(ProgressIndicator* progressIndicator)
{
    wxString stmt = "select rdb$trigger_name from rdb$triggers"
        " where (rdb$system_flag = 0 or rdb$system_flag is null)"
        " order by 1";
    setItems(getDatabase()->loadIdentifiers(stmt, progressIndicator));
}

void Triggers::loadChildren()
{
    load(0);
}

const wxString Triggers::getTypeName() const
{
    return "TRIGGER_COLLECTION";
}

