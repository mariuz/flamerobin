/*
  Copyright (c) 2004-2022 The FlameRobin Development Team

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
#include "firebird/constants.h"


/* static */
Trigger::FiringTime Trigger::getFiringTime(int type)
{
    switch (type & TRIGGER_TYPE_MASK)
    {
    case TRIGGER_TYPE_DML:
    {
        if (((type + 1) & 1) == 0)
            return beforeIUD;
        else
            return afterIUD;
    }
/*    case TRIGGER_TYPE_DB:
        switch (type & ~TRIGGER_TYPE_DB)
        {
        case DB_TRIGGER_CONNECT:
            return DB_TRIGGER_CONNECT;
        case DB_TRIGGER_DISCONNECT:
            return DB_TRIGGER_DISCONNECT;
        case DB_TRIGGER_TRANS_START:
            return DB_TRIGGER_TRANS_START;
        case DB_TRIGGER_TRANS_COMMIT:
            return DB_TRIGGER_TRANS_COMMIT;
        case DB_TRIGGER_TRANS_ROLLBACK:
            return DB_TRIGGER_TRANS_ROLLBACK;
        default:
            return invalid;
        }
    case TRIGGER_TYPE_DDL:
        return TRIGGER_TYPE_DDL;*/
    default:
       return invalid;
    }        
}

Trigger::Trigger(NodeType type, DatabasePtr database, const wxString& name)
    : MetadataItem(type, database.get(), name), activeM(true)
{
}

void Trigger::setActive(bool active)
{
    activeM = active;
}

bool Trigger::getActive()
{
    //ensurePropertiesLoaded();
    return activeM;
}

bool Trigger::isActive()
{
    return getActive();
}

wxString Trigger::getFiringEvent()
{
    ensurePropertiesLoaded();

    StatementBuilder sb;
    wxString result;
    FiringTime time = getFiringTime(typeM);
    if ((typeM & TRIGGER_TYPE_MASK) == TRIGGER_TYPE_DB) {
        switch (typeM & ~TRIGGER_TYPE_DB)
        {
        case DB_TRIGGER_CONNECT:
            sb << kwON << ' ' << kwCONNECT;
            break;
        case DB_TRIGGER_DISCONNECT:
            sb << kwON << ' ' << kwDISCONNECT;
            break;
        case DB_TRIGGER_TRANS_START:
            sb << kwON << ' ' << kwTRANSACTION << ' ' << kwSTART;
            break;
        case DB_TRIGGER_TRANS_COMMIT:
            sb << kwON << ' ' << kwTRANSACTION << ' ' << kwCOMMIT;
            break;
        case DB_TRIGGER_TRANS_ROLLBACK:
            sb << kwON << ' ' << kwTRANSACTION << ' ' << kwROLLBACK;
            break;
        default:
            break;
        }
    }
    if (((typeM & TRIGGER_TYPE_MASK) == TRIGGER_TYPE_DML)) {
        if (time == beforeIUD || time == afterIUD)
        {
            if (((typeM + 1) & 1) == 0)
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
                    sb << types[(type % 4) - 1];
                }
            }
        }
    }
    if (((typeM & TRIGGER_TYPE_MASK) == TRIGGER_TYPE_DDL)) {
        bool first = true;
        if ((typeM & 1) == DDL_TRIGGER_BEFORE)
            sb << kwBEFORE;
        else
            sb << kwAFTER;
        if ((typeM & DDL_TRIGGER_ANY) == DDL_TRIGGER_ANY)
            sb << " ANY DDL STATEMENT ";
        else
        {
            for (SINT64 pos = 1; pos < 64; ++pos)
            {
                if (((1LL << pos) & TRIGGER_TYPE_MASK) || (typeM & (1LL << pos)) == 0)
                    continue;

                if (first)
                    first = false;
                else
                    sb << " OR";

                sb << " ";

                if (pos < FB_NELEM(DDL_TRIGGER_ACTION_NAMES))
                {
                    sb << wxString(DDL_TRIGGER_ACTION_NAMES[pos][0]) + " " +
                        DDL_TRIGGER_ACTION_NAMES[pos][1];
                }
                else
                    sb << "<unknown>";
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
    if (isDMLTrigger())
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
    sourceM.clear();

    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);
    wxMBConv* converter = db->getCharsetConverter();

	std::string sql("select t.rdb$relation_name, t.rdb$trigger_sequence, "
		"t.rdb$trigger_inactive, t.rdb$trigger_type, rdb$trigger_source, "
	);
    sql += db->getInfo().getODSVersionIsHigherOrEqualTo(12, 0) ? " rdb$entrypoint, rdb$engine_name,  ": " null, null, ";
    sql += db->getInfo().getODSVersionIsHigherOrEqualTo(13, 0) ? " rdb$sql_security " : " null ";

    sql += "from rdb$triggers t where rdb$trigger_name = ? ";

    IBPP::Statement& st1 = loader->getStatement(sql);

    st1->Set(1, wx2std(getName_(), converter));
    st1->Execute();
    if (st1->Fetch())
    {
        if (st1->IsNull(1))
            relationNameM.clear();
        else
        {
            std::string objname;
            st1->Get(1, objname);
            relationNameM = std2wxIdentifier(objname, converter);
        }
        st1->Get(2, &positionM);

        short temp;
        if (st1->IsNull(3))
            temp = 0;
        else
            st1->Get(3, &temp);
        activeM = (temp == 0);

        st1->Get(4, &typeM);
        st1->Get(4, typeM);


        if (!st1->IsNull(8))
        {
            bool b;
            st1->Get(8, b);
            sqlSecurityM = b ? "SQL SECURITY DEFINER" : "SQL SECURITY INVOKER";
        }
        else
            sqlSecurityM.clear();

		if (!st1->IsNull(6))
		{
			std::string s;
			st1->Get(6, s);
			sourceM += "EXTERNAL NAME '" + std2wxIdentifier(s, converter) + "'\n";
			entryPointM = std2wxIdentifier(s, converter);
            if (!st1->IsNull(7))
            {
                //std::string s;
                st1->Get(7, s);
                sourceM += "ENGINE " + std2wxIdentifier(s, converter) + "\n";
                engineNameM = std2wxIdentifier(s, converter);
            }
            else
                engineNameM.clear();
		}
        else
        {
            entryPointM.clear();
            engineNameM.clear();
        }
        if (!st1->IsNull(5))
        {
            wxString source1;
            readBlob(st1, 5, source1, converter);
            source1.Trim(false);     // remove leading whitespace
            sourceM += "\n" + source1 + "\n";
        }
    }
    else // maybe trigger was dropped?
    {
        relationNameM.clear();
        activeM = false;
        positionM = -1;
        sourceM.clear();
        typeM = 0;
		entryPointM.clear();
		engineNameM.clear();
        sqlSecurityM.clear();
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
    if (this->getRelationName().IsEmpty())  //TODO: Get better info and improve for DDL triggers
        sb << kwCREATE << ' ' << kwOR << ' ';
    sb << kwALTER << ' ' << kwTRIGGER << ' ' << getQuotedName() << ' ';
    if (activeM)
        sb << kwACTIVE;
    else
        sb << kwINACTIVE;
    sb << StatementBuilder::NewLine << getFiringEvent()
        << ' ' << kwPOSITION << ' ' << wxString::Format("%d", positionM)
        << StatementBuilder::NewLine;
    sb << getSqlSecurity() << StatementBuilder::NewLine;

    sb << sourceM + "^" << StatementBuilder::NewLine;

    sb << kwSET << ' ' << kwTERMINATOR << " ; ^"
        << StatementBuilder::NewLine;
    return sb;
}

bool Trigger::isDBTrigger()
{
    ensurePropertiesLoaded();
    return (typeM & TRIGGER_TYPE_MASK) == TRIGGER_TYPE_DB;
}

bool Trigger::isDDLTrigger()
{
    ensurePropertiesLoaded();
    return (typeM & TRIGGER_TYPE_MASK) == TRIGGER_TYPE_DDL;
}

bool Trigger::isDMLTrigger()
{
    ensurePropertiesLoaded();
    return (typeM & TRIGGER_TYPE_MASK) == TRIGGER_TYPE_DML;
}

wxString Trigger::getSqlSecurity()
{
    ensurePropertiesLoaded();
    return sqlSecurityM;
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
DMLTriggers::DMLTriggers(DatabasePtr database)
    : MetadataCollection<DMLTrigger>(ntDMLTriggers, database, _("Triggers"))
{
}

void DMLTriggers::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitDMLTriggers(*this);
}

void DMLTriggers::load(ProgressIndicator* progressIndicator)
{
    wxString stmt = "select rdb$trigger_name from rdb$triggers"
        " where (rdb$system_flag = 0 or rdb$system_flag is null) "
        " and rdb$relation_name is not null  ";
    if (getDatabase()->getInfo().getODSVersionIsHigherOrEqualTo(12, 0))
        stmt += "and BIN_AND(rdb$trigger_type,"+ std::to_string(TRIGGER_TYPE_MASK)+") = "+ std::to_string(TRIGGER_TYPE_DML);
    stmt += " order by 1";

    setItems(getDatabase()->loadIdentifiers(stmt, progressIndicator));

    stmt = "select rdb$trigger_name from rdb$triggers"
        " where (rdb$system_flag = 0 or rdb$system_flag is null)  and rdb$trigger_inactive = 1"
        " and rdb$relation_name is not null  ";
    if (getDatabase()->getInfo().getODSVersionIsHigherOrEqualTo(12, 0))
        stmt += "and BIN_AND(rdb$trigger_type," + std::to_string(TRIGGER_TYPE_MASK) + ") = " + std::to_string(TRIGGER_TYPE_DML);
    stmt += " order by 1";

    setInactiveItems(getDatabase()->loadIdentifiers(stmt, progressIndicator));
}

void DMLTriggers::loadChildren()
{
    load(0);
}

const wxString DMLTriggers::getTypeName() const
{
    return "TRIGGER_COLLECTION";
}

// DB Triggers collection
DBTriggers::DBTriggers(DatabasePtr database)
    : MetadataCollection<DBTrigger>(ntDBTriggers, database, _("Database Triggers"))
{
}

void DBTriggers::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitDBTriggers(*this);
}

void DBTriggers::load(ProgressIndicator* progressIndicator)
{
    wxString stmt = "select rdb$trigger_name from rdb$triggers"
        " where (rdb$system_flag = 0 or rdb$system_flag is null) "
        " and BIN_AND(rdb$trigger_type," + std::to_string(TRIGGER_TYPE_MASK) + ") = " + std::to_string(TRIGGER_TYPE_DB)+
        " order by 1";
    setItems(getDatabase()->loadIdentifiers(stmt, progressIndicator)); 

    stmt = "select rdb$trigger_name from rdb$triggers"
        " where (rdb$system_flag = 0 or rdb$system_flag is null) and rdb$trigger_inactive = 1"
        " and BIN_AND(rdb$trigger_type," + std::to_string(TRIGGER_TYPE_MASK) + ") = " + std::to_string(TRIGGER_TYPE_DB) +
        " order by 1";

    setInactiveItems(getDatabase()->loadIdentifiers(stmt, progressIndicator));

}



void DBTriggers::loadChildren()
{
    load(0);
}

const wxString DBTriggers::getTypeName() const
{
    return "DBTRIGGER_COLLECTION";
}


// DDL Triggers collection
DDLTriggers::DDLTriggers(DatabasePtr database)
    : MetadataCollection<DDLTrigger>(ntDDLTriggers, database, _("DDL Triggers"))
{
}

void DDLTriggers::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitDDLTriggers(*this);
}

void DDLTriggers::load(ProgressIndicator* progressIndicator)
{
    wxString stmt = "select rdb$trigger_name from rdb$triggers"
        " where (rdb$system_flag = 0 or rdb$system_flag is null) "
        " and BIN_AND(rdb$trigger_type," + std::to_string(TRIGGER_TYPE_MASK) + ") = " + std::to_string(TRIGGER_TYPE_DDL) +
        " order by 1";
    setItems(getDatabase()->loadIdentifiers(stmt, progressIndicator));

    stmt = "select rdb$trigger_name from rdb$triggers"
        " where (rdb$system_flag = 0 or rdb$system_flag is null) and rdb$trigger_inactive = 1 "
        " and BIN_AND(rdb$trigger_type," + std::to_string(TRIGGER_TYPE_MASK) + ") = " + std::to_string(TRIGGER_TYPE_DDL) +
        " order by 1";

    setInactiveItems(getDatabase()->loadIdentifiers(stmt, progressIndicator));
}

void DDLTriggers::loadChildren()
{
    load(0);
}

const wxString DDLTriggers::getTypeName() const
{
    return "DDLTRIGGER_COLLECTION";
}

DDLTrigger::DDLTrigger(DatabasePtr dataBase, const wxString& name)
    :Trigger(ntDDLTrigger, dataBase, name)
{

}

void DDLTrigger::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitDDLTrigger(*this);
}

DBTrigger::DBTrigger(DatabasePtr dataBase, const wxString& name)
    :Trigger(ntDBTrigger, dataBase, name)
{
}

void DBTrigger::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitDBTrigger(*this);
}

DMLTrigger::DMLTrigger(DatabasePtr dataBase, const wxString& name)
    :Trigger(ntDMLTrigger, dataBase, name)
{

}

void DMLTrigger::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitDMLTrigger(*this);

}
