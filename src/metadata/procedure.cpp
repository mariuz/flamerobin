/*
  Copyright (c) 2004-2010 The FlameRobin Development Team

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

// needed for platform independent EOL
#include <wx/textbuf.h>

#include <string>

#include <ibpp.h>

#include "core/StringUtils.h"
#include "core/FRError.h"
#include "engine/MetadataLoader.h"
#include "frutils.h"
#include "gui/AdvancedMessageDialog.h"
#include "metadata/collection.h"
#include "metadata/database.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/procedure.h"
#include "sql/StatementBuilder.h"
//-----------------------------------------------------------------------------
typedef MetadataCollection <Parameter>::const_iterator ParameterCollCIter;
//-----------------------------------------------------------------------------
Procedure::Procedure()
    : MetadataItem(ntProcedure)
{
    parametersM.setParent(this);
}
//-----------------------------------------------------------------------------
Procedure::Procedure(const Procedure& rhs)
    : MetadataItem(rhs), parametersM(rhs.parametersM)
{
    parametersM.setParent(this);
}
//-----------------------------------------------------------------------------
void Procedure::loadChildren()
{
    setChildrenLoaded(false);
    parametersM.clear();

    Database* d = getDatabase(wxT("Procedure::loadChildren"));
    MetadataLoader* loader = d->getMetadataLoader();
    // first start a transaction for metadata loading, then lock the procedure
    // when objects go out of scope and are destroyed, procedure will be
    // unlocked before the transaction is committed - any update() calls on
    // observers can possibly use the same transaction
    MetadataLoaderTransaction tr(loader);
    SubjectLocker lock(this);

    std::string sql(
        "select p.rdb$parameter_name, p.rdb$field_source, p.rdb$parameter_type"
    );
    if (d->getInfo().getODSVersionIsHigherOrEqualTo(11, 1))
        sql += ", RDB$PARAMETER_MECHANISM ";
    else
        sql += ", -1 ";
    sql +=  " from rdb$procedure_parameters p"
            " where p.rdb$PROCEDURE_name = ? "
            " order by p.rdb$parameter_type, rdb$PARAMETER_number ";

    IBPP::Statement st1 = loader->getStatement(sql);
    st1->Set(1, wx2std(getName_(), d->getCharsetConverter()));
    st1->Execute();

    while (st1->Fetch())
    {
        std::string s;
        st1->Get(1, s);
        wxString column_name(std2wxIdentifier(s, d->getCharsetConverter()));
        st1->Get(2, s);
        wxString source(std2wxIdentifier(s, d->getCharsetConverter()));

        short partype, mechanism = -1;
        st1->Get(3, &partype);
        if (!st1->IsNull(4))
            st1->Get(4, mechanism);

        Parameter p(source, partype, mechanism);
        p.setName_(column_name);
        Parameter* pp = parametersM.add(p);
        pp->setParent(this);
    }

    setChildrenLoaded(true);
    notifyObservers();
}
//-----------------------------------------------------------------------------
bool Procedure::getChildren(std::vector<MetadataItem *>& temp)
{
    return parametersM.getChildren(temp);
}
//-----------------------------------------------------------------------------
void Procedure::lockChildren()
{
    parametersM.lockSubject();
}
//-----------------------------------------------------------------------------
void Procedure::unlockChildren()
{
    parametersM.unlockSubject();
}
//-----------------------------------------------------------------------------
wxString Procedure::getExecuteStatement()
{
    ensureChildrenLoaded();

    wxArrayString columns, params;
    columns.Alloc(parametersM.getChildrenCount());
    params.Alloc(parametersM.getChildrenCount());

    for (ParameterCollCIter it = parametersM.begin(); it != parametersM.end();
        ++it)
    {
        if ((*it).isOutputParameter())
            columns.Add((*it).getQuotedName());
        else
            params.Add((*it).getQuotedName());
    }

    StatementBuilder sb;
    if (!columns.empty())
    {
        sb << kwSELECT << ' ' << StatementBuilder::IncIndent;

        // use "<<" only after concatenating everything
        // that shouldn't be split apart in line wrapping calculation
        for (size_t i = 0; i < columns.size() - 1; ++i)
            sb << wxT("p.") + columns[i] + wxT(", ");
        sb << wxT("p.") + columns.Last();

        sb << StatementBuilder::DecIndent << StatementBuilder::NewLine
            << kwFROM << ' ' << getQuotedName();
    }
    else
    {
        sb << kwEXECUTE << ' ' << kwPROCEDURE << ' ' << getQuotedName();
    }

    if (!params.empty())
    {
        sb << wxT(" (") << StatementBuilder::IncIndent;

        // use "<<" only after concatenating everything
        // that shouldn't be split apart in line wrapping calculation
        for (size_t i = 0; i < params.size() - 1; ++i)
            sb << params[i] + wxT(", ");
        sb << params.Last() + wxT(")");

        sb << StatementBuilder::DecIndent;
    }
    sb << wxT(" p");

    return sb;
}
//-----------------------------------------------------------------------------
MetadataCollection<Parameter>::iterator Procedure::begin()
{
    // please - don't load here
    // this code is used to get columns we want to alert about changes
    // but if there aren't any columns, we don't want to waste time
    // loading them
    return parametersM.begin();
}
//-----------------------------------------------------------------------------
MetadataCollection<Parameter>::iterator Procedure::end()
{
    // please see comment for begin()
    return parametersM.end();
}
//-----------------------------------------------------------------------------
MetadataCollection<Parameter>::const_iterator Procedure::begin() const
{
    return parametersM.begin();
}
//-----------------------------------------------------------------------------
MetadataCollection<Parameter>::const_iterator Procedure::end() const
{
    return parametersM.end();
}
//-----------------------------------------------------------------------------
size_t Procedure::getParamCount() const
{
    return parametersM.getChildrenCount();
}
//-----------------------------------------------------------------------------
wxString Procedure::getOwner()
{
    Database* d = getDatabase(wxT("Procedure::getOwner"));
    MetadataLoader* loader = d->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);

    IBPP::Statement st1 = loader->getStatement(
        "select rdb$owner_name from rdb$procedures where rdb$procedure_name = ?");
    st1->Set(1, wx2std(getName_(), d->getCharsetConverter()));
    st1->Execute();
    st1->Fetch();
    std::string name;
    st1->Get(1, name);
    return std2wxIdentifier(name, d->getCharsetConverter());
}
//-----------------------------------------------------------------------------
wxString Procedure::getSource()
{
    Database* d = getDatabase(wxT("Procedure::getSource"));
    MetadataLoader* loader = d->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);

    IBPP::Statement st1 = loader->getStatement(
        "select rdb$procedure_source from rdb$procedures where rdb$procedure_name = ?");
    st1->Set(1, wx2std(getName_(), d->getCharsetConverter()));
    st1->Execute();
    st1->Fetch();
    wxString source;
    readBlob(st1, 1, source, d->getCharsetConverter());
    source.Trim(false);     // remove leading whitespace
    return source;
}
//-----------------------------------------------------------------------------
wxString Procedure::getDefinition()
{
    ensureChildrenLoaded();
    wxString collist, parlist;
    MetadataCollection <Parameter>::const_iterator lastInput, lastOutput;
    for (MetadataCollection <Parameter>::const_iterator it =
        parametersM.begin(); it != parametersM.end(); ++it)
    {
        if ((*it).isOutputParameter())
            lastOutput = it;
        else
            lastInput = it;
    }
    for (MetadataCollection <Parameter>::const_iterator it =
        parametersM.begin(); it != parametersM.end(); ++it)
    {
        // No need to quote domains, as currently only regular datatypes can be
        // used for SP parameters
        if ((*it).isOutputParameter())
        {
            collist += wxT("    ") + (*it).getQuotedName() + wxT(" ")
                + (*it).getDomain()->getDatatypeAsString();
            if (it != lastOutput)
                collist += wxT(",");
            collist += wxT("\n");
        }
        else
        {
            parlist += wxT("    ") + (*it).getQuotedName() + wxT(" ")
                + (*it).getDomain()->getDatatypeAsString();
            if (it != lastInput)
                parlist += wxT(",");
            parlist += wxT("\n");
        }
    }
    wxString retval = getQuotedName();
    if (!parlist.empty())
        retval += wxT("(\n") + parlist + wxT(")");
    retval += wxT("\n");
    if (!collist.empty())
        retval += wxT("returns:\n") + collist;
    return retval;
}
//-----------------------------------------------------------------------------
wxString Procedure::getAlterSql(bool full)
{
    ensureChildrenLoaded();

    Database* db = getDatabase(wxT("Procedure::getAlterSql"));

    wxString sql = wxT("SET TERM ^ ;\nALTER PROCEDURE ") + getQuotedName();
    if (!parametersM.empty())
    {
        wxString input, output;
        for (MetadataCollection <Parameter>::const_iterator it =
            parametersM.begin(); it != parametersM.end(); ++it)
        {
            wxString charset;
            wxString param = (*it).getQuotedName() + wxT(" ");
            wxString collate = (*it).getCollation();
            Domain* dm = (*it).getDomain();
            if (dm)
            {
                if (dm->isSystem()) // autogenerated domain -> use datatype
                {
                    param += dm->getDatatypeAsString();
                    charset = dm->getCharset();
                    if (!charset.IsEmpty())
                    {
                        if (charset != db->getDatabaseCharset())
                            charset = wxT(" CHARACTER SET ") + charset;
                        else
                            charset = wxT("");
                    }
                    if (db->isDefaultCollation(charset, collate))
                        collate.clear();    // don't show default collations
                }
                else
                {
                    if ((*it).getMechanism() == 1)
                        param += wxT("TYPE OF ") + dm->getQuotedName();
                    else
                        param += dm->getQuotedName();
                }
            }
            else
                param += (*it).getSource();

            if (!collate.IsEmpty())
                charset += wxT(" COLLATE ") + collate;

            if ((*it).isOutputParameter())
            {
                if (output.empty())
                    output += wxT("\nRETURNS (\n    ");
                else
                    output += wxT(",\n    ");
                output += param + charset;
            }
            else
            {
                if (input.empty())
                    input += wxT(" (\n    ");
                else
                    input += wxT(",\n    ");
                input += param;
                //if (!(*it).isNullable(false))   // false = don't check domain
                //    input += wxT(" NOT NULL");
                if (dm && dm->hasDefault())
                    input += wxT(" DEFAULT ") + dm->getDefault();
                input += charset;
            }
        }

        if (!input.empty())
            sql += input + wxT(" )");
        if (!output.empty())
            sql += output + wxT(" )");
    }
    sql += wxT("\nAS\n");
    if (full)
        sql += getSource();
    else
        sql += wxT("BEGIN SUSPEND; END");
    sql += wxT("^\nSET TERM ; ^\n");
    return sql;
}
//-----------------------------------------------------------------------------
void Procedure::checkDependentProcedures()
{
    // check dependencies and parameters
    ensureChildrenLoaded();
    std::vector<Dependency> deps;
    getDependencies(deps, false);

    // if there is a dependency, but parameter doesn't exist, warn the user
    int count = 0;
    wxString missing;
    for (std::vector<Dependency>::iterator it = deps.begin();
        it != deps.end(); ++it)
    {
        std::vector<wxString> fields;
        (*it).getFields(fields);
        for (std::vector<wxString>::const_iterator ci = fields.begin();
            ci != fields.end(); ++ci)
        {
            bool found = false;
            for (MetadataCollection<Parameter>::iterator i2 = begin();
                i2 != end(); ++i2)
            {
                if ((*i2).getName_() == (*ci))
                {
                    found = true;
                    break;
                }
            }
            if (!found && ++count < 20)
            {
                missing += wxString::Format(
                    _("Procedure %s depends on parameter %s.%s"),
                    (*it).getName_().c_str(),
                    (*ci).c_str(),
                    wxTextBuffer::GetEOL()
                );
            }
        }
    }
    if (count > 0)
    {
        if (count > 19)
        {
            missing += wxTextBuffer::GetEOL()
            + wxString::Format(_("%d total dependencies (20 shown)."), count);
        }
        showWarningDialog(0,
            _("Dependencies broken"),
            wxString::Format(
                _("Some other procedures depend on %s:%s%s%s"),
                getName_().c_str(),
                wxTextBuffer::GetEOL(),
                wxTextBuffer::GetEOL(),
                missing.c_str()),
            AdvancedMessageDialogButtonsOk()
        );
    }
}
//-----------------------------------------------------------------------------
std::vector<Privilege>* Procedure::getPrivileges()
{
    // load privileges from database and return the pointer to collection
    Database* d = getDatabase(wxT("Procedure::getPrivileges"));
    MetadataLoader* loader = d->getMetadataLoader();
    // first start a transaction for metadata loading, then lock the procedure
    // when objects go out of scope and are destroyed, procedure will be
    // unlocked before the transaction is committed - any update() calls on
    // observers can possibly use the same transaction
    MetadataLoaderTransaction tr(loader);
    SubjectLocker lock(this);

    privilegesM.clear();

    IBPP::Statement st1 = loader->getStatement(
        "select RDB$USER, RDB$USER_TYPE, RDB$GRANTOR, RDB$PRIVILEGE, "
        "RDB$GRANT_OPTION, RDB$FIELD_NAME "
        "from RDB$USER_PRIVILEGES "
        "where RDB$RELATION_NAME = ? and rdb$object_type = 5 "
        "order by rdb$user, rdb$user_type, rdb$privilege"
    );
    st1->Set(1, wx2std(getName_(), d->getCharsetConverter()));
    st1->Execute();
    std::string lastuser;
    int lasttype = -1;
    Privilege *pr = 0;
    while (st1->Fetch())
    {
        std::string user, grantor, privilege, field;
        int usertype, grantoption = 0;
        st1->Get(1, user);
        st1->Get(2, usertype);
        st1->Get(3, grantor);
        st1->Get(4, privilege);
        if (!st1->IsNull(5))
            st1->Get(5, grantoption);
        st1->Get(6, field);
        if (!pr || user != lastuser || usertype != lasttype)
        {
            Privilege p(this, std2wxIdentifier(user, d->getCharsetConverter()),
                usertype);
            privilegesM.push_back(p);
            pr = &privilegesM.back();
            lastuser = user;
            lasttype = usertype;
        }
        pr->addPrivilege(privilege[0],
            std2wx(grantor, d->getCharsetConverter()), grantoption == 1);
    }
    return &privilegesM;
}
//-----------------------------------------------------------------------------
wxString Procedure::getCreateSqlTemplate() const
{
    wxString s(wxT("SET TERM ^ ;\n\n")
            wxT("CREATE PROCEDURE name \n")
            wxT(" ( input_parameter_name < datatype>, ... ) \n")
            wxT("RETURNS \n")
            wxT(" ( output_parameter_name < datatype>, ... )\n")
            wxT("AS \n")
            wxT("DECLARE VARIABLE variable_name < datatype>; \n")
            wxT("BEGIN\n")
            wxT("  /* write your code here */ \n")
            wxT("END^\n\n")
            wxT("SET TERM ; ^\n"));
    return s;
}
//-----------------------------------------------------------------------------
const wxString Procedure::getTypeName() const
{
    return wxT("PROCEDURE");
}
//-----------------------------------------------------------------------------
void Procedure::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitProcedure(*this);
}
//-----------------------------------------------------------------------------
