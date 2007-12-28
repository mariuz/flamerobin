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
//-----------------------------------------------------------------------------
typedef MetadataCollection <Parameter>::const_iterator ParameterCollCIter;
//-----------------------------------------------------------------------------
Procedure::Procedure()
{
    parametersM.setParent(this);
    typeM = ntProcedure;
    parametersLoadedM = false;
}
//-----------------------------------------------------------------------------
Procedure::Procedure(const Procedure& rhs)
    : MetadataItem(rhs), parametersM(rhs.parametersM)
{
    parametersLoadedM = rhs.parametersLoadedM;
    parametersM.setParent(this);
}
//-----------------------------------------------------------------------------
Parameter* Procedure::addParameter(Parameter &c)
{
    if (!parametersLoadedM)
        loadParameters();
    Parameter *cc = parametersM.add(c);
    cc->setParent(this);
    return cc;
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
    if (!parametersLoadedM)
        loadParameters();
    wxString columns, params;
    for (ParameterCollCIter it = parametersM.begin(); it != parametersM.end();
        ++it)
    {
        if ((*it).isOutputParameter())
        {
            if (!columns.empty())
                columns += wxT(", ");
            columns += wxT("p.") + (*it).getQuotedName();
        }
        else
        {
            if (!params.empty())
                params += wxT(", ");
            params += (*it).getQuotedName();
        }
    }

    wxString sql;
    if (!columns.empty())
    {
        sql = wxT("SELECT ") + columns + wxT("\nFROM ")
          + getQuotedName();
        if (!params.empty())
            sql += wxT("(") + params + wxT(")");
        sql += wxT(" p");
    }
    else
    {
        sql = wxT("EXECUTE PROCEDURE ") + getQuotedName();
        if (!params.empty())
            sql += wxT("(") + params + wxT(")");
    }
    return sql;
}
//-----------------------------------------------------------------------------
void Procedure::checkAndLoadParameters(bool force)
{
    if (force || !parametersLoadedM)
        loadParameters();
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
//! returns false if error occurs, and places the error text in error variable
void Procedure::loadParameters()
{
    parametersLoadedM = false;
    parametersM.clear();

    Database* d = getDatabase(wxT("Procedure::loadParameters"));
    MetadataLoader* loader = d->getMetadataLoader();
    // first start a transaction for metadata loading, then lock the procedure
    // when objects go out of scope and are destroyed, procedure will be
    // unlocked before the transaction is committed - any update() calls on
    // observers can possibly use the same transaction
    MetadataLoaderTransaction tr(loader);
    SubjectLocker lock(this);

    IBPP::Statement st1 = loader->getStatement(
        "select p.rdb$parameter_name, p.rdb$field_source, p.rdb$parameter_type"
        " from rdb$procedure_parameters p"
        " where p.rdb$PROCEDURE_name = ? "
        " order by p.rdb$parameter_type, rdb$PARAMETER_number "
    );
    st1->Set(1, wx2std(getName_()));
    st1->Execute();

    while (st1->Fetch())
    {
        std::string column_name, source;
        short partype;
        st1->Get(1, column_name);
        st1->Get(2, source);
        st1->Get(3, &partype);

        Parameter p(std2wx(source), partype);
        p.setName_(std2wx(column_name));
        Parameter* pp = parametersM.add(p);
        pp->setParent(this);
    }

    parametersLoadedM = true;
    notifyObservers();
}
//-----------------------------------------------------------------------------
wxString Procedure::getOwner()
{
    Database* d = getDatabase(wxT("Procedure::getOwner"));
    MetadataLoader* loader = d->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);

    IBPP::Statement st1 = loader->getStatement(
        "select rdb$owner_name from rdb$procedures where rdb$procedure_name = ?");
    st1->Set(1, wx2std(getName_()));
    st1->Execute();
    st1->Fetch();
    std::string name;
    st1->Get(1, name);
    return std2wx(name).Trim();
}
//-----------------------------------------------------------------------------
wxString Procedure::getSource()
{
    Database* d = getDatabase(wxT("Procedure::getSource"));
    MetadataLoader* loader = d->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);

    IBPP::Statement st1 = loader->getStatement(
        "select rdb$procedure_source from rdb$procedures where rdb$procedure_name = ?");
    st1->Set(1, wx2std(getName_()));
    st1->Execute();
    st1->Fetch();
    wxString source;
    readBlob(st1, 1, source);
    source.Trim(false);     // remove leading whitespace
    return source;
}
//-----------------------------------------------------------------------------
wxString Procedure::getDefinition()
{
    checkAndLoadParameters();
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
    if (!parametersLoadedM)
        loadParameters();

    Database* db = getDatabase(wxT("Procedure::getAlterSql"));

    wxString sql = wxT("SET TERM ^ ;\nALTER PROCEDURE ") + getQuotedName();
    if (!parametersM.empty())
    {
        wxString input, output;
        for (MetadataCollection <Parameter>::const_iterator it =
            parametersM.begin(); it != parametersM.end(); ++it)
        {
            wxString charset;
            Domain* dm = (*it).getDomain();
            if (dm)
                charset = dm->getCharset();
            if ((*it).isOutputParameter())
            {
                if (output.empty())
                    output += wxT("\nRETURNS (\n    ");
                else
                    output += wxT(",\n    ");
                output += (*it).getQuotedName() + wxT(" ") +
                    ((dm) ? dm->getDatatypeAsString() : (*it).getSource());
                if (!charset.IsEmpty() && charset != db->getDatabaseCharset())
                    output += wxT(" CHARACTER SET ") + charset;
            }
            else
            {
                if (input.empty())
                    input += wxT(" (\n    ");
                else
                    input += wxT(",\n    ");
                input += (*it).getQuotedName() + wxT(" ") +
                    ((dm) ? dm->getDatatypeAsString() : (*it).getSource());
                if (dm && dm->hasDefault())
                    input += wxT(" DEFAULT ") + dm->getDefault();
                if (!charset.IsEmpty() && charset != db->getDatabaseCharset())
                    input += wxT(" CHARACTER SET ") + charset;
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
        sql += wxT("BEGIN EXIT; END");
    sql += wxT("^\nSET TERM ; ^\n");
    return sql;
}
//-----------------------------------------------------------------------------
void Procedure::checkDependentProcedures()
{
    // check dependencies and parameters
    checkAndLoadParameters();
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
    st1->Set(1, wx2std(getName_()));
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
            Privilege p(this, std2wx(user).Strip(), usertype);
            privilegesM.push_back(p);
            pr = &privilegesM.back();
            lastuser = user;
            lasttype = usertype;
        }
        pr->addPrivilege(privilege[0], std2wx(grantor).Strip(),
            grantoption == 1);
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
void Procedure::loadDescription()
{
    MetadataItem::loadDescription(
        wxT("select RDB$DESCRIPTION from RDB$PROCEDURES ")
        wxT("where RDB$PROCEDURE_NAME = ?"));
}
//-----------------------------------------------------------------------------
void Procedure::saveDescription(wxString description)
{
    MetadataItem::saveDescription(
        wxT("update RDB$PROCEDURES set RDB$DESCRIPTION = ? ")
        wxT("where RDB$PROCEDURE_NAME = ?"),
        description);
}
//-----------------------------------------------------------------------------
void Procedure::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitProcedure(*this);
}
//-----------------------------------------------------------------------------
