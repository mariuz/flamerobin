/*
  Copyright (c) 2004-2012 The FlameRobin Development Team

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
//-----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "metadata/MetadataItemCreateStatementVisitor.h"
#include "sql/StatementBuilder.h"
//-----------------------------------------------------------------------------
/*static*/
wxString MetadataItemCreateStatementVisitor::getCreateDomainStatement()
{
    return  wxT("CREATE DOMAIN domain_name\n")
            wxT("AS datatype [CHARACTER SET charset]\n")
            wxT("DEFAULT {literal | NULL | USER}\n")
            wxT("[NOT NULL]\n")
            wxT("[CHECK (dom_search_condition)]\n")
            wxT("COLLATE collation;\n");
}
//-----------------------------------------------------------------------------
/*static*/
wxString MetadataItemCreateStatementVisitor::getCreateExceptionStatement()
{
    StatementBuilder sb;
    sb << kwCREATE << ' ' << kwEXCEPTION << wxT(" name 'exception message';")
        << StatementBuilder::NewLine;
    return sb;
}
//-----------------------------------------------------------------------------
/*static*/
wxString MetadataItemCreateStatementVisitor::getCreateFunctionStatement()
{
    return wxT("DECLARE EXTERNAL FUNCTION name [datatype | CSTRING (int) ")
           wxT("[, datatype | CSTRING (int) ...]]\n")
           wxT("RETURNS {datatype [BY VALUE] | CSTRING (int)} [FREE_IT]\n")
           wxT("ENTRY_POINT 'entryname'\n")
           wxT("MODULE_NAME 'modulename';\n");
}
//-----------------------------------------------------------------------------
/*static*/
wxString MetadataItemCreateStatementVisitor::getCreateGeneratorStatement()
{
    StatementBuilder sb;
    sb << kwCREATE << ' ' << kwGENERATOR << wxT(" name;")
        << StatementBuilder::NewLine
        << kwSET << ' ' << kwGENERATOR << wxT(" name ") << kwTO
        << wxT(" value;") << StatementBuilder::NewLine;
    return sb;
}
//-----------------------------------------------------------------------------
/*static*/
wxString MetadataItemCreateStatementVisitor::getCreateProcedureStatement()
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
/*static*/
wxString MetadataItemCreateStatementVisitor::getCreateRoleStatement()
{
    return  wxT("CREATE ROLE role_name;\n");
}
//-----------------------------------------------------------------------------
/*static*/
wxString MetadataItemCreateStatementVisitor::getCreateTableStatement()
{
    return wxT("CREATE TABLE table_name\n")
        wxT("(\n")
        wxT("    column_name {< datatype> | COMPUTED BY (< expr>) | domain}\n")
        wxT("        [DEFAULT { literal | NULL | USER}] [NOT NULL]\n")
        wxT("    ...\n")
        wxT("    CONSTRAINT constraint_name\n")
        wxT("        PRIMARY KEY (column_list),\n")
        wxT("        UNIQUE      (column_list),\n")
        wxT("        FOREIGN KEY (column_list) REFERENCES other_table (column_list),\n")
        wxT("        CHECK       (condition),\n")
        wxT("    ...\n")
        wxT(");\n");
}
//-----------------------------------------------------------------------------
/*static*/
wxString MetadataItemCreateStatementVisitor::getCreateTriggerStatement()
{
    return wxT("SET TERM ^ ;\n\n")
        wxT("CREATE TRIGGER name [FOR table/view] \n")
        wxT(" [IN]ACTIVE \n")
        wxT(" [ON {[DIS]CONNECT | TRANSACTION {START | COMMIT | ROLLBACK}} ] \n")
        wxT(" [{BEFORE | AFTER} INSERT OR UPDATE OR DELETE] \n")
        wxT(" POSITION number \n")
        wxT("AS \n")
        wxT("BEGIN \n")
        wxT("    /* enter trigger code here */ \n")
        wxT("END^\n\n")
        wxT("SET TERM ; ^\n");
}
//-----------------------------------------------------------------------------
/*static*/
wxString MetadataItemCreateStatementVisitor::getCreateViewStatement()
{
    StatementBuilder sb;
    sb << kwCREATE << ' ' << kwVIEW << wxT(" name ( view_column, ...)")
        << StatementBuilder::NewLine << kwAS << StatementBuilder::NewLine
        << wxT("/* write select statement here */")
        << StatementBuilder::NewLine
        << kwWITH << ' ' << kwCHECK << ' ' << kwOPTION << ';'
        << StatementBuilder::NewLine;
    return sb;
}
//-----------------------------------------------------------------------------
void MetadataItemCreateStatementVisitor::visitDomains(Domains& /*domains*/)
{
    statementM = getCreateDomainStatement();
}
//-----------------------------------------------------------------------------
void MetadataItemCreateStatementVisitor::visitExceptions(
    Exceptions& /*exceptions*/)
{
    statementM = getCreateExceptionStatement();
}
//-----------------------------------------------------------------------------
void MetadataItemCreateStatementVisitor::visitFunctions(
    Functions& /*functions*/)
{
    statementM = getCreateFunctionStatement();
}
//-----------------------------------------------------------------------------
void MetadataItemCreateStatementVisitor::visitGenerators(
    Generators& /*generators*/)
{
    statementM = getCreateGeneratorStatement();
}
//-----------------------------------------------------------------------------
void MetadataItemCreateStatementVisitor::visitProcedures(
    Procedures& /*procedures*/)
{
    statementM = getCreateProcedureStatement();
}
//-----------------------------------------------------------------------------
void MetadataItemCreateStatementVisitor::visitRoles(Roles& /*roles*/)
{
    statementM = getCreateRoleStatement();
}
//-----------------------------------------------------------------------------
void MetadataItemCreateStatementVisitor::visitTables(Tables& /*tables*/)
{
    statementM = getCreateTableStatement();
}
//-----------------------------------------------------------------------------
void MetadataItemCreateStatementVisitor::visitTriggers(Triggers& /*triggers*/)
{
    statementM = getCreateTriggerStatement();
}
//-----------------------------------------------------------------------------
void MetadataItemCreateStatementVisitor::visitViews(Views& /*views*/)
{
    statementM = getCreateViewStatement();
}
//-----------------------------------------------------------------------------
wxString MetadataItemCreateStatementVisitor::getStatement() const
{
    return statementM;
}
//-----------------------------------------------------------------------------
